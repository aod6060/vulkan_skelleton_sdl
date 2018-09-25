#define NOMINMAX

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <ctime>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <optional>
#include <set>

#include <SDL/SDL.h>
#include <SDL/SDL_syswm.h>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

std::string caption = "Vulkan";
uint32_t width = 800;
uint32_t height = 600;
bool running = true;
SDL_Window* window = nullptr;


void app_init();
void app_release();
void app_update(float delta);
void app_render();

int main(int argc, char** argv)
{
	SDL_Init(SDL_INIT_EVERYTHING);
	window = SDL_CreateWindow(
		caption.c_str(),
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		width,
		height,
		SDL_WINDOW_SHOWN
	);
	SDL_Event e;
	uint32_t pre = SDL_GetTicks();
	uint32_t curr = 0;
	float delta = 0.0f;
	app_init();

	while (running)
	{

		curr = SDL_GetTicks();
		delta = (curr - pre) / 1000.0f;
		pre = curr;

		while (SDL_PollEvent(&e))
		{
			if (e.type == SDL_QUIT)
			{
				running = false;
			}
		}


		app_update(delta);
		app_render();
	}

	app_release();
	SDL_DestroyWindow(window);
	SDL_Quit();
	
	std::getchar();

	return 0;
}

struct QueueFamilyIndices
{
	//int32_t graphicsFamily = -1;
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool isCompete()
	{
		return 
			this->graphicsFamily.has_value() && 
			this->presentFamily.has_value();
	}
};

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR caps;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct VulkanTest
{
	bool useLayer = true;

	// Instance
	VkInstance instance;

	// Debug Callback
	VkDebugReportCallbackEXT debugCallback;

	// Surface
	VkSurfaceKHR surface;

	// Physical Device
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

	// Devices
	VkDevice device;
	VkQueue graphicsQueue;
	VkQueue presentQueue;

	// Swap Chain
	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	std::vector<VkImageView> swapChainImageViews;
	uint32_t swapChainIndex = 0;

	// Command Pool
	VkCommandPool commandPool;

	// Command Buffer List
	std::vector<VkCommandBuffer> commandBufferList;

	// Render Pass
	VkRenderPass clearRenderPass;
	VkRenderPass drawRenderPass;

	// Framebuffers
	std::vector<VkFramebuffer> clearFramebuffers;
	std::vector<VkFramebuffer> drawFramebuffers;

	// Semaphores
	VkSemaphore imageAvailable;
	VkSemaphore renderFinish;

	// Fence
	VkFence inFlight;

	void init();
	
	void clear(const glm::vec3& color);

	void present();

	void release();

	void createInstance();
	void createDebugReportCallback();

	void createSurface();

	void createPhysicalDevice();
	bool isDeviceSuitable(VkPhysicalDevice device);
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

	void createLogicalDevice();

	void createSwapChain();

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& presentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& caps);

	void createSwapChainImageViews();

	void createCommandPool();

	void createRenderPass();

	void createFramebuffers();

	void createSemaphore();

	void createFence();

	VkCommandBuffer allocCommandBuffer();

	VkCommandBuffer clearCommand(glm::vec3 color);
};

VulkanTest test;

void app_init()
{
	test.init();
}

void app_release()
{
	test.release();
}

void app_update(float delta)
{
}

void app_render()
{
	test.clear(glm::vec3(1.0f, 0.0f, 0.0f));


	test.present();
}

void VulkanTest::init()
{
	this->createInstance();
	
	if (this->useLayer)
	{
		this->createDebugReportCallback();
	}

	this->createSurface();

	this->createPhysicalDevice();
	this->createLogicalDevice();
	this->createSwapChain();

	this->createSwapChainImageViews();

	this->createCommandPool();

	this->createRenderPass();

	this->createFramebuffers();

	this->createSemaphore();

	this->createFence();
}

void VulkanTest::clear(const glm::vec3& color)
{
	vkAcquireNextImageKHR(
		device,
		swapChain,
		std::numeric_limits<uint64_t>::max(),
		imageAvailable,
		VK_NULL_HANDLE,
		&swapChainIndex
	);

	this->commandBufferList.push_back(
		this->clearCommand(color)
	);
}

void VulkanTest::present()
{
	VkResult r;

	vkWaitForFences(device, 1, &this->inFlight, VK_TRUE, std::numeric_limits<uint64_t>::max());
	vkResetFences(device, 1, &this->inFlight);

	// Submit
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &this->imageAvailable;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = this->commandBufferList.size();
	submitInfo.pCommandBuffers = commandBufferList.data();

	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &renderFinish;

	r = vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlight);

	if (r != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to submit to graphics queue...");
	}

	// Present
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &this->renderFinish;

	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &this->swapChain;
	presentInfo.pImageIndices = &this->swapChainIndex;

	r = vkQueuePresentKHR(this->presentQueue, &presentInfo);

	vkQueueWaitIdle(this->presentQueue);

	// Release Command Buffer List
	vkFreeCommandBuffers(
		device,
		this->commandPool,
		this->commandBufferList.size(),
		this->commandBufferList.data()
	);

	this->commandBufferList.clear();
}

void VulkanTest::release()
{
	vkDestroyFence(device, inFlight, nullptr);

	vkDestroySemaphore(device, renderFinish, nullptr);
	vkDestroySemaphore(device, this->imageAvailable, nullptr);

	for (uint32_t i = 0; i < this->drawFramebuffers.size(); i++)
	{
		vkDestroyFramebuffer(this->device, this->drawFramebuffers[i], nullptr);
	}

	for (uint32_t i = 0; i < this->clearFramebuffers.size(); i++)
	{
		vkDestroyFramebuffer(this->device, this->clearFramebuffers[i], nullptr);
	}

	vkDestroyRenderPass(this->device, this->drawRenderPass, nullptr);
	vkDestroyRenderPass(this->device, this->clearRenderPass, nullptr);

	vkDestroyCommandPool(this->device, this->commandPool, nullptr);

	for (auto imageView : this->swapChainImageViews)
	{
		vkDestroyImageView(device, imageView, nullptr);
	}

	vkDestroySwapchainKHR(device, swapChain, nullptr);

	vkDestroyDevice(device, nullptr);

	vkDestroySurfaceKHR(instance, surface, nullptr);

	if (this->useLayer)
	{
		auto debugDestroy = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(this->instance, "vkDestroyDebugReportCallbackEXT");
		debugDestroy(instance, this->debugCallback, nullptr);
	}

	vkDestroyInstance(instance, nullptr);
}

void VulkanTest::createInstance()
{
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = caption.c_str();
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = caption.c_str();
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_1;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	std::vector<const char*> layers;
	std::vector<const char*> extensions;

	// Layers
	if (this->useLayer)
	{
		layers.push_back("VK_LAYER_LUNARG_standard_validation");
		extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}

	extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);


	if (layers.size() > 0)
	{
		createInfo.enabledLayerCount = layers.size();
		createInfo.ppEnabledLayerNames = layers.data();
	}

	if (extensions.size() > 0)
	{
		createInfo.enabledExtensionCount = extensions.size();
		createInfo.ppEnabledExtensionNames = extensions.data();
	}

	VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create instance!");
	}
}

static VkBool32 VKAPI_CALL _debugCallback(
	VkDebugReportFlagsEXT			flags,
	VkDebugReportObjectTypeEXT		objType,
	uint64_t						obj,
	size_t							location,
	int32_t							msgCode,
	const char*						layerPrefix,
	const char*						msg,
	void*							useData);

void VulkanTest::createDebugReportCallback()
{
	VkDebugReportCallbackCreateInfoEXT debugInfo = {};
	debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
	debugInfo.pfnCallback = _debugCallback;
	debugInfo.flags = VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT;

	auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");

	if (func != nullptr)
	{
		VkResult r = func(instance, &debugInfo, nullptr, &this->debugCallback);

		if (r != VK_SUCCESS)
		{
			throw std::runtime_error("Didn't create debug report callback.");
		}
	}
}

static VkBool32 VKAPI_CALL _debugCallback(
	VkDebugReportFlagsEXT			flags,
	VkDebugReportObjectTypeEXT		objType,
	uint64_t						obj,
	size_t							location,
	int32_t							msgCode,
	const char*						layerPrefix,
	const char*						msg,
	void*							useData)
{
	std::cout << location << std::endl;
	std::cout << layerPrefix << ": " << msg << std::endl;
	return VK_FALSE;
}

void VulkanTest::createSurface()
{
	SDL_SysWMinfo info;
	SDL_VERSION(&info.version);
	SDL_GetWindowWMInfo(window, &info);

	VkWin32SurfaceCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.hinstance = info.info.win.hinstance;
	createInfo.hwnd = info.info.win.window;

	auto func = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR");

	if (func != nullptr)
	{
		VkResult r = func(instance, &createInfo, nullptr, &this->surface);

		if (r != VK_SUCCESS)
		{
			throw std::runtime_error("Surface wasn't created");
		}
	}
	else
	{
		throw std::runtime_error("Vulkan surface wasn't create due to extensions");
	}
}

void VulkanTest::createPhysicalDevice()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (deviceCount == 0)
	{
		throw std::runtime_error("Failed to find GPUs with Vulkan support!");
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	for (const auto& device : devices)
	{
		if (this->isDeviceSuitable(device))
		{
			this->physicalDevice = device;
			break;
		}
	}

	if (this->physicalDevice == VK_NULL_HANDLE)
	{
		throw std::runtime_error("Failed to find a suitable GPU!");
	}


}

bool VulkanTest::isDeviceSuitable(VkPhysicalDevice device)
{
	QueueFamilyIndices indices = this->findQueueFamilies(device);
	return indices.isCompete();
}

QueueFamilyIndices VulkanTest::findQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilies)
	{
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.graphicsFamily = i;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

		if (queueFamily.queueCount > 0 && presentSupport)
		{
			indices.presentFamily = i;
		}

		if (indices.isCompete())
		{
			break;
		}

		i++;
	}

	return indices;
}

void VulkanTest::createLogicalDevice()
{
	QueueFamilyIndices indices = this->findQueueFamilies(this->physicalDevice);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = {
		indices.graphicsFamily.value(),
		indices.presentFamily.value()
	};

	float queuePriority = 1.0f;

	for (uint32_t queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures = {};

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = queueCreateInfos.size();

	createInfo.pEnabledFeatures = &deviceFeatures;

	std::vector<const char*> layers;
	std::vector<const char*> extensions;

	if (this->useLayer)
	{
		layers.push_back("VK_LAYER_LUNARG_standard_validation");
	}

	extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

	if (layers.size() > 0)
	{
		createInfo.enabledLayerCount = layers.size();
		createInfo.ppEnabledLayerNames = layers.data();
	}

	if (extensions.size() > 0)
	{
		createInfo.enabledExtensionCount = extensions.size();
		createInfo.ppEnabledExtensionNames = extensions.data();
	}

	VkResult r = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);

	if (r != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create logical device!");
	}

	vkGetDeviceQueue(this->device, indices.graphicsFamily.value(), 0, &this->graphicsQueue);
	vkGetDeviceQueue(this->device, indices.presentFamily.value(), 0, &this->presentQueue);

}

void VulkanTest::createSwapChain()
{
	SwapChainSupportDetails swapChainSupport = this->querySwapChainSupport(physicalDevice);
	VkSurfaceFormatKHR surfaceFormat = this->chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = this->chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = this->chooseSwapExtent(swapChainSupport.caps);

	uint32_t imageCount = swapChainSupport.caps.minImageCount + 1;

	if (swapChainSupport.caps.maxImageCount > 0 && imageCount > swapChainSupport.caps.maxImageCount)
	{
		imageCount = swapChainSupport.caps.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = this->findQueueFamilies(physicalDevice);

	std::vector<uint32_t> queueFams = {
		indices.graphicsFamily.value(),
		indices.presentFamily.value()
	};

	if (indices.graphicsFamily.value() != indices.presentFamily.value())
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = queueFams.size();
		createInfo.pQueueFamilyIndices = queueFams.data();
		std::cout << "Concurrent Mode" << std::endl;
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		std::cout << "Exclusive Mode" << std::endl;
	}

	createInfo.preTransform = swapChainSupport.caps.currentTransform;
	
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	createInfo.oldSwapchain = VK_NULL_HANDLE;

	VkResult r = vkCreateSwapchainKHR(device, &createInfo, nullptr, &this->swapChain);

	if (r != VK_SUCCESS)
	{
		throw std::runtime_error("Swapchain failed to create...");
	}


	uint32_t count = 0;
	vkGetSwapchainImagesKHR(this->device, this->swapChain, &count, nullptr);
	swapChainImages.resize(count);
	vkGetSwapchainImagesKHR(this->device, this->swapChain, &count, this->swapChainImages.data());

	this->swapChainImageFormat = surfaceFormat.format;
	this->swapChainExtent = extent;


}

SwapChainSupportDetails VulkanTest::querySwapChainSupport(VkPhysicalDevice device)
{
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.caps);

	uint32_t count;

	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, nullptr);

	if (count != 0)
	{
		details.formats.resize(count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, details.formats.data());
	}

	count = 0;

	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, nullptr);

	if (count != 0)
	{
		details.presentModes.resize(count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, details.presentModes.data());
	}

	return details;
}

VkSurfaceFormatKHR VulkanTest::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
{
	if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
	{
		return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	for (const auto& f : formats)
	{
		if (f.format == VK_FORMAT_B8G8R8A8_UNORM &&
			f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return f;
		}
	}

	return formats[0];
}

VkPresentModeKHR VulkanTest::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& modes)
{
	VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

	for (const auto& m : modes)
	{
		if (m == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return m;
		}
		else if (m == VK_PRESENT_MODE_IMMEDIATE_KHR)
		{
			return m;
		}
	}

	return bestMode;
}

VkExtent2D VulkanTest::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& caps)
{
	if (caps.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return caps.currentExtent;
	}
	else
	{
		VkExtent2D actualExtent = { width, height };

		actualExtent.width = std::max(caps.minImageExtent.width, std::min(caps.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(caps.minImageExtent.height, std::min(caps.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}
}

void VulkanTest::createSwapChainImageViews()
{
	this->swapChainImageViews.resize(this->swapChainImages.size());

	for (uint32_t i = 0; i < swapChainImages.size(); i++)
	{
		VkImageViewCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = this->swapChainImages[i];

		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = this->swapChainImageFormat;

		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		VkResult r = vkCreateImageView(device, &createInfo, nullptr, &this->swapChainImageViews[i]);

		if (r != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create image view!");
		}
	}
}

void VulkanTest::createCommandPool()
{
	QueueFamilyIndices indices = this->findQueueFamilies(this->physicalDevice);

	VkCommandPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.queueFamilyIndex = indices.graphicsFamily.value();
	createInfo.flags = 0;

	VkResult r = vkCreateCommandPool(device, &createInfo, nullptr, &this->commandPool);

	if (r != VK_SUCCESS)
	{
		std::runtime_error("Command Pool wasn't initialized.");
	}
}

void VulkanTest::createRenderPass()
{
	VkResult r;

	// Clear Render Pass

	// Attachment Description
	VkAttachmentDescription clearColorAttachment = {};
	clearColorAttachment.format = this->swapChainImageFormat;
	clearColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	
	clearColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	clearColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	clearColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	clearColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	clearColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	clearColorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	// Attachment Reference
	VkAttachmentReference clearColorAttachmentReference = {};
	clearColorAttachmentReference.attachment = 0;
	clearColorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// Subpass
	VkSubpassDescription clearSubpass = {};
	clearSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	clearSubpass.colorAttachmentCount = 1;
	clearSubpass.pColorAttachments = &clearColorAttachmentReference;

	// Subpass Dependency
	VkSubpassDependency clearSubpassDep = {};
	clearSubpassDep.srcSubpass = VK_SUBPASS_EXTERNAL;
	clearSubpassDep.dstSubpass = 0;

	clearSubpassDep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	clearSubpassDep.srcAccessMask = 0;

	clearSubpassDep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	clearSubpassDep.dstAccessMask =
		VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	// Create Info
	VkRenderPassCreateInfo clearCreateInfo = {};
	clearCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	clearCreateInfo.attachmentCount = 1;
	clearCreateInfo.pAttachments = &clearColorAttachment;
	clearCreateInfo.subpassCount = 1;
	clearCreateInfo.pSubpasses = &clearSubpass;
	clearCreateInfo.dependencyCount = 1;
	clearCreateInfo.pDependencies = &clearSubpassDep;

	r = vkCreateRenderPass(this->device, &clearCreateInfo, nullptr, &this->clearRenderPass);

	if (r != VK_SUCCESS)
	{
		std::runtime_error("Failed to create clear render pass.");
	}

	// Draw Render Pass

	// Attachment Description
	VkAttachmentDescription drawColorAttachment = {};
	drawColorAttachment.format = this->swapChainImageFormat;
	drawColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

	drawColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	drawColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	drawColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	drawColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	drawColorAttachment.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	drawColorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	// Attachment Reference
	VkAttachmentReference drawColorAttachmentReference = {};
	drawColorAttachmentReference.attachment = 0;
	drawColorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// Subpass
	VkSubpassDescription drawSubpass = {};
	drawSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	drawSubpass.colorAttachmentCount = 1;
	drawSubpass.pColorAttachments = &drawColorAttachmentReference;

	// Subpass Reference
	VkSubpassDependency drawSubpassDep = {};
	drawSubpassDep.srcSubpass = VK_SUBPASS_EXTERNAL;
	drawSubpassDep.dstSubpass = 0;

	drawSubpassDep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	drawSubpassDep.srcAccessMask = 0;

	drawSubpassDep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	drawSubpassDep.dstAccessMask =
		VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	// Create Info
	VkRenderPassCreateInfo drawCreateInfo = {};
	drawCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	drawCreateInfo.attachmentCount = 1;
	drawCreateInfo.pAttachments = &drawColorAttachment;
	drawCreateInfo.subpassCount = 1;
	drawCreateInfo.pSubpasses = &drawSubpass;
	drawCreateInfo.dependencyCount = 1;
	drawCreateInfo.pDependencies = &drawSubpassDep;

	r = vkCreateRenderPass(this->device, &drawCreateInfo, nullptr, &this->drawRenderPass);


	if (r != VK_SUCCESS)
	{
		std::runtime_error("Failed to create draw render pass.");
	}
}

void VulkanTest::createFramebuffers()
{
	VkResult r;

	// Clear
	this->clearFramebuffers.resize(this->swapChainImageViews.size());
	for (uint32_t i = 0; i < this->swapChainImageViews.size(); i++)
	{
		VkFramebufferCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		createInfo.renderPass = clearRenderPass;
		createInfo.attachmentCount = 1;
		createInfo.pAttachments = &this->swapChainImageViews[i];
		createInfo.width = swapChainExtent.width;
		createInfo.height = swapChainExtent.height;
		createInfo.layers = 1;
		r = vkCreateFramebuffer(device, &createInfo, nullptr, &clearFramebuffers[i]);

		if (r != VK_SUCCESS)
		{
			std::runtime_error("Failed to create clear framebuffer.");
		}
	}

	// Draw
	this->drawFramebuffers.resize(this->swapChainImageViews.size());
	for (uint32_t i = 0; i < this->swapChainImageViews.size(); i++)
	{
		VkFramebufferCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		createInfo.renderPass = drawRenderPass;
		createInfo.attachmentCount = 1;
		createInfo.pAttachments = &this->swapChainImageViews[i];
		createInfo.width = swapChainExtent.width;
		createInfo.height = swapChainExtent.height;
		createInfo.layers = 1;
		r = vkCreateFramebuffer(device, &createInfo, nullptr, &drawFramebuffers[i]);

		if (r != VK_SUCCESS)
		{
			std::runtime_error("Failed to create draw framebuffers.");
		}
	}
}

void VulkanTest::createSemaphore()
{
	VkResult r;

	VkSemaphoreCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	r = vkCreateSemaphore(this->device, &createInfo, nullptr, &this->imageAvailable);

	if (r != VK_SUCCESS)
	{
		std::runtime_error("Image Available semaphore...");
	}

	r = vkCreateSemaphore(this->device, &createInfo, nullptr, &this->renderFinish);

	if (r != VK_SUCCESS)
	{
		std::runtime_error("Render Finished semaphore...");
	}
}

void VulkanTest::createFence()
{
	VkResult r;
	VkFenceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	r = vkCreateFence(
		this->device,
		&createInfo,
		nullptr,
		&this->inFlight
	);

	if (r != VK_SUCCESS)
	{
		std::runtime_error("In Flight Fence...");
	}
}

VkCommandBuffer VulkanTest::allocCommandBuffer()
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = this->commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer temp;
	vkAllocateCommandBuffers(device, &allocInfo, &temp);

	return temp;
}

VkCommandBuffer VulkanTest::clearCommand(glm::vec3 color)
{
	VkResult r;
	VkCommandBuffer temp = this->allocCommandBuffer();

	VkClearValue value = {};
	value.color = {
		color.r,
		color.g,
		color.b,
		1.0f
	};

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VkRenderPassBeginInfo rp = {};
	rp.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rp.renderPass = this->clearRenderPass;
	rp.renderArea.offset = { 0, 0 };
	rp.renderArea.extent = this->swapChainExtent;
	rp.clearValueCount = 1;
	rp.pClearValues = &value;
	rp.framebuffer = this->clearFramebuffers[this->swapChainIndex];

	vkBeginCommandBuffer(temp, &beginInfo);
	vkCmdBeginRenderPass(temp, &rp, VK_SUBPASS_CONTENTS_INLINE);

	// Do nothing...
	vkCmdEndRenderPass(temp);
	r = vkEndCommandBuffer(temp);

	if (r != VK_SUCCESS)
	{
		std::runtime_error("Falied to kill command buffer...");
	}

	return temp;
}