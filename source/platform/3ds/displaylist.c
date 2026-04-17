#include <3ds.h>
#include <citro3d.h>
#include <stdlib.h>
#include <string.h>

#define MAX_VERTICES 16384 

typedef struct {
    float position[3]; 
    uint8_t color[4];    
    float texcoord[2]; 
} vertex_3ds; 

struct displaylist {
    vertex_3ds* vbo;       
    size_t capacity;       
    size_t vertex_count;  
    bool direct_color;
    vertex_3ds current;   
    bool is_linear; 
};

// THE CRITICAL FIX: The Global Staging Buffer.
// Reuses one static 393KB block of RAM for meshing, completely eradicating malloc OOM crashes.
static vertex_3ds staging_buffer[MAX_VERTICES];

void displaylist_init(struct displaylist* l, size_t vertices, bool direct_color) {
    l->direct_color = direct_color;
    l->capacity = MAX_VERTICES; 
    l->vertex_count = 0;
    l->is_linear = false; 
    
    // Point the VBO directly to the shared staging buffer
    l->vbo = staging_buffer;
    memset(&l->current, 0, sizeof(vertex_3ds));
}

void displaylist_destroy(struct displaylist* l) {
    if (l->vbo && l->is_linear) {
        linearFree(l->vbo);
    }
    l->vbo = NULL;
}

void displaylist_reset(struct displaylist* l) {
    if (l->vbo && l->is_linear) {
        linearFree(l->vbo);
    }
    l->vertex_count = 0; 
    l->is_linear = false;
    
    // Return to the staging buffer
    l->vbo = staging_buffer;
}

void displaylist_finalize(struct displaylist* l, uint16_t vtxcnt) {
    if (l->is_linear || l->vertex_count == 0) return;

    // Allocate ONLY the exact required bytes in the GPU's Linear Heap
    vertex_3ds* final_vbo = (vertex_3ds*)linearAlloc(l->vertex_count * sizeof(vertex_3ds));
    if (final_vbo) {
        // Copy out of the staging buffer into permanent GPU memory
        memcpy(final_vbo, staging_buffer, l->vertex_count * sizeof(vertex_3ds));
        l->vbo = final_vbo; 
        l->is_linear = true; 
        GSPGPU_FlushDataCache(l->vbo, l->vertex_count * sizeof(vertex_3ds));
    } else {
        // Safe fallback if the GPU runs out of VRAM
        l->vbo = NULL;
        l->vertex_count = 0;
    }
}

void displaylist_pos(struct displaylist* l, int16_t x, int16_t y, int16_t z) {
    // Lock modification if already committed to GPU
    if (l->is_linear) return; 

    // Hard clamp to prevent buffer overflow
    if (l->vertex_count >= l->capacity) {
        return; 
    }
    
    l->current.position[0] = (float)x;
    l->current.position[1] = (float)y;
    l->current.position[2] = (float)z;
}

void displaylist_color(struct displaylist* l, uint8_t index) {
    l->current.color[0] = 255; l->current.color[1] = 255;
    l->current.color[2] = 255; l->current.color[3] = 255;
}

void displaylist_color_rgba(struct displaylist* l, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    l->current.color[0] = r; l->current.color[1] = g;
    l->current.color[2] = b; l->current.color[3] = a;
}

void displaylist_texcoord(struct displaylist* l, uint8_t s, uint8_t t) {
    if (!l->vbo || l->vertex_count >= l->capacity || l->is_linear) return;

    l->current.texcoord[0] = (float)s;
    l->current.texcoord[1] = (float)t;
    l->vbo[l->vertex_count++] = l->current;
}

extern u16* quad_indices; 

void displaylist_render(struct displaylist* l) {
    if (l->vertex_count == 0 || !l->is_linear || !l->vbo) return; 

    C3D_BufInfo* bufInfo = C3D_GetBufInfo();
    BufInfo_Init(bufInfo);
    BufInfo_Add(bufInfo, l->vbo, sizeof(vertex_3ds), 3, 0x210);
    C3D_DrawElements(GPU_TRIANGLES, (l->vertex_count / 4) * 6, C3D_UNSIGNED_SHORT, quad_indices);
}

void displaylist_render_immediate(struct displaylist* l, uint16_t vtxcnt) {}
void gfx_draw_quads_flt(size_t vertex_count, const float* vertices, const uint8_t* colors, const float* texcoords) {}
