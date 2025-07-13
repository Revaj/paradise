#include "platform.h"

#if __linux__

#include "../core/logger.h"
#include "../core/event.h"
#include "../core/input.h"

#include "../containers/darray.h"
#include <xcb/xcb.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>

#include <wayland-client.h>
#include <wayland-client-protocol.h>
#include "protocol.h"
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>

#if _POSIX_C_SOURCE >= 199309L
#include <time.h>
#else
#include <unistd.h>
#endif

typedef struct internal_state {
    struct wl_display *wl_display;
    struct wl_registry *wl_registry;
    struct wl_shm *wl_shm;
    struct wl_compositor *wl_compositor;
    struct xdg_wm_base *xdg_wm_base;
    struct wl_surface *wl_surface;
    struct xdg_surface *xdg_surface;
    struct xdg_toplevel *xdg_toplevel;

} internal_state;

keys translate_keycode(uint32_t x_keycode);


struct wl_buffer * draw_frame(struct internal_state *state);
int allocate_shm_file(size_t size);
int create_shm_file(void);
void xdg_surface_configure(void *data,
        struct xdg_surface *xdg_surface, uint32_t serial);
void wl_buffer_release(void *data, struct wl_buffer *wl_buffer);
void randname(char *buf);

static const struct wl_buffer_listener wl_buffer_listener = {
    .release = wl_buffer_release,
};

struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_configure,
};

void registry_global(void *data, struct wl_registry *wl_registry,
                     uint32_t name, const char *interface, uint32_t version);


struct wl_registry_listener wl_registry_listener = {
    .global = registry_global,
};


uint8_t platform_startup(
    platform_state* plat_state,
    const char* application_name,
    int32_t x,
    int32_t y,
    int32_t width,
    int32_t height) {

    plat_state->internal_state = malloc(sizeof(internal_state));
    internal_state* state = (internal_state*)plat_state->internal_state;

    state->wl_display = wl_display_connect(0);
    if (!state->wl_display) {
        KFATAL("Failed to create wayland display");
        return 0;
    }

    state->wl_registry = wl_display_get_registry(state->wl_display);

    wl_registry_add_listener(state->wl_registry, &wl_registry_listener, state);
    wl_display_roundtrip(state->wl_display);

    
    state->wl_surface = wl_compositor_create_surface(state->wl_compositor);

    state->xdg_surface = xdg_wm_base_get_xdg_surface(
            state->xdg_wm_base, state->wl_surface);

    xdg_surface_add_listener(state->xdg_surface, &xdg_surface_listener, state);
    state->xdg_toplevel = xdg_surface_get_toplevel(state->xdg_surface);
    
    xdg_toplevel_set_title(state->xdg_toplevel, application_name);
    wl_surface_commit(state->wl_surface);

    return 1;
}

void randname(char *buf)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    long r = ts.tv_nsec;
    for (int i = 0; i < 6; ++i) {
        buf[i] = 'A'+(r&15)+(r&16)*2;
        r >>= 5;
    }
}

int create_shm_file(void)
{
    int retries = 100;
    do {
        char name[] = "/wl_shm-XXXXXX";
        randname(name + sizeof(name) - 7);
        --retries;
        int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
        if (fd >= 0) {
            shm_unlink(name);
            return fd;
        }
    } while (retries > 0 && errno == EEXIST);
    return -1;
}

int allocate_shm_file(size_t size) {
    int fd = create_shm_file();
    if (fd < 0)
        return -1;
    int ret;
    do {
        ret = ftruncate(fd, size);
    } while (ret < 0 && errno == EINTR);
    if (ret < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

void wl_buffer_release(void *data, struct wl_buffer *wl_buffer)
{
    wl_buffer_destroy(wl_buffer);
}


struct wl_buffer * draw_frame(struct internal_state *state)
{
    const int width = 640, height = 480;
    int stride = width * 4;
    int size = stride * height;

    int fd = allocate_shm_file(size);
    if (fd == -1) {
        return NULL;
    }

    uint32_t *data = mmap(NULL, size,
            PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        close(fd);
        return NULL;
    }

    struct wl_shm_pool *pool = wl_shm_create_pool(state->wl_shm, fd, size);
    struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0,
            width, height, stride, WL_SHM_FORMAT_XRGB8888);
    wl_shm_pool_destroy(pool);
    close(fd);

    /* Draw checkerboxed background */
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if ((x + y / 8 * 8) % 16 < 8)
                data[y * width + x] = 0xFF666666;
            else
                data[y * width + x] = 0xFFEEEEEE;
        }
    }

    munmap(data, size);
    wl_buffer_add_listener(buffer, &wl_buffer_listener, NULL);
    return buffer;
}


void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial)
{
    struct internal_state *state = data;
    xdg_surface_ack_configure(xdg_surface, serial);

    struct wl_buffer *buffer = draw_frame(state);
    wl_surface_attach(state->wl_surface, buffer, 0, 0);
    wl_surface_commit(state->wl_surface);
}

void registry_global(void *data, struct wl_registry *wl_registry,
                     uint32_t name, const char *interface, uint32_t version)
{
    struct internal_state *state = data;
    if (strcmp(interface, wl_shm_interface.name) == 0) {
        state->wl_shm = wl_registry_bind(
                wl_registry, name, &wl_shm_interface, version);
    } else if (strcmp(interface, wl_compositor_interface.name) == 0) {
        state->wl_compositor = wl_registry_bind(
                wl_registry, name, &wl_compositor_interface, version);
    } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
        state->xdg_wm_base = wl_registry_bind(
                wl_registry, name, &xdg_wm_base_interface, version);
    }
}

void platform_shutdown(platform_state* plat_state) {
    internal_state* state = (internal_state*)plat_state->internal_state;


}

uint8_t platform_pump_messages(platform_state* plat_state) {
    internal_state* state = (internal_state*)plat_state->internal_state;


    //TODO: Maybe move this part to other section? future rendering section maybe
    while(wl_display_prepare_read(state->wl_display) != 0) {
        wl_display_dispatch_pending(state->wl_display);
    }

    wl_display_flush(state->wl_display);
    wl_display_read_events(state->wl_display);
    wl_display_dispatch_pending(state->wl_display);



    return 1;
}

void* platform_allocate(uint64_t size, uint8_t aligned) {
    return malloc(size);
}

void platform_free(void* block, uint8_t aligned) {
    return free(block);
}

void* platform_zero_memory(void* block, uint64_t size) {
    return memset(block, 0, size);
}

void* platform_copy_memory(void* dest, const void* source, uint64_t size) {
    return memcpy(dest, source, size);
}

void* platform_set_memory(void* dest, int32_t value, uint64_t size) {
    return memset(dest, value, size);
}

void platform_console_write(const char* message, uint8_t colour) {
    const char* colour_strings[] = { "0;41", "1;31", "1;31", "1;33", "1;32", "1;34", "1;30" };
    printf("\033[%sm%s\033[0m", colour_strings[colour], message);
}

void platform_console_write_error(const char* message, uint8_t colour) {
    const char* colour_strings[] = { "0;41", "1;31", "1;31", "1;33", "1;32", "1;34", "1;30" };
    printf("\033[%sm%s\033[0m", colour_strings[colour], message);
}

double platform_get_absolute_time() {

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return now.tv_sec + now.tv_nsec * 0.000000001;
}

void platform_sleep(uint64_t ms) {
#if _POSIX_C_SOURCE >= 199309L
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000 * 1000;
    nanosleep(&ts, 0);
#else
    if (ms >= 1000) {
        sleep(ms / 1000);
    }
    usleep((ms % 1000) * 1000);
#endif
}

void platform_get_required_extension_names(const char*** names_darray) {
    darray_push(*names_darray, &"VK_KHR_xcb_surface");
}

keys translate_keycode(uint32_t x_keycode) {
    switch(x_keycode) {
        case XK_BackSpace:
            return KEY_BACKSPACE;
        case XK_Return:
            return KEY_ENTER;
        case XK_Tab:
            return KEY_TAB;
        case XK_Pause:
            return KEY_PAUSE;
        case XK_Caps_Lock:
            return KEY_CAPITAL;
        case XK_Escape:
            return KEY_ESCAPE;
        case XK_Mode_switch:
            return KEY_MODECHANGE;
        case XK_space:
            return KEY_SPACE;
        case XK_Prior:
            return KEY_PRIOR;
        case XK_Next:
            return KEY_NEXT;
        case XK_End:
            return KEY_END;
        case XK_Home:
            return KEY_HOME;
        case XK_Left:
            return KEY_LEFT;
        case XK_Up:
            return KEY_UP;
        case XK_Right:
            return KEY_RIGHT;
        case XK_Down:
            return KEY_DOWN;
        case XK_Select:
            return KEY_SELECT;
        case XK_Print:
            return KEY_PRINT;
        case XK_Execute:
            return KEY_EXECUTE;
        case XK_Insert:
            return KEY_INSERT;
        case XK_Delete:
            return KEY_DELETE;
        case XK_Help:
            return KEY_HELP;
        case XK_Meta_L:
            return KEY_LWIN;
        case XK_Meta_R:
            return KEY_RWIN;
        case XK_KP_0:
            return KEY_NUMPAD0;
        case XK_KP_1:
            return KEY_NUMPAD1;
        case XK_KP_2:
            return KEY_NUMPAD2;
        case XK_KP_3:
            return KEY_NUMPAD3;
        case XK_KP_4:
            return KEY_NUMPAD4;
        case XK_KP_5:
            return KEY_NUMPAD5;
        case XK_KP_6:
            return KEY_NUMPAD6;
        case XK_KP_7:
            return KEY_NUMPAD7;
        case XK_KP_8:
            return KEY_NUMPAD8;
        case XK_KP_9:
            return KEY_NUMPAD9;
        
        case XK_multiply:
            return KEY_MULTIPLY;
        case XK_KP_Add:
            return KEY_ADD;
        case XK_KP_Subtract:
            return KEY_SUBTRACT;
        case XK_KP_Separator:
            return KEY_SEPARATOR;
        case XK_KP_Decimal:
            return KEY_DECIMAL;
        case XK_KP_Divide:
            return KEY_DIVIDE;
        case XK_F1:
            return KEY_F1;
        case XK_F2:
            return KEY_F2;
        case XK_F3:
            return KEY_F3;
        case XK_F4:
            return KEY_F4;
        case XK_F5:
            return KEY_F5;
        case XK_F6:
            return KEY_F6;
        case XK_F7:
            return KEY_F7;
        case XK_F8:
            return KEY_F8;
        case XK_F9:
            return KEY_F9;
        case XK_F10:
            return KEY_F10;
        case XK_F11:
            return KEY_F11;
        case XK_F12:
            return KEY_F12;
        case XK_F13:
            return KEY_F13;
        case XK_F14:
            return KEY_F14;
        case XK_F15:
            return KEY_F15;
        case XK_F16:
            return KEY_F16;
        case XK_F17:
            return KEY_F17;
        case XK_F18:
            return KEY_F18;
        case XK_F19:
            return KEY_F19;
        case XK_F20:
            return KEY_F20;
        case XK_F21:
            return KEY_F21;
        case XK_F22:
            return KEY_F22;
        case XK_F23:
            return KEY_F23;
        case XK_F24:
            return KEY_F24;
        
        case XK_Num_Lock:
            return KEY_NUMLOCK;
        case XK_Scroll_Lock:
            return KEY_SCROLL;
        
        case XK_KP_Equal:
            return KEY_NUMPAD_EQUAL;
        
        case XK_Shift_L:
            return KEY_LSHIFT;
        case XK_Shift_R:
            return KEY_RSHIFT;
        case XK_Control_L:
            return KEY_LCONTROL;
        case XK_Control_R:
            return KEY_RCONTROL;
        case XK_Menu:
            return KEY_RMENU;

        case XK_semicolon:
            return KEY_SEMICOLON;
        case XK_plus:
            return KEY_PLUS;
        case XK_comma:
            return KEY_COMMA;
        case XK_minus:
            return KEY_MINUS;
        case XK_period:
            return KEY_PERIOD;
        case XK_slash:
            return KEY_SLASH;
        case XK_grave:
            return KEY_GRAVE;

        case XK_a:
        case XK_A:
            return KEY_A;
        case XK_b:
        case XK_B:
            return KEY_B;
        case XK_C:
        case XK_c:
            return KEY_C;
        case XK_D:
        case XK_d:
            return KEY_D;
        case XK_e:
        case XK_E:
            return KEY_E;
        case XK_f:
        case XK_F:
            return KEY_F;
        case XK_G:
        case XK_g:
            return KEY_G;
        case XK_H:
        case XK_h:
            return KEY_H;
        case XK_I:
        case XK_i:
            return KEY_I;
        case XK_J:
        case XK_j:
            return KEY_J;
        case XK_K:
        case XK_k:
            return KEY_K;
        case XK_L:
        case XK_l:
            return KEY_L;
        case XK_M:
        case XK_m:
            return KEY_M;
        case XK_N:
        case XK_n:
            return KEY_N;
        case XK_o:
        case XK_O:
            return KEY_O;
        case XK_P:
        case XK_p:
            return KEY_P;
        case XK_Q:
        case XK_q:
            return KEY_Q;
        case XK_R:
        case XK_r:
            return KEY_R;
        case XK_S:
        case XK_s:
            return KEY_S;
        case XK_T:
        case XK_t:
            return KEY_T;
        case XK_u:
        case XK_U:
            return KEY_U;
        case XK_V:
        case XK_v:
            return KEY_V;
        case XK_W:
        case XK_w:
            return KEY_W;
        case XK_X:
        case XK_x:
            return KEY_X;
        case XK_Y:
        case XK_y:
            return KEY_Y;
        case XK_Z:
        case XK_z:
            return KEY_Z;
        default:
            return 0;

    }
}

#endif