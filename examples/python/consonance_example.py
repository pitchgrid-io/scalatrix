#!/usr/bin/env python3
"""Consonance analysis validation: C++ vs Python reference values."""

import sys
sys.path.insert(0, '/home/john/repos/scalatrix/build')
import scalatrix as sx


def get_mos_intervals_extended(depth, generator, equave, max_cents=2000.0):
    mos0 = sx.MOS.fromG(depth, 0, generator, equave, 1)
    n = mos0.n
    equave_cents = equave * 1200

    nodes0 = mos0.base_scale.getNodes()
    mos_last = sx.MOS.fromG(depth, n - 1, generator, equave, 1)
    nodes1 = mos_last.base_scale.getNodes()

    n_equaves = int(max_cents / equave_cents) + 2
    pitches0, pitches1 = [], []
    for eq in range(n_equaves):
        for i in range(n):
            pitches0.append(nodes0[i].tuning_coord.x * 1200 + eq * equave_cents)
            pitches1.append(nodes1[i].tuning_coord.x * 1200 + eq * equave_cents)

    intervals = [("P1", 0.0)]
    deg = 1
    for i in range(1, len(pitches0)):
        if pitches0[i] > max_cents + 1:
            break
        v0 = round(pitches0[i], 6)
        v1 = round(pitches1[i], 6)
        if abs(v0 - v1) < 0.01:
            intervals.append((f"P{deg}", v0))
        else:
            lo, hi = sorted([v0, v1])
            intervals.append((f"m{deg}", lo))
            intervals.append((f"M{deg}", hi))
        deg += 1
    return intervals, mos0


# Reference values from Python prototype (total_consonance.py)
REFERENCE = {
    "12-TET (harmonic)":         (22, 5.959, 0.2709),
    "12-TET (pseudoharmonic)":   (22, 7.518, 0.3417),
    "Pythagorean (harmonic)":    (22, 5.831, 0.2651),
    "1/4-comma Meantone":        (22, 7.026, 0.3194),
    "Bohlen-Pierce (odd harm.)": (18, 5.690, 0.3161),
    "Porcupine[8] (harmonic)":   (25, 6.559, 0.2624),
    "Slendric[11] (harmonic)":   (35, 9.499, 0.2714),
}

configs = [
    ("12-TET (harmonic)",         3, 0.5833333,          1.0,                "harmonic", 10),
    ("12-TET (pseudoharmonic)",   3, 0.5833333,          1.0,                "pseudo",   10),
    ("Pythagorean (harmonic)",    3, 0.5849708914756775,  1.0,               "harmonic", 10),
    ("1/4-comma Meantone",        3, 0.5804959535598755,  1.0,               "harmonic", 10),
    ("Bohlen-Pierce (odd harm.)", 4, 0.7683332562446594,  1.583345055580139, "odd",      13),
    ("Porcupine[8] (harmonic)",   6, 0.8647956252098083,  1.0,               "harmonic", 13),
    ("Slendric[11] (harmonic)",   5, 0.8052850961685181,  1.000265121459961, "harmonic", 13),
]

print(f"{'Configuration':<35} {'#Iv':>3} {'Total C++':>9} {'Total Py':>9} {'Mean C++':>8} {'Mean Py':>8} {'Δ Mean':>7}")
print(f"{'─'*35} {'─'*3} {'─'*9} {'─'*9} {'─'*8} {'─'*8} {'─'*7}")

all_pass = True
for name, depth, gen, equave, spec_type, n_partials in configs:
    if spec_type == "harmonic":
        spectrum = sx.Spectrum.harmonic(n_partials)
    elif spec_type == "pseudo":
        spectrum = sx.Spectrum.pseudoharmonic(n_partials)
    elif spec_type == "odd":
        spectrum = sx.Spectrum.oddHarmonic(n_partials)

    intervals, mos = get_mos_intervals_extended(depth, gen, equave, max_cents=2000.0)
    result = sx.analyzeScale(spectrum, 261.63, intervals, 2000.0)
    
    ref_n, ref_total, ref_mean = REFERENCE[name]
    delta = result.mean_consonance - ref_mean
    status = "✓" if abs(delta) < 0.001 else "✗"
    if abs(delta) >= 0.001:
        all_pass = False
    
    print(f"{name:<35} {len(result.intervals):>3} {result.total_consonance:9.3f} {ref_total:9.3f} "
          f"{result.mean_consonance:8.4f} {ref_mean:8.4f} {delta:+7.4f} {status}")

print()
if all_pass:
    print("ALL PASS — C++ matches Python reference within 0.001")
else:
    print("SOME VALUES DIFFER — needs investigation")
