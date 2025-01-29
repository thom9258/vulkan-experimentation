#pragma once 

#include <iostream>
#include <sstream>
#include <fstream>
#include <variant>
#include <algorithm>
#include <chrono>
#include <thread>
#include <limits>


//#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Utils.hpp"
#include "DebugMessenger.hpp"

#include "Texture.hpp"

constexpr std::string resources_root = "../resources";

class VulkanRenderer 
{
public:
    explicit VulkanRenderer();
    ~VulkanRenderer(); 

	[[nodiscard]]
	vk::Extent2D WindowExtent() const noexcept;
	void DrawFrame();

	vk::Device& device();
	vk::PhysicalDevice& physical_device();
	vk::CommandPool& command_pool();
	vk::Queue& graphics_queue();
	
private:
	void CreateContext();
	void CreateWindow();
	void CreateInstance();
	void CreateWindowSurface();
	void CreateDebugMessenger();
	void GetPhysicalDevice();
	void GetQueueFamilyIndices();
	void CreateDevice();
	void CreateIndexQueues();
	void CreateSwapChain();
	void CreateGraphicsPipeline();
	void CreateRenderPass();
	void CreateFramebuffers();
	void CreateCommandpool();
	void CreateCommandbuffers();
	void CreateSyncObjects();
	void CreateDrawTextures();

	void RecordCommandbuffer(vk::CommandBuffer& commandbuffer,
							 const uint32_t index,
							 const vk::ClearValue clear_value);

	const int maxFramesInFlight_ = 2;
	uint32_t current_frame_{0};
	uint32_t total_frames_{0};

	SDL_Window* window_;
	vk::UniqueInstance instance_;
	//TODO: I cannot seem to load the function pointers for debugutils messenger..
	//vk::UniqueDebugUtilsMessengerEXT debug_messenger_;
	vk::PhysicalDevice physical_device_;
	//TODO: get some automatic destructon onto this surface
	VkSurfaceKHR raw_window_surface_;
	GraphicsPresentIndices graphics_present_indices_;
	vk::UniqueDevice device_;
	IndexQueues index_queues_;

	vk::SurfaceFormatKHR swapchain_format_;
	vk::UniqueSwapchainKHR swapchain_;
	std::vector<vk::Image> swapchain_images_;
	std::vector<vk::UniqueImageView> swapchain_imageviews_;

	vk::UniqueRenderPass renderpass_;
	vk::UniquePipelineLayout pipelineLayout_;
    vk::Pipeline pipeline_;

	Bitmap2D draw_image_;
	std::vector<Texture> draw_textures_;
	
	std::vector<vk::UniqueFramebuffer> framebuffers_;
	vk::UniqueCommandPool commandpool_;
	std::vector<vk::UniqueCommandBuffer> commandbuffers_;
	

	std::vector<vk::UniqueSemaphore> imageAvailableSemaphores_;
	std::vector<vk::UniqueSemaphore> renderFinishedSemaphores_;
	std::vector<vk::UniqueFence> inFlightFences_;
};


vk::Device& VulkanRenderer::device()
{
	return device_.get();
}

vk::PhysicalDevice& VulkanRenderer::physical_device()
{
	return physical_device_;
}

vk::CommandPool& VulkanRenderer::command_pool()
{
	return commandpool_.get();
}

vk::Queue& VulkanRenderer::graphics_queue()
{
	return ::graphics_queue(index_queues_);
}

VulkanRenderer::~VulkanRenderer()
{
	device_->waitIdle();

	// TODO: Port over the ResourceWrapperRuntime so we can automatically destroy all this stuff..
	// Note we need to destroy the swapchain manually so it happens before the surface...
	swapchain_.reset();
	instance_->destroySurfaceKHR(raw_window_surface_, nullptr);
	device_->destroyPipeline(pipeline_);
	SDL_DestroyWindow(window_);
	SDL_Vulkan_UnloadLibrary();
	SDL_Quit();
}

VulkanRenderer::VulkanRenderer()
{
	CreateContext();
	CreateWindow();
	CreateInstance();
	CreateWindowSurface();
	CreateDebugMessenger();
	GetPhysicalDevice();
	GetQueueFamilyIndices();
	CreateDevice();
	CreateIndexQueues();
	CreateSwapChain();
	CreateRenderPass();
	CreateGraphicsPipeline();
	CreateFramebuffers();
	CreateCommandpool();
	CreateCommandbuffers();
	CreateSyncObjects();
	CreateDrawTextures();
}

void VulkanRenderer::CreateContext()
{
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_Vulkan_LoadLibrary(nullptr);
}

void VulkanRenderer::CreateWindow()
{
	window_ = SDL_CreateWindow("render window",
							   SDL_WINDOWPOS_UNDEFINED,
							   SDL_WINDOWPOS_UNDEFINED,
							   1200,
							   800,
							   0 | SDL_WINDOW_VULKAN);
}

[[nodiscard]]
bool
is_validation_layer_available(const char* layer)
{
	std::string str_layer = layer;
	std::vector<vk::LayerProperties> properties = vk::enumerateInstanceLayerProperties();
	for (vk::LayerProperties& property: properties) {
		std::string str_available = property.layerName;
		if (str_layer == str_available)
			return true;
	}

	return false;
}

void VulkanRenderer::CreateInstance()
{
	std::vector<const char*> sdl_instance_extensions = get_sdl2_instance_extensions(window_);
	auto instance_extensions = sdl_instance_extensions;
	instance_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	
	std::cout << "Instance Extensions:\n";
	for (const auto& extension: instance_extensions)
		std::cout << "-> " << extension << "\n";
	std::cout << std::endl;

	//NOTE Only do validation layers for debugging
	std::vector<const char*> validation_layers{
		"VK_LAYER_KHRONOS_validation"
	};
	
	for (auto wanted: validation_layers)
		if (!is_validation_layer_available(wanted))
			throw std::runtime_error("Validation layer not available");
	
	auto applicationInfo = vk::ApplicationInfo{}
		.setPApplicationName("app")
		.setPEngineName("engine")
		.setApplicationVersion(VK_MAKE_VERSION(1, 0, 0))
		.setEngineVersion(VK_MAKE_VERSION(1, 0, 0))
		.setApiVersion(VK_MAKE_VERSION(1, 0, 0));

	auto instanceCreateInfo = vk::InstanceCreateInfo{}
		.setPApplicationInfo(&applicationInfo)
		.setPEnabledLayerNames(validation_layers)
		.setPEnabledExtensionNames(instance_extensions);
	
    instance_ = vk::createInstanceUnique(instanceCreateInfo);
	std::cout << "Created Vulkan Instance!" << std::endl;
}

void VulkanRenderer::CreateWindowSurface()
{
	SDL_Vulkan_CreateSurface(window_, *instance_, &raw_window_surface_);
}

void VulkanRenderer::CreateDebugMessenger()
{
#if 0	
	auto debugMessengerCreateInfo = vk::DebugUtilsMessengerCreateInfoEXT{}
	.setFlags(vk::DebugUtilsMessengerCreateFlagsEXT())
	.setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eError 
					  | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
					  | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
					  | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo)
	.setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral 
				  | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation 
				  | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
	.setPfnUserCallback(&debugUtilsMessengerCallback);

	debug_messenger_ = instance_->createDebugUtilsMessengerEXTUnique(debugMessengerCreateInfo);
	std::cout << "Created Debug Messenger!" << std::endl;
#endif
	std::cout << "TODO: Cannot Create Debug Messenger!" << std::endl;
}

void VulkanRenderer::GetPhysicalDevice()
{
    std::vector<vk::PhysicalDevice> devices = instance_->enumeratePhysicalDevices();
	physical_device_ = devices.front();

	std::cout << PhysicalDevice_string(physical_device_);
}

void VulkanRenderer::GetQueueFamilyIndices()
{
	auto graphics_present = get_graphics_present_indices(physical_device_,
														 raw_window_surface_);

	if (!graphics_present)
		throw std::runtime_error("could not get graphics and present indices");

	graphics_present_indices_ = *graphics_present;
	
	if (std::holds_alternative<SharedGraphicsPresentIndex>(graphics_present_indices_)) {
		std::cout << "> found SHARED graphics present indices" << std::endl;
	}
	else {
		std::cout << "> found SPLIT graphics present indices" << std::endl;
	}
}

void VulkanRenderer::CreateDevice()
{
	float queuePriority = 1.0f;
	uint32_t device_index = present_index(graphics_present_indices_);

	auto deviceQueueCreateInfo = vk::DeviceQueueCreateInfo{}
		.setFlags({})
		.setQueueFamilyIndex(device_index)
		.setPQueuePriorities(&queuePriority)
		.setQueueCount(1);

	const std::vector<const char*> device_extensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};
	
	auto deviceCreateInfo = vk::DeviceCreateInfo{}
		.setQueueCreateInfoCount(1)
		.setQueueCreateInfos(deviceQueueCreateInfo)
		.setPpEnabledExtensionNames(device_extensions.data())
		.setEnabledExtensionCount(device_extensions.size());
	
	device_ = physical_device_.createDeviceUnique(deviceCreateInfo);
	std::cout << "Created Logical Device!" << std::endl;
}	

void VulkanRenderer::CreateIndexQueues()
{
	index_queues_ = get_index_queues(*device_,
									 graphics_present_indices_);

	if (std::holds_alternative<SharedIndexQueue>(index_queues_)) {
		std::cout << "> created SHARED index queue" << std::endl;
	}
	else {
		std::cout << "> created SPLIT index queues" << std::endl;
	}
}


[[nodiscard]]
vk::Extent2D VulkanRenderer::WindowExtent() const noexcept
{
	int width;
	int height;
	SDL_GetWindowSize(window_, &width, &height);
	return vk::Extent2D{}.setWidth(width).setHeight(height);
}

void VulkanRenderer::CreateSwapChain()
{
	const auto surface_capabilities =
		physical_device_.getSurfaceCapabilitiesKHR(raw_window_surface_);

	const auto window_extent = WindowExtent();
	std::cout << "> Window width:  " << window_extent.width << "\n"
			  << "         height: " << window_extent.height << std::endl;

	vk::Extent2D swapchain_extent;
	const bool surface_size_is_undefined = 
		surface_capabilities.currentExtent.width == std::numeric_limits<uint32_t>::max();
	if (surface_size_is_undefined) {
		// If the surface size is undefined, the size is set to the size of the images requested.
		swapchain_extent.width = std::clamp(window_extent.width,
											surface_capabilities.minImageExtent.width,
											surface_capabilities.maxImageExtent.width);
		
		swapchain_extent.height = std::clamp(window_extent.height,
											 surface_capabilities.minImageExtent.height,
											 surface_capabilities.maxImageExtent.height);
	}
	else {
		// If the surface size is defined, the swap chain size must match
		swapchain_extent = surface_capabilities.currentExtent;
	}
	
	std::cout << "> SwapChain width:  " << swapchain_extent.width << "\n"
			  << "            height: " << swapchain_extent.height << std::endl;

	const std::vector<vk::SurfaceFormatKHR> formats =
		physical_device_.getSurfaceFormatsKHR(raw_window_surface_);
	if (formats.empty())
		throw std::runtime_error("Swapchain has no formats!");
	
	swapchain_format_ = get_swapchain_surface_format(formats);

	std::cout << "> Swapchain format: " << vk::to_string(swapchain_format_.format) << std::endl;

	const vk::PresentModeKHR swapchainPresentMode = vk::PresentModeKHR::eFifo;
	std::cout << "> Swapchain PresentMode: " << vk::to_string(swapchainPresentMode) << std::endl;
	// vk::PresentModeKHR::eMailBox is potentially good on low power devices..

	const auto supports_identity = 
		static_cast<bool>(surface_capabilities.supportedTransforms 
						  & vk::SurfaceTransformFlagBitsKHR::eIdentity);
	
	const vk::SurfaceTransformFlagBitsKHR preTransform =
		(supports_identity) ? vk::SurfaceTransformFlagBitsKHR::eIdentity
		                    : surface_capabilities.currentTransform;
	
	// this spaghetti determines the best compositealpha flags....
	vk::CompositeAlphaFlagBitsKHR compositeAlpha =
	( surface_capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePreMultiplied )    ? vk::CompositeAlphaFlagBitsKHR::ePreMultiplied
	: ( surface_capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePostMultiplied ) ? vk::CompositeAlphaFlagBitsKHR::ePostMultiplied
	: ( surface_capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eInherit )        ? vk::CompositeAlphaFlagBitsKHR::eInherit
	: vk::CompositeAlphaFlagBitsKHR::eOpaque;
	

	const auto wanted_surface_image_count = 3u;
	uint32_t surface_image_count{0};
	if (surface_capabilities.maxImageCount > 0) {
		surface_image_count = std::clamp(wanted_surface_image_count,
										 surface_capabilities.minImageCount,
										 surface_capabilities.maxImageCount);
	}
	else {
		surface_image_count = std::min(wanted_surface_image_count,
									   surface_capabilities.minImageCount);
	}

	std::cout << "> surface capabilities:\n"
	<< "  > min image count: " << surface_capabilities.minImageCount << "\n"
	<< "  > max image count: " << surface_capabilities.maxImageCount << "\n"
	<< "  > requested image count: " << surface_image_count 
	<< std::endl;
	
	auto swapChainCreateInfo = vk::SwapchainCreateInfoKHR{}
		.setFlags(vk::SwapchainCreateFlagsKHR())
		.setSurface(raw_window_surface_)
		.setMinImageCount(surface_image_count)
		.setImageFormat(swapchain_format_.format)
		.setImageColorSpace(swapchain_format_.colorSpace)
		.setImageExtent(swapchain_extent)
		.setImageArrayLayers(1)
		.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment
					   //| vk::ImageUsageFlagBits::eTransferSrc
					   | vk::ImageUsageFlagBits::eTransferDst
					   | vk::ImageUsageFlagBits::eSampled)
		.setClipped(true)
		.setPreTransform(preTransform)
		.setCompositeAlpha(compositeAlpha)
		//.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
		.setPresentMode(swapchainPresentMode);
	
	std::array<uint32_t, 2> splitIndices = {
		graphics_index(graphics_present_indices_),
		present_index(graphics_present_indices_)
	};

	if (std::holds_alternative<SharedGraphicsPresentIndex>(graphics_present_indices_)) {
		swapChainCreateInfo
			.setImageSharingMode(vk::SharingMode::eExclusive);
	} 
	else {
		swapChainCreateInfo
			.setImageSharingMode(vk::SharingMode::eConcurrent)
			.setQueueFamilyIndices(splitIndices);
	}

	swapchain_ = device_->createSwapchainKHRUnique(swapChainCreateInfo, nullptr);

	swapchain_images_ = device_->getSwapchainImagesKHR(*swapchain_);
	std::cout << "> SwapChain image count: " << swapchain_images_.size() << std::endl;
	
	swapchain_imageviews_.reserve(swapchain_images_.size());
	
	auto subresourceRange = vk::ImageSubresourceRange{}
		.setAspectMask(vk::ImageAspectFlagBits::eColor)
		.setBaseMipLevel(0)
		.setLevelCount(1)
		.setBaseArrayLayer(0)
		.setLayerCount(1);

	auto componentMapping = vk::ComponentMapping{}
		.setR(vk::ComponentSwizzle::eIdentity)		
		.setG(vk::ComponentSwizzle::eIdentity)
		.setB(vk::ComponentSwizzle::eIdentity)
		.setA(vk::ComponentSwizzle::eIdentity);

	auto imageViewCreateInfo = vk::ImageViewCreateInfo{}
		.setSubresourceRange(subresourceRange)
		.setViewType(vk::ImageViewType::e2D)
		.setFormat(swapchain_format_.format)
		.setComponents(componentMapping);
	
	for (auto& image : swapchain_images_) {
		imageViewCreateInfo.setImage(image);
		swapchain_imageviews_.push_back(device_->createImageViewUnique(imageViewCreateInfo));
	}

	std::cout << "> created SwapChain!" << std::endl;
}

void VulkanRenderer::CreateRenderPass()
{
    const std::vector<vk::AttachmentDescription> attachmentDescriptions{
		vk::AttachmentDescription{}
		.setFlags(vk::AttachmentDescriptionFlags())
		.setFormat(swapchain_format_.format)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::ePresentSrcKHR),
	};

	auto colorReference = vk::AttachmentReference{}
		.setAttachment(0)
		.setLayout(vk::ImageLayout::eColorAttachmentOptimal);
	
    auto subpass = vk::SubpassDescription{}
		.setFlags(vk::SubpassDescriptionFlags())
		.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
		.setInputAttachments({})
		.setResolveAttachments({})
		.setColorAttachments(colorReference)
		.setPDepthStencilAttachment(nullptr);
	
	auto dependency = vk::SubpassDependency{}
		.setSrcSubpass(vk::SubpassExternal)
		.setDstSubpass(0)
		.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
		.setSrcAccessMask(vk::AccessFlags())
		.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
		.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead 
						  | vk::AccessFlagBits::eColorAttachmentWrite);
	
    auto renderPassCreateInfo = vk::RenderPassCreateInfo{}
		.setFlags(vk::RenderPassCreateFlags())
		.setAttachments(attachmentDescriptions)
		.setDependencies(dependency)
		.setSubpasses(subpass);

    renderpass_ = device_->createRenderPassUnique(renderPassCreateInfo);

	std::cout << "> created Render Pass!" << std::endl;
}

void VulkanRenderer::CreateGraphicsPipeline()
{
	const auto vert = read_binary_file(resources_root + "/triangle.vert.spv");
	if (!vert)
		throw std::runtime_error("COULD NOT LOAD VERTEX SHADER BINARY");
	
	auto vertexShaderModuleCreateInfo = vk::ShaderModuleCreateInfo{}
		.setFlags(vk::ShaderModuleCreateFlags())
		.setCode(*vert);

	vk::UniqueShaderModule vertex_module =
		device_->createShaderModuleUnique(vertexShaderModuleCreateInfo);
	
	const auto frag = read_binary_file(resources_root + "/triangle.frag.spv");
	if (!frag)
		throw std::runtime_error("COULD NOT LOAD FRAGMENT SHADER BINARY");
	
	auto fragmentShaderModuleCreateInfo = vk::ShaderModuleCreateInfo{}
		.setFlags(vk::ShaderModuleCreateFlags())
		.setCode(*frag);

	vk::UniqueShaderModule fragment_module =
		device_->createShaderModuleUnique(fragmentShaderModuleCreateInfo);
	
	std::array<vk::PipelineShaderStageCreateInfo, 2> pipelineShaderStageCreateInfos{
		vk::PipelineShaderStageCreateInfo{}
		.setStage(vk::ShaderStageFlagBits::eVertex)
		.setFlags(vk::PipelineShaderStageCreateFlags())
		.setModule(*vertex_module)
		.setPName("main"),
		
		vk::PipelineShaderStageCreateInfo{}
		.setStage(vk::ShaderStageFlagBits::eFragment)
		.setFlags(vk::PipelineShaderStageCreateFlags())
		.setModule(*fragment_module)
		.setPName("main"),
	};

    std::array<vk::DynamicState, 2> dynamicStates{
		vk::DynamicState::eViewport,
		vk::DynamicState::eScissor
	};

	auto pipelineDynamicStateCreateInfo = vk::PipelineDynamicStateCreateInfo{}
		.setFlags(vk::PipelineDynamicStateCreateFlags())
		.setDynamicStates(dynamicStates);
	
	auto pipelineVertexInputStateCreateInfo = vk::PipelineVertexInputStateCreateInfo{}
		.setFlags(vk::PipelineVertexInputStateCreateFlags())
		.setVertexBindingDescriptions(nullptr)
		.setVertexAttributeDescriptions(nullptr);

    auto pipelineInputAssemblyStateCreateInfo = vk::PipelineInputAssemblyStateCreateInfo{}
		.setFlags(vk::PipelineInputAssemblyStateCreateFlags())
		.setPrimitiveRestartEnable(vk::False)
		.setTopology(vk::PrimitiveTopology::eTriangleList);
	
	const auto window_extent = WindowExtent();
	
	vk::Viewport default_viewport{};
	default_viewport
		.setX(0.0f)
		.setY(0.0f)
		.setWidth(static_cast<float>(window_extent.width))
		.setHeight(static_cast<float>(window_extent.height))
		.setMinDepth(0.0f)
		.setMaxDepth(1.0f);
	
	std::cout << "> wiewport width:     " << default_viewport.width << "\n"
			  << "           height:    " << default_viewport.height << "\n"
			  << "           x:         " << default_viewport.x << "\n"
			  << "           y:         " << default_viewport.y << "\n"
			  << "           min depth: " << default_viewport.minDepth << "\n"
			  << "           max depth: " << default_viewport.maxDepth
			  << std::endl;

	
	auto default_scissor  = vk::Rect2D{}
		.setOffset(vk::Offset2D{}.setX(0.0f)
				                 .setY(0.0f));


	std::cout << "> scissor x: " << default_scissor.offset.x << "\n"
			  << "          y: " << default_scissor.offset.y << std::endl;

    auto pipelineViewportStateCreateInfo = vk::PipelineViewportStateCreateInfo{}
		.setFlags(vk::PipelineViewportStateCreateFlags())
		.setViewportCount(1)
		.setViewports(default_viewport)
		.setScissorCount(1)
		.setScissors(default_scissor);
	
    auto pipelineRasterizationStateCreateInfo = vk::PipelineRasterizationStateCreateInfo{}
		.setFlags(vk::PipelineRasterizationStateCreateFlags())
		.setDepthClampEnable(false)
		.setRasterizerDiscardEnable(false)
		.setPolygonMode(vk::PolygonMode::eFill)
		.setCullMode(vk::CullModeFlagBits::eBack)
		.setFrontFace(vk::FrontFace::eClockwise)
		.setDepthBiasEnable(false)
		.setDepthBiasConstantFactor(0.0f)
		.setDepthBiasClamp(0.0f)
		.setDepthBiasSlopeFactor(0.0f)
		.setLineWidth(1.0f);

    vk::PipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo{};
	pipelineMultisampleStateCreateInfo
		.setFlags(vk::PipelineMultisampleStateCreateFlags())
		.setSampleShadingEnable(false)
		.setRasterizationSamples(vk::SampleCountFlagBits::e1);

    const vk::ColorComponentFlags colorComponentFlags(vk::ColorComponentFlagBits::eR 
													| vk::ColorComponentFlagBits::eG 
													| vk::ColorComponentFlagBits::eB 
													| vk::ColorComponentFlagBits::eA);

	auto pipelineColorBlendAttachmentState = vk::PipelineColorBlendAttachmentState{}
		.setBlendEnable(false)
		.setSrcColorBlendFactor(vk::BlendFactor::eOne)
		.setDstColorBlendFactor(vk::BlendFactor::eZero)
		.setColorBlendOp(vk::BlendOp::eAdd)
		.setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
		.setDstAlphaBlendFactor(vk::BlendFactor::eZero)
		.setAlphaBlendOp(vk::BlendOp::eAdd)
		.setColorWriteMask(colorComponentFlags);
	
	auto pipelineColorBlendStateCreateInfo = vk::PipelineColorBlendStateCreateInfo{}
		.setFlags(vk::PipelineColorBlendStateCreateFlags())
		.setLogicOpEnable(false)
		.setLogicOp(vk::LogicOp::eNoOp)
		.setAttachments(pipelineColorBlendAttachmentState)
		.setBlendConstants({ 1.0f, 1.0f, 1.0f, 1.0f });
	
    auto pipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo{}
		.setFlags(vk::PipelineLayoutCreateFlags())
		.setSetLayouts(nullptr)
		.setPushConstantRanges(nullptr);
	
	pipelineLayout_ = device_->createPipelineLayoutUnique(pipelineLayoutCreateInfo);
	
	auto graphicsPipelineCreateInfo = vk::GraphicsPipelineCreateInfo{}
		.setFlags(vk::PipelineCreateFlags())
		.setStages(pipelineShaderStageCreateInfos)
		.setPVertexInputState(&pipelineVertexInputStateCreateInfo)
		.setPInputAssemblyState(&pipelineInputAssemblyStateCreateInfo)
		.setPTessellationState(nullptr)
		.setPViewportState(&pipelineViewportStateCreateInfo)
		.setPRasterizationState(&pipelineRasterizationStateCreateInfo)
		.setPMultisampleState(&pipelineMultisampleStateCreateInfo)
		.setPDepthStencilState(nullptr)
		.setPColorBlendState(&pipelineColorBlendStateCreateInfo)
		.setPDynamicState(&pipelineDynamicStateCreateInfo)
		.setLayout(*pipelineLayout_)
		.setRenderPass(*renderpass_);

    vk::Result result;
    std::tie(result, pipeline_) = device_->createGraphicsPipeline(nullptr,
																  graphicsPipelineCreateInfo);
	
    switch (result) {
	case vk::Result::eSuccess:
		break;
	case vk::Result::ePipelineCompileRequiredEXT:
		// something meaningfull here
		break;
	default: 
		throw std::runtime_error("Invalid Result state");
    }

	std::cout << "> created Graphics Pipeline!" << std::endl;
}

void VulkanRenderer::CreateFramebuffers()
{
	const auto window_extent = WindowExtent();
	framebuffers_.resize(swapchain_imageviews_.size());
	for (size_t i = 0; i < swapchain_imageviews_.size(); i++) {
		std::array<vk::ImageView, 1> framebuffer_attachments{
			*(swapchain_imageviews_[i]),
		};
		auto framebufferCreateInfo = vk::FramebufferCreateInfo{}
			.setFlags(vk::FramebufferCreateFlags())
			.setAttachments(framebuffer_attachments)
			.setWidth(window_extent.width)
			.setHeight(window_extent.height)
			.setRenderPass(*renderpass_)
			.setLayers(1);
      framebuffers_[i] = device_->createFramebufferUnique(framebufferCreateInfo);
	}
	
	std::cout << "> Created Framebuffer Count: " << framebuffers_.size() << std::endl;
}

void VulkanRenderer::CreateDrawTextures()
{
	Texture texture;
	auto optbitmap = load_bitmap("../texture.jpg", BitmapPixelFormat::RGBA);

	//TODO: Here it would be nice to have a failsafe texture, that is an embedded bitmap
	//      such as a simple checkerboard texture to notify that texture load failed.
	//      Then using this failsafe we can write a load_bitmap_or function that 
	//      guarantees return of a bitmap that can be used.
	if (std::holds_alternative<Bitmap2D>(optbitmap)) {
		draw_image_ = std::move(std::get<Bitmap2D>(optbitmap));
	}
	else if (std::holds_alternative<InvalidPath>(optbitmap)) {
		std::cout << "Invalid path: " << std::get<InvalidPath>(optbitmap).path << std::endl;
		throw std::runtime_error("Image failure");
	}
	else if (std::holds_alternative<LoadError>(optbitmap)) {
		std::cout << "Load Error: " << std::get<LoadError>(optbitmap).why << std::endl;
		throw std::runtime_error("Image failure");
	}
	
	for (size_t i = 0; i < swapchain_images_.size(); i++) {
		Texture texture = copy_bitmap_to_gpu(physical_device(),
											 device(),
											 command_pool(),
											 graphics_queue(),
											 vk::MemoryPropertyFlagBits::eDeviceLocal,
											 draw_image_);
		
		with_buffer_submit(device(),
						   command_pool(),
						   graphics_queue(),
						   [&] (vk::CommandBuffer& commandbuffer)
						   {
							   auto range = vk::ImageSubresourceRange{}
								   .setAspectMask(vk::ImageAspectFlagBits::eColor)
								   .setBaseMipLevel(0)
								   .setLevelCount(1)
								   .setBaseArrayLayer(0)
								   .setLayerCount(1);
		
							   auto barrier = vk::ImageMemoryBarrier{}
								   .setOldLayout(vk::ImageLayout::eTransferDstOptimal)
								   .setNewLayout(vk::ImageLayout::eTransferSrcOptimal)
								   .setImage(texture.allocated.image.get())
								   .setSubresourceRange(range)
								   .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
								   .setDstAccessMask(vk::AccessFlagBits::eTransferWrite)
								   .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
								   .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
		
							   commandbuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
															 vk::PipelineStageFlagBits::eTransfer,
															 vk::DependencyFlags(),
															 nullptr,
															 nullptr,
															 barrier);
						   });
		
		texture.layout = vk::ImageLayout::eTransferSrcOptimal;
		draw_textures_.push_back(std::move(texture));
	}
}

void VulkanRenderer::CreateCommandpool()
{
    auto commandPoolCreateInfo = vk::CommandPoolCreateInfo{}
		.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
		.setQueueFamilyIndex(graphics_index(graphics_present_indices_));
	commandpool_ = device_->createCommandPoolUnique(commandPoolCreateInfo, nullptr);

	std::cout << "> Created Command pool" << std::endl;
}

void VulkanRenderer::CreateCommandbuffers()
{
	// allocate a CommandBuffer from the CommandPool
    vk::CommandBufferAllocateInfo commandBufferAllocateInfo{};
	commandBufferAllocateInfo
		.setCommandPool(*commandpool_)
		.setLevel(vk::CommandBufferLevel::ePrimary)
		.setCommandBufferCount(maxFramesInFlight_);

	commandbuffers_ = device_->allocateCommandBuffersUnique(commandBufferAllocateInfo);
	std::cout << "> Created " << commandbuffers_.size() << " Command buffers" << std::endl;
}

void VulkanRenderer::CreateSyncObjects()
{
	const auto semaphoreCreateInfo = vk::SemaphoreCreateInfo{};
	const auto fenceCreateInfo = vk::FenceCreateInfo{}
		.setFlags(vk::FenceCreateFlagBits::eSignaled);
	// Signaled means that the fence is engaged at creation, this
	// simplifies the first time we want to wait for it to not need
	// extra logic

	for (int i = 0; i < maxFramesInFlight_; i++) {
		imageAvailableSemaphores_.push_back(device_->createSemaphoreUnique(semaphoreCreateInfo,
																		   nullptr));
		renderFinishedSemaphores_.push_back(device_->createSemaphoreUnique(semaphoreCreateInfo,
																		   nullptr));
		inFlightFences_.push_back(device_->createFenceUnique(fenceCreateInfo, nullptr));
	}

	std::cout << "> Created Sync Objects" << std::endl;
}

void VulkanRenderer::DrawFrame()
{
	const auto maxTimeout = std::numeric_limits<unsigned int>::max();
	auto waitresult = device_->waitForFences(*(inFlightFences_[current_frame_]),
										 true,
										 maxTimeout);
	device_->resetFences(*(inFlightFences_[current_frame_]));

	if (waitresult != vk::Result::eSuccess)
		throw std::runtime_error("Could not wait for inFlightFence");
	
	auto [result, imageIndex] =
		device_->acquireNextImageKHR(*swapchain_,
									 maxTimeout,
									 *(imageAvailableSemaphores_[current_frame_]));

	assert(result == vk::Result::eSuccess);
	assert(imageIndex < swapchain_imageviews_.size());
#if 0
	const std::string line = ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>";
	std::cout << line 
			  << "\n > index:         " << imageIndex  
			  << "\n > current frame: " << current_frame_ 
			  << "\n > total frames:  " << total_frames_ 
			  << std::endl;
#endif	

	commandbuffers_[current_frame_]->reset(vk::CommandBufferResetFlags());
	
	const float flash = std::abs(std::sin(total_frames_ / 120.f));
	const auto clear_color = vk::ClearValue{}
		.setColor({0.0f, 0.0f, flash, 1.0f});

	RecordCommandbuffer(*(commandbuffers_[current_frame_]), imageIndex, clear_color);

	const std::vector<vk::Semaphore> waitSemaphores{
		*(imageAvailableSemaphores_[current_frame_]),
	};
	const std::vector<vk::Semaphore> signalSemaphores{
		*(renderFinishedSemaphores_[current_frame_]),
	};
	
	const std::vector<vk::PipelineStageFlags> waitStages{
		vk::PipelineStageFlagBits::eColorAttachmentOutput,
	};

	auto submitInfo = vk::SubmitInfo{}
		.setWaitSemaphores(waitSemaphores)
		.setWaitDstStageMask(waitStages)
		.setCommandBuffers(*(commandbuffers_[current_frame_]))
		.setSignalSemaphores(signalSemaphores);
	
	::graphics_queue(index_queues_).submit(submitInfo, *(inFlightFences_[current_frame_]));

	const std::vector<vk::SwapchainKHR> swapchains = {*swapchain_};
	const std::vector<uint32_t> imageIndices = {imageIndex};
	auto presentInfo = vk::PresentInfoKHR{}
		.setSwapchains(swapchains)
		.setImageIndices(imageIndices)
		.setWaitSemaphores(signalSemaphores);
	
	auto presentResult = present_queue(index_queues_).presentKHR(presentInfo);
	if (presentResult != vk::Result::eSuccess)
		throw std::runtime_error("Could not wait for inFlightFence");

    current_frame_ = (current_frame_ + 1) % maxFramesInFlight_;
	total_frames_++;
	
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}

void VulkanRenderer::RecordCommandbuffer(vk::CommandBuffer& commandbuffer,
										 const uint32_t index,
										 const vk::ClearValue clear_color)
{
	const auto beginInfo = vk::CommandBufferBeginInfo{};
	commandbuffer.begin(beginInfo);
	
	const auto window_extent = WindowExtent();
	const auto renderArea = vk::Rect2D{}
		.setExtent(window_extent)
		.setOffset(vk::Offset2D{}.setX(0.0f)
				                 .setY(0.0f));
	
	const auto renderPassInfo = vk::RenderPassBeginInfo{}
		.setRenderPass(*renderpass_)
		.setFramebuffer(*framebuffers_[index])
		.setRenderArea(renderArea)
		.setClearValues(clear_color);

	commandbuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
	commandbuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline_);
	
	const std::vector<vk::Viewport> viewports{
		vk::Viewport{}
		.setX(0.0f)
		.setY(0.0f)
		.setWidth(window_extent.width)
		.setHeight(window_extent.height)
		.setMinDepth(0.0f)
		.setMaxDepth(1.0f),
	};
	const uint32_t viewport_start = 0;
	commandbuffer.setViewport(viewport_start, viewports);

	const std::vector<vk::Rect2D> scissors{
		vk::Rect2D{}
		.setExtent(window_extent)
		.setOffset(vk::Offset2D{}.setX(0.0f)
				                 .setY(0.0f)),
	};
	const uint32_t scissor_start = 0;
	commandbuffer.setScissor(scissor_start, scissors);

    const uint32_t vertexCount = 3;
    const uint32_t instanceCount = 1;
    const uint32_t firstVertex = 0;
    const uint32_t firstInstance = 0;
	commandbuffer.draw(vertexCount, instanceCount, firstVertex, firstInstance);
	commandbuffer.endRenderPass();
	
	/*
https://github.com/googlesamples/vulkan-basic-samples/blob/master/API-Samples/copy_blit_image/copy_blit_image.cpp
https://github.com/KhronosGroup/Vulkan-Hpp/blob/main/samples/CopyBlitImage/CopyBlitImage.cpp
	 */
	{
		auto range = vk::ImageSubresourceRange{}
			.setAspectMask(vk::ImageAspectFlagBits::eColor)
			.setBaseMipLevel(0)
			.setLevelCount(1)
			.setBaseArrayLayer(0)
			.setLayerCount(1);
		
		auto barrier = vk::ImageMemoryBarrier{}
			// Usually it is faster to use eUndefined for old layout, as in most
			// cases we do not care about previous data in the swapchain.
			// here we however do, as we draw to it, so we explicitly specify ePresentSrc
			.setOldLayout(vk::ImageLayout::ePresentSrcKHR)
			.setNewLayout(vk::ImageLayout::eTransferDstOptimal)
			.setImage(swapchain_images_[index])
			.setSubresourceRange(range)
			// source access flags is eNone because there are no access
			// flags for synchronization with the presentation engine
			.setSrcAccessMask(vk::AccessFlagBits::eNone)
			.setDstAccessMask(vk::AccessFlagBits::eTransferWrite)
			.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
		
		commandbuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
									  vk::PipelineStageFlagBits::eTransfer,
									  vk::DependencyFlags(),
									  nullptr,
									  nullptr,
									  barrier);
	}
	{
		auto src_subresource = vk::ImageSubresourceLayers{}
			.setAspectMask(vk::ImageAspectFlagBits::eColor)
			.setBaseArrayLayer(0)
			.setLayerCount(1)
			.setMipLevel(0);
		const std::array<vk::Offset3D, 2> src_offsets{ 
			vk::Offset3D(0, 0, 0),
			vk::Offset3D(draw_textures_[index].extent.width,
						 draw_textures_[index].extent.height,
						 1)
		};
		
		auto dst_subresource = vk::ImageSubresourceLayers{}
			.setAspectMask(vk::ImageAspectFlagBits::eColor)
			.setBaseArrayLayer(0)
			.setLayerCount(1)
			.setMipLevel(0);
		const auto window = WindowExtent();
		const std::array<vk::Offset3D, 2> dst_offsets{ 
			vk::Offset3D(0, 0, 0),
			vk::Offset3D(window.width/2, window.height/2, 1)
		};
		
		auto image_blit = vk::ImageBlit{}
			.setSrcOffsets(src_offsets)
			.setSrcSubresource(src_subresource)
			.setDstOffsets(dst_offsets)
			.setDstSubresource(dst_subresource);
		
		commandbuffer.blitImage(draw_textures_[index].allocated.image.get(),
								vk::ImageLayout::eTransferSrcOptimal,
								swapchain_images_[index],
								vk::ImageLayout::eTransferDstOptimal,
								image_blit,
								vk::Filter::eLinear);
	}
	{
		auto range = vk::ImageSubresourceRange{}
			.setAspectMask(vk::ImageAspectFlagBits::eColor)
			.setBaseMipLevel(0)
			.setLevelCount(1)
			.setBaseArrayLayer(0)
			.setLayerCount(1);
		
		auto barrier = vk::ImageMemoryBarrier{}
			.setOldLayout(vk::ImageLayout::eTransferDstOptimal)
			.setNewLayout(vk::ImageLayout::ePresentSrcKHR)
			.setImage(swapchain_images_[index])
			.setSubresourceRange(range)
			// are these access flags correct if layout is present??
			.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
			.setDstAccessMask(vk::AccessFlagBits::eTransferRead)
			.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
		
		commandbuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
									  vk::PipelineStageFlagBits::eTransfer,
									  vk::DependencyFlags(),
									  nullptr,
									  nullptr,
									  barrier);
	}

	commandbuffer.end();
}
