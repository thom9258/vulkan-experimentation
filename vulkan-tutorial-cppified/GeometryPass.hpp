#pragma once 

#include <filesystem>
#include "VertexPosColor.hpp"

struct GeometryFramePass
{
	Texture2D rendertarget;
	vk::UniqueImageView view;
	vk::UniqueFramebuffer framebuffer;
};

struct GeometryPass
{
	bool debug_print;
	Texture2D draw_texture;
	vk::UniqueRenderPass renderpass;
	vk::UniquePipelineLayout pipeline_layout;
    vk::Pipeline pipeline;
	std::vector<GeometryFramePass> frame_passes;
};

GeometryPass
create_geometry_pass(vk::PhysicalDevice& physical_device,
					 vk::Device& device,
					 vk::CommandPool& command_pool,
					 vk::Queue& graphics_queue,
					 Texture2D&& draw_texture,
					 const vk::Extent2D render_extent,
					 const uint32_t frames_in_flight,
					 const std::filesystem::path vertexshader,
					 const std::filesystem::path fragmentshader) noexcept
{
	constexpr auto render_format = vk::Format::eR8G8B8A8Srgb;

	const auto rendertarget_extent = vk::Extent3D{}
		.setWidth(render_extent.width)
		.setHeight(render_extent.height)
		.setDepth(1);


	GeometryPass render_blit_pass;
	render_blit_pass.debug_print = false;
	render_blit_pass.draw_texture = std::move(draw_texture);
	

	/* Setup the renderpass
	 */
    const std::array<vk::AttachmentDescription, 1> attachments{
		vk::AttachmentDescription{}
		.setFlags(vk::AttachmentDescriptionFlags())
		.setFormat(render_format)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		// NOTE these are important, as they determine the layout of the image before and after
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::eTransferDstOptimal),
	};
	std::cout 
		<< "> RenderPass setup:\n"
		<< "> \tInitialLayout: " << vk::to_string(attachments[0].initialLayout) << "\n"
		<< "> \tFinalLayout:   " << vk::to_string(attachments[0].finalLayout) << "\n"
		<< "> \tFormat:        " << vk::to_string(attachments[0].format) << "\n"
		<< std::endl;

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
		.setAttachments(attachments)
		.setDependencies(dependency)
		.setSubpasses(subpass);

    render_blit_pass.renderpass = device.createRenderPassUnique(renderPassCreateInfo);
	std::cout << "> created Render Pass!" << std::endl;
	
	const auto vert = read_binary_file(vertexshader.string().c_str());
	if (!vert)
		throw std::runtime_error("COULD NOT LOAD VERTEX SHADER BINARY");
	
	auto vertexShaderModuleCreateInfo = vk::ShaderModuleCreateInfo{}
		.setFlags(vk::ShaderModuleCreateFlags())
		.setCode(*vert);

	vk::UniqueShaderModule vertex_module =
		device.createShaderModuleUnique(vertexShaderModuleCreateInfo);
	
	const auto frag = read_binary_file(fragmentshader.string().c_str());
	if (!frag)
		throw std::runtime_error("COULD NOT LOAD FRAGMENT SHADER BINARY");
	
	auto fragmentShaderModuleCreateInfo = vk::ShaderModuleCreateInfo{}
		.setFlags(vk::ShaderModuleCreateFlags())
		.setCode(*frag);

	vk::UniqueShaderModule fragment_module =
		device.createShaderModuleUnique(fragmentShaderModuleCreateInfo);
	
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

	std::cout << "> Created shaders" << std::endl;

    std::array<vk::DynamicState, 2> dynamicStates{
		vk::DynamicState::eViewport,
		vk::DynamicState::eScissor
	};

	auto pipelineDynamicStateCreateInfo = vk::PipelineDynamicStateCreateInfo{}
		.setFlags(vk::PipelineDynamicStateCreateFlags())
		.setDynamicStates(dynamicStates);
	
	const auto bindingDescriptions = get_binding_descriptions(VertexPosColor{});
	const auto attributeDescriptions = get_attribute_descriptions(VertexPosColor{});
	
	auto pipelineVertexInputStateCreateInfo = vk::PipelineVertexInputStateCreateInfo{}
		.setFlags(vk::PipelineVertexInputStateCreateFlags())
		.setVertexBindingDescriptions(bindingDescriptions)
		.setVertexAttributeDescriptions(attributeDescriptions);

    auto pipelineInputAssemblyStateCreateInfo = vk::PipelineInputAssemblyStateCreateInfo{}
		.setFlags(vk::PipelineInputAssemblyStateCreateFlags())
		.setPrimitiveRestartEnable(vk::False)
		.setTopology(vk::PrimitiveTopology::eTriangleList);
	
	 const auto initial_viewport = vk::Viewport{}
		.setX(0.0f)
		.setY(0.0f)
		.setWidth(static_cast<float>(render_extent.width))
		.setHeight(static_cast<float>(render_extent.height))
		.setMinDepth(0.0f)
		.setMaxDepth(1.0f);
	
	auto initial_scissor = vk::Rect2D{}
		.setOffset(vk::Offset2D{}.setX(0.0f)
				                 .setY(0.0f));


    auto pipelineViewportStateCreateInfo = vk::PipelineViewportStateCreateInfo{}
		.setFlags(vk::PipelineViewportStateCreateFlags())
		.setViewports(initial_viewport)
		.setScissors(initial_scissor);
	
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

	render_blit_pass.pipeline_layout = 
		device.createPipelineLayoutUnique(pipelineLayoutCreateInfo);
	
	std::cout << "> Created Pipeline Layout" << std::endl;
	
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
		.setLayout(render_blit_pass.pipeline_layout.get())
		.setRenderPass(render_blit_pass.renderpass.get());

    vk::Result result;
    std::tie(result, render_blit_pass.pipeline) =
		device.createGraphicsPipeline(nullptr,
										graphicsPipelineCreateInfo);
	
    switch (result) {
	case vk::Result::eSuccess:
		break;
	case vk::Result::ePipelineCompileRequiredEXT:
		throw std::runtime_error("Creating pipeline error: PipelineCompileRequiredEXT");
	default: 
		throw std::runtime_error("Invalid Result state");
    }
	std::cout << "> created Graphics Pipeline!" << std::endl;
	
	for (size_t i = 0; i < frames_in_flight; i++) {
		GeometryFramePass frame_pass;
		
		/* Setup the rendertarget for the render pass
		 */
		frame_pass.rendertarget =
			create_empty_rendertarget_texture(physical_device,
											  device,
											  render_format,
											  rendertarget_extent,
											  vk::ImageTiling::eOptimal,
											  vk::MemoryPropertyFlagBits::eDeviceLocal);
		
		auto transition_to_transfer_src = [&] (vk::CommandBuffer& commandbuffer)
		{
			frame_pass.rendertarget.layout =
				transition_image_color_override(get_image(frame_pass.rendertarget),
												commandbuffer);
		};

		with_buffer_submit(device,
						   command_pool,
						   graphics_queue,
						   transition_to_transfer_src);
	
		/* Setup the rendertarget view
		 */
		frame_pass.view = create_texture_view(device,
											  frame_pass.rendertarget,
											  vk::ImageAspectFlagBits::eColor);
		
		/* Setup the FrameBuffers
		 */
		std::array<vk::ImageView, 1> attachments{
			frame_pass.view.get(),
		};
		auto framebufferCreateInfo = vk::FramebufferCreateInfo{}
			.setFlags(vk::FramebufferCreateFlags())
			.setAttachments(attachments)
			.setWidth(render_extent.width)
			.setHeight(render_extent.height)
			.setRenderPass(render_blit_pass.renderpass.get())
			.setLayers(1);
		frame_pass.framebuffer = device.createFramebufferUnique(framebufferCreateInfo);
		
		render_blit_pass.frame_passes.push_back(std::move(frame_pass));
	}

	std::cout << "> Created FramePasses" << std::endl;
	return render_blit_pass;
}

Texture2D*
generate_next_frame(GeometryPass& pass,
					const uint32_t current_frame_in_flight,
					const uint64_t total_frames,
					vk::Device& device,
					vk::CommandPool& command_pool,
					vk::Queue& queue)
{
	GeometryFramePass& frame_pass = pass.frame_passes[current_frame_in_flight];

	const auto render_extent = vk::Extent2D{}
		.setWidth(frame_pass.rendertarget.extent.width) 
		.setHeight(frame_pass.rendertarget.extent.height); 

	const float flash = std::abs(std::sin(total_frames / 120.f));
	const auto renderpass_initial_clear_color = vk::ClearValue{}
		.setColor({0.0f, 0.0f, flash, 1.0f});

	auto generate_frame = [&] (vk::CommandBuffer& commandbuffer) 
	{
		const auto render_area = vk::Rect2D{}
			.setOffset(vk::Offset2D{}.setX(0.0f).setY(0.0f))
			.setExtent(render_extent);
		
		const auto renderPassInfo = vk::RenderPassBeginInfo{}
			.setRenderPass(pass.renderpass.get())
			.setFramebuffer(frame_pass.framebuffer.get())
			.setRenderArea(render_area)
			.setClearValues(renderpass_initial_clear_color);

		commandbuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
		commandbuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pass.pipeline);
		const std::vector<vk::Viewport> viewports{
			vk::Viewport{}
			.setX(0.0f)
			.setY(0.0f)
			.setWidth(render_extent.width)
			.setHeight(render_extent.height)
			.setMinDepth(0.0f)
			.setMaxDepth(1.0f),
		};
		const uint32_t viewport_start = 0;
		commandbuffer.setViewport(viewport_start, viewports);

		const std::vector<vk::Rect2D> scissors{
			vk::Rect2D{}
			.setOffset(vk::Offset2D{}.setX(0.0f).setY(0.0f))
			.setExtent(render_extent),
		};
		const uint32_t scissor_start = 0;
		commandbuffer.setScissor(scissor_start, scissors);

		const uint32_t vertexCount = 3;
		const uint32_t instanceCount = 1;
		const uint32_t firstVertex = 0;
		const uint32_t firstInstance = 0;
		commandbuffer.draw(vertexCount, instanceCount, firstVertex, firstInstance);
		commandbuffer.endRenderPass();

		if (pass.debug_print) {
			std::cout << "Pass: rendered to rendertarget" << std::endl;
			std::cout << "=======================================" << std::endl;
		}


		/** Layout is in TransferDstOptimal as specified by the render pipeline.
		 *   This means we can just blit to it directly afterwards.
		 */

		if (true) {
			auto src_dst_subresource = vk::ImageSubresourceLayers{}
				.setAspectMask(vk::ImageAspectFlagBits::eColor)
				.setBaseArrayLayer(0)
				.setLayerCount(1)
				.setMipLevel(0);
			const std::array<vk::Offset3D, 2> src_offsets{ 
				vk::Offset3D(0, 0, 0),
				vk::Offset3D(pass.draw_texture.extent.width,
							 pass.draw_texture.extent.height,
							 1)
			};
			const std::array<vk::Offset3D, 2> dst_offsets{ 
				vk::Offset3D(0, 0, 0),
				vk::Offset3D(pass.draw_texture.extent.width / 3,
							 pass.draw_texture.extent.height / 3,
							 1)
			};
			
			auto image_blit = vk::ImageBlit{}
				.setSrcOffsets(src_offsets)
				.setSrcSubresource(src_dst_subresource)
				.setDstOffsets(dst_offsets)
				.setDstSubresource(src_dst_subresource);
			
			commandbuffer.blitImage(get_image(pass.draw_texture),
									vk::ImageLayout::eTransferSrcOptimal,
									get_image(frame_pass.rendertarget),
									vk::ImageLayout::eTransferDstOptimal,
									image_blit,
									// blit over using nearest
									vk::Filter::eNearest);
			if (pass.debug_print) {
				std::cout << "Pass: Blitted draw_texture to rendertarget" << std::endl;
				std::cout << "=======================================" << std::endl;
			}
		}
		
		if (true) {
			auto range = vk::ImageSubresourceRange{}
				.setAspectMask(vk::ImageAspectFlagBits::eColor)
				.setBaseMipLevel(0)
				.setLevelCount(1)
				.setBaseArrayLayer(0)
				.setLayerCount(1);
			
			auto barrier = vk::ImageMemoryBarrier{}
				.setImage(get_image(frame_pass.rendertarget))
				.setSubresourceRange(range)
				.setOldLayout(vk::ImageLayout::eTransferDstOptimal)
				.setNewLayout(vk::ImageLayout::eTransferSrcOptimal)
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
			
			if (pass.debug_print) {
				std::cout << "Pass: Transfered rendertarget from TransferDst to TransferSrc" 
						  << std::endl;
				std::cout << "=======================================" << std::endl;
			}
		}
	};

	with_buffer_submit(device,
					   command_pool,
					   queue,
					   generate_frame);

	return &frame_pass.rendertarget;
}
