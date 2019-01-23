#pragma once
class Launcher
{
public:
	//Launcher();
	Launcher(int width, int height);
	//~Launcher();
	void launch();

private:
	struct SwapChainSupportDetails {
		vk::SurfaceCapabilitiesKHR capabilities;
		std::vector<vk::SurfaceFormatKHR> formats;
		std::vector<vk::PresentModeKHR> presentModes;
	};

	static const int kMaxFramesInFlight = 2;

	size_t _currentFrame = 0;
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
	std::vector<const char*> deviceExtensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
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
};
