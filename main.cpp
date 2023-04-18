#include <iostream>

#include "emu/Emulator.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "debugger/DebugWindow.hpp"

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

	GBA::debugger::DebugWindow debugger{};

	debugger.SetProcessor(&ctx.processor);
	debugger.SetBus(&ctx.bus);
	debugger.SetGamePack(&ctx.pack);

	debugger.Init();

	if (!debugger.IsRunning()) {
		std::cerr << "SDL Init failed : " << SDL_GetError() << std::endl;
		std::exit(0);
	}

	while (!debugger.StopRequested()) {
		SDL_Event ev;

		while (SDL_PollEvent(&ev)) {
			if (debugger.ConfirmEventTarget(&ev))
				debugger.ProcessEvent(&ev);
		}

		if (!debugger.StopRequested()) {
			debugger.Update();
		}

		emu.RunEmulation(debugger.GetEmulatorStatus());
	}


	SDL_Quit();

	std::cin.get();
}