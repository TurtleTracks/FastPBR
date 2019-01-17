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

	int initializeVulkan();
	int run();
	vk::PhysicalDevice pickPhysicalDevice(std::vector<vk::PhysicalDevice> physicalDevices);
	bool deviceSupportsExtensions(vk::PhysicalDevice physicalDevice);
	SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice physicalDevice);
	vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& formats);
	vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& presentModes);
	vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);
	vk::ShaderModule Launcher::createShaderModule(const std::vector<char>& code);
	void createSwapChain();
	void Launcher::createImageViews();
	void Launcher::createGraphicsPipeline();
};
