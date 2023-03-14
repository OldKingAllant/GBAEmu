#include <iostream>

#include "cpu/core/ARM7TDI.hpp"
#include "gamepack/GamePack.hpp"

int main(int argc, char* argv[]) {
	std::string rom = "./testRoms/arm/arm.gba";

	GBA::gamepack::GamePack gp;

	gp.LoadFrom(rom);

	bool res = GBA::gamepack::VerifyHeader(&gp.GetHeader());

	std::cin.get();
}