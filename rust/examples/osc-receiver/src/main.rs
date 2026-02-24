//! PitchGrid OSC Receiver Example
//!
//! Demonstrates how to:
//! 1. Receive tuning/mapping data from the PitchGrid plugin via OSC
//! 2. Instantiate a MOS scale system from the received parameters
//! 3. Generate a mapped scale and inspect the resulting nodes
//!
//! The PitchGrid plugin sends two OSC messages on each parameter change:
//!
//! - `/pitchgrid/plugin/tuning`  — current live tuning parameters
//! - `/pitchgrid/plugin/mapping` — MIDI mapping parameters (frozen when mapping is locked)
//!
//! Both messages carry the same 8 arguments:
//!   (mode: i32, root_freq: f32, stretch: f32, skew: f32,
//!    mode_offset: i32, steps: i32, mos_a: i32, mos_b: i32)
//!
//! This example also sends heartbeats to the plugin so it knows we're connected,
//! and handles heartbeat acknowledgments.
//!
//! Usage:
//!   cargo run -p osc-receiver
//!
//! The plugin's OSC server listens on port 34562 (our send target)
//! and sends to port 34561 (our receive port).

use std::net::UdpSocket;
use std::time::{Duration, Instant};

use rosc::{OscMessage, OscPacket, OscType};
use scalatrix::Mos;

/// Default ports matching the PitchGrid plugin's OSC configuration.
const RECEIVE_PORT: u16 = 34561; // We listen here (plugin sends to this port)
const PLUGIN_PORT: u16 = 34562;  // Plugin listens here (we send heartbeats here)

/// Parsed tuning/mapping parameters from an OSC message.
#[derive(Debug, Clone)]
struct PitchGridParams {
    mode: i32,
    root_freq: f64,
    stretch: f64,
    skew: f64,
    mode_offset: i32,
    steps: i32,
    mos_a: i32,
    mos_b: i32,
}

impl PitchGridParams {
    /// Try to parse from OSC args: (i32, f32, f32, f32, i32, i32, i32, i32)
    fn from_osc_args(args: &[OscType]) -> Option<Self> {
        if args.len() < 8 {
            return None;
        }
        Some(Self {
            mode:        args[0].clone().int()?,
            root_freq:   args[1].clone().float()? as f64,
            stretch:     args[2].clone().float()? as f64,
            skew:        args[3].clone().float()? as f64,
            mode_offset: args[4].clone().int()?,
            steps:       args[5].clone().int()?,
            mos_a:       args[6].clone().int()?,
            mos_b:       args[7].clone().int()?,
        })
    }
}

/// Send a heartbeat message to the PitchGrid plugin.
fn send_heartbeat(socket: &UdpSocket) {
    let msg = OscMessage {
        addr: "/pitchgrid/heartbeat".into(),
        args: vec![OscType::Int(1)],
    };
    let packet = rosc::encoder::encode(&OscPacket::Message(msg)).unwrap();
    let _ = socket.send_to(&packet, format!("127.0.0.1:{PLUGIN_PORT}"));
}

/// Process mapping parameters: create MOS and generate scale.
fn process_mapping(params: &PitchGridParams) {
    // Create a MOS from the received parameters
    let mos = Mos::from_params(
        params.mos_a,
        params.mos_b,
        params.mode,
        params.stretch,    // equave
        params.skew,       // generator
        1,                 // repetitions
    );

    println!("  MOS: {mos} — ({}, {}) n={}", mos.a(), mos.b(), mos.n());
    println!("  Generator: {:.6}, Equave: {:.6}", mos.generator(), mos.equave());
    println!("  Large step: {:.4}, Small step: {:.4}, Chroma: {:.4}",
        mos.large_step_ratio(), mos.small_step_ratio(), mos.chroma_ratio());

    // Generate the MIDI-mapped scale
    let scale = mos.generate_mapped_scale(
        params.steps,
        params.mode_offset as f64,
        params.root_freq,
        128,  // MIDI range
        60,   // root = middle C
    );

    println!("  Scale: {} nodes, root at index {}", scale.len(), scale.root_idx());

    // Build coord → index lookup (what downstream apps need for MIDI mapping)
    let coord_map = scale.coord_to_index();
    println!("  Mapped {} unique coordinates", coord_map.len());

    // Show a few nodes around middle C
    println!("  Nodes around root (index 58-62):");
    for i in 58..=62 {
        if let Some(node) = scale.node(i) {
            let in_scale = mos.node_in_scale(scalatrix::Vec2i::new(
                node.natural_coord.x, node.natural_coord.y));
            println!("    [{:3}] ({:3}, {:3}) -> {:10.4} Hz  in_scale={}",
                i, node.natural_coord.x, node.natural_coord.y, node.pitch, in_scale);
        }
    }
    println!();
}

fn main() {
    println!("PitchGrid OSC Receiver");
    println!("======================");
    println!("Listening on port {RECEIVE_PORT}, sending heartbeats to port {PLUGIN_PORT}");
    println!();

    // Bind receive socket
    let socket = UdpSocket::bind(format!("127.0.0.1:{RECEIVE_PORT}"))
        .expect("Failed to bind receive socket");
    socket
        .set_read_timeout(Some(Duration::from_millis(500)))
        .unwrap();

    let mut buf = [0u8; 2048];
    let mut last_heartbeat = Instant::now();
    let heartbeat_interval = Duration::from_secs(1);

    // Track last received params to avoid redundant processing
    let mut last_mapping: Option<String> = None;

    println!("Waiting for PitchGrid plugin...\n");

    loop {
        // Send periodic heartbeat
        if last_heartbeat.elapsed() >= heartbeat_interval {
            send_heartbeat(&socket);
            last_heartbeat = Instant::now();
        }

        // Try to receive
        match socket.recv_from(&mut buf) {
            Ok((size, _addr)) => {
                let packet = match rosc::decoder::decode_udp(&buf[..size]) {
                    Ok((_, packet)) => packet,
                    Err(e) => {
                        eprintln!("OSC decode error: {e}");
                        continue;
                    }
                };

                if let OscPacket::Message(msg) = packet {
                    match msg.addr.as_str() {
                        "/pitchgrid/heartbeat/ack" => {
                            // Plugin acknowledged our heartbeat — connection is alive
                        }

                        "/pitchgrid/plugin/tuning" => {
                            if let Some(params) = PitchGridParams::from_osc_args(&msg.args) {
                                println!("[tuning] mode={}, root={:.2}Hz, stretch={:.6}, skew={:.6}, offset={}, steps={}, mos=({},{})",
                                    params.mode, params.root_freq, params.stretch, params.skew,
                                    params.mode_offset, params.steps, params.mos_a, params.mos_b);
                            }
                        }

                        "/pitchgrid/plugin/mapping" => {
                            if let Some(params) = PitchGridParams::from_osc_args(&msg.args) {
                                // Dedup: only process if params actually changed
                                let key = format!("{:?}", params);
                                if last_mapping.as_ref() != Some(&key) {
                                    println!("[mapping] mode={}, root={:.2}Hz, stretch={:.6}, skew={:.6}, offset={}, steps={}, mos=({},{})",
                                        params.mode, params.root_freq, params.stretch, params.skew,
                                        params.mode_offset, params.steps, params.mos_a, params.mos_b);
                                    process_mapping(&params);
                                    last_mapping = Some(key);
                                }
                            }
                        }

                        addr => {
                            println!("[unknown] {addr}: {:?}", msg.args);
                        }
                    }
                }
            }
            Err(ref e) if e.kind() == std::io::ErrorKind::WouldBlock => {
                // Timeout — loop back to send heartbeat
            }
            Err(e) => {
                eprintln!("Socket error: {e}");
            }
        }
    }
}
