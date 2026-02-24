//! Raw FFI bindings to the scalatrix C library.
//!
//! These are low-level, unsafe bindings. Prefer the safe `scalatrix` crate.

#![allow(non_camel_case_types)]

use std::os::raw::c_int;

/// Opaque MOS handle.
#[repr(C)]
pub struct scalatrix_mos_t {
    _opaque: [u8; 0],
}

/// Opaque Scale handle.
#[repr(C)]
pub struct scalatrix_scale_t {
    _opaque: [u8; 0],
}

/// Integer 2D vector (lattice coordinates).
#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct scalatrix_vec2i {
    pub x: c_int,
    pub y: c_int,
}

/// Double-precision 2D vector.
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct scalatrix_vec2d {
    pub x: f64,
    pub y: f64,
}

/// Scale node data.
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct scalatrix_node {
    pub natural_coord: scalatrix_vec2i,
    pub tuning_coord: scalatrix_vec2d,
    pub pitch: f64,
}

extern "C" {
    // ── MOS lifecycle ──────────────────────────────────────────────

    pub fn scalatrix_mos_from_params(
        a: c_int, b: c_int, mode: c_int,
        equave: f64, generator: f64, repetitions: c_int,
    ) -> *mut scalatrix_mos_t;

    pub fn scalatrix_mos_from_g(
        depth: c_int, mode: c_int,
        generator: f64, equave: f64, repetitions: c_int,
    ) -> *mut scalatrix_mos_t;

    pub fn scalatrix_mos_free(mos: *mut scalatrix_mos_t);

    // ── MOS accessors ──────────────────────────────────────────────

    pub fn scalatrix_mos_a(mos: *const scalatrix_mos_t) -> c_int;
    pub fn scalatrix_mos_b(mos: *const scalatrix_mos_t) -> c_int;
    pub fn scalatrix_mos_n(mos: *const scalatrix_mos_t) -> c_int;
    pub fn scalatrix_mos_a0(mos: *const scalatrix_mos_t) -> c_int;
    pub fn scalatrix_mos_b0(mos: *const scalatrix_mos_t) -> c_int;
    pub fn scalatrix_mos_n0(mos: *const scalatrix_mos_t) -> c_int;
    pub fn scalatrix_mos_mode(mos: *const scalatrix_mos_t) -> c_int;
    pub fn scalatrix_mos_nL(mos: *const scalatrix_mos_t) -> c_int;
    pub fn scalatrix_mos_nS(mos: *const scalatrix_mos_t) -> c_int;
    pub fn scalatrix_mos_depth(mos: *const scalatrix_mos_t) -> c_int;
    pub fn scalatrix_mos_repetitions(mos: *const scalatrix_mos_t) -> c_int;
    pub fn scalatrix_mos_equave(mos: *const scalatrix_mos_t) -> f64;
    pub fn scalatrix_mos_period(mos: *const scalatrix_mos_t) -> f64;
    pub fn scalatrix_mos_generator(mos: *const scalatrix_mos_t) -> f64;
    pub fn scalatrix_mos_L_fr(mos: *const scalatrix_mos_t) -> f64;
    pub fn scalatrix_mos_s_fr(mos: *const scalatrix_mos_t) -> f64;
    pub fn scalatrix_mos_chroma_fr(mos: *const scalatrix_mos_t) -> f64;
    pub fn scalatrix_mos_L_vec(mos: *const scalatrix_mos_t) -> scalatrix_vec2i;
    pub fn scalatrix_mos_s_vec(mos: *const scalatrix_mos_t) -> scalatrix_vec2i;
    pub fn scalatrix_mos_chroma_vec(mos: *const scalatrix_mos_t) -> scalatrix_vec2i;
    pub fn scalatrix_mos_v_gen(mos: *const scalatrix_mos_t) -> scalatrix_vec2i;

    // ── MOS mutation ───────────────────────────────────────────────

    pub fn scalatrix_mos_adjust_params(
        mos: *mut scalatrix_mos_t,
        a: c_int, b: c_int, mode: c_int,
        equave: f64, generator: f64, repetitions: c_int,
    );

    pub fn scalatrix_mos_adjust_tuning(
        mos: *mut scalatrix_mos_t,
        mode: c_int, equave: f64, generator: f64,
    );

    // ── MOS queries ────────────────────────────────────────────────

    pub fn scalatrix_mos_node_in_scale(
        mos: *const scalatrix_mos_t, v: scalatrix_vec2i,
    ) -> c_int;

    pub fn scalatrix_mos_node_scale_degree(
        mos: *const scalatrix_mos_t, v: scalatrix_vec2i,
    ) -> c_int;

    pub fn scalatrix_mos_node_equave_nr(
        mos: *const scalatrix_mos_t, v: scalatrix_vec2i,
    ) -> c_int;

    pub fn scalatrix_mos_node_accidental(
        mos: *const scalatrix_mos_t, v: scalatrix_vec2i,
    ) -> c_int;

    pub fn scalatrix_mos_coord_to_freq(
        mos: *mut scalatrix_mos_t, x: f64, y: f64, base_freq: f64,
    ) -> f64;

    // ── Scale generation ───────────────────────────────────────────

    pub fn scalatrix_mos_generate_mapped_scale(
        mos: *const scalatrix_mos_t,
        steps: c_int, offset: f64, base_freq: f64, n_nodes: c_int, root: c_int,
    ) -> *mut scalatrix_scale_t;

    pub fn scalatrix_mos_generate_scale(
        mos: *mut scalatrix_mos_t,
        base_freq: f64, n_nodes: c_int, root: c_int,
    ) -> *mut scalatrix_scale_t;

    // ── Scale access ───────────────────────────────────────────────

    pub fn scalatrix_scale_free(scale: *mut scalatrix_scale_t);
    pub fn scalatrix_scale_node_count(scale: *const scalatrix_scale_t) -> c_int;
    pub fn scalatrix_scale_root_idx(scale: *const scalatrix_scale_t) -> c_int;
    pub fn scalatrix_scale_base_freq(scale: *const scalatrix_scale_t) -> f64;

    pub fn scalatrix_scale_get_node(
        scale: *const scalatrix_scale_t, index: c_int, out: *mut scalatrix_node,
    ) -> c_int;
}
