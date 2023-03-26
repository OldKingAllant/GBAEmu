#include <iostream>

#include "cpu/core/ARM7TDI.hpp"
#include "gamepack/GamePack.hpp"
#include "memory/Bus.hpp"
#include "cpu/arm/ARM_Implementation.h"

#include <imgui.h>
#include <backends>
#include <SDL2/SDL.h>

int main(int argc, char* argv[]) {
	std::string rom = "./testRoms/arm/arm.gba";

	GBA::gamepack::GamePack gp;
	GBA::memory::Bus bus;

	gp.LoadFrom(rom);

	bool res = GBA::gamepack::VerifyHeader(&gp.GetHeader());

	bus.ConnectGamepack(&gp);

	GBA::cpu::ARM7TDI arm(&bus);

	arm.SkipBios();

	arm.Step();
	arm.Step();
	arm.Step();

	auto ctx = arm.GetContext();

	GBA::cpu::arm::ARMInstruction instr{ 0 };
	//GBA::cpu::arm::ARMBlockTransfer transfer(instr);

	instr.data = 0xe92d0000;

	bool branch = false;

	GBA::cpu::arm::ExecuteArm(instr, ctx, &bus, branch);

	if (!SDL_Init(SDL_INIT_VIDEO)) {
		std::exit(0);
	}

	SDL_Window* window = SDL_CreateWindow("GBA Emulator",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 600, 600,
		SDL_WINDOW_SHOWN
	);

	SDL_Renderer* rend = SDL_CreateRenderer(window, -1,
		SDL_RENDERER_ACCELERATED);


	ImGui::CreateContext();
	ImGui::

	std::cin.get();
}