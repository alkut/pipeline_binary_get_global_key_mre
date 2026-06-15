## Minimal reproducable example

https://forums.developer.nvidia.com/t/possible-stack-buffer-overflow-by-1-caused-overwrite-callee-saved-register-in-vkgetpipelinekeykhr-when-ppipelinecreateinfo-nullptr-get-global-key/373306

## TL;DR

```
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
```

## Requirements

- cmake 3.31 or newer
- c++20 compiler (only msvc tested)
- vulkan headers (only Vulkan SDK 1.4.350 tested)

## Build steps

From project root directory:

```
mkdir build
cmake -S . -B ./build
cmake --build ./build
./build/Debug/pipeline_binary_get_global_key_mre.exe
```

## Sample output

```
alexs@DESKTOP-TCAEACT MINGW64 ~/source/repos/alkut/pipeline_binary_get_global_key_mre (master)
$ ./build/Debug/pipeline_binary_get_global_key_mre.exe
Before: 0x000000cc23eff6e0
After:  0x000000cc00000001
```