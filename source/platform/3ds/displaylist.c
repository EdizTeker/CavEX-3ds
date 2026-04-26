	#include "../displaylist.h"
	#include <3ds.h>
	#include <citro3d.h>
	#include <stdlib.h>
	#include <string.h>
	
	#define MAX_VERTICES 32768 
	#define INITIAL_CAPACITY 512
	
	typedef struct {
	    float position[3]; 
	    uint8_t color[4];    
	    float texcoord[2]; 
	} vertex_3ds; 
	
	// ==========================================
	// PIPELINE A: 10MB TERRAIN RING BUFFER
	// Permanently holds static chunks. Never frees.
	// ==========================================
	#define TERRAIN_RING_SIZE (1024 * 1024 * 8)
	static uint8_t* terrain_ring_buffer = NULL;
	static size_t terrain_ring_offset = 0;
	
	static void* ring_alloc_terrain(size_t size) {
	    size = (size + 127) & ~127; 
	    if (!terrain_ring_buffer) terrain_ring_buffer = (uint8_t*)linearAlloc(TERRAIN_RING_SIZE);
	    if (!terrain_ring_buffer) return NULL; 
	    
	    if (terrain_ring_offset + size > TERRAIN_RING_SIZE) terrain_ring_offset = 0; 
	    
	    void* ptr = terrain_ring_buffer + terrain_ring_offset;
	    terrain_ring_offset += size;
	    return ptr;
	}
	
	// ==========================================
	// PIPELINE B: 2MB UI RING BUFFER
	// Handles 60FPS dynamic geometry safely.
	// ==========================================
	#define UI_RING_SIZE (1024 * 1024 * 2) 
	static uint8_t* ui_ring_buffer = NULL;
	static size_t ui_ring_offset = 0;
	
	static void* ring_alloc_ui(size_t size) {
	    size = (size + 127) & ~127; 
	    if (!ui_ring_buffer) ui_ring_buffer = (uint8_t*)linearAlloc(UI_RING_SIZE);
	    if (!ui_ring_buffer) return NULL; 
	    
	    if (ui_ring_offset + size > UI_RING_SIZE) ui_ring_offset = 0; 
	    
	    void* ptr = ui_ring_buffer + ui_ring_offset;
	    ui_ring_offset += size;
	    return ptr;
	}
	// ==========================================
	
	void displaylist_init(struct displaylist* l, size_t vertices, size_t vertex_size) {
	    l->direct_color = false; // Ignore the parameter, hardcode the struct state
	    l->index = INITIAL_CAPACITY; 
	    l->length = 0;              
	    l->finished = false;         
	    l->is_scratch = false; 
	    
	    l->data = malloc(l->index * sizeof(vertex_3ds));
	    
	    memset(l->current_pos, 0, sizeof(l->current_pos));
	    memset(l->current_color, 0, sizeof(l->current_color));
	    memset(l->current_texcoord, 0, sizeof(l->current_texcoord));
	}
	
	void displaylist_init_scratch(struct displaylist* l, void* scratch_buffer, size_t capacity) {
	    l->direct_color = false;
	    l->index = capacity; 
	    l->length = 0;
	    l->finished = false;
	    l->is_scratch = true; 
	    l->data = scratch_buffer; 
	    
	    memset(l->current_pos, 0, sizeof(l->current_pos));
	    memset(l->current_color, 0, sizeof(l->current_color));
	    memset(l->current_texcoord, 0, sizeof(l->current_texcoord));
	}
	
	void displaylist_destroy(struct displaylist* l) {
	    // SHIELD: The GPU is protected. linearFree is permanently banned.
	    if (l->data && !l->finished && !l->is_scratch) {
	        free(l->data); // We only free CPU RAM if it wasn't finalized to FCRAM yet
	    }
	    l->data = NULL;
	    l->index = 0;
	    l->finished = false;
	}
	
	void displaylist_reset(struct displaylist* l) {
	    if (l->data && !l->finished && !l->is_scratch) {
	        free(l->data);
	    }
	    l->index = INITIAL_CAPACITY;
	    l->length = 0; 
	    l->finished = false;
	    l->is_scratch = false;
	    l->data = malloc(l->index * sizeof(vertex_3ds));
	}
	int telemetry_chunks_meshed = 0;
	
	void displaylist_finalize(struct displaylist* l, uint16_t vtxcnt) {
	    if (vtxcnt > 0) l->length = vtxcnt;
	    if (l->finished || l->length == 0 || !l->data) return;
	
	    size_t valid_vertices = (l->length / 4) * 4;
	    if (valid_vertices == 0) return;
	
	    size_t exact_bytes = valid_vertices * sizeof(vertex_3ds);
	    
	    // Unconditionally route finalized chunks to the Terrain Ring Buffer
	    vertex_3ds* final_vbo = (vertex_3ds*)ring_alloc_terrain(exact_bytes);
	    
	    if (final_vbo) {
	        void* old_data = l->data;
	        memcpy(final_vbo, old_data, exact_bytes);
	        
	        // CRITICAL: Stop the Memory Leak. Free the temporary CPU heap.
	        free(old_data); 
	        
	        l->data = final_vbo; 
	        l->finished = true; 
	        l->length = valid_vertices; 
	        
	        GSPGPU_FlushDataCache(l->data, exact_bytes);
	        telemetry_chunks_meshed++;
	    }
	}
	void displaylist_pos(struct displaylist* l, int16_t x, int16_t y, int16_t z) {
	    if (l->finished || !l->data) return; 
	    l->current_pos[0] = (float)x;
	    l->current_pos[1] = (float)y;
	    l->current_pos[2] = (float)z;
	}
	
	void displaylist_color(struct displaylist* l, uint8_t index) {
	    l->current_color[0] = 255; l->current_color[1] = 255;
	    l->current_color[2] = 255; l->current_color[3] = 255;
	}
	
	void displaylist_color_rgba(struct displaylist* l, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
	    l->current_color[0] = r; l->current_color[1] = g;
	    l->current_color[2] = b; l->current_color[3] = a;
	}
	
    void displaylist_texcoord(struct displaylist* l, uint8_t s, uint8_t t) {
        if (l->finished || !l->data) return;
    
        if (l->length >= l->index) {
            if (l->is_scratch) return; 
            
            if (l->index >= MAX_VERTICES) return;
            size_t new_cap = l->index * 2;
            if (new_cap > MAX_VERTICES) new_cap = MAX_VERTICES;
            
            void* new_vbo = realloc(l->data, new_cap * sizeof(vertex_3ds));
            if (!new_vbo) {
                // OUT OF BOUNDS ARMOR: If RAM is full, safely lock the chunk and stop drawing
                l->finished = true; 
                return; 
            }
            l->data = new_vbo;
            l->index = new_cap;
        }
    
        l->current_texcoord[0] = (float)s / 256.0f;
        l->current_texcoord[1] = (float)t / 256.0f;
    
        vertex_3ds* vbo = (vertex_3ds*)l->data;
        vbo[l->length].position[0] = l->current_pos[0];
        vbo[l->length].position[1] = l->current_pos[1];
        vbo[l->length].position[2] = l->current_pos[2];
        
        vbo[l->length].color[0] = l->current_color[0];
        vbo[l->length].color[1] = l->current_color[1];
        vbo[l->length].color[2] = l->current_color[2];
        vbo[l->length].color[3] = l->current_color[3];
        
        vbo[l->length].texcoord[0] = l->current_texcoord[0];
        vbo[l->length].texcoord[1] = l->current_texcoord[1];
        
        l->length++;
    }
	
	int telemetry_drawn_vertices = 0;
	extern u16* quad_indices; 
	
	void displaylist_render(struct displaylist* l) {
	    if (l->length == 0 || !l->finished || !l->data) return; 
	
	    // RECORD THE HARDWARE COMMAND
	    telemetry_drawn_vertices += l->length;
	
	    C3D_BufInfo* bufInfo = C3D_GetBufInfo();
	    BufInfo_Init(bufInfo);
	    BufInfo_Add(bufInfo, l->data, sizeof(vertex_3ds), 3, 0x210);
	    C3D_DrawElements(GPU_TRIANGLES, (l->length / 4) * 6, C3D_UNSIGNED_SHORT, quad_indices);
	}
	
	void displaylist_render_immediate(struct displaylist* l, uint16_t vtxcnt) {}
	void gfx_draw_quads_flt(size_t vertex_count, const float* vertices, const uint8_t* colors, const float* texcoords) {}
