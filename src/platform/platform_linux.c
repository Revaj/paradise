#include "platform.h"

#if __linux__

#include "../core/logger.h"
#include "../core/event.h"
#include "../core/input.h"

#include "../containers/darray.h"

#include <wayland-client.h>
#include <wayland-client-protocol.h>
#include <xkbcommon/xkbcommon.h>
//Generated in compile time
#include "protocol.h"

#include <assert.h>
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

#define VK_USE_PLATFORM_WAYLAND_KHR
#include <vulkan/vulkan.h>
#include "../renderer/vulkan/vulkan_types.inl"

enum pointer_event_mask {
    POINTER_EVENT_ENTER = 1 << 0,
    POINTER_EVENT_LEAVE = 1 << 1,
    POINTER_EVENT_MOTION = 1 << 2,
    POINTER_EVENT_BUTTON = 1 << 3,
    POINTER_EVENT_AXIS = 1 << 4,
    POINTER_EVENT_AXIS_SOURCE = 1 << 5,
    POINTER_EVENT_AXIS_STOP = 1 << 6,
    POINTER_EVENT_AXIS_DISCRETE = 1 << 7,
};

struct pointer_event {
    uint32_t event_mask;
    wl_fixed_t surface_x, surface_y;
    uint32_t button, state;
    uint32_t time;
    uint32_t serial;
    struct { 
        bool valid;
        wl_fixed_t value;
        int32_t discrete;
    } axes[2];
    uint32_t axis_source;
};


typedef struct internal_state {
    struct wl_display *wl_display;
    struct wl_registry *wl_registry;
    struct wl_shm *wl_shm;
    struct wl_compositor *wl_compositor;
    struct xdg_wm_base *xdg_wm_base;
    struct wl_seat *wl_seat;
    struct wl_surface *wl_surface;
    struct xdg_surface *xdg_surface;
    struct xdg_toplevel *xdg_toplevel;

    struct wl_keyboard *wl_keyboard;
    struct wl_pointer *wl_pointer;
    struct xkb_state *xkb_state;
    struct xkb_context *xkb_context;
    struct xkb_keymap *xkb_keymap;

    //vulkan stuff
    VkSurfaceKHR surface;
    float offset;
    uint32_t last_frame;
    int width, height;
    bool closed;
    struct pointer_event pointer_event;

} internal_state;

keys translate_keycode(uint32_t x_keycode);


struct wl_buffer * draw_frame(struct internal_state *state);
int allocate_shm_file(size_t size);
int create_shm_file(void);
void xdg_surface_configure(void *data,
        struct xdg_surface *xdg_surface, uint32_t serial);
void wl_buffer_release(void *data, struct wl_buffer *wl_buffer);

void wl_seat_capabilities(void* data, struct wl_seat *wl_seat, uint32_t capabilities);
void wl_seat_name(void *data, struct wl_seat *wl_seat, const char *name);

void wl_pointer_enter(void * data, struct wl_pointer *wl_pointer, uint32_t serial,
                      struct wl_surface *surface, wl_fixed_t surface_x, wl_fixed_t surface_y);
void wl_pointer_leave(void* data, struct wl_pointer *wl_pointer, uint32_t serial,
                      struct wl_surface *surface);
void wl_pointer_motion(void* data, struct wl_pointer *wl_pointer, uint32_t time,
                        wl_fixed_t surface_x, wl_fixed_t surface_y);
void wl_pointer_button(void *data, struct wl_pointer *wl_pointer, uint32_t serial,
                       uint32_t time, uint32_t button, uint32_t state);
void wl_pointer_axis(void* data, struct wl_pointer *wl_pointer, uint32_t time,
                     uint32_t axis, wl_fixed_t value);
void wl_pointer_axis_source(void* data, struct wl_pointer *wl_pointer, uint32_t axis_source);
void wl_pointer_axis_stop(void* data, struct wl_pointer *wl_pointer, uint32_t time, uint32_t axis);
void wl_pointer_axis_discrete(void *data, struct wl_pointer *wl_pointer, uint32_t axis, int32_t discrete);
void wl_pointer_frame(void *data, struct wl_pointer *wl_pointer);

void wl_keyboard_keymap(void *data, struct wl_keyboard *wl_keyboard,
                        uint32_t format, int32_t fd, uint32_t size);
void wl_keyboard_enter(void *data, struct wl_keyboard *wl_keyboard,
                    uint32_t serial, struct wl_surface *surface,
                    struct wl_array *keys);
void wl_keyboard_key(void *data, struct wl_keyboard *wl_keyboard,
                uint32_t serial, uint32_t time, uint32_t key, uint32_t state);
void wl_keyboard_leave(void *data, struct wl_keyboard *wl_keyboard,
                    uint32_t serial, struct wl_surface *surface);
void wl_keyboard_modifiers(void *data, struct wl_keyboard *wl_keyboard,
                        uint32_t serial, uint32_t mods_depressed,
                        uint32_t mods_latched, uint32_t mods_locked,
                        uint32_t group);
void wl_keyboard_repeat_info(void *data, struct wl_keyboard *wl_keyboard, int32_t rate, int32_t delay);

void randname(char *buf);

struct wl_keyboard_listener wl_keyboard_listener = {
    .keymap = wl_keyboard_keymap,
    .enter = wl_keyboard_enter,
    .leave = wl_keyboard_leave,
    .key = wl_keyboard_key,
    .modifiers = wl_keyboard_modifiers,
    .repeat_info = wl_keyboard_repeat_info,
};

struct wl_pointer_listener wl_pointer_listener = {
    .enter = wl_pointer_enter,
    .leave = wl_pointer_leave,
    .motion = wl_pointer_motion,
    .button = wl_pointer_button,
    .axis = wl_pointer_axis,
    .frame = wl_pointer_frame,
    .axis_source = wl_pointer_axis_source,
    .axis_stop = wl_pointer_axis_stop,
    .axis_discrete = wl_pointer_axis_discrete,
};

struct wl_seat_listener wl_seat_listener = {
    .capabilities = wl_seat_capabilities,
    .name = wl_seat_name,
};

struct wl_buffer_listener wl_buffer_listener = {
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
    state->xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

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

void wl_keyboard_repeat_info(void *data, struct wl_keyboard *wl_keyboard, int32_t rate, int32_t delay) {
       /* DO NOTHING*/
}

void wl_keyboard_modifiers(void *data, struct wl_keyboard *wl_keyboard,
                        uint32_t serial, uint32_t mods_depressed,
                        uint32_t mods_latched, uint32_t mods_locked,
                        uint32_t group) {
    struct internal_state *client_state = data;
    xkb_state_update_mask(client_state->xkb_state,
        mods_depressed, mods_latched, mods_locked, 0, 0, group);
}
void wl_keyboard_leave(void *data, struct wl_keyboard *wl_keyboard,
                    uint32_t serial, struct wl_surface *surface) {
    KINFO("Keyboard leave");
}

void wl_keyboard_key(void *data, struct wl_keyboard *wl_keyboard,
                uint32_t serial, uint32_t time, uint32_t key, uint32_t state) {
    internal_state *client_state = data;
    char buf[128];
    uint32_t keycode = key + 8;
    xkb_keysym_t sym = xkb_state_key_get_one_sym(
                    client_state->xkb_state, keycode);
    xkb_keysym_get_name(sym, buf, sizeof(buf));
    const char *action = state == WL_KEYBOARD_KEY_STATE_PRESSED ? "press" : "release";
    KINFO(" key %s: sym: %-12s (%d) ", action, buf, sym);
    xkb_state_key_get_utf8(client_state->xkb_state, keycode, buf, sizeof(buf));
    KINFO(" utf8: '%s'", buf);
}

void wl_keyboard_enter(void *data, struct wl_keyboard *wl_keyboard,
                    uint32_t serial, struct wl_surface *surface,
                    struct wl_array *keys) {
    struct internal_state *internal_state = data;
    KINFO("Keyboard enter; keys pressed are: ");
    uint32_t *key;
    wl_array_for_each(key, keys) {
        char buf[128];
        xkb_keysym_t sym = xkb_state_key_get_one_sym(
                        internal_state->xkb_state, *key + 8);
        xkb_keysym_get_name(sym, buf, sizeof(buf));
        KINFO(" sym: %-12s (%d)", buf, sym);
        xkb_state_key_get_utf8(internal_state->xkb_state,
                            *key + 8, buf, sizeof(buf));
        KINFO(" utf8: '%s'", buf);
    }
}


void wl_keyboard_keymap(void *data, struct wl_keyboard *wl_keyboard,
                        uint32_t format, int32_t fd, uint32_t size) {
    struct internal_state *client_state = data;
    assert(format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1);

    char *map_shm = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    assert(map_shm != MAP_FAILED);

    struct xkb_keymap *xkb_keymap = xkb_keymap_new_from_string(
                        client_state->xkb_context, map_shm,
                        XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
    
    munmap(map_shm, size);
    close(fd);

    struct xkb_state *xkb_state = xkb_state_new(xkb_keymap);
    xkb_keymap_unref(client_state->xkb_keymap);
    xkb_state_unref(client_state->xkb_state);
    client_state->xkb_keymap = xkb_keymap;
    client_state->xkb_state = xkb_state;
}

void wl_pointer_frame(void *data, struct wl_pointer *wl_pointer) {
    struct internal_state *client_state = data;
    struct pointer_event *event = &client_state->pointer_event;

    KINFO("Pointer frame %d", event->time);

    if (event->event_mask & POINTER_EVENT_ENTER) {
        KINFO("Entered %f, %f", wl_fixed_to_double(event->surface_x),
                                wl_fixed_to_double(event->surface_y));
    }
    if (event->event_mask & POINTER_EVENT_LEAVE) {
        KINFO("Leave");
    }
    if (event->event_mask & POINTER_EVENT_MOTION) {
        KINFO("Motion %f, %f", wl_fixed_to_double(event->surface_x),
                               wl_fixed_to_double(event->surface_y));        
    }
    if (event->event_mask & POINTER_EVENT_BUTTON) {
        char *state = event->state == WL_POINTER_BUTTON_STATE_RELEASED ?
                    "released" : "pressed";
        KINFO("Button %d %s", event->button, state);
    }

    uint32_t axis_events = POINTER_EVENT_AXIS | POINTER_EVENT_AXIS_SOURCE | POINTER_EVENT_AXIS_STOP | POINTER_EVENT_AXIS_DISCRETE;
    char *axis_name[2] = {
        [WL_POINTER_AXIS_VERTICAL_SCROLL] = "vertical",
        [WL_POINTER_AXIS_HORIZONTAL_SCROLL] = "horizontal",
    };

    char *axis_source[4] = {
        [WL_POINTER_AXIS_SOURCE_WHEEL] = "wheel",
        [WL_POINTER_AXIS_SOURCE_FINGER] = "finger",
        [WL_POINTER_AXIS_SOURCE_CONTINUOUS] = "continuous",
        [WL_POINTER_AXIS_SOURCE_WHEEL_TILT] = "wheel tilt",
    };

    if (event->event_mask & axis_events) {
        for (size_t i = 0; i < 2; ++i) {
            if (!event->axes[i].valid) {
                continue;
            }

            KINFO("%s axis ", axis_name[i]);
            if (event->event_mask & POINTER_EVENT_AXIS) {
                KINFO("value %f ", wl_fixed_to_double(event->axes[i].value));
            }
            if (event->event_mask & POINTER_EVENT_AXIS_DISCRETE) {
                KINFO("discrete %d ", event->axes[i].discrete);
            }
            if (event->event_mask & POINTER_EVENT_AXIS_SOURCE) {
                KINFO("via %s ", axis_source[event->axis_source]);
            }
            if (event->event_mask & POINTER_EVENT_AXIS_STOP) {
                KINFO("(STOPPED) ");
            }
        }
    }
    memset(event, 0, sizeof(event));
}

void wl_pointer_axis(void* data, struct wl_pointer *wl_pointer, uint32_t time,
                     uint32_t axis, wl_fixed_t value) {
    struct internal_state *client_state = data;
    client_state->pointer_event.event_mask |= POINTER_EVENT_AXIS;
    client_state->pointer_event.time = time;
    client_state->pointer_event.axes[axis].valid = true;
    client_state->pointer_event.axes[axis].value = value;
}

void wl_pointer_axis_source(void* data, struct wl_pointer *wl_pointer, uint32_t axis_source) {
    struct internal_state *client_state = data;
    client_state->pointer_event.event_mask |= POINTER_EVENT_AXIS_SOURCE;
    client_state->pointer_event.axis_source = axis_source;
}

void wl_pointer_axis_stop(void* data, struct wl_pointer *wl_pointer, uint32_t time, uint32_t axis) {
    struct internal_state *client_state = data;
    client_state->pointer_event.time = time;
    client_state->pointer_event.event_mask |= POINTER_EVENT_AXIS_STOP;
    client_state->pointer_event.axes[axis].valid = true;
}

void wl_pointer_axis_discrete(void *data, struct wl_pointer *wl_pointer, uint32_t axis, int32_t discrete) {
    struct internal_state *client_state = data;
    client_state->pointer_event.event_mask |= POINTER_EVENT_AXIS_DISCRETE;
    client_state->pointer_event.axes[axis].valid = true;
    client_state->pointer_event.axes[axis].discrete = discrete;
}

void wl_pointer_motion(void* data, struct wl_pointer *wl_pointer, uint32_t time,
                        wl_fixed_t surface_x, wl_fixed_t surface_y) {
    struct internal_state *client_state = data;
    client_state->pointer_event.event_mask |= POINTER_EVENT_MOTION;
    client_state->pointer_event.time = time;
    client_state->pointer_event.surface_x = surface_x;
    client_state->pointer_event.surface_y = surface_y;
}

void wl_pointer_button(void *data, struct wl_pointer *wl_pointer, uint32_t serial,
                       uint32_t time, uint32_t button, uint32_t state) {
    struct internal_state *client_state = data;
    client_state->pointer_event.event_mask |= POINTER_EVENT_BUTTON;
    client_state->pointer_event.time = time;
    client_state->pointer_event.serial = serial;
    client_state->pointer_event.button = button;
    client_state->pointer_event.state = state;
}

void wl_pointer_enter(void * data, struct wl_pointer *wl_pointer, uint32_t serial,
                      struct wl_surface *surface, wl_fixed_t surface_x, wl_fixed_t surface_y) {
    struct internal_state* client_state = data;
    client_state->pointer_event.event_mask |= POINTER_EVENT_ENTER;
    client_state->pointer_event.serial = serial;
    client_state->pointer_event.surface_x = surface_x;
    client_state->pointer_event.surface_y = surface_y;
}

void wl_pointer_leave(void* data, struct wl_pointer *wl_pointer, uint32_t serial,
                      struct wl_surface *surface) {
    struct internal_state *client_state = data;
    client_state->pointer_event.serial = serial;
    client_state->pointer_event.event_mask |= POINTER_EVENT_LEAVE;
}

void wl_seat_name(void *data, struct wl_seat *wl_seat, const char *name) {
 /* DO NOTHING*/
}

void wl_seat_capabilities(void* data, struct wl_seat *wl_seat, uint32_t capabilities) {
    struct internal_state *state = data;

    bool have_pointer = capabilities & WL_SEAT_CAPABILITY_POINTER;
    if (have_pointer && state->wl_pointer == NULL) {
        state->wl_pointer = wl_seat_get_pointer(state->wl_seat);
        wl_pointer_add_listener(state->wl_pointer,
                            &wl_pointer_listener, state);
    } else if (!have_pointer && state->wl_pointer != NULL) {
        wl_pointer_release(state->wl_pointer);
        state->wl_pointer == NULL;
    }

    bool have_keyboard = capabilities & WL_SEAT_CAPABILITY_KEYBOARD;
    if (have_keyboard && state->wl_keyboard == NULL) {
        state->wl_keyboard = wl_seat_get_keyboard(state->wl_seat);
        wl_keyboard_add_listener(state->wl_keyboard,
                                &wl_keyboard_listener, state);
    } else if (!have_keyboard && state->wl_keyboard != NULL) {
        wl_keyboard_release(state->wl_keyboard);
        state->wl_keyboard = NULL;
    }
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
    } else if (strcmp(interface, wl_seat_interface.name) == 0) {
        state->wl_seat = wl_registry_bind(
                        wl_registry, name, &wl_seat_interface, 7);
        wl_seat_add_listener(state->wl_seat,
                            &wl_seat_listener, state);
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
    darray_push(*names_darray, &"VK_KHR_wayland_surface");
}

int8_t platform_create_vulkan_surface(platform_state* plat_state, vulkan_context* context) {
    internal_state* state = (internal_state*)plat_state->internal_state;

    VkWaylandSurfaceCreateInfoKHR create_info = { VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR };
    create_info.hinstance = state->h_instance;
    create_info.hwnd = state->hwnd;

    VkResult result = vkCreateWaylandSurfaceKHR(context->instance, &create_info, context->allocator, &state->surface);
    if (result != VK_SUCCESS) {
        KFATAL("Vulkan surface creation failed");
        return 0;
    }

    return 1;
}

keys translate_keycode(uint32_t x_keycode) {
    switch(x_keycode) {
        case XKB_KEY_BackSpace:
            return KEY_BACKSPACE;
        case XKB_KEY_Return:
            return KEY_ENTER;
        case XKB_KEY_Tab:
            return KEY_TAB;
        case XKB_KEY_Pause:
            return KEY_PAUSE;
        case XKB_KEY_Caps_Lock:
            return KEY_CAPITAL;
        case XKB_KEY_Escape:
            return KEY_ESCAPE;
        case XKB_KEY_Mode_switch:
            return KEY_MODECHANGE;
        case XKB_KEY_space:
            return KEY_SPACE;
        case XKB_KEY_Prior:
            return KEY_PRIOR;
        case XKB_KEY_Next:
            return KEY_NEXT;
        case XKB_KEY_End:
            return KEY_END;
        case XKB_KEY_Home:
            return KEY_HOME;
        case XKB_KEY_Left:
            return KEY_LEFT;
        case XKB_KEY_Up:
            return KEY_UP;
        case XKB_KEY_Right:
            return KEY_RIGHT;
        case XKB_KEY_Down:
            return KEY_DOWN;
        case XKB_KEY_Select:
            return KEY_SELECT;
        case XKB_KEY_Print:
            return KEY_PRINT;
        case XKB_KEY_Execute:
            return KEY_EXECUTE;
        case XKB_KEY_Insert:
            return KEY_INSERT;
        case XKB_KEY_Delete:
            return KEY_DELETE;
        case XKB_KEY_Help:
            return KEY_HELP;
        case XKB_KEY_Meta_L:
            return KEY_LWIN;
        case XKB_KEY_Meta_R:
            return KEY_RWIN;
        case XKB_KEY_KP_0:
            return KEY_NUMPAD0;
        case XKB_KEY_KP_1:
            return KEY_NUMPAD1;
        case XKB_KEY_KP_2:
            return KEY_NUMPAD2;
        case XKB_KEY_KP_3:
            return KEY_NUMPAD3;
        case XKB_KEY_KP_4:
            return KEY_NUMPAD4;
        case XKB_KEY_KP_5:
            return KEY_NUMPAD5;
        case XKB_KEY_KP_6:
            return KEY_NUMPAD6;
        case XKB_KEY_KP_7:
            return KEY_NUMPAD7;
        case XKB_KEY_KP_8:
            return KEY_NUMPAD8;
        case XKB_KEY_KP_9:
            return KEY_NUMPAD9;
        
        case XKB_KEY_multiply:
            return KEY_MULTIPLY;
        case XKB_KEY_KP_Add:
            return KEY_ADD;
        case XKB_KEY_KP_Subtract:
            return KEY_SUBTRACT;
        case XKB_KEY_KP_Separator:
            return KEY_SEPARATOR;
        case XKB_KEY_KP_Decimal:
            return KEY_DECIMAL;
        case XKB_KEY_KP_Divide:
            return KEY_DIVIDE;
        case XKB_KEY_F1:
            return KEY_F1;
        case XKB_KEY_F2:
            return KEY_F2;
        case XKB_KEY_F3:
            return KEY_F3;
        case XKB_KEY_F4:
            return KEY_F4;
        case XKB_KEY_F5:
            return KEY_F5;
        case XKB_KEY_F6:
            return KEY_F6;
        case XKB_KEY_F7:
            return KEY_F7;
        case XKB_KEY_F8:
            return KEY_F8;
        case XKB_KEY_F9:
            return KEY_F9;
        case XKB_KEY_F10:
            return KEY_F10;
        case XKB_KEY_F11:
            return KEY_F11;
        case XKB_KEY_F12:
            return KEY_F12;
        case XKB_KEY_F13:
            return KEY_F13;
        case XKB_KEY_F14:
            return KEY_F14;
        case XKB_KEY_F15:
            return KEY_F15;
        case XKB_KEY_F16:
            return KEY_F16;
        case XKB_KEY_F17:
            return KEY_F17;
        case XKB_KEY_F18:
            return KEY_F18;
        case XKB_KEY_F19:
            return KEY_F19;
        case XKB_KEY_F20:
            return KEY_F20;
        case XKB_KEY_F21:
            return KEY_F21;
        case XKB_KEY_F22:
            return KEY_F22;
        case XKB_KEY_F23:
            return KEY_F23;
        case XKB_KEY_F24:
            return KEY_F24;
        
        case XKB_KEY_Num_Lock:
            return KEY_NUMLOCK;
        case XKB_KEY_Scroll_Lock:
            return KEY_SCROLL;
        
        case XKB_KEY_KP_Equal:
            return KEY_NUMPAD_EQUAL;
        
        case XKB_KEY_Shift_L:
            return KEY_LSHIFT;
        case XKB_KEY_Shift_R:
            return KEY_RSHIFT;
        case XKB_KEY_Control_L:
            return KEY_LCONTROL;
        case XKB_KEY_Control_R:
            return KEY_RCONTROL;
        case XKB_KEY_Menu:
            return KEY_RMENU;

        case XKB_KEY_semicolon:
            return KEY_SEMICOLON;
        case XKB_KEY_plus:
            return KEY_PLUS;
        case XKB_KEY_comma:
            return KEY_COMMA;
        case XKB_KEY_minus:
            return KEY_MINUS;
        case XKB_KEY_period:
            return KEY_PERIOD;
        case XKB_KEY_slash:
            return KEY_SLASH;
        case XKB_KEY_grave:
            return KEY_GRAVE;

        case XKB_KEY_a:
        case XKB_KEY_A:
            return KEY_A;
        case XKB_KEY_b:
        case XKB_KEY_B:
            return KEY_B;
        case XKB_KEY_C:
        case XKB_KEY_c:
            return KEY_C;
        case XKB_KEY_D:
        case XKB_KEY_d:
            return KEY_D;
        case XKB_KEY_e:
        case XKB_KEY_E:
            return KEY_E;
        case XKB_KEY_f:
        case XKB_KEY_F:
            return KEY_F;
        case XKB_KEY_G:
        case XKB_KEY_g:
            return KEY_G;
        case XKB_KEY_H:
        case XKB_KEY_h:
            return KEY_H;
        case XKB_KEY_I:
        case XKB_KEY_i:
            return KEY_I;
        case XKB_KEY_J:
        case XKB_KEY_j:
            return KEY_J;
        case XKB_KEY_K:
        case XKB_KEY_k:
            return KEY_K;
        case XKB_KEY_L:
        case XKB_KEY_l:
            return KEY_L;
        case XKB_KEY_M:
        case XKB_KEY_m:
            return KEY_M;
        case XKB_KEY_N:
        case XKB_KEY_n:
            return KEY_N;
        case XKB_KEY_o:
        case XKB_KEY_O:
            return KEY_O;
        case XKB_KEY_P:
        case XKB_KEY_p:
            return KEY_P;
        case XKB_KEY_Q:
        case XKB_KEY_q:
            return KEY_Q;
        case XKB_KEY_R:
        case XKB_KEY_r:
            return KEY_R;
        case XKB_KEY_S:
        case XKB_KEY_s:
            return KEY_S;
        case XKB_KEY_T:
        case XKB_KEY_t:
            return KEY_T;
        case XKB_KEY_u:
        case XKB_KEY_U:
            return KEY_U;
        case XKB_KEY_V:
        case XKB_KEY_v:
            return KEY_V;
        case XKB_KEY_W:
        case XKB_KEY_w:
            return KEY_W;
        case XKB_KEY_X:
        case XKB_KEY_x:
            return KEY_X;
        case XKB_KEY_Y:
        case XKB_KEY_y:
            return KEY_Y;
        case XKB_KEY_Z:
        case XKB_KEY_z:
            return KEY_Z;
        default:
            return 0;

    }
}

#endif