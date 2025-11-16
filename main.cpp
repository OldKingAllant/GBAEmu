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

#include "emu/TASParser.hpp"

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

	unsigned int rewind_buf_size{}, rewind_interval{};
	bool rewind_enable{ false };

	{
		rewind_buf_size = unsigned(parse_int(conf.data["EMU"]["rewind_buf_size"]).value_or(30));
		rewind_interval = unsigned(parse_int(conf.data["EMU"]["rewind_interval_seconds"])
			.value_or(1));
		rewind_enable = conf.data["EMU"]["rewind_enable"] == "true";
	}

	bool enable_hooks	 = conf.data["CHEATS"]["enable_hooks"] == "true";
	bool enable_hle		 = conf.data["BIOS"]["enable_hle"] == "true";
	bool log_hle		 = conf.data["BIOS"]["log_hle"] == "true";
	bool enable_fastmem  = conf.data["EMU"]["enable_fastmem"] == "true";
	bool precise_bios	 = conf.data["EMU"]["precise_bios_access"] == "true";
	bool precise_ppu	 = conf.data["EMU"]["precise_ppu_access"] == "true";

	bool threaded_render = conf.data["VIDEO"]["threaded_render"] == "true";
	bool threaded_line_threshold_enable = conf.data["VIDEO"]["enable_threaded_line_threshold"] == "true";

	unsigned threaded_line_threshold = unsigned(parse_int(conf.data["VIDEO"]["threaded_line_threshold"])
		.value_or(1));

	bool enable_cached_interpreter = conf.data["INTERPRETER"]["enable_cached"] == "true";
	unsigned max_block_len = unsigned(parse_int(conf.data["INTERPRETER"]["max_block_len"])
		.value_or(100));
	unsigned region_size = unsigned(parse_int(conf.data["INTERPRETER"]["region_size"])
		.value_or(1024));
	bool enable_waitloop_detect = conf.data["INTERPRETER"]["waitloop_detection"] == "true";

	bool enable_achievements = conf.data["ACHIEVEMENTS"]["enable"] == "true";
	std::string credentials_file = conf.data["ACHIEVEMENTS"]["credentials_file"];

	bool enable_fake_prefetch = conf.data["ROM"]["fake_prefetch"] == "true";

	auto emu_init = [&](std::string rom) {
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
				fmt::print("[EMU] Loading save from {}\n", save_path);
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
		emu->SetFastmemEnable(enable_fastmem, precise_bios, precise_ppu);

		emu->SetRewindEnable(rewind_enable);
		emu->SetRewindBufferSize(GBA::common::u32(rewind_buf_size));

		emu->SetHleEnable(enable_hle);
		emu->SetLogHleEnable(log_hle);

		if (threaded_render) {
			emu->RunThreadedRender();
			emu->SetEnableScanlineDiffThreshold(threaded_line_threshold_enable);
			emu->SetScanlineDiffThreshold(GBA::common::u8(threaded_line_threshold));
		}

		if (enable_cached_interpreter) {
			emu->SetCachedInterpreterPageSize(region_size);
			emu->SetCachedInterpreterBlockSize(max_block_len);
			emu->EnableCachedInterpreter();

			if (enable_waitloop_detect) {
				emu->EnableWaitloopDetection();
			}
		}

		if (enable_achievements) {
			emu->EnableAchievements(credentials_file);
		}

		if (enable_fake_prefetch) {
			emu->EnableFakePrefetch(enable_fake_prefetch);
		}

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

	/////////////////////////////////////////////////////////////////

	GBA::video::renderer::OpenGL opengl_rend{paused, enable_hooks};

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

	opengl_rend.SetRomSelectedAction([&emu_init](std::string rom) {
		emu_init(rom);
	});

	opengl_rend.SetAudio(audio);
	opengl_rend.SetEmu(emu);

	opengl_rend.OnPauseChange(paused);

	audio->Start();

	auto& ctx = emu->GetContext();

	/////////////////////////////////////////////////////////////////////

	auto last_save_timestamp   = std::chrono::system_clock::now();
	auto last_idle_update      = last_save_timestamp;

	bool has_shown_notifications = false;

	while (!opengl_rend.Stopped())
	{
		SDL_Event ev{};

		while (SDL_PollEvent(&ev)) {
			opengl_rend.ProcessEvent(&ev);
		}

		if (!has_shown_notifications && has_rom) {
			has_shown_notifications = true;
			opengl_rend.SetupNotifications();
		}

		if (!opengl_rend.IsPaused() && has_rom) {
			emu->RunTillVblank();

			auto framebuffer = ctx.ppu.GetFrame();
			opengl_rend.SetFrame(framebuffer);
			

			if (emu->IsRewindEnabled()) {
				auto now = std::chrono::system_clock::now();
				auto diff = std::chrono::duration_cast<std::chrono::seconds>(
					now - last_save_timestamp
				).count();

				if (diff >= (long long)(rewind_interval)) {
					last_save_timestamp = now;
					emu->RewindPush();
				}
			}
		}
		else {
			auto now = std::chrono::system_clock::now();
			auto diff = std::chrono::duration_cast<std::chrono::seconds>(
				now - last_idle_update
			).count();

			if (diff >= 1) {
				last_idle_update = now;
				emu->RetroAchievementsClientIdle();
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