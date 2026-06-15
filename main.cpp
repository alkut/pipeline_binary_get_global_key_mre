#include "vulkan/vulkan_core.h"
#include <cstdio>
#include <cstdlib>
#include <vector>

#ifndef _WIN32
#error Only Windows Supported
#endif

#define VALIDATION_LAYERS_ENABLED 0
#define EXIT_CODE_ON_ANY_VALIDATION_LAYER_ERROR 42

extern "C" {
struct HINSTANCE__;
typedef struct HINSTANCE__* HINSTANCE;
typedef HINSTANCE HMODULE;
typedef int (__stdcall *FARPROC)();

__declspec(dllimport) HMODULE __stdcall LoadLibraryA(const char* lpLibFileName);
__declspec(dllimport) FARPROC __stdcall GetProcAddress(HMODULE hModule, const char* lpProcName);
__declspec(dllimport) int __stdcall FreeLibrary(HMODULE hModule);
}

VkBool32 ValidationLayerDebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT /*messageSeverity*/,
        VkDebugUtilsMessageTypeFlagsEXT /*messageType*/,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
        void *const /*pUserData */
) {
    std::printf("Validation Layer: %s\n", pCallbackData->pMessage);
    std::fflush(stdout);
    std::exit(EXIT_CODE_ON_ANY_VALIDATION_LAYER_ERROR);
    return VK_TRUE;
}

VkDebugUtilsMessengerCreateInfoEXT GetDebugUtilsMessengerCreateInfo() {
    constexpr VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .pNext = nullptr,
            .flags = 0,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = ValidationLayerDebugCallback,
            .pUserData = nullptr,
    };
    return debugCreateInfo;
}

VkDebugUtilsMessengerEXT CreateDebugUtilsMessengerEXT(PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr, VkInstance instance, const VkAllocationCallbacks *pAllocator) {
    const auto vkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
    const VkDebugUtilsMessengerCreateInfoEXT createInfo = GetDebugUtilsMessengerCreateInfo();
    VkDebugUtilsMessengerEXT debugMessenger;
    vkCreateDebugUtilsMessengerEXT(instance, &createInfo, pAllocator, &debugMessenger);
    return debugMessenger;
}

void DestroyDebugUtilsMessengerEXT(PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr, VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator) {
    const auto vkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
    vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, pAllocator);
}

VkInstance CreateInstance(PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr, const VkAllocationCallbacks* pAllocator, bool enableValidationLayers) {
    const auto vkCreateInstance = reinterpret_cast<PFN_vkCreateInstance>(vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkCreateInstance"));
    static constexpr VkApplicationInfo applicationInfo{
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext = nullptr,
            .pApplicationName = nullptr,
            .applicationVersion = 0,
            .pEngineName = nullptr,
            .engineVersion = 0,
            .apiVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
    };
    const auto debugUtilsMessengerCreateInfo = GetDebugUtilsMessengerCreateInfo();
    constexpr const char* validationLayers[] = {
        "VK_LAYER_KHRONOS_validation",
    };
    constexpr const char* instanceExtensions[] = {
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME
    };
    const VkInstanceCreateInfo instanceCreateInfo{
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = (enableValidationLayers ? &debugUtilsMessengerCreateInfo : nullptr),
            .flags = 0,
            .pApplicationInfo = &applicationInfo,
            .enabledLayerCount = enableValidationLayers,
            .ppEnabledLayerNames = validationLayers,
            .enabledExtensionCount = enableValidationLayers,
            .ppEnabledExtensionNames = instanceExtensions,
    };
    VkInstance instance;
    vkCreateInstance(&instanceCreateInfo, pAllocator, &instance);
    return instance;
}

void DestroyInstance(PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr, VkInstance instance, const VkAllocationCallbacks* pAllocator) {
    const auto vkDestroyInstance = reinterpret_cast<PFN_vkDestroyInstance>(vkGetInstanceProcAddr(instance, "vkDestroyInstance"));
    vkDestroyInstance(instance, pAllocator);
}

VkPhysicalDevice GetPhysicalDevice(PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr, VkInstance instance) {
    const auto vkEnumeratePhysicalDevices = reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(vkGetInstanceProcAddr(instance, "vkEnumeratePhysicalDevices"));
    const auto vkGetPhysicalDeviceProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties>(vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties"));
    std::uint32_t physicalDeviceCount;
    vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());
    return *std::ranges::find_if(physicalDevices, [vkGetPhysicalDeviceProperties](VkPhysicalDevice physicalDevice) {
        VkPhysicalDeviceProperties physicalDeviceProperties;
        vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
        return physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
    });
}

VkDevice CreateDevice(PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr, VkInstance instance, VkPhysicalDevice physicalDevice, std::uint32_t queueFamilyIndex, const VkAllocationCallbacks* pAllocator) {
    const auto vkCreateDevice = reinterpret_cast<PFN_vkCreateDevice>(vkGetInstanceProcAddr(instance, "vkCreateDevice"));
    constexpr float kQueuePriority = 1.0f;
    const VkDeviceQueueCreateInfo deviceQueueCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueFamilyIndex = queueFamilyIndex,
            .queueCount = 1,
            .pQueuePriorities = &kQueuePriority,
    };
    const VkPhysicalDeviceFeatures enabledVulkan10Features{};
    VkPhysicalDevicePipelineBinaryFeaturesKHR pipelineBinaryFeatures{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_BINARY_FEATURES_KHR,
        .pNext = nullptr,
        .pipelineBinaries = VK_TRUE,
    };
    constexpr const char* deviceExtensions[] = {
        VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,
        VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME,
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
        VK_KHR_MAINTENANCE_5_EXTENSION_NAME,
        VK_KHR_PIPELINE_BINARY_EXTENSION_NAME,
    };
    const VkDeviceCreateInfo deviceCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = &pipelineBinaryFeatures,
            .flags = 0,
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &deviceQueueCreateInfo,
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = nullptr,
            .enabledExtensionCount = std::size(deviceExtensions),
            .ppEnabledExtensionNames = deviceExtensions,
            .pEnabledFeatures = &enabledVulkan10Features,
    };
    VkDevice device;
    vkCreateDevice(physicalDevice, &deviceCreateInfo, pAllocator, &device);
    return device;
}

void DestroyDevice(PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr, VkInstance instance, VkDevice device, const VkAllocationCallbacks* pAllocator) {
    const auto vkDestroyDevice = reinterpret_cast<PFN_vkDestroyDevice>(vkGetInstanceProcAddr(instance, "vkDestroyDevice"));
    vkDestroyDevice(device, pAllocator);
}

/*
MASM implementation:

.code
GetRDI proc
    mov rax, rdi
    ret
GetRDI endp
end
 */
extern "C" uint64_t GetRDI();

int main() {
    const VkAllocationCallbacks* pAllocator = nullptr;
    constexpr const char* kVulkanDllName = "vulkan-1.dll";
    HMODULE vulkanModule = LoadLibraryA(kVulkanDllName);
    const auto vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(GetProcAddress(vulkanModule, "vkGetInstanceProcAddr"));
    const auto instance = CreateInstance(vkGetInstanceProcAddr, pAllocator, /* enableValidationLayers= */ VALIDATION_LAYERS_ENABLED);
    const auto debugMessenger = VALIDATION_LAYERS_ENABLED ? CreateDebugUtilsMessengerEXT(vkGetInstanceProcAddr, instance, pAllocator) : VK_NULL_HANDLE;
    const auto physicalDevice = GetPhysicalDevice(vkGetInstanceProcAddr, instance);
    const auto queueFamilyIndex = 0;
    const auto device = CreateDevice(vkGetInstanceProcAddr, instance, physicalDevice, queueFamilyIndex, pAllocator);
    const auto vkGetDeviceProcAddr = reinterpret_cast<PFN_vkGetDeviceProcAddr>(vkGetInstanceProcAddr(instance, "vkGetDeviceProcAddr"));


    const auto vkGetPipelineKeyKHR = reinterpret_cast<PFN_vkGetPipelineKeyKHR>(vkGetDeviceProcAddr(device, "vkGetPipelineKeyKHR"));
    VkPipelineBinaryKeyKHR globalKey{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_BINARY_KEY_KHR,
		.pNext = nullptr,
		.keySize = 0,
		.key = {},
	};
    const auto RDIBeforeCall = GetRDI();
    vkGetPipelineKeyKHR(device, nullptr, &globalKey);
    const auto RDIAfterCall = GetRDI();
    std::printf("Before: 0x%016llx\nAfter:  0x%016llx\n", RDIBeforeCall, RDIAfterCall);



    DestroyDevice(vkGetInstanceProcAddr, instance, device, pAllocator);
    if constexpr (VALIDATION_LAYERS_ENABLED) {
        DestroyDebugUtilsMessengerEXT(vkGetInstanceProcAddr, instance, debugMessenger, pAllocator);
    }
    DestroyInstance(vkGetInstanceProcAddr, instance, pAllocator);
    FreeLibrary(vulkanModule);
}
