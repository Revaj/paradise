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
#include <sys/time.h>
#include <stdio.h>
#include <string.h>

#if _POSIX_C_SOURCE >= 199309L
#include <time.h>
#else
#include <unistd.h>
#endif

typedef struct internal_state {
    Display* display;
    xcb_connection_t* connection;
    xcb_window_t window;
    xcb_screen_t* screen;
    xcb_atom_t wm_protocols;
    xcb_atom_t wm_delete_win;
} internal_state;

keys translate_keycode(uint32_t x_keycode);



uint8_t platform_startup(
    platform_state* plat_state,
    const char* application_name,
    int32_t x,
    int32_t y,
    int32_t width,
    int32_t height) {

    plat_state->internal_state = malloc(sizeof(internal_state));
    internal_state* state = (internal_state*)plat_state->internal_state;

    state->display = XOpenDisplay(NULL);

    XAutoRepeatOff(state->display);

    state->connection = XGetXCBConnection(state->display);

    if (xcb_connection_has_error(state->connection)) {
        KFATAL("Failed to connect to X server via XCB");
        return 0;
    }

    const struct xcb_setup_t* setup = xcb_get_setup(state->connection);

    xcb_screen_iterator_t it = xcb_setup_roots_iterator(setup);
    int screen_p = 0;
    for (int32_t s = screen_p; s > 0; s--) {
        xcb_screen_next(&it);
    }

    state->screen = it.data;

    state->window = xcb_generate_id(state->connection);

    uint32_t event_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;

    uint32_t event_values = XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE |
        XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE |
        XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_POINTER_MOTION |
        XCB_EVENT_MASK_STRUCTURE_NOTIFY;

    uint32_t value_list[] = { state->screen->black_pixel, event_values };

    xcb_void_cookie_t cookie = xcb_create_window(
        state->connection,
        XCB_COPY_FROM_PARENT,
        state->window,
        state->screen->root,
        x,
        y,
        width,
        height,
        0,
        XCB_WINDOW_CLASS_INPUT_OUTPUT,
        state->screen->root_visual,
        event_mask,
        value_list);

    xcb_change_property(
        state->connection,
        XCB_PROP_MODE_REPLACE,
        state->window,
        XCB_ATOM_WM_NAME,
        XCB_ATOM_STRING,
        8,
        strlen(application_name),
        application_name);

    xcb_intern_atom_cookie_t wm_delete_cookie = xcb_intern_atom(
        state->connection,
        0,
        strlen("WM_DELETE_WINDOW"),
        "WM_DELETE_WINDOW");
    xcb_intern_atom_cookie_t wm_protocols_cookie = xcb_intern_atom(
        state->connection,
        0,
        strlen("WM_PROTOCOLS"),
        "WM_PROTOCOLS");
    xcb_intern_atom_reply_t* wm_delete_reply = xcb_intern_atom_reply(
        state->connection,
        wm_delete_cookie,
        NULL);
    xcb_intern_atom_reply_t* wm_protocols_reply = xcb_intern_atom_reply(
        state->connection,
        wm_protocols_cookie,
        NULL);
    state->wm_delete_win = wm_delete_reply->atom;
    state->wm_protocols = wm_protocols_reply->atom;

    xcb_change_property(
        state->connection,
        XCB_PROP_MODE_REPLACE,
        state->window,
        wm_protocols_reply->atom,
        4,
        32,
        1,
        &wm_delete_reply->atom);

    xcb_map_window(state->connection, state->window);

    int32_t stream_result = xcb_flush(state->connection);
    if (stream_result <= 0) {
        KFATAL("An error occurred when flushing the stream %d", stream_result);
        return -1;
    }
    return 1;
}

void platform_shutdown(platform_state* plat_state) {
    internal_state* state = (internal_state*)plat_state->internal_state;

    //XAutoRepeatON(state->display);

    xcb_destroy_window(state->connection, state->window);
}

uint8_t platform_pump_messages(platform_state* plat_state) {
    internal_state* state = (internal_state*)plat_state->internal_state;

    xcb_generic_event_t* event;
    xcb_client_message_event_t* cm;

    uint8_t quit_flagged = 0;

    while (event != 0) {
        event = xcb_poll_for_event(state->connection);
        if (event == 0) {
            break;
        }
        switch (event->response_type & ~0x80) {
        case XCB_KEY_PRESS:
        case XCB_KEY_RELEASE: {
            xcb_key_press_event_t *kb_event = (xcb_key_press_event_t *)event;
            uint8_t pressed = event->response_type == XCB_KEY_PRESS;
            xcb_keycode_t code = kb_event->detail;
            KeySym key_sym = XkbKeycodeToKeysym(
                                state->display,
                                (KeyCode)code,
                                0,
                                code & ShiftMask ? 1 : 0);
            keys key = translate_keycode(key_sym);
            input_process_key(key, pressed);

        }break;

        case XCB_BUTTON_PRESS:
        case XCB_BUTTON_RELEASE: {
            xcb_button_press_event_t *mouse_event = (xcb_button_press_event_t*)event;
            int8_t pressed = event->response_type == XCB_BUTTON_PRESS;
            buttons mouse_button = BUTTON_MAX_BUTTONS;
            switch (mouse_event->detail) {
                case XCB_BUTTON_INDEX_1:
                    mouse_button = BUTTON_LEFT;
                    break;
                case XCB_BUTTON_INDEX_2:
                    mouse_button = BUTTON_MIDDLE;
                    break;
                case XCB_BUTTON_INDEX_3:
                    mouse_button = BUTTON_RIGHT;
                    break;
            }

            if (mouse_button != BUTTON_MAX_BUTTONS) {
                input_process_button(mouse_button, pressed);
            }

        }
            break;
        case XCB_MOTION_NOTIFY: {
            xcb_motion_notify_event_t* move_event = (xcb_motion_notify_event_t*)event;
            input_process_mouse_move(move_event->event_x, move_event->event_y);
        }
            break;
        case XCB_CONFIGURE_NOTIFY: {

        }

        case XCB_CLIENT_MESSAGE: {
            cm = (xcb_client_message_event_t*)event;

            if (cm->data.data32[0] == state->wm_delete_win) {
                quit_flagged = 1;
            }
        } break;

        default:
            break;
        }

        free(event);
    }

    return !quit_flagged;
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