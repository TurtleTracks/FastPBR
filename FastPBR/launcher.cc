#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <fstream>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <iostream>
#include "kernel.h"
#include "launcher.hh"

int main() {
	Launcher app = Launcher(1280, 720);
	app.launch();
	return 0;
}

Launcher::Launcher(int width, int height)
{
	_size.width = width;
	_size.height = height;
}

void Launcher::launch()
{
	run();
	//size.width = 1024;
	//size.height = 720;
}

static std::vector<char> readFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}
	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();

	return buffer;
}

int Launcher::initializeVulkan()
{
	vk::ApplicationInfo appInfo = {};
	appInfo.pApplicationName = "Hello Triangle";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	vk::InstanceCreateInfo createInfo = {};
	createInfo.pApplicationInfo = &appInfo;


	createInfo.enabledExtensionCount = glfwExtensionCount;
	createInfo.ppEnabledExtensionNames = glfwExtensions;

	createInfo.enabledLayerCount = 0;
	// throw not needed, vulkan-hpp already throws
	if (vk::createInstance(&createInfo, nullptr, &_instance) != vk::Result::eSuccess) 
	{
		throw std::runtime_error("failed to create instance!");
	}	

	// create surface
	
	if (glfwCreateWindowSurface(_instance, _window, nullptr, reinterpret_cast<VkSurfaceKHR*>(&_surface))
		!= VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}

	// Pick Physical Device 

	std::vector<vk::PhysicalDevice> physicalDevices = _instance.enumeratePhysicalDevices();
	if (physicalDevices.size() == 0) 
	{
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
	}
	_gpu = pickPhysicalDevice(physicalDevices);

	// create logical device
	float queuePriority = 1.0f;
	std::vector<vk::DeviceQueueCreateInfo> queueCreateInfo = std::vector<vk::DeviceQueueCreateInfo>(2);
	queueCreateInfo[0].queueFamilyIndex = _graphicsFamilyIndex;
	queueCreateInfo[0].queueCount = 1;
	queueCreateInfo[0].pQueuePriorities = &queuePriority;

	queueCreateInfo[1].queueFamilyIndex = _presentFamilyIndex;
	queueCreateInfo[1].queueCount = 1;
	queueCreateInfo[1].pQueuePriorities = &queuePriority;
	vk::PhysicalDeviceFeatures deviceFeatures = {};

	vk::DeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfo.data();
	deviceCreateInfo.queueCreateInfoCount = 2;
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
	deviceCreateInfo.enabledExtensionCount = deviceExtensions.size();
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

	// alternatively, something about validation layer data
	deviceCreateInfo.enabledLayerCount = 0;

	_device = _gpu.createDevice(deviceCreateInfo);
	_graphicsQueue = _device.getQueue(_graphicsFamilyIndex, 0);
	_presentQueue = _device.getQueue(_presentFamilyIndex, 0);

	createSwapChain();

	return 0;
}

vk::PhysicalDevice Launcher::pickPhysicalDevice(std::vector<vk::PhysicalDevice> physicalDevices) {
	for (const auto& device : physicalDevices) {
		// if device is suitable, return 
		auto deviceProperties = device.getProperties();
		if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
			// check if right queue families or whatever
			uint32_t i = 0;
			auto familyProperties = device.getQueueFamilyProperties();
			for (const auto& queueFamily : familyProperties) {
				bool foundGraphicsFamily = false;
				bool foundPresentFamily = false;
				if (queueFamily.queueCount > 0) {
					if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
						// device has queue family with graphics queues
						// or whatever
						_graphicsFamilyIndex = i;
						foundGraphicsFamily = true;
					}
					if (device.getSurfaceSupportKHR(i, _surface)) {
						_presentFamilyIndex = i;
						foundPresentFamily = true;
					}
					if (foundGraphicsFamily && foundPresentFamily 
						&& deviceSupportsExtensions(device)) {
						return device;
					}
					i++;
				}
			}
		}
	}
	return nullptr;
}

bool Launcher::deviceSupportsExtensions(vk::PhysicalDevice physicalDevice) {
	uint32_t extensionCount; 
	auto extensionProperties = physicalDevice.enumerateDeviceExtensionProperties();
	
	int32_t numExtensions = deviceExtensions.size();
	for (const auto& extension : extensionProperties) {
		for (const auto&  needed : deviceExtensions) {
			if (extension.extensionName == needed) {
				numExtensions--;
			}
		}
	}
	if (numExtensions == 0) {
		auto swapChainSupport = querySwapChainSupport(physicalDevice);
		return !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}
}

Launcher::SwapChainSupportDetails Launcher::querySwapChainSupport(vk::PhysicalDevice physicalDevice) {
	SwapChainSupportDetails details;
	details.capabilities = physicalDevice.getSurfaceCapabilitiesKHR(_surface);
	details.formats = physicalDevice.getSurfaceFormatsKHR(_surface);
	details.presentModes = physicalDevice.getSurfacePresentModesKHR(_surface);
	return details;
}

vk::SurfaceFormatKHR Launcher::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& formats) {
	if (formats.size() == 1 && formats[0].format == vk::Format::eUndefined) {
		return { vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear };
	}

	for (const auto& format : formats) {
		if (format.format == vk::Format::eB8G8R8A8Unorm
			&& format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
			return format;
		}
	}

	return formats[0];
}

vk::PresentModeKHR Launcher::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& presentModes) {
	for (const auto& presentMode : presentModes) {
		if (presentMode == vk::PresentModeKHR::eMailbox) {
			return presentMode;
		}
	}
	return vk::PresentModeKHR::eFifo;
}

vk::Extent2D Launcher::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) {
	// I'm not going to worry returning anything other than the expected resolution
	return { _size.width, _size.height };
}

void Launcher::createSwapChain() {
	auto swapChainSupport = querySwapChainSupport(_gpu);
	auto surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	auto presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	_swapchainExtent = chooseSwapExtent(swapChainSupport.capabilities);
	_swapchainImageFormat = surfaceFormat.format;

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	vk::SwapchainCreateInfoKHR swapchainCreateInfo;
	swapchainCreateInfo.surface = _surface;
	swapchainCreateInfo.minImageCount = imageCount;
	swapchainCreateInfo.imageFormat = surfaceFormat.format;
	swapchainCreateInfo.imageExtent = _swapchainExtent;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
	swapchainCreateInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	swapchainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
	swapchainCreateInfo.presentMode = presentMode;
	swapchainCreateInfo.clipped = VK_TRUE;
	swapchainCreateInfo.setOldSwapchain(nullptr);

	if (_graphicsFamilyIndex != _presentFamilyIndex) {
		swapchainCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
		swapchainCreateInfo.queueFamilyIndexCount = 2; 
		uint32_t indices[] = { _graphicsFamilyIndex, _presentFamilyIndex };
		swapchainCreateInfo.pQueueFamilyIndices = indices;
	} else {
		swapchainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
	}
	_swapchain = _device.createSwapchainKHR(swapchainCreateInfo);
	_swapchainImages = _device.getSwapchainImagesKHR(_swapchain);
}

void Launcher::createImageViews() {
	_swapchainImageViews.resize(_swapchainImages.size());

	for (size_t i = 0; i < _swapchainImages.size(); ++i) {
		vk::ImageViewCreateInfo imCreateInfo;
		imCreateInfo.image = _swapchainImages[i];

		imCreateInfo.viewType = vk::ImageViewType::e2D;
		imCreateInfo.format = _swapchainImageFormat;
		imCreateInfo.components.r = vk::ComponentSwizzle::eIdentity;
		imCreateInfo.components.g = vk::ComponentSwizzle::eIdentity;
		imCreateInfo.components.b = vk::ComponentSwizzle::eIdentity;
		imCreateInfo.components.a = vk::ComponentSwizzle::eIdentity;

		imCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		imCreateInfo.subresourceRange.baseMipLevel = 0;
		imCreateInfo.subresourceRange.levelCount = 1;
		imCreateInfo.subresourceRange.baseArrayLayer = 0;
		imCreateInfo.subresourceRange.layerCount = 1;

		_swapchainImageViews[i] = _device.createImageView(imCreateInfo);
	}
}

void Launcher::createGraphicsPipeline() {
	auto vertShaderCode = readFile("shaders/vert.spv");
	auto fragShaderCode = readFile("shaders/frag.spv");

	vk::ShaderModule vertShaderModule = createShaderModule(vertShaderCode);
	vk::ShaderModule fragShaderModule = createShaderModule(fragShaderCode);

	vk::PipelineShaderStageCreateInfo vertShaderStageInfo;
	vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	vk::PipelineShaderStageCreateInfo fragShaderStageInfo;
	fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	auto shaderStages = { vertShaderStageInfo, fragShaderStageInfo };
	// cleanup
	_device.destroyShaderModule(vertShaderModule);
	_device.destroyShaderModule(fragShaderModule);
}

vk::ShaderModule Launcher::createShaderModule(const std::vector<char>& code) {
	vk::ShaderModuleCreateInfo createInfo;

	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	return _device.createShaderModule(createInfo);
}

int Launcher::run()
{
	// Set Context, Create Window
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	_window = glfwCreateWindow(_size.width, _size.height, "FastPBR", nullptr, nullptr);
	if (_window == nullptr)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwSetWindowUserPointer(_window, (void*)this);
	glfwMakeContextCurrent(_window);
	//glfwSetKeyCallback(window, key_callback);
	//glfwSetMouseButtonCallback(window, mouse_callback);

	initializeVulkan();

	// Game Loop
	while (!glfwWindowShouldClose(_window))
	{
		glfwPollEvents();
		//updateSystem(window);
		//renderScene();
		//glfwSwapBuffers(window);
	}
	//vkDestroyInstance(_instance, nullptr);

	for (auto imageView : _swapchainImageViews) {
		_device.destroyImageView(imageView);
	}
	_device.destroySwapchainKHR(_swapchain);
	_device.destroy();
	_instance.destroySurfaceKHR(_surface);
	_instance.destroy();
	glfwDestroyWindow(_window);

	glfwTerminate();
	return 0;
}
