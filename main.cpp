#include <iostream>

#include "emu/Emulator.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "debugger/DebugWindow.hpp"
#include "debugger/Debugger.hpp"

#include "video/renderers/OpenGL_Renderer.hpp"

#include <chrono>

#include "audio_device/sdl/SdlAudioDevice.hpp"

#ifdef main
#undef main
#endif // main


int main(int argc, char* argv[]) {
	std::string rom = "./testRoms/tonc/tmr_demo.gba";
	//std::string rom = "./testRoms/SuperMarioWorld.gba";
	std::string bios_path = "./testRoms/gba_bios.bin";

	GBA::emulation::Emulator emu{rom, std::string_view(bios_path)};
	emu.SkipBios();

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
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

	debug_window.Init();

	if (!debug_window.IsRunning()) {
		std::cerr << "SDL Init failed : " << SDL_GetError() << std::endl;
		std::exit(0);
	}

	GBA::video::renderer::OpenGL opengl_rend{};

	opengl_rend.SetKeypad(&emu.GetContext().keypad);

	if (!opengl_rend.Init(3, 3)) {
		std::exit(0);
	}

	GBA::audio::AudioDevice* audio = new GBA::audio::SdlAudioDevice();
	
	ctx.apu.SetCallback(
		[](GBA::common::i16*) {
			bool tth = true;
		}, 2048
	);

	audio->Start();

	unsigned long long prev_second = 0;
	unsigned tot_frames = 0;

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
			tot_frames++;

			auto now = 
				std::chrono::time_point_cast<std::chrono::milliseconds>
				(std::chrono::high_resolution_clock::now()).time_since_epoch().count();

			auto diff = now - prev_second;

			if (diff > 1000) {
				prev_second = now;

				//std::cout << tot_frames << std::endl;
				tot_frames = 0;
			}

			opengl_rend.SetFrame(ctx.ppu.GetFrame());
		}
	}

	audio->Stop();

	SDL_Quit();

	std::cin.get();
}