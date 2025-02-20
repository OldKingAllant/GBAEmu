#include <iostream>
#include <optional>
#include <chrono>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "emu/Emulator.hpp"
#include "video/renderers/OpenGL_Renderer.hpp"
#include "config/Config.hpp"

#include "audio_device/sdl/SdlAudioDevice.hpp"

#include "debugger/Debugger.hpp"
#include "debugger/DebugWindow.hpp"

#ifdef main
#undef main
#endif // main

static std::optional<int> parse_int(std::string const& str) {
	try {
		int value = std::stoi(str);
		return value;
	}
	catch (std::exception const&) {}

	return std::nullopt;
}


int main(int argc, char* argv[]) {
	GBA::config::Config conf{};

	if (!conf.Load("config.txt")) {
		std::cout << "Could not load config file, use default" << std::endl;
		
		conf.Default();
	}

	std::string bios_path = conf.data.get("BIOS").get("file");

	GBA::emulation::Emulator* emu = new GBA::emulation::Emulator{
		std::string_view(bios_path)
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

	/////////////////////////////////////////////////////////////

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

		if (conf.data["EMU"]["startup_load_save"] == "true") {
			std::string save_path = conf.data["EMU"].has("game_save_path") ?
				conf.data["EMU"]["game_save_path"] : "./saves";

			std::string fname = emu->GetContext().pack.GetRomPath().stem().string();
			save_path += "/" + fname + ".save";

			if (std::filesystem::exists(save_path)) {
				emu->GetContext().pack.LoadBackup(std::filesystem::path(save_path));
			}
		}

		if (conf.data["BIOS"]["skip"] == "true")
			emu->SkipBios();

		auto& ctx = emu->GetContext();

		ctx.apu.SetCallback(
			[audio](GBA::common::i16 sample_l, GBA::common::i16 sample_r) {
				audio->PushSample(sample_l, sample_r);
			}, 1024
		);

		ctx.apu.SetFreq(44100);

		emu->SaveResetState();

		has_rom = true;
	};

	if (conf.data.get("ROM").has("default_rom")) {
		std::string rom = conf.data.get("ROM").get("default_rom");
		emu_init(rom);
	}

	std::string scale = conf.data.get("EMU").has("scale") ?
		conf.data.get("EMU").get("scale") :
		std::string("4");

	unsigned int scale_val = parse_int(scale).value_or(4);

	if (scale_val == 0 || scale_val > 10)
		scale_val = 4;

	unsigned int rewind_buf_size{}, rewind_interval{};
	bool rewind_enable{ false };

	{
		rewind_buf_size = unsigned(parse_int(conf.data["EMU"]["rewind_buf_size"]).value_or(30));
		rewind_interval = unsigned(parse_int(conf.data["EMU"]["rewind_interval_seconds"])
			.value_or(1));
		rewind_enable = conf.data["EMU"]["rewind_enable"] == "true";
	}

	emu->SetRewindBufferSize(GBA::common::u32(rewind_buf_size));

	/////////////////////////////////////////////////////////////////

	GBA::video::renderer::OpenGL opengl_rend{paused};

	opengl_rend.SetKeypad(&emu->GetContext().keypad);

	if (!opengl_rend.Init(scale_val, scale_val)) {
		std::exit(0);
	}

	opengl_rend.SetQuickSaveAction([&conf, emu, &opengl_rend](bool save) {
		std::string quick_save_path = conf.data.has("quick_save_path") ?
			conf.data.get("EMU").get("quick_save_path") : 
			std::string{"./quick_state"};

		if (!std::filesystem::is_directory(quick_save_path)) {
			std::cout << "Invalid quick save path, using default" << std::endl;
			quick_save_path = "./quick_state";
		}

		if (!std::filesystem::exists(quick_save_path)) {
			std::filesystem::create_directories(quick_save_path);
		}

		std::string fname = emu->GetContext().pack.GetRomPath().stem().string();
		quick_save_path += "/" + fname + ".state";

		if (save) {
			emu->StoreState(quick_save_path);
		}
		else {
			emu->LoadState(quick_save_path);
			auto framebuf = emu->GetContext()
				.ppu
				.GetFrame();
			opengl_rend.SetFrame(framebuf);
		}
		
	});

	opengl_rend.SetOnPause([&paused, emu](bool val) { 
		if (paused != val && !val) {
			emu->RewindPop();
		}

		paused = val; 
	});

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

	opengl_rend.SetSaveStateAction([emu, &opengl_rend](std::string path, bool store) {
		if (store)
			emu->StoreState(path);
		else {
			emu->LoadState(path);
			auto framebuf = emu->GetContext()
				.ppu
				.GetFrame();
			opengl_rend.SetFrame(framebuf);
		}
	});

	opengl_rend.SetRewindAction([emu, &opengl_rend](bool forward) {
		if (forward) {
			if (emu->RewindForward()) {
				auto framebuf = emu->GetContext()
					.ppu
					.GetFrame();
				opengl_rend.SetFrame(framebuf);
			}
			else {
				std::cout << "Cannot forward" << std::endl;
			}
		}
		else {
			if (emu->RewindBackward()) {
				auto framebuf = emu->GetContext()
					.ppu
					.GetFrame();
				opengl_rend.SetFrame(framebuf);
			}
			else {
				std::cout << "Cannot backward" << std::endl;
			}
		}
	});

	opengl_rend.SetResetAction([emu, &opengl_rend]() {
		if (emu->Reset()) {
			auto framebuf = emu->GetContext()
				.ppu
				.GetFrame();
			opengl_rend.SetFrame(framebuf);
		}
		else {
			std::cout << "Reset failed" << std::endl;
		}
	});

	opengl_rend.SetSyncToAudioCallback([audio](bool sync) { audio->AudioSync(sync); });

	audio->Start();

	auto& ctx = emu->GetContext();

	/////////////////////////////////////////////////////////////////////
	using GBA::cheats::CheatType;
	/*emu->AddCheat({
		"FF363D32 86978AE6",
		"9E9098F2 D85C96B3",
		"3769FF8D 71474369"
	}, CheatType::ACTION_REPLAY);*/

	emu->AddCheat({
		"FF363D32 86978AE6",
		"9E9098F2 D85C96B3", 
		"3769FF8D 71474369"
	}, CheatType::ACTION_REPLAY, "Items cost $1");
	emu->EnableCheat("Items cost $1");

	auto last_save_timestamp = std::chrono::system_clock::now();

	while (!opengl_rend.Stopped())
	{
		SDL_Event ev{};

		while (SDL_PollEvent(&ev)) {
			opengl_rend.ProcessEvent(&ev);
		}

		if (!paused && has_rom) {
			emu->RunTillVblank();
			
			if (ctx.ppu.HasFrame()) {
				auto framebuffer = ctx.ppu.GetFrame();
				opengl_rend.SetFrame(framebuffer);
			}

			if (rewind_enable) {
				auto now = std::chrono::system_clock::now();
				auto diff = std::chrono::duration_cast<std::chrono::seconds>(
					now - last_save_timestamp
				).count();

				if (diff >= long long(rewind_interval)) {
					last_save_timestamp = now;
					emu->RewindPush();
				}
			}
		}

		opengl_rend.PresentFrame();
	}

	//////////////////////////////////////////////////////////////////////////

	audio->Stop();

	SDL_Quit();

	delete emu;

	conf.Store("config.txt");

	std::cin.get();
}