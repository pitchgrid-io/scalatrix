#!/usr/bin/env python3
"""
PitchGrid MIDI Retuner — Translate standard MIDI to MPE using Scalatrix tuning.

Parses a PitchGrid preset XML, builds the tuning table via Scalatrix,
reads a standard MIDI file, and writes a retuned MPE MIDI file.

Usage:
    python midi_retune.py --preset preset.xml --input input.mid --output output.mid
    python midi_retune.py --preset preset.xml --input input.mid  # prints to stdout info
    python midi_retune.py --preset preset.xml --dump-table       # dump frequency table
"""

import argparse
import math
import sys
import xml.etree.ElementTree as ET
from dataclasses import dataclass
from typing import Optional

import mido
import scalatrix as sx


# ---------------------------------------------------------------------------
# Preset parsing
# ---------------------------------------------------------------------------

@dataclass
class PitchGridPreset:
    """Parameters extracted from a PitchGrid plugin preset XML."""
    name: str
    mode: int
    depth: float
    skew: float        # = generator
    stretch: float     # = equave
    steps_raw: float
    offset_raw: float
    base_tune: float
    pitch_bend_range: int = 48

    @property
    def steps(self) -> int:
        return round(self.steps_raw)

    @property
    def offset(self) -> int:
        return round(self.offset_raw * self.steps)


def parse_preset_xml(path: str) -> PitchGridPreset:
    """Parse a PitchGrid plugin preset XML file."""
    tree = ET.parse(path)
    root = tree.getroot()

    # Extract top-level attributes
    name = root.find('.//PARAMETERS').get('_currentPresetName', 'Unknown')
    mode = int(root.find('.//PARAMETERS').get('_mode', '1'))

    # Extract PARAM values
    params = {}
    for param in root.findall('.//PARAM'):
        pid = param.get('id')
        val = param.get('value')
        if pid and val:
            params[pid] = float(val)

    return PitchGridPreset(
        name=name,
        mode=mode,
        depth=params.get('depth', 3.0),
        skew=params.get('skew', 7/12),
        stretch=params.get('stretch', 1.0),
        steps_raw=params.get('steps', 12.0),
        offset_raw=params.get('offset', 0.5),
        base_tune=params.get('base_tune', 0.0),
        pitch_bend_range=int(params.get('midiPitchBendRange', 48)),
    )


# ---------------------------------------------------------------------------
# Tuning table generation
# ---------------------------------------------------------------------------

@dataclass
class TuningEntry:
    """A single entry in the 128-note tuning table."""
    midi_note: int
    frequency_hz: float
    cents_from_root: float
    natural_coord: tuple  # (x, y)
    on_scale: bool


def build_tuning_table(preset: PitchGridPreset, base_freq_hz: float = 261.625565) -> list[TuningEntry]:
    """
    Build a 128-entry tuning table from a PitchGrid preset.
    
    Pipeline:
    1. Create MOS from preset parameters
    2. Get the MOS implied affine transform
    3. Apply y-stretch (n/steps) and y-translation ((offset - mode) / steps)
    4. Generate scale via Scale.fromAffine with 128 nodes, root at 60
    """
    # Step 1: Create MOS
    depth = round(preset.depth)
    mos = sx.MOS.fromG(depth, preset.mode, preset.skew, preset.stretch, 1)

    # Step 2: Get implied affine
    ia = mos.impliedAffine
    n = mos.n
    steps = preset.steps
    offset = preset.offset

    # Step 3: Apply window transform (y-stretch + y-translation)
    # Window offset = offset - mode (compensates for MOS mode rotation)
    wo = offset - preset.mode
    sf = n / steps  # scale factor (y-stretch)

    # Compose: squeezed = windowAffine * impliedAffine
    # windowAffine = (1, 0, 0, sf, 0, wo/steps)
    squeezed = sx.AffineTransform(
        ia.a,               # a (unchanged)
        ia.b,               # b (unchanged)
        sf * ia.c,          # c (y-stretched)
        sf * ia.d,          # d (y-stretched)
        ia.tx,              # tx (unchanged)
        sf * ia.ty + wo / steps  # ty (y-stretched + translated)
    )

    # Apply base_tune offset (semitones → log2)
    # base_tune shifts the base frequency
    adjusted_freq = base_freq_hz * (2.0 ** (preset.base_tune / 12.0))

    # Step 4: Generate scale
    scale = sx.Scale.fromAffine(squeezed, adjusted_freq, 128, 60)
    nodes = scale.getNodes()

    # Build table
    table = []
    for i in range(128):
        node = nodes[i]
        freq = node.pitch
        cents = 1200.0 * math.log2(freq / adjusted_freq) if freq > 0 else 0.0
        coord = node.natural_coord
        on_scale = mos.nodeInScale(coord)
        table.append(TuningEntry(
            midi_note=i,
            frequency_hz=freq,
            cents_from_root=cents,
            natural_coord=(coord.x, coord.y),
            on_scale=on_scale,
        ))

    return table


# ---------------------------------------------------------------------------
# MIDI → MPE conversion
# ---------------------------------------------------------------------------

def freq_to_midi_and_bend(freq_hz: float, pitch_bend_range: int = 48) -> tuple[int, int]:
    """
    Convert a frequency to the nearest MIDI note number + pitch bend value.
    
    Returns (midi_note, pitch_bend) where pitch_bend is 0-16383 (8192 = center).
    """
    if freq_hz <= 0:
        return (0, 8192)

    # Exact MIDI note number (fractional)
    exact_midi = 69.0 + 12.0 * math.log2(freq_hz / 440.0)
    midi_note = round(exact_midi)
    midi_note = max(0, min(127, midi_note))

    # Pitch bend in semitones
    bend_semitones = exact_midi - midi_note

    # Convert to 14-bit pitch bend value
    # bend_semitones / pitch_bend_range maps to [-1, 1], then to [0, 16383]
    bend_normalized = bend_semitones / pitch_bend_range
    bend_value = int(round(8192 + 8191 * bend_normalized))
    bend_value = max(0, min(16383, bend_value))

    return (midi_note, bend_value)


def retune_midi(input_path: str, output_path: str, tuning_table: list[TuningEntry],
                pitch_bend_range: int = 48):
    """
    Read a standard MIDI file and write a retuned MPE version.
    
    MPE layout:
    - Channel 0: manager channel (RPN for pitch bend range)
    - Channels 1-15: note channels (one note per channel)
    """
    mid = mido.MidiFile(input_path)
    out = mido.MidiFile(ticks_per_beat=mid.ticks_per_beat)

    # Channel allocation for MPE (skip channel 9 = GM drums)
    channel_pool = [ch for ch in range(1, 16) if ch != 9]
    note_to_channel: dict[int, int] = {}  # original_note → MPE channel
    free_channels = list(channel_pool)

    for track_idx, track in enumerate(mid.tracks):
        out_track = mido.MidiTrack()
        out.tracks.append(out_track)

        # Add MPE configuration on first track
        if track_idx == 0:
            # RPN 0x00 0x06 = MPE Configuration Message on channel 0
            # Set MCM: 15 member channels
            out_track.append(mido.Message('control_change', channel=0, control=101, value=0, time=0))
            out_track.append(mido.Message('control_change', channel=0, control=100, value=6, time=0))
            out_track.append(mido.Message('control_change', channel=0, control=6, value=15, time=0))
            out_track.append(mido.Message('control_change', channel=0, control=38, value=0, time=0))

            # Set pitch bend range on all member channels (excl. ch 9 = GM drums)
            for ch in channel_pool:
                out_track.append(mido.Message('control_change', channel=ch, control=101, value=0, time=0))
                out_track.append(mido.Message('control_change', channel=ch, control=100, value=0, time=0))
                out_track.append(mido.Message('control_change', channel=ch, control=6, value=pitch_bend_range, time=0))
                out_track.append(mido.Message('control_change', channel=ch, control=38, value=0, time=0))

        for msg in track:
            if msg.type == 'note_on' and msg.velocity > 0:
                orig_note = msg.note
                if orig_note < 0 or orig_note > 127:
                    out_track.append(msg)
                    continue

                # Look up tuned frequency
                entry = tuning_table[orig_note]
                new_note, bend = freq_to_midi_and_bend(entry.frequency_hz, pitch_bend_range)

                # Allocate MPE channel
                if free_channels:
                    ch = free_channels.pop(0)
                else:
                    # All channels occupied — steal the oldest
                    ch = 1  # fallback
                note_to_channel[orig_note] = ch

                # Send pitch bend BEFORE note-on (per MPE spec)
                out_track.append(mido.Message(
                    'pitchwheel', channel=ch,
                    pitch=bend - 8192,  # mido uses -8192 to 8191
                    time=msg.time
                ))
                out_track.append(mido.Message(
                    'note_on', channel=ch,
                    note=new_note, velocity=msg.velocity,
                    time=0  # same tick as pitch bend
                ))

            elif msg.type == 'note_off' or (msg.type == 'note_on' and msg.velocity == 0):
                orig_note = msg.note
                ch = note_to_channel.pop(orig_note, msg.channel)

                # Find the retuned note number
                if 0 <= orig_note <= 127:
                    entry = tuning_table[orig_note]
                    new_note, _ = freq_to_midi_and_bend(entry.frequency_hz, pitch_bend_range)
                else:
                    new_note = orig_note

                out_track.append(mido.Message(
                    'note_off', channel=ch,
                    note=new_note, velocity=msg.velocity if hasattr(msg, 'velocity') else 0,
                    time=msg.time
                ))

                # Return channel to pool
                if ch in channel_pool and ch not in free_channels:
                    free_channels.append(ch)

            elif msg.type in ('control_change', 'program_change'):
                # Pass through on manager channel
                out_track.append(msg.copy(channel=0, time=msg.time))

            elif msg.is_meta:
                out_track.append(msg)

            else:
                # Pass through other messages unchanged
                out_track.append(msg)

    out.save(output_path)
    return out


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def dump_table(table: list[TuningEntry], preset: PitchGridPreset):
    """Print the tuning table in a readable format."""
    print(f"Tuning table for preset: {preset.name}")
    print(f"MOS params: depth={round(preset.depth)}, mode={preset.mode}, "
          f"generator={preset.skew:.6f}, equave={preset.stretch:.6f}")
    print(f"Steps={preset.steps}, Offset={preset.offset}")
    print(f"{'MIDI':>4}  {'Freq (Hz)':>12}  {'Cents':>10}  {'Coord':>10}  {'OnScale':>7}")
    print("-" * 55)
    for e in table:
        scale_marker = "  *" if e.on_scale else ""
        print(f"{e.midi_note:4d}  {e.frequency_hz:12.3f}  {e.cents_from_root:+10.3f}  "
              f"({e.natural_coord[0]:3d},{e.natural_coord[1]:3d})  {scale_marker}")


def main():
    parser = argparse.ArgumentParser(
        description='PitchGrid MIDI Retuner — convert standard MIDI to MPE via Scalatrix tuning')
    parser.add_argument('--preset', '-p', required=True, help='PitchGrid preset XML file')
    parser.add_argument('--input', '-i', help='Input MIDI file')
    parser.add_argument('--output', '-o', help='Output MPE MIDI file')
    parser.add_argument('--base-freq', type=float, default=261.625565,
                        help='Base frequency in Hz (default: middle C = 261.625565)')
    parser.add_argument('--pitch-bend-range', type=int, default=None,
                        help='MPE pitch bend range in semitones (default: from preset, typically 48)')
    parser.add_argument('--dump-table', action='store_true', help='Dump the tuning table and exit')
    args = parser.parse_args()

    # Parse preset
    preset = parse_preset_xml(args.preset)
    print(f"Loaded preset: {preset.name}", file=sys.stderr)
    print(f"  MOS: depth={round(preset.depth)}, mode={preset.mode}, "
          f"gen={preset.skew:.6f}, equave={preset.stretch:.6f}", file=sys.stderr)
    print(f"  Steps={preset.steps} (raw={preset.steps_raw:.2f}), "
          f"Offset={preset.offset} (raw={preset.offset_raw:.4f})", file=sys.stderr)

    # Override pitch bend range if specified
    pb_range = args.pitch_bend_range or preset.pitch_bend_range
    print(f"  Pitch bend range: ±{pb_range} semitones", file=sys.stderr)

    # Build tuning table
    table = build_tuning_table(preset, args.base_freq)

    if args.dump_table:
        dump_table(table, preset)
        return

    if not args.input:
        parser.error("--input is required unless --dump-table is used")

    output_path = args.output or args.input.replace('.mid', '_mpe.mid')
    if output_path == args.input:
        output_path = args.input.replace('.mid', '_retuned.mid')

    print(f"  Input: {args.input}", file=sys.stderr)
    print(f"  Output: {output_path}", file=sys.stderr)

    retune_midi(args.input, output_path, table, pb_range)
    print(f"Done! MPE file written to: {output_path}", file=sys.stderr)


if __name__ == '__main__':
    main()
