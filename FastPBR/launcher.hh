#pragma once
#include "geometry.hh"
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

class Launcher
{
public:
	//Launcher();
	Launcher(int width, int height);
	//~Launcher();
	void launch();
	void setFramebufferResize(bool resized);

private:
	struct SwapChainSupportDetails {
		vk::SurfaceCapabilitiesKHR capabilities;
		std::vector<vk::SurfaceFormatKHR> formats;
		std::vector<vk::PresentModeKHR> presentModes;
	};

	static const int kMaxFramesInFlight = 2;

	size_t _currentFrame = 0;
	bool _framebufferResized = false;
	GLFWwindow *_window;
	vk::Extent2D _size{ 1280, 720 };
	vk::Extent2D _swapchainExtent;
	vk::Format _swapchainImageFormat;
	vk::Instance _instance;
	vk::Device _device;
	vk::PhysicalDevice _gpu;
	uint32_t _graphicsFamilyIndex;
	uint32_t _presentFamilyIndex;
	vk::Queue _graphicsQueue;
	vk::Queue _presentQueue;
	vk::SurfaceKHR _surface;
	std::vector<const char *> _deviceExtensions{ "VK_KHR_swapchain" };
	std::vector<vk::Image> _swapchainImages;
	std::vector<vk::ImageView> _swapchainImageViews;
	vk::SwapchainKHR _swapchain;
	vk::PipelineLayout _pipelineLayout;
	vk::RenderPass _renderPass;
	vk::Pipeline _graphicsPipeline;
	vk::PipelineCache _pipelineCache;
	std::vector<vk::Framebuffer> _swapchainFramebuffers;
	vk::CommandPool _commandPool;
	std::vector<vk::CommandBuffer> _commandBuffers;
	std::vector<vk::Semaphore> _imageAvailableSemaphore;
	std::vector<vk::Semaphore> _renderFinishedSemaphore;
	std::vector<vk::Fence> _inFlightFences;
	std::vector<Vertex> _vertices;
	vk::Buffer _vertexBuffer;
	vk::DeviceMemory _vertexBufferMemory;
	std::vector<uint16_t> _indices;
	vk::Buffer _indexBuffer;
	vk::DeviceMemory _indexBufferMemory;
	std::vector<vk::Buffer> _uniformBuffers;
	std::vector<vk::DeviceMemory> _uniformBufferMemories; 
	vk::DescriptorSetLayout _descriptorSetLayout;
	vk::DescriptorPool _descriptorPool;
	std::vector<vk::DescriptorSet> _descriptorSets;

	int initializeVulkan();
	void recreateSwapchain();
	void cleanupSwapchain();
	int run();
	vk::PhysicalDevice pickPhysicalDevice(std::vector<vk::PhysicalDevice> physicalDevices);
	bool deviceSupportsExtensions(vk::PhysicalDevice physicalDevice);
	SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice physicalDevice);
	vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& formats);
	vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& presentModes);
	vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);
	vk::ShaderModule Launcher::createShaderModule(const std::vector<char>& code);
	void createSwapChain();
	void createImageViews();
	void createGraphicsPipeline();
	void createRenderPass();
	void createFramebuffers();
	void createCommandPool();
	void createCommandBuffers();
	void drawFrame();
	void createSyncObjects();
	void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties,
		vk::Buffer& buffer, vk::DeviceMemory& deviceMemory);
	void copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size);
	void createVertexBuffer();
	void createIndexBuffer();
	uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
	void createDescriptorSetLayout();
	void createUniformBuffers();
	void updateUniformBuffer(uint32_t currentImage);
	void createDescriptorPool();
	void createDescriptorSets();
};
