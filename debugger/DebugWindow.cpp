#include "DebugWindow.hpp"

#include "../ImGui/imgui.h"
#include "../ImGui/backends/imgui_impl_sdl2.h"
#include "../ImGui/backends/imgui_impl_sdlrenderer.h"

#include "../cpu/core/CPUContext.hpp"
#include "../cpu/core/ARM7TDI.hpp"

#include "../common/ImGuiColors.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <string_view>
#include <iomanip>
#include <sstream>

#include "../common/DebugCommon.hpp"
#include "DisassembleARM.hpp"
#include "DisassembleTHUMB.hpp"
#include "Debugger.hpp"

namespace GBA::debugger {
	DebugWindow::DebugWindow(Debugger& debug) : 
		m_debugger(debug),
		m_processor(nullptr), 
		m_bus(nullptr),
		m_pack(nullptr),
		m_running(false),
		m_stop_request(false),
		m_sdl_window(nullptr), m_sdl_renderer(nullptr),
		m_disassembler_address{}, m_follow_pc(true),
		m_up_arrow(nullptr), m_down_arrow(nullptr),
		m_emu_status{}, m_memory_view_addresses{},
		m_break_address{}
	{
		/*
		*	{ 0x00000000, 0x00003FFF },
			{ 0x02000000, 0x0203FFFF },
			{ 0x03000000, 0x03007FFF }, 
			{ 0x05000000, 0x050003FF }, 
			{ 0x06000000, 0x06017FFF },
			{ 0x07000000, 0x070003FF },
			{ 0x08000000, 0x09FFFFFF },
			{ 0x0E000000, 0x0E00FFFF }
		*/
		m_memory_view_addresses[0] = 0x00000000;
		m_memory_view_addresses[1] = 0x02000000;
		m_memory_view_addresses[2] = 0x03000000;
		m_memory_view_addresses[3] = 0x05000000;
		m_memory_view_addresses[4] = 0x06000000;
		m_memory_view_addresses[5] = 0x07000000;
		m_memory_view_addresses[6] = 0x08000000;
		m_memory_view_addresses[7] = 0x0E000000;

		m_emu_status.stopped = true;
		m_emu_status.update_ui = true;
	}

	void DebugWindow::Init() {
		m_sdl_window = SDL_CreateWindow("GBA Emulator",
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1200, 800,
			SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI
		);

		if (!m_sdl_window)
			return;

		m_sdl_renderer = SDL_CreateRenderer(m_sdl_window, -1,
			SDL_RENDERER_ACCELERATED);

		if (!m_sdl_renderer)
			return;

		ImGui::CreateContext();
		ImGui::StyleColorsDark();

		ImGui_ImplSDL2_InitForSDLRenderer(m_sdl_window, m_sdl_renderer);
		ImGui_ImplSDLRenderer_Init(m_sdl_renderer);

		ImGuiIO& io = ImGui::GetIO();

		io.Fonts->Flags |= ImFontAtlasFlags_NoBakedLines;

		m_up_arrow = IMG_LoadTexture(m_sdl_renderer, "./up_arrow.png");
		m_down_arrow = IMG_LoadTexture(m_sdl_renderer, "./down_arrow.png");

		m_running = true;
	}

	void DebugWindow::Update() {
		ImGui_ImplSDLRenderer_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		DrawCpuControlWindow();
		DrawDisassemblerWindow();
		DrawControlWindow();
		DrawMemoryWindow();

		ImGui::Render();

		SDL_SetRenderDrawColor(m_sdl_renderer, 255, 255, 255, 0);
		SDL_RenderClear(m_sdl_renderer);

		ImDrawData* data = ImGui::GetDrawData();

		ImGui_ImplSDLRenderer_RenderDrawData(data);

		SDL_RenderPresent(m_sdl_renderer);

		SDL_Delay(1);
	}

	void DebugWindow::DrawCPSR(cpu::CPSR& cpsr) {
		float old_font_size = ImGui::GetFont()->Scale;
		ImGui::GetFont()->Scale *= 2;
		ImGui::PushFont(ImGui::GetFont());

		auto show_row = [](std::string_view str1,
			std::string_view val1, std::string_view str2 = "",
			std::string_view val2 = "") {
			ImGui::TableNextRow();
			ImGui::TableNextColumn();

			ImGui::TextColored(gui_colors::YELLOW, str1.data());
			ImGui::SameLine();
			ImGui::Text(val1.data());

			ImGui::TableNextColumn();

			if (str2.empty() || val2.empty())
				return;

			ImGui::TextColored(gui_colors::YELLOW, str2.data());
			ImGui::SameLine();
			ImGui::Text(val2.data());
		};

		if (!ImGui::BeginTable("CPSR", 2, ImGuiTableFlags_Borders))
			return;

		show_row("Mode : ", cpu::GetStringFromMode( cpsr.mode ), "State : ", cpu::GetStringFromState(cpsr.instr_state));
		show_row("FIQ Disable : ", BoolToString(cpsr.fiq_disable), "IRQ Disable : ", BoolToString(cpsr.irq_disable));
		show_row("Sticky OV : ", BoolToString(cpsr.sticky_ov), "Overflow : ", BoolToString(cpsr.overflow));
		show_row("Carry : ", BoolToString(cpsr.carry), "Zero : ", BoolToString(cpsr.zero));
		show_row("Sign : ", BoolToString(cpsr.sign));

		ImGui::GetFont()->Scale = old_font_size;
		ImGui::PopFont();

		ImGui::EndTable();
	}

	void DebugWindow::DrawSPSR() {
		cpu::CPUContext& ctx = m_processor->GetContext();

		constexpr const char* modes[] = {
			"FIQ", "IRQ", "SWI", 
			"ABRT", "UND"
		};

		ImGui::BeginTabBar("#SPSR");

		u8 i = 0;

		for (const char* mode_name : modes) {
			if (ImGui::BeginTabItem(mode_name)) {
				DrawCPSR(ctx.m_spsr[i]);
				ImGui::EndTabItem();
			}

			i++;
		}

		ImGui::EndTabBar();
	}

	void DebugWindow::DrawRegisters() {
		ImGui::BeginTabBar("#CPURegs");

		cpu::CPUContext& ctx = m_processor->GetContext();

		auto show_row = [](u32 reg_id1,
			u32 val1, u32 reg_id2,
			u32 val2) {
				ImGui::TableNextRow();
				ImGui::TableNextColumn();

				ImGui::TextColored(gui_colors::YELLOW, "r%d : ", reg_id1);
				ImGui::SameLine();
				ImGui::Text("0x%x", val1);

				ImGui::TableNextColumn();

				if (reg_id2 == 13)
					return;

				ImGui::TextColored(gui_colors::YELLOW, "r%d : ", reg_id2);
				ImGui::SameLine();
				ImGui::Text("0x%x", val2);
		};

		if (ImGui::BeginTabItem("Current")) {
			if (ImGui::BeginTable("Registers", 2, ImGuiTableFlags_Borders)) {
				for (u8 id = 0; id < 13; id += 2) {
					show_row(id, ctx.m_regs.GetReg(id), id + 1,
						ctx.m_regs.GetReg(id + 1));
				}

				ImGui::TextColored(gui_colors::YELLOW, "SP : ");
				ImGui::SameLine();
				ImGui::Text("0x%x", ctx.m_regs.GetReg(13));

				ImGui::TableNextRow();
				ImGui::TableNextColumn();

				ImGui::TextColored(gui_colors::YELLOW, "LR : ");
				ImGui::SameLine();
				ImGui::Text("0x%x", ctx.m_regs.GetReg(14));

				ImGui::TableNextColumn();

				ImGui::TextColored(gui_colors::YELLOW, "PC : ");
				ImGui::SameLine();
				ImGui::Text("0x%x", ctx.m_regs.GetReg(15));

				ImGui::EndTable();
			}
			
			ImGui::EndTabItem();
		}

		constexpr const char* modes[] = {
			"USR/SYS", "FIQ", "IRQ", "SWI",
			"ABRT", "UND"
		};

		u8 i = 0;

		auto draw_single = [](u32 id, u32 value) {
			ImGui::Spacing();
			ImGui::TextColored(gui_colors::YELLOW, "r%d : ", id);
			ImGui::SameLine();
			ImGui::Text("0x%x", value);
			ImGui::Separator();
			ImGui::Spacing();
		};

		for (const char* mode_name : modes) {
			if (ImGui::BeginTabItem(mode_name)) {
				float old_font_size = ImGui::GetFont()->Scale;
				ImGui::GetFont()->Scale *= 2;
				ImGui::PushFont(ImGui::GetFont());

				for (u8 id = 8 + cpu::RegisterManager::BANK_START[i]; id < 15; id++) {
					draw_single(id, ctx.m_regs.GetReg(cpu::GetIDFromMode(i), id));
				}

				ImGui::GetFont()->Scale = old_font_size;
				ImGui::PopFont();

				ImGui::EndTabItem();
			}

			i++;
		}

		ImGui::EndTabBar();
	}

	void DebugWindow::DrawCpuControlWindow() {
		ImGui::Begin("CPU Control Window");

		using namespace cpu;

		cpu::CPUContext& ctx = m_processor->GetContext();
		
		ImGui::BeginTabBar("#CPUTabs");

		if (ImGui::BeginTabItem("Registers")) {
			DrawRegisters();
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("CPSR")) {
			CPSR& cpsr = ctx.m_cpsr;

			DrawCPSR(cpsr);

			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("SPSR")) {
			DrawSPSR();
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Pipeline")) {
			float old_font_size = ImGui::GetFont()->Scale;
			ImGui::GetFont()->Scale *= 2;
			ImGui::PushFont(ImGui::GetFont());

			auto show_pipeline_var = [](std::string_view str, u32 value) {
				ImGui::Dummy(ImVec2(0.0f, 20.0f));
				ImGui::TextColored(gui_colors::YELLOW, str.data());
				ImGui::SameLine();
				ImGui::Text("0x%x", value);
				ImGui::Dummy(ImVec2(0.0f, 20.0f));
				ImGui::Separator();
			};

			show_pipeline_var("Fetch IP : ", ctx.m_pipeline.GetFetchPC());
			show_pipeline_var("Fetched : ", ctx.m_pipeline.GetFetched());
			show_pipeline_var("Decoded : ", ctx.m_pipeline.GetDecoded());

			ImGui::EndTabItem();

			ImGui::GetFont()->Scale = old_font_size;
			ImGui::PopFont();
		}

		ImGui::EndTabBar();

		
		ImGui::End();
	}

	template <typename T>
	std::string to_hex(T const& value) {
		std::ostringstream stream = {};

		stream << "0x"
			<< std::setfill('0') 
			<< std::setw(sizeof(T) * 2) << std::hex
			<< value;

		return stream.str();
	}

	void DebugWindow::DrawDisassemblerWindow() {
		ImGui::Begin("Disassembly");

		cpu::CPUContext& ctx = m_processor->GetContext();

		ImGui::InputScalar("Address", ImGuiDataType_U32, &m_disassembler_address,
			nullptr, nullptr, "%x", ImGuiInputTextFlags_CharsHexadecimal);
		ImGui::Checkbox("Follow PC", &m_follow_pc);

		if (m_follow_pc)
			m_disassembler_address = ctx.m_regs.GetReg(15);

		u8 increment = 4;

		if (ctx.m_cpsr.instr_state == cpu::InstructionMode::THUMB)
			increment = 2;

		if (ctx.m_cpsr.instr_state == cpu::InstructionMode::ARM)
			m_disassembler_address &= ~3;
		else
			m_disassembler_address &= ~1;

		if (ImGui::ImageButton("#UP", (ImTextureID)m_up_arrow, ImVec2(20, 20))) {
			m_disassembler_address += increment;
		}

		ImGui::SameLine();

		if (ImGui::ImageButton("#DOWN", (ImTextureID)m_down_arrow, ImVec2(20, 20))) {
			m_disassembler_address -= increment;
		}

		ImGui::BeginChild("#Disassembly", ImVec2(0, 0), true);

		float available_height = ImGui::GetContentRegionAvail().y;
		float text_sz = ImGui::GetTextLineHeightWithSpacing();

		u32 current_address = m_disassembler_address;

		while (available_height - text_sz >= 0) {
			ImGui::TextColored(gui_colors::BLUE, to_hex(current_address).c_str());
			ImGui::SameLine();

			std::string text = " ";

			if (ctx.m_cpsr.instr_state == cpu::InstructionMode::ARM)
				text += DisassembleARM(m_bus->DebuggerRead32(current_address), ctx);
			else
				text += DisassembleTHUMB(m_bus->DebuggerRead16(current_address), ctx);

			if (current_address == ctx.m_regs.GetReg(15))
				ImGui::TextColored(gui_colors::YELLOW, text.c_str());
			else
				ImGui::Text(text.c_str());

			ImGui::Spacing();
			available_height -= text_sz;

			if (ctx.m_cpsr.instr_state == cpu::InstructionMode::ARM)
				current_address += 4;
			else
				current_address += 2;
		}

		ImGui::EndChild();

		ImGui::End();
	}

	void DebugWindow::DrawControlWindow() {
		ImGui::Begin("Control");

		if (ImGui::CollapsingHeader("Breakpoints")) {
			//ImGui::BeginChild("#Breakpoints");
			ImGui::Checkbox("Allow UI update", &m_emu_status.update_ui);

			ImGui::InputScalar("Address", ImGuiDataType_U32, &m_break_address, nullptr,
				nullptr, "%x", ImGuiInputTextFlags_CharsHexadecimal);

			ImGui::Spacing();

			if (ImGui::Button("Add", ImVec2(40, 30))) {
				m_debugger.AddBreakpoint(m_break_address);
			}

			for (Breakpoint& br : m_debugger.GetBreakpoints()) {
				float old_font_scale = ImGui::GetFont()->Scale;
				ImGui::TextColored(gui_colors::BLUE, "0x%x", br.address);

				ImGui::SameLine();
				ImGui::Checkbox("Enable", &br.enabled);
				ImGui::SameLine();
				
				if (ImGui::Button("Remove", ImVec2(60, 20))) {
					m_debugger.RemoveBreakpoint(br.address);
				}
			}

			//ImGui::EndChild();
		}

		ImGui::Spacing();
		ImGui::Spacing();

		if (ImGui::Button("Step", ImVec2(50, 30))) {
			//m_emu_status = {};
			m_emu_status.single_step = true;
			m_emu_status.stopped = false;
			m_emu_status.until_breakpoint = false;
		}

		ImGui::SameLine();
		ImGui::Spacing();
		ImGui::SameLine();

		if (ImGui::Button("Continue", ImVec2(80, 30))) {
			m_emu_status.single_step = false;
			m_emu_status.stopped = false;
			m_emu_status.until_breakpoint = true;
		}

		ImGui::SameLine();
		ImGui::Spacing();
		ImGui::SameLine();

		if (ImGui::Button("Stop", ImVec2(80, 30))) {
			m_emu_status.single_step = false;
			m_emu_status.stopped = true;
			m_emu_status.until_breakpoint = false;
		}

		std::string text_stopped = std::string("Stopped : ") + 
			std::string(BoolToString(m_emu_status.stopped));

		ImGui::TextColored(gui_colors::YELLOW, text_stopped.c_str());

		std::string run_till_break = std::string("Run until breakpoint : ") +
			std::string(BoolToString(m_emu_status.until_breakpoint));

		ImGui::TextColored(gui_colors::YELLOW, run_till_break.c_str());

		ImGui::End();
	}

	void DebugWindow::DrawMemoryRegion(unsigned id) {
		constexpr const unsigned ranges[][2] = {
			{ 0x00000000, 0x00003FFF },
			{ 0x02000000, 0x0203FFFF },
			{ 0x03000000, 0x03007FFF }, 
			{ 0x05000000, 0x050003FF }, 
			{ 0x06000000, 0x06017FFF },
			{ 0x07000000, 0x070003FF },
			{ 0x08000000, 0x09FFFFFF },
			{ 0x0E000000, 0x0E00FFFF }
		};

		ImGui::InputScalar("Goto address", ImGuiDataType_U32, &m_memory_view_addresses[id],
			nullptr, nullptr, "%x", ImGuiInputTextFlags_CharsHexadecimal);

		ImGui::Spacing();

		if (ImGui::ImageButton((void*)m_up_arrow, ImVec2(20, 20))) {
			m_memory_view_addresses[id] += 0x10;
		}

		ImGui::SameLine();

		if (ImGui::ImageButton((void*)m_down_arrow, ImVec2(20, 20))) {
			m_memory_view_addresses[id] -= 0x10;
		}

		if (m_memory_view_addresses[id] > ranges[id][1] ||
			m_memory_view_addresses[id] < ranges[id][0])
			m_memory_view_addresses[id] = ranges[id][0];
		
		float y_space = ImGui::GetContentRegionAvail().y;
		float text_sz_y = ImGui::GetTextLineHeightWithSpacing();

		ImGui::BeginChild("#View");

		u32 address = m_memory_view_addresses[id];
		address -= address % 0x10;

		std::ostringstream buf{};

		while (y_space > 0 && address < ranges[id][1]) {
			float x_space = ImGui::GetContentRegionAvail().x;

			std::string addr_str = to_hex(address);

			x_space -= ImGui::CalcTextSize(addr_str.c_str()).x;

			ImGui::TextColored(gui_colors::BLUE, addr_str.c_str());
			ImGui::SameLine();

			u8 shown_values = 0;

			while (x_space > 0 && shown_values < 16 && address < ranges[id][1]) {
				buf.str("");

				u16 value = m_bus->DebuggerRead16(address);

				if (address % 2)
					value = (value >> 8) & 0xFF;
				else
					value &= 0xFF;

				buf << std::setfill('0') 
					<< std::setw(2) 
					<< std::hex
					<< value;

				std::string text = buf.str();

				float text_sz = ImGui::CalcTextSize(text.c_str()).x;

				if (shown_values % 2) {
					ImGui::TextColored(gui_colors::BROWN, text.c_str());
				}
				else {
					ImGui::TextColored(gui_colors::WHITE, text.c_str());
				}
				
				ImGui::SameLine();

				x_space -= text_sz;

				++address;
				++shown_values;
			}

			ImGui::Spacing();

			y_space -= text_sz_y;
		}

		ImGui::EndChild();
	}

	void DebugWindow::DrawMemoryWindow() {
		ImGui::Begin("Memory visualizer");

		ImGui::BeginTabBar("#Regions");

		constexpr const char* mem_regions[] = {
			"BIOS", "WRAM", "IWRAM", "Palette",
			"VRAM", "OAM", "ROM", "SRAM"
		};

		unsigned id = 0;

		for (const char* region : mem_regions) {
			if (ImGui::BeginTabItem(region)) {
				DrawMemoryRegion(id);
				ImGui::EndTabItem();
			}

			id++;
		}

		ImGui::EndTabBar();

		ImGui::End();
	}

	DebugWindow::~DebugWindow() {
		Stop();
	}

	bool DebugWindow::IsRunning() {
		return m_running;
	}

	void DebugWindow::SetProcessor(cpu::ARM7TDI* processor) {
		m_processor = processor;
	}

	void DebugWindow::SetBus(memory::Bus* bus) {
		m_bus = bus;
	}

	void DebugWindow::SetGamePack(gamepack::GamePack* pack) {
		m_pack = pack;
	}

	void DebugWindow::Stop() {
		if (!m_running)
			return;

		ImGui_ImplSDLRenderer_Shutdown();
		ImGui_ImplSDL2_Shutdown();
		ImGui::DestroyContext();

		SDL_DestroyRenderer(m_sdl_renderer);
		SDL_DestroyWindow(m_sdl_window);

		m_running = false;
	}

	bool DebugWindow::StopRequested() {
		return m_stop_request;
	}

	bool DebugWindow::ConfirmEventTarget(SDL_Event* sdl_event) {
		return SDL_GetWindowFromID(sdl_event->window.windowID) == m_sdl_window;
	}

	void DebugWindow::ProcessEvent(SDL_Event* sdl_event) {
		ImGui_ImplSDL2_ProcessEvent(sdl_event);

		switch (sdl_event->type)
		{
		case SDL_WINDOWEVENT: {
			if (sdl_event->window.event == SDL_WINDOWEVENT_CLOSE)
				m_stop_request = true;
		}
		break;

		default:
			break;
		}
	}
}