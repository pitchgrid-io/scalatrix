//! Safe Rust bindings to the scalatrix microtonal scale library.
//!
//! Scalatrix provides Moment of Symmetry (MOS) scale generation and mapping,
//! the mathematical foundation of the PitchGrid framework.
//!
//! # Quick start
//!
//! ```rust
//! use scalatrix::{Mos, Vec2i};
//!
//! // Create a standard diatonic scale: 5 large + 2 small steps
//! let mos = Mos::from_params(5, 2, 0, 1.0, 0.583333, 1);
//!
//! // Generate MIDI-mapped scale (128 notes, root at index 60)
//! let scale = mos.generate_mapped_scale(12, 1.0, 261.63, 128, 60);
//!
//! // Access nodes
//! for node in scale.nodes() {
//!     println!("({}, {}) -> {:.2} Hz",
//!         node.natural_coord.x, node.natural_coord.y, node.pitch);
//! }
//! ```

use scalatrix_sys as ffi;

/// Integer 2D vector representing lattice coordinates.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Vec2i {
    pub x: i32,
    pub y: i32,
}

impl Vec2i {
    pub fn new(x: i32, y: i32) -> Self {
        Self { x, y }
    }
}

impl From<ffi::scalatrix_vec2i> for Vec2i {
    fn from(v: ffi::scalatrix_vec2i) -> Self {
        Self { x: v.x, y: v.y }
    }
}

impl From<Vec2i> for ffi::scalatrix_vec2i {
    fn from(v: Vec2i) -> Self {
        ffi::scalatrix_vec2i { x: v.x, y: v.y }
    }
}

/// Double-precision 2D vector.
#[derive(Debug, Clone, Copy)]
pub struct Vec2d {
    pub x: f64,
    pub y: f64,
}

impl From<ffi::scalatrix_vec2d> for Vec2d {
    fn from(v: ffi::scalatrix_vec2d) -> Self {
        Self { x: v.x, y: v.y }
    }
}

/// A scale node: lattice coordinate + tuning coordinate + pitch.
#[derive(Debug, Clone, Copy)]
pub struct Node {
    /// Integer coordinates in the scale's natural 2D lattice.
    pub natural_coord: Vec2i,
    /// Floating-point coordinates in tuning space.
    pub tuning_coord: Vec2d,
    /// Frequency in Hz.
    pub pitch: f64,
}

/// Moment of Symmetry scale system.
///
/// Defines a generalized diatonic scale on a 2D lattice, parameterized by:
/// - **(a, b)**: counts of large and small intervals (coprime pair)
/// - **mode**: rotation index within the scale
/// - **equave**: log2 frequency ratio of the interval of equivalence
/// - **generator**: generator interval as a fraction of the period (0..1)
pub struct Mos {
    ptr: *mut ffi::scalatrix_mos_t,
}

// SAFETY: The C++ MOS object has no thread-local state.
unsafe impl Send for Mos {}

impl Drop for Mos {
    fn drop(&mut self) {
        unsafe { ffi::scalatrix_mos_free(self.ptr) }
    }
}

impl Mos {
    /// Create a MOS from explicit structure parameters.
    ///
    /// - `a`, `b`: interval counts (coprime pair defining the scale structure)
    /// - `mode`: mode index (rotation)
    /// - `equave`: log2 frequency ratio of the equave (1.0 = octave)
    /// - `generator`: generator as fraction of period (e.g. 0.583 ≈ 7/12 for diatonic)
    /// - `repetitions`: number of period repetitions per equave
    pub fn from_params(a: i32, b: i32, mode: i32, equave: f64, generator: f64, repetitions: i32) -> Self {
        let ptr = unsafe {
            ffi::scalatrix_mos_from_params(a, b, mode, equave, generator, repetitions)
        };
        assert!(!ptr.is_null(), "scalatrix_mos_from_params returned null");
        Self { ptr }
    }

    /// Create a MOS by specifying depth in the coprime tree and a generator value.
    ///
    /// The (a, b) pair is determined by walking the Stern-Brocot tree to the
    /// given depth using the generator to choose left/right at each level.
    pub fn from_g(depth: i32, mode: i32, generator: f64, equave: f64, repetitions: i32) -> Self {
        let ptr = unsafe {
            ffi::scalatrix_mos_from_g(depth, mode, generator, equave, repetitions)
        };
        assert!(!ptr.is_null(), "scalatrix_mos_from_g returned null");
        Self { ptr }
    }

    // ── Accessors ──────────────────────────────────────────────────

    /// Number of intervals of type A.
    pub fn a(&self) -> i32 { unsafe { ffi::scalatrix_mos_a(self.ptr) } }
    /// Number of intervals of type B.
    pub fn b(&self) -> i32 { unsafe { ffi::scalatrix_mos_b(self.ptr) } }
    /// Total scale size (a + b).
    pub fn n(&self) -> i32 { unsafe { ffi::scalatrix_mos_n(self.ptr) } }
    /// Coprime A count (a / repetitions).
    pub fn a0(&self) -> i32 { unsafe { ffi::scalatrix_mos_a0(self.ptr) } }
    /// Coprime B count (b / repetitions).
    pub fn b0(&self) -> i32 { unsafe { ffi::scalatrix_mos_b0(self.ptr) } }
    /// Coprime scale size (n / repetitions).
    pub fn n0(&self) -> i32 { unsafe { ffi::scalatrix_mos_n0(self.ptr) } }
    /// Mode index.
    pub fn mode(&self) -> i32 { unsafe { ffi::scalatrix_mos_mode(self.ptr) } }
    /// Count of large intervals.
    pub fn n_large(&self) -> i32 { unsafe { ffi::scalatrix_mos_nL(self.ptr) } }
    /// Count of small intervals.
    pub fn n_small(&self) -> i32 { unsafe { ffi::scalatrix_mos_nS(self.ptr) } }
    /// Depth in coprime tree.
    pub fn depth(&self) -> i32 { unsafe { ffi::scalatrix_mos_depth(self.ptr) } }
    /// Number of period repetitions per equave.
    pub fn repetitions(&self) -> i32 { unsafe { ffi::scalatrix_mos_repetitions(self.ptr) } }
    /// log2 frequency ratio of equave.
    pub fn equave(&self) -> f64 { unsafe { ffi::scalatrix_mos_equave(self.ptr) } }
    /// log2 frequency ratio of period.
    pub fn period(&self) -> f64 { unsafe { ffi::scalatrix_mos_period(self.ptr) } }
    /// Generator as fraction of period.
    pub fn generator(&self) -> f64 { unsafe { ffi::scalatrix_mos_generator(self.ptr) } }
    /// log2 frequency ratio of the large interval.
    pub fn large_step_ratio(&self) -> f64 { unsafe { ffi::scalatrix_mos_L_fr(self.ptr) } }
    /// log2 frequency ratio of the small interval.
    pub fn small_step_ratio(&self) -> f64 { unsafe { ffi::scalatrix_mos_s_fr(self.ptr) } }
    /// log2 frequency ratio of the chroma (L - s).
    pub fn chroma_ratio(&self) -> f64 { unsafe { ffi::scalatrix_mos_chroma_fr(self.ptr) } }
    /// Lattice vector for the large interval.
    pub fn large_vec(&self) -> Vec2i { unsafe { ffi::scalatrix_mos_L_vec(self.ptr) }.into() }
    /// Lattice vector for the small interval.
    pub fn small_vec(&self) -> Vec2i { unsafe { ffi::scalatrix_mos_s_vec(self.ptr) }.into() }
    /// Lattice vector for the chroma.
    pub fn chroma_vec(&self) -> Vec2i { unsafe { ffi::scalatrix_mos_chroma_vec(self.ptr) }.into() }
    /// Generator vector in lattice coordinates.
    pub fn gen_vec(&self) -> Vec2i { unsafe { ffi::scalatrix_mos_v_gen(self.ptr) }.into() }

    // ── Mutation ───────────────────────────────────────────────────

    /// Change both structure and tuning parameters.
    pub fn adjust_params(&mut self, a: i32, b: i32, mode: i32, equave: f64, generator: f64, repetitions: i32) {
        unsafe {
            ffi::scalatrix_mos_adjust_params(self.ptr, a, b, mode, equave, generator, repetitions);
        }
    }

    /// Change only tuning parameters (mode, equave, generator) without
    /// changing the scale structure (a, b).
    pub fn adjust_tuning(&mut self, mode: i32, equave: f64, generator: f64) {
        unsafe { ffi::scalatrix_mos_adjust_tuning(self.ptr, mode, equave, generator) }
    }

    // ── Queries ────────────────────────────────────────────────────

    /// Check if a lattice coordinate is within the current scale.
    pub fn node_in_scale(&self, v: Vec2i) -> bool {
        unsafe { ffi::scalatrix_mos_node_in_scale(self.ptr, v.into()) != 0 }
    }

    /// Get the scale degree (0..n-1) for a lattice coordinate.
    pub fn node_scale_degree(&self, v: Vec2i) -> i32 {
        unsafe { ffi::scalatrix_mos_node_scale_degree(self.ptr, v.into()) }
    }

    /// Get the equave number for a lattice coordinate.
    pub fn node_equave_nr(&self, v: Vec2i) -> i32 {
        unsafe { ffi::scalatrix_mos_node_equave_nr(self.ptr, v.into()) }
    }

    /// Get the accidental count (positive=sharp, negative=flat).
    pub fn node_accidental(&self, v: Vec2i) -> i32 {
        unsafe { ffi::scalatrix_mos_node_accidental(self.ptr, v.into()) }
    }

    /// Convert lattice coordinates to frequency.
    pub fn coord_to_freq(&mut self, x: f64, y: f64, base_freq: f64) -> f64 {
        unsafe { ffi::scalatrix_mos_coord_to_freq(self.ptr, x, y, base_freq) }
    }

    // ── Scale generation ───────────────────────────────────────────

    /// Generate a MIDI-mapped scale.
    ///
    /// This is the canonical method for computing MIDI note assignments.
    /// It applies a squeeze transform to map the MOS lattice onto a linear
    /// scale with the given number of steps.
    ///
    /// - `steps`: number of EDO steps (e.g. 12 for 12-TET mapping)
    /// - `offset`: mode offset within the mapping
    /// - `base_freq`: frequency of the root note in Hz
    /// - `n_nodes`: total number of scale nodes (typically 128 for MIDI)
    /// - `root`: index of the root node (typically 60 for middle C)
    pub fn generate_mapped_scale(&self, steps: i32, offset: f64, base_freq: f64, n_nodes: i32, root: i32) -> Scale {
        let ptr = unsafe {
            ffi::scalatrix_mos_generate_mapped_scale(self.ptr, steps, offset, base_freq, n_nodes, root)
        };
        assert!(!ptr.is_null(), "scalatrix_mos_generate_mapped_scale returned null");
        Scale { ptr }
    }

    /// Generate a scale directly from MOS structure.
    pub fn generate_scale(&mut self, base_freq: f64, n_nodes: i32, root: i32) -> Scale {
        let ptr = unsafe {
            ffi::scalatrix_mos_generate_scale(self.ptr, base_freq, n_nodes, root)
        };
        assert!(!ptr.is_null(), "scalatrix_mos_generate_scale returned null");
        Scale { ptr }
    }
}

impl std::fmt::Debug for Mos {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("Mos")
            .field("a", &self.a())
            .field("b", &self.b())
            .field("mode", &self.mode())
            .field("generator", &self.generator())
            .field("equave", &self.equave())
            .finish()
    }
}

impl std::fmt::Display for Mos {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{}L {}s (mode {})", self.n_large(), self.n_small(), self.mode())
    }
}

/// A generated scale: an ordered collection of nodes mapping lattice
/// coordinates to pitches.
pub struct Scale {
    ptr: *mut ffi::scalatrix_scale_t,
}

unsafe impl Send for Scale {}

impl Drop for Scale {
    fn drop(&mut self) {
        unsafe { ffi::scalatrix_scale_free(self.ptr) }
    }
}

impl Scale {
    /// Number of nodes in the scale.
    pub fn len(&self) -> usize {
        unsafe { ffi::scalatrix_scale_node_count(self.ptr) as usize }
    }

    /// Whether the scale is empty.
    pub fn is_empty(&self) -> bool {
        self.len() == 0
    }

    /// Index of the root node.
    pub fn root_idx(&self) -> usize {
        unsafe { ffi::scalatrix_scale_root_idx(self.ptr) as usize }
    }

    /// Base frequency.
    pub fn base_freq(&self) -> f64 {
        unsafe { ffi::scalatrix_scale_base_freq(self.ptr) }
    }

    /// Get a single node by index.
    pub fn node(&self, index: usize) -> Option<Node> {
        let mut raw = ffi::scalatrix_node {
            natural_coord: ffi::scalatrix_vec2i { x: 0, y: 0 },
            tuning_coord: ffi::scalatrix_vec2d { x: 0.0, y: 0.0 },
            pitch: 0.0,
        };
        let ret = unsafe { ffi::scalatrix_scale_get_node(self.ptr, index as i32, &mut raw) };
        if ret != 0 {
            return None;
        }
        Some(Node {
            natural_coord: raw.natural_coord.into(),
            tuning_coord: raw.tuning_coord.into(),
            pitch: raw.pitch,
        })
    }

    /// Collect all nodes into a Vec.
    pub fn nodes(&self) -> Vec<Node> {
        (0..self.len()).filter_map(|i| self.node(i)).collect()
    }

    /// Build a lookup table from natural coordinate to scale index.
    ///
    /// This is the primary data structure downstream apps need for
    /// mapping controller coordinates to MIDI note numbers.
    pub fn coord_to_index(&self) -> std::collections::HashMap<(i32, i32), usize> {
        let mut map = std::collections::HashMap::new();
        for (i, node) in self.nodes().iter().enumerate() {
            map.insert((node.natural_coord.x, node.natural_coord.y), i);
        }
        map
    }
}

impl std::fmt::Debug for Scale {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("Scale")
            .field("len", &self.len())
            .field("root_idx", &self.root_idx())
            .field("base_freq", &self.base_freq())
            .finish()
    }
}
