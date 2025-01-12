#include <iostream>
#include <sstream>
#include <fstream>
#include <variant>
#include <algorithm>
#include <chrono>
#include <thread>

#include <polymorph/polymorph.hpp>

#include "VulkanRenderer.hpp"


int main()
{
	VulkanRenderer renderer{};

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
					// DO RESIZE
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
		renderer.DrawFrame();

		// wait for each frame
	    //std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
	
	return 0;
}
