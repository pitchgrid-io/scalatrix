#ifndef SCALATRIX_C_API_H
#define SCALATRIX_C_API_H

#ifdef __cplusplus
extern "C" {
#endif

/* ── Opaque handles ────────────────────────────────────────────────── */

typedef struct scalatrix_mos   scalatrix_mos_t;
typedef struct scalatrix_scale scalatrix_scale_t;

/* ── Plain-data types (C-compatible) ───────────────────────────────── */

typedef struct { int x, y; }       scalatrix_vec2i;
typedef struct { double x, y; }    scalatrix_vec2d;

typedef struct {
    scalatrix_vec2i natural_coord;
    scalatrix_vec2d tuning_coord;
    double          pitch;         /* frequency in Hz */
} scalatrix_node;

/* ── MOS lifecycle ─────────────────────────────────────────────────── */

scalatrix_mos_t* scalatrix_mos_from_params(
    int a, int b, int mode, double equave, double generator, int repetitions);

scalatrix_mos_t* scalatrix_mos_from_g(
    int depth, int mode, double generator, double equave, int repetitions);

void scalatrix_mos_free(scalatrix_mos_t* mos);

/* ── MOS accessors ─────────────────────────────────────────────────── */

int    scalatrix_mos_a(const scalatrix_mos_t* mos);
int    scalatrix_mos_b(const scalatrix_mos_t* mos);
int    scalatrix_mos_n(const scalatrix_mos_t* mos);
int    scalatrix_mos_a0(const scalatrix_mos_t* mos);
int    scalatrix_mos_b0(const scalatrix_mos_t* mos);
int    scalatrix_mos_n0(const scalatrix_mos_t* mos);
int    scalatrix_mos_mode(const scalatrix_mos_t* mos);
int    scalatrix_mos_nL(const scalatrix_mos_t* mos);
int    scalatrix_mos_nS(const scalatrix_mos_t* mos);
int    scalatrix_mos_depth(const scalatrix_mos_t* mos);
int    scalatrix_mos_repetitions(const scalatrix_mos_t* mos);
double scalatrix_mos_equave(const scalatrix_mos_t* mos);
double scalatrix_mos_period(const scalatrix_mos_t* mos);
double scalatrix_mos_generator(const scalatrix_mos_t* mos);
double scalatrix_mos_L_fr(const scalatrix_mos_t* mos);
double scalatrix_mos_s_fr(const scalatrix_mos_t* mos);
double scalatrix_mos_chroma_fr(const scalatrix_mos_t* mos);
scalatrix_vec2i scalatrix_mos_L_vec(const scalatrix_mos_t* mos);
scalatrix_vec2i scalatrix_mos_s_vec(const scalatrix_mos_t* mos);
scalatrix_vec2i scalatrix_mos_chroma_vec(const scalatrix_mos_t* mos);
scalatrix_vec2i scalatrix_mos_v_gen(const scalatrix_mos_t* mos);

/* ── MOS mutation ──────────────────────────────────────────────────── */

void scalatrix_mos_adjust_params(
    scalatrix_mos_t* mos,
    int a, int b, int mode, double equave, double generator, int repetitions);

void scalatrix_mos_adjust_tuning(
    scalatrix_mos_t* mos, int mode, double equave, double generator);

/* ── MOS queries ───────────────────────────────────────────────────── */

int  scalatrix_mos_node_in_scale(const scalatrix_mos_t* mos, scalatrix_vec2i v);
int  scalatrix_mos_node_scale_degree(const scalatrix_mos_t* mos, scalatrix_vec2i v);
int  scalatrix_mos_node_equave_nr(const scalatrix_mos_t* mos, scalatrix_vec2i v);
int  scalatrix_mos_node_accidental(const scalatrix_mos_t* mos, scalatrix_vec2i v);

double scalatrix_mos_coord_to_freq(
    scalatrix_mos_t* mos, double x, double y, double base_freq);

/* ── Scale generation ──────────────────────────────────────────────── */

scalatrix_scale_t* scalatrix_mos_generate_mapped_scale(
    const scalatrix_mos_t* mos,
    int steps, double offset, double base_freq, int n_nodes, int root);

scalatrix_scale_t* scalatrix_mos_generate_scale(
    scalatrix_mos_t* mos, double base_freq, int n_nodes, int root);

/* ── Scale lifecycle and access ────────────────────────────────────── */

void   scalatrix_scale_free(scalatrix_scale_t* scale);
int    scalatrix_scale_node_count(const scalatrix_scale_t* scale);
int    scalatrix_scale_root_idx(const scalatrix_scale_t* scale);
double scalatrix_scale_base_freq(const scalatrix_scale_t* scale);

/* Copies node data into caller-provided struct. Returns 0 on success, -1 on out-of-range. */
int scalatrix_scale_get_node(
    const scalatrix_scale_t* scale, int index, scalatrix_node* out);

#ifdef __cplusplus
}
#endif

#endif /* SCALATRIX_C_API_H */
