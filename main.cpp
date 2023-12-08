#include <iostream>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "emu/Emulator.hpp"
#include "video/renderers/OpenGL_Renderer.hpp"
#include "config/Config.hpp"

#include "audio_device/sdl/SdlAudioDevice.hpp"

#ifdef main
#undef main
#endif // main


int main(int argc, char* argv[]) {
	GBA::config::Config conf{};

	if (!conf.Load("config.txt")) {
		std::cout << "Could not load config file, use default" << std::endl;
		
		conf.Default();
	}

	std::string bios_path = conf.data.get("BIOS").get("file");

	GBA::emulation::Emulator* emu = new GBA::emulation::Emulator{
		 std::optional{ std::string_view(bios_path) }
	};

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

	bool paused = conf.data.get("EMU").get("start_paused") == "true";
	bool has_rom = false;

	GBA::audio::AudioDevice* audio = new GBA::audio::SdlAudioDevice();

	auto emu_init = [emu, &has_rom, &conf, audio](std::string rom) {
		if (has_rom) {
			std::cout << "Rom swapping not implemented" << std::endl;
			return;
		}

		if (!emu->LoadRom(rom)) {
			std::cout << "Rom load failed" << std::endl;
			return;
		}

		emu->Init();

		if (conf.data.get("BIOS").get("skip") == "true")
			emu->SkipBios();

		auto& ctx = emu->GetContext();

		ctx.apu.SetCallback(
			[audio](GBA::common::i16 sample_l, GBA::common::i16 sample_r) {
				audio->PushSample(sample_l, sample_r);
			}, 1024
		);

		ctx.apu.SetFreq(44100);

		has_rom = true;
	};

	if (conf.data.get("ROM").has("default_rom")) {
		std::string rom = conf.data.get("ROM").get("default_rom");
		emu_init(rom);
	}

	GBA::video::renderer::OpenGL opengl_rend{paused};
	opengl_rend.SetKeypad(&emu->GetContext().keypad);
	if (!opengl_rend.Init(3, 3)) {
		std::exit(0);
	}

	opengl_rend.SetConfigChangeCallback([&conf](std::string _1, std::string _2, std::string _3) {
		if (!conf.Change(_1, _2, _3))
			std::cout << "Could not change configuration" << std::endl;
	});

	opengl_rend.SetOnPause([&paused](bool val) { paused = val; });
	opengl_rend.SetRomSelectedAction([&emu_init](std::string rom) {
		emu_init(rom);
	});
	opengl_rend.SetSaveSelectedAction([emu](std::string file_path) {
		if (!emu->GetContext().pack.LoadBackup(file_path))
			std::cout << "Load failed" << std::endl;
		else
			std::cout << "Load successfull" << std::endl;
	});
	opengl_rend.SetSaveStoreAction([emu](std::string dest) {
		if (!emu->GetContext().pack.StoreBackup(dest))
			std::cout << "Save failed" << std::endl;
		else
			std::cout << "Save ok" << std::endl;
	});

	audio->Start();

	auto& ctx = emu->GetContext();

	while (!opengl_rend.Stopped())
	{
		SDL_Event ev{};

		while (SDL_PollEvent(&ev)) {
			opengl_rend.ProcessEvent(&ev);
		}

		if (!paused && has_rom) {
			emu->RunTillVblank();
			auto framebuffer = ctx.ppu.GetFrame();
			opengl_rend.SetFrame(framebuffer);
		}

		opengl_rend.PresentFrame();
	}

	audio->Stop();

	SDL_Quit();

	delete emu;

	conf.Store("config.txt");

	std::cin.get();
}