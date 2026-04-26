#include <3ds.h>
#include <citro3d.h>
#include <stdlib.h>
#include "../texture.h"

void tex_init_pre(void) {}

void tex_gfx_load(struct tex_gfx* tex, void* img, size_t width, size_t height, enum tex_format type, bool linear) {
    // Free the image from RAM so the engine doesn't leak memory, 
    // but do not attempt to swizzle or push to the 3DS GPU right now.
    if (img) {
        free(img);
    }
}

void tex_gfx_bind(struct tex_gfx* tex, int slot) {
    // SATISFIES THE LINKER.
    // Leaves the GPU texture state blank for our telemetry test.
}

void tex_gfx_wrap_mode(struct tex_gfx* tex, bool repeat) {
    // Stub
}
