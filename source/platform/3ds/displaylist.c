#include <3ds.h>
#include <citro3d.h>
#include <stdlib.h>
#include <string.h>

struct displaylist {
    int dummy;
};

// --- INITIALIZATION ---
void displaylist_init() {}
struct displaylist* displaylist_create() { return NULL; }
void displaylist_destroy(struct displaylist* list) {}
void displaylist_reset() {}

// --- RECORDING GEOMETRY ---
void displaylist_pos(float x, float y, float z) {}
void displaylist_color(uint8_t r, uint8_t g, uint8_t b) {}
void displaylist_color_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {}
void displaylist_texcoord(float u, float v) {}
void displaylist_normal(float x, float y, float z) {}

// --- EXECUTION ---
struct displaylist* displaylist_finalize() { return NULL; }
void displaylist_render(struct displaylist* list) {}
void displaylist_render_immediate() {}

void gfx_draw_quads_flt(size_t vertex_count, const float* vertices, const uint8_t* colors, const float* texcoords) {}
