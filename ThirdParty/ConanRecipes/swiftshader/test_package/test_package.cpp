#include <vulkan/vulkan.h>

#include <iostream>
#include <vector>

int main() {
    VkApplicationInfo applicationInfo = {};
    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.pApplicationName = "SwiftShader Conan test package";
    applicationInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &applicationInfo;

    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &instance);
    if (result != VK_SUCCESS) {
        std::cerr << "vkCreateInstance failed with VkResult " << result << std::endl;
        return 1;
    }

    uint32_t physicalDeviceCount = 0;
    result = vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
    if (result != VK_SUCCESS || physicalDeviceCount == 0) {
        std::cerr << "SwiftShader exposed no Vulkan physical devices" << std::endl;
        vkDestroyInstance(instance, nullptr);
        return 1;
    }

    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    result = vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());
    if (result != VK_SUCCESS) {
        std::cerr << "vkEnumeratePhysicalDevices failed with VkResult " << result << std::endl;
        vkDestroyInstance(instance, nullptr);
        return 1;
    }

    VkPhysicalDeviceProperties properties = {};
    vkGetPhysicalDeviceProperties(physicalDevices[0], &properties);
    std::cout << "Loaded Vulkan device: " << properties.deviceName << std::endl;
    vkDestroyInstance(instance, nullptr);
    return 0;
}
