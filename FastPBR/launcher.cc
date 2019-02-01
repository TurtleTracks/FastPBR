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

Launcher::Launcher(int width, int height) {
	_size.width = width;
	_size.height = height;
}

void Launcher::launch() {
	run();
}

void Launcher::setFramebufferResize(bool resized) {
	_framebufferResized = resized;
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

int Launcher::initializeVulkan() {
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
	createImageViews();
	createRenderPass();
	createGraphicsPipeline();
	createFramebuffers();
	createCommandPool();
	createCommandBuffers();
	createSyncObjects();
	return 0;
}

void Launcher::recreateSwapchain() {
	// pause rendering while app is minimized
	int width = 0, height = 0;
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(_window, &width, &height);
		glfwWaitEvents();
	}

	_device.waitIdle();

	cleanupSwapchain();

	createSwapChain();
	createImageViews();
	createRenderPass();
	createGraphicsPipeline();
	createFramebuffers();
	createCommandBuffers();
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
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	}
	else {
		return { _size.width, _size.height };
	}
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
	swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapchainCreateInfo.imageExtent = _swapchainExtent;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
	swapchainCreateInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	swapchainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
	swapchainCreateInfo.presentMode = presentMode;
	swapchainCreateInfo.clipped = VK_TRUE;
	swapchainCreateInfo.setOldSwapchain(_swapchain);

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

	vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	// pVertex[xxx]Descriptions refer to array of structs
	vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
	vertexInputInfo.setVertexBindingDescriptionCount(0);
	vertexInputInfo.setPVertexAttributeDescriptions(nullptr);
	vertexInputInfo.setVertexAttributeDescriptionCount(0);
	vertexInputInfo.setPVertexAttributeDescriptions(nullptr);

	vk::PipelineInputAssemblyStateCreateInfo inputAssembly; 
	inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
	inputAssembly.setPrimitiveRestartEnable(false);

	vk::Viewport viewport;
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = _swapchainExtent.width;
	viewport.height = _swapchainExtent.height;
	viewport.minDepth = 0;
	viewport.maxDepth = 1;

	// something about scissors
	vk::Rect2D scissor;
	scissor.offset = { 0, 0 };
	scissor.extent = _swapchainExtent;

	vk::PipelineViewportStateCreateInfo viewportState; 
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	// Rasterizer 
	vk::PipelineRasterizationStateCreateInfo rasterizer; 
	rasterizer.depthClampEnable = false; 
	rasterizer.rasterizerDiscardEnable = false;
	rasterizer.polygonMode = vk::PolygonMode::eFill;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = vk::CullModeFlagBits::eBack;
	rasterizer.frontFace = vk::FrontFace::eClockwise;
	rasterizer.depthBiasEnable = false;

	// multisampling 
	vk::PipelineMultisampleStateCreateInfo multisampling; 
	multisampling.sampleShadingEnable = false;
	multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;

	// color blending 
	vk::PipelineColorBlendAttachmentState colorBlendAttachment;
	colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR 
		| vk::ColorComponentFlagBits::eG| vk::ColorComponentFlagBits::eB
		| vk::ColorComponentFlagBits::eA;
	colorBlendAttachment.blendEnable = false;

	// dynamic state
	vk::DynamicState dynamicStates[] = {
		vk::DynamicState::eViewport,
		vk::DynamicState::eLineWidth
	};

	vk::PipelineDynamicStateCreateInfo dynamicState;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates;

	vk::PipelineColorBlendStateCreateInfo colorBlending;
	colorBlending.logicOpEnable = false;
	colorBlending.logicOp = vk::LogicOp::eCopy;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;

	// pipeline layout
	vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo; 
	_device.createPipelineLayout(&pipelineLayoutCreateInfo, nullptr, &_pipelineLayout);

	// create graphics pipeline
	vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo;
	graphicsPipelineCreateInfo.stageCount = 2;
	graphicsPipelineCreateInfo.pStages = shaderStages;
	graphicsPipelineCreateInfo.pVertexInputState = &vertexInputInfo;
	graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssembly;
	graphicsPipelineCreateInfo.pViewportState = &viewportState;
	graphicsPipelineCreateInfo.pRasterizationState = &rasterizer;
	graphicsPipelineCreateInfo.pMultisampleState = &multisampling;
	graphicsPipelineCreateInfo.pDepthStencilState = nullptr;	// optional
	graphicsPipelineCreateInfo.pColorBlendState = &colorBlending;
	graphicsPipelineCreateInfo.pDynamicState = nullptr;		//optional 
	graphicsPipelineCreateInfo.layout = _pipelineLayout;
	graphicsPipelineCreateInfo.renderPass = _renderPass;
	graphicsPipelineCreateInfo.subpass = 0;

	graphicsPipelineCreateInfo.basePipelineHandle = nullptr; // Optional
	graphicsPipelineCreateInfo.basePipelineIndex = -1; // Optional
	
	vk::PipelineCacheCreateInfo pipelineCacheCreateInfo;
	_pipelineCache = _device.createPipelineCache(pipelineCacheCreateInfo);
	_device.createGraphicsPipelines(_pipelineCache, 1, &graphicsPipelineCreateInfo,
									nullptr, &_graphicsPipeline);
	// cleanup
	_device.destroyShaderModule(vertShaderModule);
	_device.destroyShaderModule(fragShaderModule);
}

void Launcher::createRenderPass() {
	vk::AttachmentDescription colorAttachment;
	colorAttachment.format = _swapchainImageFormat;
	colorAttachment.samples = vk::SampleCountFlagBits::e1;
	colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
	colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
	colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
	colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

	vk::AttachmentReference colorAttachmentReference;
	colorAttachmentReference.attachment = 0;
	colorAttachmentReference.layout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::SubpassDescription subpass;
	subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentReference;

	vk::SubpassDependency subpassDependency;
	subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependency.dstSubpass = 0;
	subpassDependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	subpassDependency.srcAccessMask = vk::AccessFlagBits::eColorAttachmentRead;
	subpassDependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	subpassDependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead |
		vk::AccessFlagBits::eColorAttachmentWrite;

	vk::RenderPassCreateInfo renderPassCreateInfo; 
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments = &colorAttachment;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;
	renderPassCreateInfo.dependencyCount = 1;
	renderPassCreateInfo.pDependencies = &subpassDependency;

	_renderPass = _device.createRenderPass(renderPassCreateInfo, nullptr);
}

void Launcher::createFramebuffers() {
	_swapchainFramebuffers.resize(_swapchainImageViews.size());
	for (size_t i = 0; i < _swapchainImageViews.size(); i++) {
		vk::ImageView attachments[] = {
			_swapchainImageViews[i]
		};

		vk::FramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.renderPass = _renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = _swapchainExtent.width;
		framebufferInfo.height = _swapchainExtent.height;
		framebufferInfo.layers = 1;

		_swapchainFramebuffers[i] = _device.createFramebuffer(framebufferInfo, nullptr);
	}
}

void Launcher::createCommandPool() {

	vk::CommandPoolCreateInfo poolCreateInfo;
	poolCreateInfo.queueFamilyIndex = _graphicsFamilyIndex;
	_commandPool = _device.createCommandPool(poolCreateInfo);
}

void Launcher::createCommandBuffers() {
	_commandBuffers.resize(_swapchainFramebuffers.size());

	vk::CommandBufferAllocateInfo commandBufferAllocateInfo;
	commandBufferAllocateInfo.commandPool = _commandPool;
	commandBufferAllocateInfo.level = vk::CommandBufferLevel::ePrimary;
	commandBufferAllocateInfo.commandBufferCount = (uint32_t) _commandBuffers.size();

	_commandBuffers = _device.allocateCommandBuffers(commandBufferAllocateInfo);

	for (size_t i = 0; i < _commandBuffers.size(); ++i) {
		vk::CommandBufferBeginInfo commandBufferBeginInfo;
		commandBufferBeginInfo.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse;
		
		_commandBuffers[i].begin(commandBufferBeginInfo);

		vk::RenderPassBeginInfo renderPassBeginInfo;
		renderPassBeginInfo.renderPass = _renderPass;
		renderPassBeginInfo.framebuffer = _swapchainFramebuffers[i];
		renderPassBeginInfo.renderArea.offset = { 0, 0 };
		renderPassBeginInfo.renderArea.extent = _swapchainExtent;

		vk::ClearValue clearColor;
		clearColor.color.setFloat32({ 0, 0, 0, 1 });
		renderPassBeginInfo.clearValueCount = 1;
		renderPassBeginInfo.pClearValues = &clearColor;

		_commandBuffers[i].beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

		_commandBuffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, _graphicsPipeline);
		_commandBuffers[i].draw(3, 1, 0, 0);

		_commandBuffers[i].endRenderPass();
		_commandBuffers[i].end();
	}

	
}

vk::ShaderModule Launcher::createShaderModule(const std::vector<char>& code) {
	vk::ShaderModuleCreateInfo createInfo;

	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	return _device.createShaderModule(createInfo);
}

void Launcher::createSyncObjects() {
	vk::SemaphoreCreateInfo semaphoreCreateInfo;
	vk::FenceCreateInfo fenceCreateInfo;
	fenceCreateInfo.flags = vk::FenceCreateFlagBits::eSignaled;
	for (size_t i = 0; i < kMaxFramesInFlight; ++i) {
		_imageAvailableSemaphore.push_back(_device.createSemaphore(semaphoreCreateInfo));
		_renderFinishedSemaphore.push_back(_device.createSemaphore(semaphoreCreateInfo));
		_inFlightFences.push_back(_device.createFence(fenceCreateInfo));
	}
}

void Launcher::drawFrame() {
	_device.waitForFences(1, &_inFlightFences[_currentFrame], true, std::numeric_limits<uint64_t>::max());
	auto imageIndex = _device.acquireNextImageKHR(_swapchain,
		std::numeric_limits<uint64_t>::max(),
		_imageAvailableSemaphore[_currentFrame], nullptr);

	if (imageIndex.result == vk::Result::eErrorOutOfDateKHR) {
		recreateSwapchain();
		return;
	} 

	vk::SubmitInfo submitInfo;

	vk::Semaphore waitSemaphores[] = { _imageAvailableSemaphore[_currentFrame] };
	vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };

	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &_commandBuffers[imageIndex.value];

	vk::Semaphore signalSemaphores[] = { _renderFinishedSemaphore[_currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	_device.resetFences(1, &_inFlightFences[_currentFrame]);

	_graphicsQueue.submit(1, &submitInfo, _inFlightFences[_currentFrame]);

	// Presentation stage
	vk::PresentInfoKHR presentInfo;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	vk::SwapchainKHR swapchains[] = { _swapchain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapchains;
	presentInfo.pImageIndices = &(imageIndex.value);
	
	auto presentQueueResult =_presentQueue.presentKHR(presentInfo);

	if (presentQueueResult == vk::Result::eErrorOutOfDateKHR
		|| presentQueueResult == vk::Result::eSuboptimalKHR
		|| _framebufferResized) {
		recreateSwapchain();
		_framebufferResized = false;
	}

	// wait for work to finish
	_currentFrame = (++_currentFrame) % kMaxFramesInFlight;
}

static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
	auto app = reinterpret_cast<Launcher*>(glfwGetWindowUserPointer(window));
	app->setFramebufferResize(true);
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
	glfwSetFramebufferSizeCallback(_window, framebufferResizeCallback);
	//glfwSetKeyCallback(window, key_callback);
	//glfwSetMouseButtonCallback(window, mouse_callback);

	initializeVulkan();

	// Game Loop
	while (!glfwWindowShouldClose(_window))
	{
		glfwPollEvents();
		drawFrame();
	}
	_device.waitIdle();

	cleanupSwapchain();
	for (int i = 0; i < kMaxFramesInFlight; ++i) {
		_device.destroySemaphore(_imageAvailableSemaphore[i]);
		_device.destroySemaphore(_renderFinishedSemaphore[i]);
		_device.destroyFence(_inFlightFences[i]);
	}
	_device.destroyCommandPool(_commandPool);
	_device.destroy();
	_instance.destroySurfaceKHR(_surface);
	_instance.destroy();
	glfwDestroyWindow(_window);

	glfwTerminate();
	return 0;
}

void Launcher::cleanupSwapchain() {
	// destroy created objects
	for (auto framebuffer : _swapchainFramebuffers) {
		_device.destroyFramebuffer(framebuffer);
	}
	for (auto imageView : _swapchainImageViews) {
		_device.destroyImageView(imageView);
	}
	_device.destroyPipeline(_graphicsPipeline);
	_device.destroyPipelineLayout(_pipelineLayout);
	_device.destroyRenderPass(_renderPass);
	_device.destroySwapchainKHR(_swapchain);
}