#include "vulkan_backend.h"

#include "vulkan_types.inl"
#include "vulkan_platform.h"
#include "vulkan_device.h"
#include "vulkan_swapchain.h"
#include "vulkan_renderpass.h"

#include "../../core/logger.h"
#include "../../core/gstring.h"

#include "../../containers/darray.h"
#include "../../platform/platform.h"

static vulkan_context context;

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
	VkDebugUtilsMessageTypeFlagsEXT message_types,
	const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
	void* user_data);

int32_t find_memory_index(uint32_t type_filter, uint32_t property_flags);

int8_t vulkan_renderer_backend_initialize(renderer_backend* backend, const char* application_name, struct platform_state* plat_state) {
	
	context.find_memory_index = find_memory_index;
	context.allocator = 0;
	
	VkApplicationInfo app_info = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
	app_info.apiVersion = VK_API_VERSION_1_2;
	app_info.pApplicationName = application_name;
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName = "Paradise Engine";
	app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);

	VkInstanceCreateInfo create_info = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	create_info.pApplicationInfo = &app_info;
	
	const char** required_extensions = darray_create(const char*);
	darray_push(required_extensions, &VK_KHR_SURFACE_EXTENSION_NAME);
	platform_get_required_extension_names(&required_extensions);
#if defined(_DEBUG)
	darray_push(required_extensions, &VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	KDEBUG("Required extensions:");
	uint32_t length = darray_length(required_extensions);
	for (uint32_t i = 0; i < length; ++i) {
		KDEBUG(required_extensions[i]);
	}

#endif
	
	create_info.enabledExtensionCount = darray_length(required_extensions);
	create_info.ppEnabledExtensionNames = required_extensions;

	const char** required_validation_layer_names = 0;
	uint32_t required_validation_layer_count = 0;

#if defined(_DEBUG)
	KINFO("Validation layers enabled. Enumerating ...");

	required_validation_layer_names = darray_create(const char*);
	darray_push(required_validation_layer_names, &"VK_LAYER_KHRONOS_validation");
	required_validation_layer_count = darray_length(required_validation_layer_names);

	uint32_t available_layer_count = 0;
	VK_CHECK(vkEnumerateInstanceLayerProperties(&available_layer_count, 0));
	VkLayerProperties* available_layers = darray_reserve(VkLayerProperties, available_layer_count);
	VK_CHECK(vkEnumerateInstanceLayerProperties(&available_layer_count, available_layers));

	for (uint32_t i = 0; i < required_validation_layer_count; ++i) {
		KINFO("Searching for layer: %s...", required_validation_layer_names[i]);
		int8_t found = 0;
		for (uint32_t j = 0; j < available_layer_count; ++j) {
			if (strings_equal(required_validation_layer_names[i], available_layers[j].layerName)) {
				found = 1;
				KINFO("Found.");
				break;
			}
		}

		if (!found) {
			KFATAL("Required validation layer is missing: %s", required_validation_layer_names[i]);
			return 0;
		}
	}

	KINFO("All required validation layers are present.");
#endif

	create_info.enabledLayerCount = required_validation_layer_count;
	create_info.ppEnabledLayerNames = required_validation_layer_names;

	VK_CHECK(vkCreateInstance(&create_info, context.allocator, &context.instance));
	KINFO("Vulkan instance created.");

#if defined(_DEBUG)
	KDEBUG("Creating Vulkan debugger...");
	uint32_t log_severity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;

	VkDebugUtilsMessengerCreateInfoEXT debug_create_info = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
	debug_create_info.messageSeverity = log_severity;
	debug_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
	debug_create_info.pfnUserCallback = vk_debug_callback;
	debug_create_info.pUserData = 0;

	PFN_vkCreateDebugUtilsMessengerEXT func =
		(PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context.instance, "vkCreateDebugUtilsMessengerEXT");
	KASSERT(func, "Failed to create debug messenger!");
	VK_CHECK(func(context.instance, &debug_create_info, context.allocator, &context.debug_messenger));
	KDEBUG("Vulkan debugger created.");
#endif
	KDEBUG("Creating Vulkan surface...");
	if (!platform_create_vulkan_surface(plat_state, &context)) {
		KERROR("Failed to create platform surface");
		return 0;
	}
	KDEBUG("Vulkan surface created");

	if (!vulkan_device_create(&context)) {
		KERROR("Failed to create device");
		return 0;
	}

	vulkan_swapchain_create(
		&context,
		context.framebuffer_width,
		context.framebuffer_height,
		&context.swapchain);

	vulkan_renderpass_create(
		&context,
		&context.main_renderpass,
		0, 0, context.framebuffer_width, context.framebuffer_height,
		0.0f, 0.0f, 0.2f, 1.0f,
		1.0f, 0);

	KINFO("Vulkan renderer initialized succesfully");
	return 1;
}

void vulkan_renderer_backend_shutdown(renderer_backend* backend) {
	
	vulkan_renderpass_destroy(&context, &context.main_renderpass);
	vulkan_swapchain_destroy(&context, &context.swapchain);

	KDEBUG("Destroying Vulkan device...");
	vulkan_device_destroy(&context);

	KDEBUG("Destroying Vulkan surface...");
	if (context.surface) {
		vkDestroySurfaceKHR(context.instance, context.surface, context.allocator);
		context.surface = 0;
	}

	KDEBUG("Destroying Vulkan debugger...");
	if (context.debug_messenger) {
		PFN_vkDestroyDebugUtilsMessengerEXT func =
			(PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context.instance, "vkDestroyDebugUtilsMessengerEXT");
		func(context.instance, context.debug_messenger, context.allocator);
	}

	KDEBUG("Destroying Vulkan instance...");
	vkDestroyInstance(context.instance, context.allocator);
}


void vulkan_renderer_backend_on_resized(renderer_backend* backend, uint16_t width, uint16_t height) {

}

int8_t vulkan_renderer_backend_begin_frame(renderer_backend* backend, float delta_time) {
	return 1;
}

int8_t vulkan_renderer_backend_end_frame(renderer_backend* backend, float delta_time) {
	return 1;
}

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
	VkDebugUtilsMessageTypeFlagsEXT message_types,
	const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
	void* user_data) {

	switch (message_severity) {
	default:
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		KERROR(callback_data->pMessage);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		KWARN(callback_data->pMessage);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
		KINFO(callback_data->pMessage);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
		KTRACE(callback_data->pMessage);
		break;
	}

	return VK_FALSE;
}

int32_t find_memory_index(uint32_t type_filter, uint32_t property_flags) {
	VkPhysicalDeviceMemoryProperties memory_properties;

	vkGetPhysicalDeviceMemoryProperties(context.device.physical_device, &memory_properties);

	for (uint32_t i = 0; i < memory_properties.memoryTypeCount; ++i) {
		if (type_filter & (1 << 1) && (memory_properties.memoryTypes[i].propertyFlags & property_flags) == property_flags) {
			return i;
		}
	}

	KWARN("Unable to find suitable memory type");
	return -1;
}