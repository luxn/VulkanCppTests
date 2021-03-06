// VkTest.cpp: Definiert den Einstiegspunkt für die Konsolenanwendung.
//

#include "stdafx.h"
//#define VK_USE_PLATFORM_WIN32_KHR
//#include "vulkan/vulkan.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
#include <fstream>
#include <stdexcept>


#define ASSERT_VULKAN(val)\
		if (val != VK_SUCCESS) {\
			__debugbreak();\
		}

// Global
VkInstance instance;
VkSurfaceKHR surface;
VkDevice device;
VkSwapchainKHR swapchain;
std::vector<VkImageView> imageViews;

VkShaderModule vertShaderModule;
VkShaderModule fragShaderModule;

VkPipelineLayout pipelineLayout;
VkRenderPass renderPass;
VkPipeline pipeline;

GLFWwindow *window;

const uint32_t WIDTH = 480;
const uint32_t HEIGHT = 320;

// ---

std::vector<char> readFile(const std::string& fileName) {
	std::ifstream file(fileName, std::ios::binary | std::ios::ate);

	if (file) {
		size_t fileSize = (size_t) file.tellg();
		std::vector<char> fileBuffer(fileSize);
		file.seekg(0);
		file.read(fileBuffer.data(), fileSize);

		file.close();

		return fileBuffer;
	}
	else {
		throw std::runtime_error("Failed to open file!!!");
	}
}


void createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule) {
	VkShaderModuleCreateInfo shaderCreateInfo;
	shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderCreateInfo.pNext = nullptr;
	shaderCreateInfo.flags = 0;
	shaderCreateInfo.codeSize = code.size();
	shaderCreateInfo.pCode = (uint32_t*) code.data(); //Vorsicht. Das klappt nur, da SPIR-V versichert, das der Code ein vielfaches von 4 (byte) ist und immer 32bit Anweisungen hat

	VkResult res = vkCreateShaderModule(device, &shaderCreateInfo, nullptr, shaderModule);
	ASSERT_VULKAN(res);
}


void printStats(VkPhysicalDevice& device) {
	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(device, &properties);

	std::cout << "Name: " << properties.deviceName << std::endl;
	uint32_t apiVer = properties.apiVersion;
	std::cout << "API Version: " << VK_VERSION_MAJOR(apiVer) << "." << VK_VERSION_MINOR(apiVer) << "." << VK_VERSION_PATCH(apiVer) << std::endl;
	std::cout << "Driver Version: " << properties.driverVersion << std::endl;
	// ... weitere Properties
	std::cout << "DiscreteQueuePriorities: " << properties.limits.discreteQueuePriorities << std::endl;


	VkPhysicalDeviceFeatures features;
	vkGetPhysicalDeviceFeatures(device, &features);
	std::cout << "Has Feature 'Geometry Shader': " << features.geometryShader << std::endl;

	VkPhysicalDeviceMemoryProperties memProps;
	vkGetPhysicalDeviceMemoryProperties(device, &memProps);
	std::cout << "Memory Heap Count: " << memProps.memoryHeapCount << std::endl;

	uint32_t amountOfQueueFamilies = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &amountOfQueueFamilies, nullptr);

	std::vector<VkQueueFamilyProperties> families(amountOfQueueFamilies);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &amountOfQueueFamilies, &families[0]);

	std::cout << "Amount of QueueFamilies: " << amountOfQueueFamilies << std::endl;

	int i = 0;
	for (auto& family : families) {
		std::cout << "QueueFamily " << i << std::endl;
		std::cout << "\tQueueCount: " << family.queueCount << std::endl;
		std::cout << "\tTimestampValidBits: " << family.timestampValidBits << std::endl;
		std::cout << "\tMinImageTransferGranularity: " << family.minImageTransferGranularity.width << "width "<< family.minImageTransferGranularity.height << "height " << family.minImageTransferGranularity.depth << "depth" << std::endl;
		std::cout << "\tVK_QUEUE_GRAPHICS_BIT: " << ((family.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) << std::endl;
		std::cout << "\tVK_QUEUE_COMPUTE_BIT: " << ((family.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0) << std::endl;
		std::cout << "\tVK_QUEUE_TRANSFER_BIT: " << ((family.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0) << std::endl;
		std::cout << "\tVK_QUEUE_SPARSE_BINDING_BIT: " << ((family.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) != 0) << std::endl;

		i++;
	}



	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &surfaceCapabilities);
	std::cout << "VkSurfaceCapabilitiesKHR" << std::endl;
	std::cout << "\tminImageCount: " << surfaceCapabilities.minImageCount << std::endl;
	std::cout << "\tmaxImageCount: " << surfaceCapabilities.maxImageCount << std::endl;
	std::cout << "\tcurrentExtent: " << surfaceCapabilities.currentExtent.width << "/" << surfaceCapabilities.currentExtent.height << std::endl;
	std::cout << "\tminImageExtent: " << surfaceCapabilities.minImageExtent.width << "/" << surfaceCapabilities.minImageExtent.height << std::endl;
	std::cout << "\tmaxImageExtent: " << surfaceCapabilities.maxImageExtent.width << "/" << surfaceCapabilities.maxImageExtent.height << std::endl;
	std::cout << "\tmaxImageArrayLayers: " << surfaceCapabilities.maxImageArrayLayers << std::endl;
	std::cout << "\tsupportedTransforms: " << surfaceCapabilities.supportedTransforms << std::endl;
	std::cout << "\tcurrentTransform: " << surfaceCapabilities.currentTransform << std::endl;
	std::cout << "\tsupportedCompositeAlpha: " << surfaceCapabilities.supportedCompositeAlpha << std::endl;
	std::cout << "\tsupportedUsageFlags: " << surfaceCapabilities.supportedUsageFlags << std::endl;


	uint32_t amountOfSurfaceFormats = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &amountOfSurfaceFormats, nullptr);
	std::vector<VkSurfaceFormatKHR> surfaceFormats(amountOfSurfaceFormats);
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &amountOfSurfaceFormats, &surfaceFormats[0]);
	
	std::cout << std::endl;
	std::cout << "Amount of SurfaceFormats: " << amountOfSurfaceFormats << std::endl;
	for (auto& format : surfaceFormats) {
		std::cout << "Format: " << format.format << std::endl;
	}


	uint32_t amountOfPresentationModes = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &amountOfPresentationModes, nullptr);
	std::vector<VkPresentModeKHR> presentationsModes(amountOfPresentationModes);
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &amountOfPresentationModes, &presentationsModes[0]);

	std::cout << "Amount of PresentationModes: " << amountOfPresentationModes << std::endl;
	for (auto& mode : presentationsModes) {
		std::cout << "Supported presentation mode: " << mode << std::endl;
	}
	
	std::cout << std::endl;
}


void startGLFW() {
	glfwInit();
	if (glfwVulkanSupported() == GLFW_FALSE) {
		std::cerr << "Vulkan is not supported" << std::endl;
		exit(-1);
	}
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	window = glfwCreateWindow((int) WIDTH, (int) HEIGHT, "Vulkan Tutorial", nullptr, nullptr);
}

void startVulkan() {
	VkApplicationInfo appInfo;
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pNext = nullptr;
	appInfo.pApplicationName = "Vulkan Tutorial";
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
	appInfo.pEngineName = "Vk Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
	appInfo.apiVersion = VK_API_VERSION_1_1;


	uint32_t amountOfLayers = 0;
	vkEnumerateInstanceLayerProperties(&amountOfLayers, nullptr);
	std::vector<VkLayerProperties> layers(amountOfLayers);
	vkEnumerateInstanceLayerProperties(&amountOfLayers, &layers[0]);

	std::cout << "Amount of Instance Layers: " << amountOfLayers << std::endl;
	for (auto& layer : layers) {
		std::cout << "\t" << "Name: " << layer.layerName << std::endl;
		std::cout << "\t" << "Description: " << layer.description << std::endl;
		std::cout << "\t" << "Spec Version: " << layer.specVersion << std::endl;
		std::cout << "\t" << "Impl Version: " << layer.implementationVersion << std::endl;
		std::cout << std::endl;
	}

	uint32_t amountOfExtensions = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &amountOfExtensions, nullptr);
	std::vector<VkExtensionProperties> extensions(amountOfExtensions);
	vkEnumerateInstanceExtensionProperties(nullptr, &amountOfExtensions, &extensions[0]);

	std::cout << "Amount of Instance Extensions: " << amountOfExtensions << std::endl;
	for (auto& extension : extensions) {
		std::cout << "\t" << "Name: " << extension.extensionName << std::endl;
		std::cout << "\t" << "Spec Version: " << extension.specVersion << std::endl;
		std::cout << std::endl;
	}

	const std::vector<const char*> validationLayers{
		"VK_LAYER_LUNARG_standard_validation"
	};

	/*const std::vector<const char*> usedExtensions{
		VK_KHR_SURFACE_EXTENSION_NAME,
		//VK_KHR_WIN32_SURFACE_EXTENSION_NAME
	};*/

	uint32_t amountOfGLFWExtensions = 0;
	auto glfwExtensions = glfwGetRequiredInstanceExtensions(&amountOfGLFWExtensions);

	VkInstanceCreateInfo instanceCreateInfo;
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pNext = nullptr;
	instanceCreateInfo.flags = 0;
	instanceCreateInfo.pApplicationInfo = &appInfo;
	instanceCreateInfo.enabledLayerCount = validationLayers.size(); // 0
	instanceCreateInfo.ppEnabledLayerNames = validationLayers.data(); // nullptr
	instanceCreateInfo.enabledExtensionCount = amountOfGLFWExtensions; // usedExtensions.size(); // 0
	instanceCreateInfo.ppEnabledExtensionNames = glfwExtensions; // usedExtensions.data(); // nullptr

	VkResult res = vkCreateInstance(&instanceCreateInfo, nullptr, &instance);
	ASSERT_VULKAN(res);


	//---Windows spezifisch
	/*
	VkWin32SurfaceCreateInfoKHR win32SurfaceCreateInfo;
	win32SurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	win32SurfaceCreateInfo.pNext = nullptr;
	win32SurfaceCreateInfo.flags = 0;
	win32SurfaceCreateInfo.hinstance = nullptr; // In echt die jeweligen Ptr aus windows.h
	win32SurfaceCreateInfo.hwnd = nullptr; // In echt die jeweligen Ptr aus windows.h

	VkSurfaceKHR surface;
	res = vkCreateWin32SurfaceKHR(instance, &win32SurfaceCreateInfo, nullptr, &surface);
	ASSERT_VULKAN(res);
	*/
	//---

	// GLFW
	res = glfwCreateWindowSurface(instance, window, nullptr, &surface);
	ASSERT_VULKAN(res);
	// ---

	uint32_t countOfDevices = 0;
	res = vkEnumeratePhysicalDevices(instance, &countOfDevices, nullptr); // Frage alle Karten ab
	ASSERT_VULKAN(res);

	std::vector<VkPhysicalDevice> devices(countOfDevices);

	res = vkEnumeratePhysicalDevices(instance, &countOfDevices, &devices[0]); // countOfDevices reglementiert die Anzahl der abzufragenden Devices
	ASSERT_VULKAN(res);

	for (auto& device : devices) {
		printStats(device);
	}

	float queuePriorities[] = { 1.0f };

	VkDeviceQueueCreateInfo deviceQueueCreateInfo;
	deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	deviceQueueCreateInfo.pNext = nullptr;
	deviceQueueCreateInfo.flags = 0;
	deviceQueueCreateInfo.queueFamilyIndex = 0; // TODO: Choose correct family index / Eigentlich sollte man hier die passende QueueFamily wählen (die die Graphics kann)
	deviceQueueCreateInfo.queueCount = 1; //4; // TODO Check if this amound is valid
	deviceQueueCreateInfo.pQueuePriorities = queuePriorities; // 0.4f, 0.5f oder 1.0f etc.

	VkPhysicalDeviceFeatures usedFeatures = {};
	//Features freischalten
	//usedFeatures.geometryShader = true;


	std::vector<const char*> deviceExtensions{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	VkDeviceCreateInfo deviceCreateInfo;
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pNext = nullptr;
	deviceCreateInfo.flags = 0;
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
	deviceCreateInfo.enabledLayerCount = 0; // TODO später
	deviceCreateInfo.ppEnabledLayerNames = nullptr; // TODO später
	deviceCreateInfo.enabledExtensionCount = deviceExtensions.size(); //0; // TODO später
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data(); //nullptr; // TODO später
	deviceCreateInfo.pEnabledFeatures = &usedFeatures;

	// TODO pick "best device" instead of first device
	res = vkCreateDevice(devices[0], &deviceCreateInfo, nullptr, &device);
	ASSERT_VULKAN(res);

	VkQueue queue;
	vkGetDeviceQueue(device, 0, 0, &queue); // Aus der ersten Family die erste Queue

	VkBool32 surfaceSupported = false;
	res = vkGetPhysicalDeviceSurfaceSupportKHR(devices[0], 0, surface, &surfaceSupported);
	ASSERT_VULKAN(res);

	if (!surfaceSupported) {
		std::cerr << "Surface not supported!" << std::endl;
		__debugbreak();
	}

	VkSwapchainCreateInfoKHR swapchainCreateInfo;
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.pNext = nullptr;
	swapchainCreateInfo.flags = 0;
	swapchainCreateInfo.surface = surface;
	swapchainCreateInfo.minImageCount = 3; //TODO: check if valid
	swapchainCreateInfo.imageFormat = VK_FORMAT_B8G8R8A8_SNORM; //TODO: check if valid
	swapchainCreateInfo.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR; //TODO: check if valid
	swapchainCreateInfo.imageExtent = VkExtent2D{ WIDTH, HEIGHT };
	swapchainCreateInfo.imageArrayLayers = 1; //meistens auf 1 (sonst Randfall)
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // wir wollen malen (!)
	swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // nur für meine Queue TODO: check if valid
	swapchainCreateInfo.queueFamilyIndexCount = 0;
	swapchainCreateInfo.pQueueFamilyIndices = nullptr;
	swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR; // keine gloable Transformation bevor ich die Bilder bekomme
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // wie wollen wir mit Alpha umgehen, alpha soll opaque werden
	swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR; // TODO: check if valid
	swapchainCreateInfo.clipped = VK_TRUE; // sollen Pixel außerhalb von Image weggeworfen werden
	swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE; // Es gibt noch keine alte Swapchain

	
	res = vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain);
	ASSERT_VULKAN(res);

	uint32_t amountOfImagesInSwapchain = 0;
	vkGetSwapchainImagesKHR(device, swapchain, &amountOfImagesInSwapchain, nullptr);
	std::vector<VkImage> swapchainImages(amountOfImagesInSwapchain);
	res = vkGetSwapchainImagesKHR(device, swapchain, &amountOfImagesInSwapchain, &swapchainImages[0]);
	ASSERT_VULKAN(res);
	
	std::cout << "Amount of Swapcahin Images: " << amountOfImagesInSwapchain << std::endl;

	imageViews.resize(amountOfImagesInSwapchain);

	for (int i = 0; i < amountOfImagesInSwapchain; ++i) {
		VkImageViewCreateInfo imageViewCreateInfo;
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.pNext = nullptr;
		imageViewCreateInfo.flags = 0;
		imageViewCreateInfo.image = swapchainImages[i];
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format = VK_FORMAT_B8G8R8A8_SNORM; //TODO: check if valid
		imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0; // eher für AR/VR
		imageViewCreateInfo.subresourceRange.layerCount = 1; // eher für AR/VR

		res = vkCreateImageView(device, &imageViewCreateInfo, nullptr, &imageViews[i]);
		ASSERT_VULKAN(res);
	}

	auto shaderCodeVert = readFile("vert.spv");
	auto shaderCodeFrag = readFile("frag.spv");
	
	createShaderModule(shaderCodeVert, &vertShaderModule);
	createShaderModule(shaderCodeFrag, &fragShaderModule);

	VkPipelineShaderStageCreateInfo shaderStageCreateInfoVert;
	shaderStageCreateInfoVert.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageCreateInfoVert.pNext = nullptr;
	shaderStageCreateInfoVert.flags = 0;
	shaderStageCreateInfoVert.stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStageCreateInfoVert.module = vertShaderModule;
	shaderStageCreateInfoVert.pName = "main";
	shaderStageCreateInfoVert.pSpecializationInfo = nullptr; //Später mehr

	VkPipelineShaderStageCreateInfo shaderStageCreateInfoFrag;
	shaderStageCreateInfoFrag.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageCreateInfoFrag.pNext = nullptr;
	shaderStageCreateInfoFrag.flags = 0;
	shaderStageCreateInfoFrag.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStageCreateInfoFrag.module = fragShaderModule;
	shaderStageCreateInfoFrag.pName = "main";
	shaderStageCreateInfoFrag.pSpecializationInfo = nullptr;  //Später mehr

	VkPipelineShaderStageCreateInfo shaderStages[] = { shaderStageCreateInfoVert, shaderStageCreateInfoFrag };

	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo;
	vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputCreateInfo.pNext = nullptr;
	vertexInputCreateInfo.flags = 0;
	vertexInputCreateInfo.vertexBindingDescriptionCount = 0;
	vertexInputCreateInfo.pVertexBindingDescriptions = nullptr;
	vertexInputCreateInfo.vertexAttributeDescriptionCount = 0;
	vertexInputCreateInfo.pVertexAttributeDescriptions = nullptr;

	VkPipelineInputAssemblyStateCreateInfo assemblyInputCreateInfo;
	assemblyInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	assemblyInputCreateInfo.pNext = nullptr;
	assemblyInputCreateInfo.flags = 0;
	assemblyInputCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	assemblyInputCreateInfo.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport;
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = WIDTH;
	viewport.height = HEIGHT;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor;
	scissor.offset = { 0, 0 };
	scissor.extent = { WIDTH, HEIGHT };

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo;
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.pNext = nullptr;
	viewportStateCreateInfo.flags = 0;
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.pViewports = &viewport;
	viewportStateCreateInfo.scissorCount = 1;
	viewportStateCreateInfo.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizationCreateInfo;
	rasterizationCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationCreateInfo.pNext = nullptr;
	rasterizationCreateInfo.flags = 0;
	rasterizationCreateInfo.depthClampEnable = VK_FALSE;
	rasterizationCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizationCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizationCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE; // Definitionssache, wie die Modelle erstellt worden sind (Vertices)
	rasterizationCreateInfo.depthBiasEnable = VK_FALSE;
	rasterizationCreateInfo.depthBiasConstantFactor = 0.0f;
	rasterizationCreateInfo.depthBiasClamp = 0.0f;
	rasterizationCreateInfo.depthBiasSlopeFactor = 0.0f;
	rasterizationCreateInfo.lineWidth = 1.0f;

	VkPipelineMultisampleStateCreateInfo multisampleCreateInfo;
	multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleCreateInfo.pNext = nullptr;
	multisampleCreateInfo.flags = 0;
	multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleCreateInfo.sampleShadingEnable = VK_FALSE;
	multisampleCreateInfo.minSampleShading = 1.0f;
	multisampleCreateInfo.pSampleMask = nullptr;
	multisampleCreateInfo.alphaToCoverageEnable = VK_FALSE;
	multisampleCreateInfo.alphaToOneEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachment;
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	

	VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo;
	colorBlendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendCreateInfo.pNext = nullptr;
	colorBlendCreateInfo.flags = 0;
	colorBlendCreateInfo.logicOpEnable = VK_FALSE;
	colorBlendCreateInfo.logicOp = VK_LOGIC_OP_NO_OP;
	colorBlendCreateInfo.attachmentCount = 1;
	colorBlendCreateInfo.pAttachments = &colorBlendAttachment;
	colorBlendCreateInfo.blendConstants[0] = 0.0f;	//R
	colorBlendCreateInfo.blendConstants[1] = 0.0f;	//G
	colorBlendCreateInfo.blendConstants[2] = 0.0f;	//B
	colorBlendCreateInfo.blendConstants[3] = 0.0f;	//A
	

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.pNext = nullptr;
	pipelineLayoutCreateInfo.flags = 0;
	pipelineLayoutCreateInfo.setLayoutCount = 0;
	pipelineLayoutCreateInfo.pSetLayouts = nullptr;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

	res = vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout);
	ASSERT_VULKAN(res);

	VkAttachmentDescription attachmentDescription;
	attachmentDescription.flags = 0;
	attachmentDescription.format = VK_FORMAT_B8G8R8_UNORM;
	attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // momentan keine Benutzung von Stencil
	attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // momentan keine Benutzung von Stencil
	attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference attachmentReference;
	attachmentReference.attachment = 0; // Das Erste (mtn. das Einzige)
	attachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpassDescription;
	subpassDescription.flags = 0;
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.inputAttachmentCount = 0;
	subpassDescription.pInputAttachments = nullptr;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &attachmentReference;
	subpassDescription.pResolveAttachments = nullptr;
	subpassDescription.pDepthStencilAttachment = nullptr;
	subpassDescription.preserveAttachmentCount = 0;
	subpassDescription.pPreserveAttachments = nullptr;

	VkRenderPassCreateInfo renderPassCreateInfo;
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.pNext = nullptr;
	renderPassCreateInfo.flags = 0;
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments = &attachmentDescription; // nicht die AttachmentReference
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpassDescription;
	renderPassCreateInfo.dependencyCount = 0;
	renderPassCreateInfo.pDependencies = nullptr;


	res = vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &renderPass);
	ASSERT_VULKAN(res);


	VkGraphicsPipelineCreateInfo pipelineCreateInfo;
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.pNext = nullptr;
	pipelineCreateInfo.flags = 0; // Für eine spätere Vererbung sinnvoll
	pipelineCreateInfo.stageCount = 2;
	pipelineCreateInfo.pStages = shaderStages;
	pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
	pipelineCreateInfo.pInputAssemblyState = &assemblyInputCreateInfo;
	pipelineCreateInfo.pTessellationState = nullptr;
	pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	pipelineCreateInfo.pRasterizationState = &rasterizationCreateInfo;
	pipelineCreateInfo.pMultisampleState = &multisampleCreateInfo;
	pipelineCreateInfo.pDepthStencilState = nullptr;
	pipelineCreateInfo.pColorBlendState = &colorBlendCreateInfo;
	pipelineCreateInfo.pDynamicState = nullptr;
	pipelineCreateInfo.layout = pipelineLayout;
	pipelineCreateInfo.renderPass = renderPass;
	pipelineCreateInfo.subpass = 0; // erster Subpass
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE; // Für Vererbung
	pipelineCreateInfo.basePipelineIndex = -1; // uninteressant wg. VK_NULL_HANDLE

	res = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline);
	ASSERT_VULKAN(res);

}

void gameLoop() {
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
	}
}

void shutdownVulkan() {
	vkDeviceWaitIdle(device);	

	vkDestroyPipeline(device, pipeline, nullptr);
	vkDestroyRenderPass(device, renderPass, nullptr);

	for (int i = 0; i < imageViews.size(); ++i) {
		vkDestroyImageView(device, imageViews[i], nullptr);
	}

	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

	vkDestroyShaderModule(device, fragShaderModule, nullptr);
	vkDestroyShaderModule(device, vertShaderModule, nullptr);

	vkDestroySwapchainKHR(device, swapchain, nullptr);
	vkDestroyDevice(device, nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyInstance(instance, nullptr);
}

void shutdownGLFW() {
	glfwDestroyWindow(window);	
}


int main()
{
	startGLFW();
	startVulkan();

	gameLoop();

	shutdownVulkan();
	shutdownGLFW();

    return 0;
}

