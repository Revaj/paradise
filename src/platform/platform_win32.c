#include "platform.h"
#include "../core/logger.h"
#include "../core/input.h"
#include "../core/event.h"

#include "../containers/darray.h"

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)

#include <windows.h>
#include <windowsx.h>
#include <WinUser.h>
#include <stdbool.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>
#include "../renderer/vulkan/vulkan_types.inl"

typedef struct platform_state {
	HINSTANCE h_instance;
	HWND hwnd;
	VkSurfaceKHR surface;
	double clock_frequency;
	LARGE_INTEGER start_time;
} platform_state;

static platform_state* state_ptr;

LRESULT CALLBACK win32_process_message(HWND hwnd, uint32_t msg, WPARAM w_param, LPARAM l_param);

bool platform_system_startup(
	uint64_t* memory_requirement,
	void* state,
	const char* application_name,
	int32_t x,
	int32_t y,
	int32_t width,
	int32_t height) {

	*memory_requirement = sizeof(platform_state);
	if (state == 0) {
		return true;
	}
	state_ptr = state;

	state_ptr->h_instance = GetModuleHandleA(0);

	HICON icon = LoadIcon(state_ptr->h_instance, IDI_APPLICATION);
	WNDCLASSA wc;
	memset(&wc, 0, sizeof(wc));
	wc.style = CS_DBLCLKS;
	wc.lpfnWndProc = win32_process_message;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = state_ptr->h_instance;
	wc.hIcon = icon;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = NULL;
	wc.lpszClassName = "windows_class";

	if (!RegisterClassA(&wc)) {
		MessageBoxA(0, "Windows registration failed", "Error", MB_ICONEXCLAMATION | MB_OK);
		return false;
	}

	uint32_t client_x = x;
	uint32_t client_y = y;
	uint32_t client_width = width;
	uint32_t client_height = height;

	uint32_t window_x = client_x;
	uint32_t window_y = client_y;
	uint32_t window_width = client_width;
	uint32_t window_height = client_height;

	uint32_t window_style = WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION;
	uint32_t window_ex_style = WS_EX_APPWINDOW;

	window_style |= WS_MAXIMIZEBOX;
	window_style |= WS_MINIMIZEBOX;
	window_style |= WS_THICKFRAME;

	RECT border_rect = { 0, 0, 0, 0 };
	AdjustWindowRectEx(&border_rect, window_style, 0, window_ex_style);

	window_x += border_rect.left;
	window_y += border_rect.top;

	window_width += border_rect.right - border_rect.left;
	window_height += border_rect.bottom - border_rect.top;

	HWND handle = CreateWindowExA(
		window_ex_style, "windows_class", application_name,
		window_style, window_x, window_y, window_width, window_height,
		0, 0, state_ptr->h_instance, 0);

	if (handle == 0) {
		MessageBoxA(NULL, "window creation failed: ", "Error:", MB_ICONEXCLAMATION | MB_OK);

		KFATAL("Window creation failed!");
		return false;
	}
	else {
		state_ptr->hwnd = handle;
	}

	bool should_activate = true;
	int32_t show_window_command_flags = should_activate ? SW_SHOW : SW_SHOWNOACTIVATE;

	ShowWindow(state_ptr->hwnd, show_window_command_flags);

	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);
	state_ptr->clock_frequency = 1.0 / (double)frequency.QuadPart;
	QueryPerformanceCounter(&state_ptr->start_time);

	return true;
}

void platform_system_shutdown(void* plat_state) {
	if (state_ptr && state_ptr->hwnd) {
		DestroyWindow(state_ptr->hwnd);
		state_ptr->hwnd = 0;
	}
}

bool platform_pump_messages() {
	if (state_ptr) {
		MSG message;
		while (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&message);
			DispatchMessageA(&message);
		}
	}
	return true;
}

void* platform_allocate(uint64_t size, uint8_t aligned) {
	return malloc(size);
}

void platform_free(void* block, uint8_t aligned) {
	free(block);
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
	HANDLE console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
	static uint8_t levels[6] = { 64, 4, 6, 2, 1, 8 };
	SetConsoleTextAttribute(console_handle, levels[colour]);

	OutputDebugStringA(message);
	uint64_t length = strlen(message);
	LPDWORD number_writen = 0;
	WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), message, (DWORD)length, number_writen, 0);
}

void platform_console_write_error(const char* message, uint8_t colour) {
	HANDLE console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
	static uint8_t levels[6] = { 64, 4, 6, 2, 1, 8 };
	SetConsoleTextAttribute(console_handle, levels[colour]);

	OutputDebugStringA(message);
	uint64_t length = strlen(message);
	LPDWORD number_writen = 0;
	WriteConsoleA(GetStdHandle(STD_ERROR_HANDLE), message, (DWORD)length, number_writen, 0);
}

double platform_get_absolute_time() {
	if (state_ptr) {
		LARGE_INTEGER now_time;
		QueryPerformanceCounter(&now_time);
		return (double)now_time.QuadPart * state_ptr->clock_frequency;
	}
	return 0;
}

void platform_sleep(uint64_t ms) {
	Sleep(1000 * ms);
}

void platform_get_required_extension_names(const char*** names_darray) {
	darray_push(*names_darray, &"VK_KHR_win32_surface");
}

bool platform_create_vulkan_surface(vulkan_context *context) {
	if (!state_ptr) {
		return false;
	}

	VkWin32SurfaceCreateInfoKHR create_info = {VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
	create_info.hinstance = state_ptr->h_instance;
	create_info.hwnd = state_ptr->hwnd;

	VkResult result = vkCreateWin32SurfaceKHR(context->instance, &create_info, context->allocator, &state_ptr->surface);
	if (result != VK_SUCCESS) {
		KFATAL("Vulkan surface creation failed");
		return false;
	}

	context->surface = state_ptr->surface;

	return true;
}

LRESULT CALLBACK win32_process_message(HWND hwnd, uint32_t msg, WPARAM w_param, LPARAM l_param) {
	switch (msg) {
	case WM_ERASEBKGND:
		return 1;
	case WM_CLOSE:
		event_context data;
		event_fire(EVENT_CODE_APPLICATION_QUIT, 0, data);
		return TRUE;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_SIZE: {
		RECT r;
		GetClientRect(hwnd, &r);
		uint32_t width = r.right - r.left;
		uint32_t height = r.bottom - r.top;
	
		event_context context;
		context.data.u16[0] = (uint16_t)width;
		context.data.u16[1] = (uint16_t)height;
		event_fire(EVENT_CODE_RESIZED, 0, context);
	}break;
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYUP: {
		int8_t pressed = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
		keys key = (uint16_t)w_param;
	
		input_process_key(key, pressed);
	} break;
	case WM_MOUSEMOVE: {
		int32_t x_position = GET_X_LPARAM(l_param);
		int32_t y_position = GET_Y_LPARAM(l_param);

		input_process_mouse_move(x_position, y_position);
	} break;
	case WM_MOUSEWHEEL: {
		int32_t z_delta = GET_WHEEL_DELTA_WPARAM(w_param);
		if (z_delta != 0) {
			z_delta = (z_delta < 0) ? -1 : 1;
			input_process_mouse_wheel(z_delta);
		}
	} break;
	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP: {
		int8_t pressed = msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN || msg == WM_MBUTTONDOWN;
		buttons mouse_button = BUTTON_MAX_BUTTONS;
		switch (msg) {
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
			mouse_button = BUTTON_LEFT;
			break;
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
			mouse_button = BUTTON_MIDDLE;
			break;
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
			mouse_button = BUTTON_RIGHT;
			break;
		}

		if (mouse_button != BUTTON_MAX_BUTTONS) {
			input_process_button(mouse_button, pressed);
		}
	}break; 
	}//switch

	return DefWindowProcA(hwnd, msg, w_param, l_param);
}

#endif // Platform WINDOW