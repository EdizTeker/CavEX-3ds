#include <assert.h>
#include "../cglm/cglm.h"
#include "gfx.h"
#include "input.h"

// ==========================================
// PC & WII STUBS (Kept so the compiler doesn't complain)
// ==========================================
#ifdef PLATFORM_PC
void input_init() {}
void input_poll() {}
void input_native_key_status(int key, bool* p, bool* r, bool* h) {}
bool input_native_key_symbol(int k, int* s, int* sh, enum input_category* c, int* p) { return false; }
bool input_native_key_any(int* key) { return false; }
void input_pointer_enable(bool enable) {}
bool input_pointer(float* x, float* y, float* angle) { return false; }
void input_native_joystick(float dt, float* dx, float* dy) {}
#endif

#ifdef PLATFORM_WII
void input_init() {}
void input_poll() {}
void input_native_key_status(int key, bool* p, bool* r, bool* h) {}
bool input_native_key_symbol(int k, int* s, int* sh, enum input_category* c, int* p) { return false; }
bool input_native_key_any(int* key) { return false; }
void input_pointer_enable(bool enable) {}
bool input_pointer(float* x, float* y, float* angle) { return false; }
void input_native_joystick(float dt, float* dx, float* dy) {}
#endif

// ==========================================
// 3DS HARDWARE CORE
// ==========================================
#ifdef PLATFORM_3DS
#include <3ds.h>

static u32 kDown = 0;
static u32 kHeld = 0;
static u32 kUp = 0;
static circlePosition circlePad;
static touchPosition touch;

void input_init() {}

void input_poll() {

	if (!aptMainLoop()) {
	        exit(0); 
	}

    hidScanInput();
    kDown = hidKeysDown();
    kHeld = hidKeysHeld();
    kUp = hidKeysUp();
    hidCircleRead(&circlePad);
    hidTouchRead(&touch);
}

void input_native_key_status(int key, bool* pressed, bool* released, bool* held) {
    *pressed = (kDown & key);
    *released = (kUp & key);
    *held = (kHeld & key);
}

bool input_native_key_symbol(int key, int* symbol, int* symbol_help, enum input_category* category, int* priority) {
    *category = INPUT_CAT_NONE;
    *priority = 1;
    *symbol = 7;
    *symbol_help = 7;
    return true;
}

bool input_native_key_any(int* key) { return false; }
void input_pointer_enable(bool enable) { }

bool input_pointer(float* x, float* y, float* angle) {
    if (kHeld & KEY_TOUCH) {
        *x = ((float)touch.px / 320.0f) * 400.0f;
        *y = touch.py; 
        *angle = 0.0f;
        return true;
    }
    // Park the cursor right on top of the Singleplayer button (Top white box)
    *x = 200.0f;
    *y = 60.0f; 
    *angle = 0.0f;
    return true; 
}

void input_native_joystick(float dt, float* dx, float* dy) {
    if (circlePad.dx > 15 || circlePad.dx < -15) {
        *dx = ((float)circlePad.dx / 156.0f) * dt;
    } else { *dx = 0.0f; }
    
    if (circlePad.dy > 15 || circlePad.dy < -15) {
        *dy = ((float)circlePad.dy / -156.0f) * dt;
    } else { *dy = 0.0f; }
}
#endif

// ==========================================
// ENGINE TRANSLATION OVERRIDE
// ==========================================
#include "../game/game_state.h"

static const char* input_config_translate(enum input_button key) { return NULL; }
bool input_symbol(enum input_button b, int* symbol, int* symbol_help, enum input_category* category) { return false; }

bool input_pressed(enum input_button b) {
#ifdef PLATFORM_3DS
    // HARD-WIRED 3DS CONTROLS: Bypass the config file completely!
    if (b == IB_GUI_CLICK || b == IB_GUI_CLICK_ALT || b == IB_ACTION1) return (kDown & (KEY_A | KEY_TOUCH)) != 0;
    if (b == IB_ACTION2) return (kDown & KEY_B) != 0;
    if (b == IB_JUMP) return (kDown & KEY_A) != 0;
    if (b == IB_SNEAK) return (kDown & KEY_L) != 0;
    if (b == IB_INVENTORY) return (kDown & KEY_X) != 0;
    if (b == IB_HOME) return (kDown & KEY_START) != 0;
    
    // D-Pad Menu Navigation
    if (b == IB_GUI_UP) return (kDown & KEY_DUP) != 0;
    if (b == IB_GUI_DOWN) return (kDown & KEY_DDOWN) != 0;
    if (b == IB_GUI_LEFT) return (kDown & KEY_DLEFT) != 0;
    if (b == IB_GUI_RIGHT) return (kDown & KEY_DRIGHT) != 0;
    return false;
#else
    return false; // PC/Wii disabled for this clean compile
#endif
}

bool input_released(enum input_button b) {
#ifdef PLATFORM_3DS
    if (b == IB_GUI_CLICK || b == IB_GUI_CLICK_ALT || b == IB_ACTION1) return (kUp & (KEY_A | KEY_TOUCH)) != 0;
    if (b == IB_ACTION2) return (kUp & KEY_B) != 0;
    if (b == IB_JUMP) return (kUp & KEY_A) != 0;
    if (b == IB_SNEAK) return (kUp & KEY_L) != 0;
    if (b == IB_INVENTORY) return (kUp & KEY_X) != 0;
    if (b == IB_HOME) return (kUp & KEY_START) != 0;
    if (b == IB_GUI_UP) return (kUp & KEY_DUP) != 0;
    if (b == IB_GUI_DOWN) return (kUp & KEY_DDOWN) != 0;
    if (b == IB_GUI_LEFT) return (kUp & KEY_DLEFT) != 0;
    if (b == IB_GUI_RIGHT) return (kUp & KEY_DRIGHT) != 0;
    return false;
#else
    return false;
#endif
}

bool input_held(enum input_button b) {
#ifdef PLATFORM_3DS
    if (b == IB_GUI_CLICK || b == IB_GUI_CLICK_ALT || b == IB_ACTION1) return (kHeld & (KEY_A | KEY_TOUCH)) != 0;
    if (b == IB_ACTION2) return (kHeld & KEY_B) != 0;
    if (b == IB_JUMP) return (kHeld & KEY_A) != 0;
    if (b == IB_SNEAK) return (kHeld & KEY_L) != 0;
    if (b == IB_INVENTORY) return (kHeld & KEY_X) != 0;
    if (b == IB_HOME) return (kHeld & KEY_START) != 0;
    if (b == IB_GUI_UP) return (kHeld & KEY_DUP) != 0;
    if (b == IB_GUI_DOWN) return (kHeld & KEY_DDOWN) != 0;
    if (b == IB_GUI_LEFT) return (kHeld & KEY_DLEFT) != 0;
    if (b == IB_GUI_RIGHT) return (kHeld & KEY_DRIGHT) != 0;
    return false;
#else
    return false;
#endif
}

bool input_joystick(float dt, float* x, float* y) {
#ifdef PLATFORM_3DS
    input_native_joystick(dt, x, y);
    return true;
#else
    return false;
#endif
}
