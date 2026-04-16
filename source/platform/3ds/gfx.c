#include <3ds.h>
#include <citro3d.h>
#include "../gfx.h"

// --- SYSTEM ---
void gfx_setup() {}
void gfx_finish(bool vsync) {}
void gfx_flip_buffers(float* gpu_wait, float* vsync_wait) {}
void gfx_clear_buffers(uint8_t r, uint8_t g, uint8_t b) {}
int gfx_width() { return 400; }
int gfx_height() { return 240; }

// --- MATRICES ---
void gfx_mode_world() {}
void gfx_mode_gui() {}
void gfx_matrix_projection(mat4 m, bool perspective) {}
void gfx_matrix_modelview(mat4 m) {}
void gfx_matrix_texture(bool enable, mat4 tex) {}

// --- STATE MACHINE ---
void gfx_fog(bool enable) {}
void gfx_fog_color(uint8_t r, uint8_t g, uint8_t b) {}
void gfx_fog_pos(float dx, float dz, float distance) {}
void gfx_lighting(bool enable) {}
void gfx_update_light(float daytime, const float* light_lookup) {}
void gfx_blending(enum gfx_blend mode) {}
void gfx_alpha_test(bool enable) {}
void gfx_depth_func(enum depth_func func) {}
void gfx_depth_range(float near, float far) {}
void gfx_cull_func(enum cull_func func) {}
void gfx_write_buffers(bool color, bool depth, bool depth_test) {}
void gfx_scissor(bool enable, uint32_t x, uint32_t y, uint32_t width, uint32_t height) {}

// --- TEXTURES ---
void gfx_texture(bool enable) {}
void gfx_bind_texture(struct tex_gfx* tex) {}
void gfx_texture_constant(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {}

// --- DRAWING ---
void gfx_draw_quads(size_t vertex_count, const int16_t* vertices, const uint8_t* colors, const uint16_t* texcoords) {}
void gfx_draw_lines(size_t vertex_count, const int16_t* vertices, const uint8_t* colors) {}
void gfx_copy_framebuffer(uint8_t* dest, size_t* width, size_t* height) {}
float gfx_lookup_light(uint8_t light) { return 1.0f; }

void tex_gfx_lookup(struct tex_gfx* tex, int x, int y, uint8_t* color) {}
