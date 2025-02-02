#include <iostream>
#include <sstream>
#include <fstream>
#include <variant>
#include <algorithm>
#include <chrono>
#include <thread>

#include "VulkanRenderer.hpp"
#include "SimpleRenderBlitPass.hpp"

int main()
{
	VulkanRenderer renderer(2);

	auto optbitmap = load_bitmap("../lulu.jpg", BitmapPixelFormat::RGBA);
	Texture2D blit_texture;
	if (std::holds_alternative<Bitmap2D>(optbitmap)) {
		blit_texture = copy_bitmap_to_gpu(renderer.physical_device(),
										  renderer.device(),
										  renderer.command_pool(),
										  renderer.graphics_queue(),
										  vk::MemoryPropertyFlagBits::eDeviceLocal,
										  std::get<Bitmap2D>(optbitmap));
	
		/*transfer the draw texture to a transferSrc layout for blitting*/
		with_buffer_submit(renderer.device(),
						   renderer.command_pool(),
						   renderer.graphics_queue(),
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
								   .setImage(get_image(blit_texture))
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
		blit_texture.layout = vk::ImageLayout::eTransferSrcOptimal;

	}
	else if (std::holds_alternative<InvalidPath>(optbitmap)) {
		std::cout << "Invalid path: " << std::get<InvalidPath>(optbitmap).path << std::endl;
		throw std::runtime_error("Image failure");
	}
	else if (std::holds_alternative<LoadError>(optbitmap)) {
		std::cout << "Load Error: " << std::get<LoadError>(optbitmap).why << std::endl;
		throw std::runtime_error("Image failure");
	}
	
	std::cout << "===========================================================\n"
			  << " Creating Simple RenderBlit Pass\n"
			  << "==========================================================="
			  << std::endl;

	
	SimpleRenderBlitPass render_blit_pass = 
		create_simple_render_blit_pass(renderer.physical_device(),
									   renderer.device(),
									   renderer.command_pool(),
									   renderer.graphics_queue(),
									   std::move(blit_texture),
									   renderer.get_window_extent(),
									   2,
									   resources_root + "/triangle.vert.spv",
									   resources_root + "/triangle.frag.spv"
									   );
	
	/** ************************************************************************
	 * Frame Loop
	 */
	SDL_Event event{};
	bool exit = false;

	while (!exit) {
		/** ************************************************************************
		 * Handle Inputs
		 */
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				exit = true;
				break;
				
			case SDL_KEYDOWN:
				switch( event.key.keysym.sym ) {
				case SDLK_ESCAPE:
					exit = true;
					break;
				}
				
				break;
				
			case SDL_WINDOWEVENT: {
				const SDL_WindowEvent& wev = event.window;
				switch (wev.event) {
				case SDL_WINDOWEVENT_RESIZED:
				case SDL_WINDOWEVENT_SIZE_CHANGED:
					renderer.window_resize_event_triggered();
					//TODO: DO RESIZE
					break;
				case SDL_WINDOWEVENT_CLOSE:
					exit = true;
					break;
				}
			} break;
			}
		}
		
		/** ************************************************************************
		 * Render Loop
		 */
		FrameGenerator frameGenerator = [&] (CurrentFrameInfo frameInfo)
			-> std::optional<Texture2D*>
		{
			auto textureptr = generate_next_frame(render_blit_pass,
												  frameInfo.current_flight_frame_index,
												  frameInfo.total_frame_count,
												  renderer.device(),
												  renderer.command_pool(),
												  renderer.graphics_queue());

			if (textureptr == nullptr)
				return std::nullopt;
			return textureptr;
		};

		renderer.with_presentation(frameGenerator);
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
	
	return 0;
}
