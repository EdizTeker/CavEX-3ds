#include <3ds.h>
#include <citro3d.h>
#include <string.h>
#include "../gfx.h"

#include "../../../build_3ds/source/platform/3ds/shader_shbin.h" 
#include "../../../build_3ds/source/platform/3ds/shader_shbin_data.h" 
#include <cglm/cglm.h>

C3D_RenderTarget* top_target;
static DVLB_s* vshader_dvlb;
static shaderProgram_s shader;
u16* quad_indices; 
static void* ui_vbo = NULL;
static size_t ui_vbo_offset = 0;
#define UI_VBO_MAX_SIZE (1024 * 512) 

void gfx_setup() {
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);

    top_target = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    C3D_RenderTargetSetOutput(top_target, GFX_TOP, GFX_LEFT, 
        GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | 
        GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | 
        GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO));

    vshader_dvlb = DVLB_ParseFile((u32*)shader_shbin, shader_shbin_len);
    shaderProgramInit(&shader);
    shaderProgramSetVsh(&shader, &vshader_dvlb->DVLE[0]);
    C3D_BindProgram(&shader);

    C3D_AttrInfo* attrInfo = C3D_GetAttrInfo();
    AttrInfo_Init(attrInfo);
    AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT, 3); 
    AttrInfo_AddLoader(attrInfo, 1, GPU_UNSIGNED_BYTE, 4); 
    AttrInfo_AddLoader(attrInfo, 2, GPU_FLOAT, 2); 
    
    quad_indices = (u16*)linearAlloc(150000 * sizeof(u16));
    for (int i = 0, v = 0; i < 150000; i += 6, v += 4) {
        if (i + 5 >= 150000) break;
        quad_indices[i+0] = v + 0; quad_indices[i+1] = v + 1; quad_indices[i+2] = v + 2;
        quad_indices[i+3] = v + 0; quad_indices[i+4] = v + 2; quad_indices[i+5] = v + 3;
    }
    ui_vbo = linearAlloc(UI_VBO_MAX_SIZE);
    GSPGPU_FlushDataCache(quad_indices, 150000 * sizeof(u16));

    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA);

    C3D_TexEnv* env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, 0, 0);
    C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
}

void gfx_clear_buffers(uint8_t r, uint8_t g, uint8_t b) {
    u32 clear_color = (((u32)255 << 24) | ((u32)b << 16) | ((u32)g << 8) | (u32)r);
    C3D_RenderTargetClear(top_target, C3D_CLEAR_ALL, clear_color, 0);
}

static C3D_Mtx current_proj_c3d;

static void cglm_to_c3d(mat4 src, C3D_Mtx* dst) {
    for (int r = 0; r < 4; r++) {
        dst->r[0].c[r] = src[r][0];
        dst->r[1].c[r] = src[r][1];
        dst->r[2].c[r] = src[r][2];
        dst->r[3].c[r] = src[r][3];
    }
}

void gfx_matrix_projection(mat4 m, bool perspective) {
    if (perspective) {
        Mtx_PerspTilt(&current_proj_c3d, C3D_AngleFromDegrees(70.0f), 400.0f/240.0f, 0.01f, 1000.0f, true);
    } else {
        Mtx_OrthoTilt(&current_proj_c3d, 0.0f, 400.0f, 240.0f, 0.0f, -256.0f, 256.0f, true);
    }
}

void gfx_mode_world() {
    C3D_DepthTest(true, GPU_GREATER, GPU_WRITE_ALL);
}

void gfx_mode_gui() {
    C3D_DepthTest(false, GPU_ALWAYS, GPU_WRITE_ALL);
    C3D_CullFace(GPU_CULL_NONE);

    C3D_Mtx proj, mv, proj_view;
    Mtx_OrthoTilt(&proj, 0.0f, 400.0f, 240.0f, 0.0f, -256.0f, 256.0f, true);
    Mtx_Identity(&mv);
    Mtx_Multiply(&proj_view, &proj, &mv);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, VSH_FVEC_projView, &proj_view);
}

void gfx_matrix_modelview(mat4 m) {
    C3D_Mtx mv_c3d, proj_view;
    cglm_to_c3d(m, &mv_c3d);
    Mtx_Multiply(&proj_view, &current_proj_c3d, &mv_c3d);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, VSH_FVEC_projView, &proj_view);
}

void gfx_write_buffers(bool color, bool depth, bool depth_test) {
    C3D_DepthTest(depth_test, GPU_ALWAYS, depth ? GPU_WRITE_ALL : GPU_WRITE_COLOR);
}

void gfx_cull_func(enum cull_func func) {
    C3D_CullFace(GPU_CULL_NONE); 
}

void gfx_draw_quads(size_t vertex_count, const int16_t* vertices, const uint8_t* colors, const uint16_t* texcoords) {
    if (vertex_count == 0) return;

    typedef struct {
        float position[3]; 
        uint8_t color[4];
        float texcoord[2]; 
    } temp_vbo; 

    size_t required_memory = vertex_count * sizeof(temp_vbo);
    if (ui_vbo_offset + required_memory > UI_VBO_MAX_SIZE) return;

    temp_vbo* vbo = (temp_vbo*)((uint8_t*)ui_vbo + ui_vbo_offset);
    
    for(size_t k = 0; k < vertex_count; k++) {
        vbo[k].position[0] = (float)vertices[k * 3 + 0];
        vbo[k].position[1] = (float)vertices[k * 3 + 1];
        vbo[k].position[2] = (float)vertices[k * 3 + 2];
        memcpy(vbo[k].color, &colors[k * 4], 4);
        vbo[k].texcoord[0] = (float)texcoords[k * 2 + 0];
        vbo[k].texcoord[1] = (float)texcoords[k * 2 + 1];
    }
    
    GSPGPU_FlushDataCache(vbo, required_memory);

    C3D_BufInfo* bufInfo = C3D_GetBufInfo();
    BufInfo_Init(bufInfo);
    BufInfo_Add(bufInfo, vbo, sizeof(temp_vbo), 3, 0x210); 

    C3D_DrawElements(GPU_TRIANGLES, (vertex_count / 4) * 6, C3D_UNSIGNED_SHORT, quad_indices);
    ui_vbo_offset += required_memory;
}

int gfx_width() { return 400; }
int gfx_height() { return 240; }

void gfx_finish(bool vsync) { ui_vbo_offset = 0; }
void gfx_flip_buffers(float* gpu_wait, float* vsync_wait) {}
void gfx_matrix_texture(bool enable, mat4 tex) {}
void gfx_fog(bool enable) {}
void gfx_fog_color(uint8_t r, uint8_t g, uint8_t b) {}
void gfx_fog_pos(float dx, float dz, float distance) {}
void gfx_lighting(bool enable) {}
void gfx_update_light(float daytime, const float* light_lookup) {}
void gfx_blending(enum gfx_blend mode) {}
void gfx_alpha_test(bool enable) {}
void gfx_depth_func(enum depth_func func) {}
void gfx_depth_range(float near, float far) {}
void gfx_scissor(bool enable, uint32_t x, uint32_t y, uint32_t width, uint32_t height) {}
void gfx_texture(bool enable) {}
void gfx_bind_texture(struct tex_gfx* tex) {}
void gfx_texture_constant(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {}
void gfx_draw_lines(size_t vertex_count, const int16_t* vertices, const uint8_t* colors) {}
void gfx_copy_framebuffer(uint8_t* dest, size_t* width, size_t* height) {}
float gfx_lookup_light(uint8_t light) { return 1.0f; }
void tex_gfx_lookup(struct tex_gfx* tex, int x, int y, uint8_t* color) {}
