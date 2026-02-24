#include "scalatrix/c_api.h"
#include "scalatrix/mos.hpp"
#include "scalatrix/scale.hpp"

using namespace scalatrix;

/* ── helpers ───────────────────────────────────────────────────────── */

static scalatrix_vec2i to_c(Vector2i v) { return {v.x, v.y}; }
static Vector2i from_c(scalatrix_vec2i v) { return {v.x, v.y}; }

/* ── MOS lifecycle ─────────────────────────────────────────────────── */

scalatrix_mos_t* scalatrix_mos_from_params(
    int a, int b, int mode, double equave, double generator, int repetitions)
{
    auto* mos = new MOS(a * repetitions, b * repetitions, mode, equave, generator);
    return reinterpret_cast<scalatrix_mos_t*>(mos);
}

scalatrix_mos_t* scalatrix_mos_from_g(
    int depth, int mode, double generator, double equave, int repetitions)
{
    auto* mos = new MOS(MOS::fromG(depth, mode, generator, equave, repetitions));
    return reinterpret_cast<scalatrix_mos_t*>(mos);
}

void scalatrix_mos_free(scalatrix_mos_t* mos) {
    delete reinterpret_cast<MOS*>(mos);
}

/* ── MOS accessors ─────────────────────────────────────────────────── */

#define MOS_PTR(p) reinterpret_cast<const MOS*>(p)
#define MOS_MUT(p) reinterpret_cast<MOS*>(p)

int    scalatrix_mos_a(const scalatrix_mos_t* m)           { return MOS_PTR(m)->a; }
int    scalatrix_mos_b(const scalatrix_mos_t* m)           { return MOS_PTR(m)->b; }
int    scalatrix_mos_n(const scalatrix_mos_t* m)           { return MOS_PTR(m)->n; }
int    scalatrix_mos_a0(const scalatrix_mos_t* m)          { return MOS_PTR(m)->a0; }
int    scalatrix_mos_b0(const scalatrix_mos_t* m)          { return MOS_PTR(m)->b0; }
int    scalatrix_mos_n0(const scalatrix_mos_t* m)          { return MOS_PTR(m)->n0; }
int    scalatrix_mos_mode(const scalatrix_mos_t* m)        { return MOS_PTR(m)->mode; }
int    scalatrix_mos_nL(const scalatrix_mos_t* m)          { return MOS_PTR(m)->nL; }
int    scalatrix_mos_nS(const scalatrix_mos_t* m)          { return MOS_PTR(m)->nS; }
int    scalatrix_mos_depth(const scalatrix_mos_t* m)       { return MOS_PTR(m)->depth; }
int    scalatrix_mos_repetitions(const scalatrix_mos_t* m) { return MOS_PTR(m)->repetitions; }
double scalatrix_mos_equave(const scalatrix_mos_t* m)      { return MOS_PTR(m)->equave; }
double scalatrix_mos_period(const scalatrix_mos_t* m)      { return MOS_PTR(m)->period; }
double scalatrix_mos_generator(const scalatrix_mos_t* m)   { return MOS_PTR(m)->generator; }
double scalatrix_mos_structure_generator(const scalatrix_mos_t* m) { return MOS_PTR(m)->structure_generator; }
double scalatrix_mos_L_fr(const scalatrix_mos_t* m)        { return MOS_PTR(m)->L_fr; }
double scalatrix_mos_s_fr(const scalatrix_mos_t* m)        { return MOS_PTR(m)->s_fr; }
double scalatrix_mos_chroma_fr(const scalatrix_mos_t* m)   { return MOS_PTR(m)->chroma_fr; }

scalatrix_vec2i scalatrix_mos_L_vec(const scalatrix_mos_t* m)      { return to_c(MOS_PTR(m)->L_vec); }
scalatrix_vec2i scalatrix_mos_s_vec(const scalatrix_mos_t* m)      { return to_c(MOS_PTR(m)->s_vec); }
scalatrix_vec2i scalatrix_mos_chroma_vec(const scalatrix_mos_t* m) { return to_c(MOS_PTR(m)->chroma_vec); }
scalatrix_vec2i scalatrix_mos_v_gen(const scalatrix_mos_t* m)      { return to_c(MOS_PTR(m)->v_gen); }

/* ── MOS mutation ──────────────────────────────────────────────────── */

void scalatrix_mos_adjust_params(
    scalatrix_mos_t* mos,
    int a, int b, int mode, double equave, double generator, int repetitions)
{
    MOS_MUT(mos)->adjustParams(a, b, mode, equave, generator, repetitions);
}


void scalatrix_mos_adjust_tuning_g(
    scalatrix_mos_t* mos, int depth, int mode, double generator, double equave, int repetitions)
{
    MOS_MUT(mos)->adjustTuningG(depth, mode, generator, equave, repetitions);
}

/* ── MOS queries ───────────────────────────────────────────────────── */

int scalatrix_mos_node_in_scale(const scalatrix_mos_t* m, scalatrix_vec2i v) {
    return MOS_PTR(m)->nodeInScale(from_c(v)) ? 1 : 0;
}

int scalatrix_mos_node_scale_degree(const scalatrix_mos_t* m, scalatrix_vec2i v) {
    return MOS_PTR(m)->nodeScaleDegree(from_c(v));
}

int scalatrix_mos_node_equave_nr(const scalatrix_mos_t* m, scalatrix_vec2i v) {
    return MOS_PTR(m)->nodeEquaveNr(from_c(v));
}

int scalatrix_mos_node_accidental(const scalatrix_mos_t* m, scalatrix_vec2i v) {
    return MOS_PTR(m)->nodeAccidental(from_c(v));
}

double scalatrix_mos_coord_to_freq(
    scalatrix_mos_t* mos, double x, double y, double base_freq)
{
    return MOS_MUT(mos)->coordToFreq(x, y, base_freq);
}

/* ── Scale generation ──────────────────────────────────────────────── */

scalatrix_scale_t* scalatrix_mos_generate_mapped_scale(
    const scalatrix_mos_t* mos,
    int steps, double offset, double base_freq, int n_nodes, int root)
{
    auto scale = MOS_PTR(mos)->generateMappedScale(steps, offset, base_freq, n_nodes, root);
    return reinterpret_cast<scalatrix_scale_t*>(new Scale(std::move(scale)));
}

scalatrix_scale_t* scalatrix_mos_generate_scale(
    scalatrix_mos_t* mos, double base_freq, int n_nodes, int root)
{
    auto scale = MOS_MUT(mos)->generateScaleFromMOS(base_freq, n_nodes, root);
    return reinterpret_cast<scalatrix_scale_t*>(new Scale(std::move(scale)));
}

/* ── Scale lifecycle and access ────────────────────────────────────── */

#define SCALE_PTR(p) reinterpret_cast<const Scale*>(p)

void scalatrix_scale_free(scalatrix_scale_t* scale) {
    delete reinterpret_cast<Scale*>(scale);
}

int scalatrix_scale_node_count(const scalatrix_scale_t* s) {
    return static_cast<int>(SCALE_PTR(s)->getNodes().size());
}

int scalatrix_scale_root_idx(const scalatrix_scale_t* s) {
    return SCALE_PTR(s)->getRootIdx();
}

double scalatrix_scale_base_freq(const scalatrix_scale_t* s) {
    return SCALE_PTR(s)->getBaseFreq();
}

int scalatrix_scale_get_node(
    const scalatrix_scale_t* s, int index, scalatrix_node* out)
{
    const auto& nodes = SCALE_PTR(s)->getNodes();
    if (index < 0 || index >= static_cast<int>(nodes.size()))
        return -1;
    const Node& n = nodes[index];
    out->natural_coord = to_c(n.natural_coord);
    out->tuning_coord  = {n.tuning_coord.x, n.tuning_coord.y};
    out->pitch         = n.pitch;
    return 0;
}
