#include <iostream>
#include <sstream>
#include <fstream>
#include <variant>
#include <algorithm>
#include <chrono>
#include <thread>

#include "VulkanRenderer.hpp"
#include "SimpleRenderBlitPass.hpp"


template <typename F, typename... Args>
std::chrono::duration<double>
with_time_measurement(F&& f, Args&&... args)
{
	using Clock = std::chrono::high_resolution_clock;
	auto start = Clock::now();
	std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
	auto end = Clock::now();
	return end - start;
}

int main()
{
	PresentationContext presentor(2);

	auto optbitmap = load_bitmap("../lulu.jpg", BitmapPixelFormat::RGBA);
	Texture2D blit_texture;
	if (std::holds_alternative<Bitmap2D>(optbitmap)) {
		blit_texture = copy_bitmap_to_gpu(presentor.physical_device,
										  presentor.device.get(),
										  presentor.command_pool(),
										  presentor.graphics_queue(),
										  vk::MemoryPropertyFlagBits::eDeviceLocal,
										  std::get<Bitmap2D>(optbitmap));
	
		/*transfer the draw texture to a transferSrc layout for blitting*/
		with_buffer_submit(presentor.device.get(),
						   presentor.command_pool(),
						   presentor.graphics_queue(),
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
		create_simple_render_blit_pass(presentor.physical_device,
									   presentor.device.get(),
									   presentor.command_pool(),
									   presentor.graphics_queue(),
									   std::move(blit_texture),
									   presentor.get_window_extent(),
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
		auto frame_time = with_time_measurement([&] () {
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
						presentor.window_resize_event_triggered();
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
			FrameProducer frameGenerator = [&] (CurrentFrameInfo frameInfo)
				-> std::optional<Texture2D*>
				{
					auto textureptr = generate_next_frame(render_blit_pass,
														  frameInfo.current_flight_frame_index,
														  frameInfo.total_frame_count,
														  presentor.device.get(),
														  presentor.command_pool(),
														  presentor.graphics_queue());
					
					if (textureptr == nullptr)
						return std::nullopt;
					return textureptr;
				};
			
			presentor.with_presentation(frameGenerator);
		});

		//std::this_thread::sleep_for(std::chrono::milliseconds(1000));

		const auto frame_time_ms =
			std::chrono::duration_cast<std::chrono::milliseconds>(frame_time);
		std::cout << "Frame Time [ms]: " << frame_time_ms.count() << std::endl;
	}
	
	presentor.device.get().waitIdle();

	return 0;
}
