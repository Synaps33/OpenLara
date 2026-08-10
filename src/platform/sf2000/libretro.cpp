#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "libretro.h"

#define _GAPI_SW 1
#define _OS_LINUX 1 // to trigger RGB565 mode in sw.h

#include "src/core.h"
#include "src/game.h"
#include <new>

static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_environment_t environ_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;
static retro_log_printf_t log_cb;

void* operator new(size_t size) {
    void* p = malloc(size);
    if (!p) {
        if (log_cb) log_cb(RETRO_LOG_ERROR, "[DIAG] std::bad_alloc in operator new (size=%u)\n", (unsigned int)size);
        abort();
    }
    return p;
}

void* operator new[](size_t size) {
    void* p = malloc(size);
    if (!p) {
        if (log_cb) log_cb(RETRO_LOG_ERROR, "[DIAG] std::bad_alloc in operator new[] (size=%u)\n", (unsigned int)size);
        abort();
    }
    return p;
}

void operator delete(void* p) noexcept {
    free(p);
}

void operator delete[](void* p) noexcept {
    free(p);
}

static uint16_t framebuffer[320 * 240];
static int current_width = 320;
static int current_height = 240;

static bool opt_audio_enable = true;
static int opt_frameskip = 0;
static int opt_controls = 0; // 0 = Classic, 1 = Modern
static bool is_inited = false;

unsigned int startTime = 0;

int osGetTimeMS() {
    struct timeval t;
    gettimeofday(&t, NULL);
    return (int)((t.tv_sec - startTime) * 1000 + t.tv_usec / 1000);
}

void* osMutexInit() { return NULL; }
void osMutexFree(void *obj) {}
void osMutexLock(void *obj) {}
void osMutexUnlock(void *obj) {}

bool osJoyReady(int index) { return index == 0; }
void osJoyVibrate(int index, float L, float R) {}

const char* osFixFileName(const char* fileName) {
    return fileName;
}

#define SND_FRAME_SIZE 4
#define SND_FRAMES 1024
Sound::Frame *sndData = NULL;

void retro_set_environment(retro_environment_t cb) {
    environ_cb = cb;
    struct retro_log_callback log;
    if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log)) {
        log_cb = log.log;
    }

    struct retro_variable vars[] = {
        { "openlara_resolution", "Resolution; 50%|75%|100%|25%" },
        { "openlara_framerate", "Framerate; 30 FPS|15 FPS (Frameskip)" },
        { "openlara_audio", "Audio; Enabled|Disabled" },
        { "openlara_controls", "Controls; Classic|Modern" },
        { NULL, NULL },
    };
    environ_cb(RETRO_ENVIRONMENT_SET_VARIABLES, vars);
}

void retro_set_video_refresh(retro_video_refresh_t cb) { video_cb = cb; }
void retro_set_audio_sample(retro_audio_sample_t cb) { audio_cb = cb; }
void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) { audio_batch_cb = cb; }
void retro_set_input_poll(retro_input_poll_t cb) { input_poll_cb = cb; }
void retro_set_input_state(retro_input_state_t cb) { input_state_cb = cb; }

void retro_init(void) {
    if (is_inited) return;
    
    struct timeval t;
    gettimeofday(&t, NULL);
    startTime = t.tv_sec;

    sndData = new Sound::Frame[SND_FRAMES];
    memset(sndData, 0, SND_FRAMES * SND_FRAME_SIZE);

    GAPI::swColor = framebuffer;
    
    Core::width = current_width;
    Core::height = current_height;
    
    is_inited = true;
}

void retro_deinit(void) {
    if (!is_inited) return;
    if (sndData) {
        delete[] sndData;
        sndData = NULL;
    }
    Game::deinit();
    is_inited = false;
}

unsigned retro_api_version(void) { return RETRO_API_VERSION; }

void retro_get_system_info(struct retro_system_info *info) {
    info->library_name = "OpenLara";
    info->library_version = "0.1";
    info->need_fullpath = true;
    info->block_extract = false;
    info->valid_extensions = "gba|bin|phd";
}

void retro_get_system_av_info(struct retro_system_av_info *info) {
    info->geometry.base_width = 320;
    info->geometry.base_height = 240;
    info->geometry.max_width = 320;
    info->geometry.max_height = 240;
    info->geometry.aspect_ratio = 320.0f / 240.0f;
    info->timing.fps = 30.0;
    info->timing.sample_rate = 44100.0;
}

void retro_set_controller_port_device(unsigned port, unsigned device) {}
void retro_reset(void) {}

void update_settings() {
    struct retro_variable var = {0};

    var.key = "openlara_resolution";
    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        if (strcmp(var.value, "100%") == 0) { current_width = 320; current_height = 240; }
        else if (strcmp(var.value, "75%") == 0) { current_width = 240; current_height = 180; }
        else if (strcmp(var.value, "50%") == 0) { current_width = 160; current_height = 120; }
        else if (strcmp(var.value, "25%") == 0) { current_width = 80; current_height = 60; }
    } else {
        current_width = 160; current_height = 120; // Default to 50% for performance
    }
    
    Core::width = current_width;
    Core::height = current_height;

    var.key = "openlara_framerate";
    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        opt_frameskip = (strcmp(var.value, "15 FPS (Frameskip)") == 0) ? 1 : 0;
    }

    var.key = "openlara_audio";
    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        opt_audio_enable = (strcmp(var.value, "Disabled") != 0);
    }

    var.key = "openlara_controls";
    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        opt_controls = (strcmp(var.value, "Modern") == 0) ? 1 : 0;
    }
}

bool retro_load_game(const struct retro_game_info *info) {
    enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_RGB565;
    if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt)) {
        if (log_cb) log_cb(RETRO_LOG_ERROR, "[OpenLara] RGB565 not supported.\n");
        return false;
    }

    update_settings();
    
    // CRITICAL: Initialize global path buffers to empty strings
    // On bare metal these are uninitialized and contain garbage!
    contentDir[0] = '\0';
    cacheDir[0] = '\0';
    saveDir[0] = '\0';

    char level_path[512] = "";
    if (info && info->path && strlen(info->path) > 0) {
        strncpy(level_path, info->path, sizeof(level_path) - 1);
        
        // If it's a dummy rom (e.g., .gba), change it to level1.phd
        if (strstr(level_path, ".gba") || strstr(level_path, ".GBA") || strstr(level_path, ".bin") || strstr(level_path, ".BIN")) {
            char *last_slash = strrchr(level_path, '/');
            if (last_slash) {
                *last_slash = '\0';
                strcat(level_path, "/level1.phd");
            }
        }
    } else {
        snprintf(level_path, sizeof(level_path), "/mnt/sda1/roms/GBA/level1.phd");
    }

    if (log_cb) log_cb(RETRO_LOG_INFO, "[OpenLara] Loading level: %s\n", level_path);
    if (log_cb) log_cb(RETRO_LOG_INFO, "[OpenLara] contentDir: '%s'\n", contentDir);

    Game::init(level_path);
    GAPI::resize(); // Allocate depth buffer
    return true;
}

bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info) { return false; }
void retro_unload_game(void) { retro_deinit(); }
unsigned retro_get_region(void) { return RETRO_REGION_NTSC; }

void process_inputs() {
    if (input_poll_cb) input_poll_cb();
    
    struct {
        unsigned libretro_id;
        JoyKey openlara_id;
    } mapping_classic[] = {
        { RETRO_DEVICE_ID_JOYPAD_A, jkA },      // Action (Right Button)
        { RETRO_DEVICE_ID_JOYPAD_B, jkB },      // Jump (Bottom Button)
        { RETRO_DEVICE_ID_JOYPAD_X, jkX },      // Roll (Top Button)
        { RETRO_DEVICE_ID_JOYPAD_Y, jkY },      // Draw Weapon (Left Button)
        { RETRO_DEVICE_ID_JOYPAD_L, jkLB },     // Walk
        { RETRO_DEVICE_ID_JOYPAD_R, jkA },      // HARDCODED R -> Action (Shoot)
        { RETRO_DEVICE_ID_JOYPAD_SELECT, jkRB }, // Look / Step
        { RETRO_DEVICE_ID_JOYPAD_START, jkSelect }, // Inventory
        { RETRO_DEVICE_ID_JOYPAD_UP, jkUp },
        { RETRO_DEVICE_ID_JOYPAD_DOWN, jkDown },
        { RETRO_DEVICE_ID_JOYPAD_LEFT, jkLeft },
        { RETRO_DEVICE_ID_JOYPAD_RIGHT, jkRight }
    };

    struct {
        unsigned libretro_id;
        JoyKey openlara_id;
    } mapping_modern[] = {
        { RETRO_DEVICE_ID_JOYPAD_A, jkB },      // Jump
        { RETRO_DEVICE_ID_JOYPAD_B, jkA },      // Action
        { RETRO_DEVICE_ID_JOYPAD_X, jkY },      // Draw Weapon
        { RETRO_DEVICE_ID_JOYPAD_Y, jkX },      // Roll
        { RETRO_DEVICE_ID_JOYPAD_L, jkLB },     // Walk
        { RETRO_DEVICE_ID_JOYPAD_R, jkA },      // HARDCODED R -> Action (Shoot)
        { RETRO_DEVICE_ID_JOYPAD_SELECT, jkRB }, // Look / Step
        { RETRO_DEVICE_ID_JOYPAD_START, jkSelect }, // Inventory
        { RETRO_DEVICE_ID_JOYPAD_UP, jkUp },
        { RETRO_DEVICE_ID_JOYPAD_DOWN, jkDown },
        { RETRO_DEVICE_ID_JOYPAD_LEFT, jkLeft },
        { RETRO_DEVICE_ID_JOYPAD_RIGHT, jkRight }
    };

    if (opt_controls == 1) {
        for (int i = 0; i < sizeof(mapping_modern) / sizeof(mapping_modern[0]); i++) {
            int state = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, mapping_modern[i].libretro_id);
            Input::setJoyDown(0, mapping_modern[i].openlara_id, state);
        }
    } else {
        for (int i = 0; i < sizeof(mapping_classic) / sizeof(mapping_classic[0]); i++) {
            int state = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, mapping_classic[i].libretro_id);
            Input::setJoyDown(0, mapping_classic[i].openlara_id, state);
        }
    }
}

static int frame_counter = 0;

void retro_run(void) {
    if (Core::isQuit) {
        environ_cb(RETRO_ENVIRONMENT_SHUTDOWN, NULL);
        return;
    }

    bool updated = false;
    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated) {
        update_settings();
    }

    process_inputs();

    bool render_this_frame = true;
    if (opt_frameskip > 0) {
        frame_counter++;
        if (frame_counter > opt_frameskip) {
            frame_counter = 0;
            render_this_frame = true;
        } else {
            render_this_frame = false;
        }
    }

    if (Game::update()) {
        if (render_this_frame) {
            Game::render();
            video_cb(framebuffer, current_width, current_height, current_width * 2);
        } else {
            // Skips drawing to screen buffer
            video_cb(NULL, current_width, current_height, current_width * 2);
        }
    } else {
        video_cb(NULL, current_width, current_height, current_width * 2);
    }
    
    if (audio_batch_cb) {
        if (opt_audio_enable) {
            Sound::fill(sndData, SND_FRAMES);
        } else {
            memset(sndData, 0, SND_FRAMES * SND_FRAME_SIZE);
        }
        audio_batch_cb((const int16_t*)sndData, SND_FRAMES);
    }
}

void *retro_get_memory_data(unsigned id) { return NULL; }
size_t retro_get_memory_size(unsigned id) { return 0; }
size_t retro_serialize_size(void) { return 0; }
bool retro_serialize(void *data, size_t size) { return false; }
bool retro_unserialize(const void *data, size_t size) { return false; }
void retro_cheat_reset(void) {}
void retro_cheat_set(unsigned index, bool enabled, const char *code) {}
