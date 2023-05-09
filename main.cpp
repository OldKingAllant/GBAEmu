#include <iostream>

#include "emu/Emulator.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "debugger/DebugWindow.hpp"
#include "debugger/Debugger.hpp"

#include "video/renderers/OpenGL_Renderer.hpp"

#ifdef main
#undef main
#endif // main


int main(int argc, char* argv[]) {
	std::string rom = "./testRoms/arm/arm.gba";

	GBA::emulation::Emulator emu{rom};

	emu.GetContext().processor.SkipBios();

	if (SDL_Init(SDL_INIT_VIDEO)) {
		std::cerr << SDL_GetError() << std::endl;

		std::cin.get();
		std::exit(0);
	}

	if (!IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG)) {
		std::cerr << SDL_GetError() << std::endl;

		std::cin.get();
		std::exit(0);
	}

	auto& ctx = emu.GetContext();

	GBA::debugger::Debugger debugger{ emu };

	GBA::debugger::DebugWindow debug_window{ debugger };

	debug_window.SetProcessor(&ctx.processor);
	debug_window.SetBus(&ctx.bus);
	debug_window.SetGamePack(&ctx.pack);

	debug_window.Init();

	if (!debug_window.IsRunning()) {
		std::cerr << "SDL Init failed : " << SDL_GetError() << std::endl;
		std::exit(0);
	}

	GBA::video::renderer::OpenGL opengl_rend{};

	if (!opengl_rend.Init(3, 3)) {
		std::exit(0);
	}

	while (!debug_window.StopRequested()
		&& !opengl_rend.Stopped()) {
		SDL_Event ev;

		while (SDL_PollEvent(&ev)) {
			if (debug_window.ConfirmEventTarget(&ev))
				debug_window.ProcessEvent(&ev);
			else if (opengl_rend.ConfirmEventTarget(&ev))
				opengl_rend.ProcessEvent(&ev);
		}

		if (!debug_window.StopRequested() && !opengl_rend.Stopped()) {
			debug_window.Update();
			opengl_rend.PresentFrame();
		}

		debugger.Run(debug_window.GetEmulatorStatus());

		if (ctx.ppu.HasFrame()) {
			opengl_rend.SetFrame(ctx.ppu.GetFrame());
		}
	}


	SDL_Quit();

	std::cin.get();
}