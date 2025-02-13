#include <iostream>
#include <sstream>
#include <fstream>
#include <variant>
#include <algorithm>
#include <chrono>
#include <thread>

#include "VulkanRenderer.hpp"
#include "SimpleRenderBlitPass.hpp"
//#include "GeometryPass.hpp"

#include "Bitmap.hpp"
#include "Canvas.hpp"


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
	const auto purple = Pixel8bitRGBA{170, 0, 170, 255};
	const auto yellow = Pixel8bitRGBA{170, 170, 0, 255};

	PresentationContext presentor(2);
	Texture2D blit_texture;
		
	auto loaded_lulu = load_bitmap("../lulu.jpg", BitmapPixelFormat::RGBA);
	if (!std::holds_alternative<LoadedBitmap2D>(loaded_lulu))
		throw std::runtime_error("Could not load bitmap image..");
	
	auto lulu_checkerboard 
		= std::move(std::get<LoadedBitmap2D>(loaded_lulu))
		| as_canvas
		| draw_checkerboard(yellow, 100)
		| draw_coordinate_system(CanvasExtent{20, 400});
	
	blit_texture = copy_to_gpu(presentor.physical_device,
							   presentor.device.get(),
							   presentor.command_pool(),
							   presentor.graphics_queue(),
							   vk::MemoryPropertyFlagBits::eDeviceLocal,
							   lulu_checkerboard);

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
							   .setDstAccessMask(vk::AccessFlagBits::eTransferRead)
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

		//const auto frame_time_ms =
		//std::chrono::duration_cast<std::chrono::milliseconds>(frame_time);
		//std::cout << "Frame Time [ms]: " << frame_time_ms.count() << std::endl;
	}
	
	presentor.device.get().waitIdle();

	return 0;
}
