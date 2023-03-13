#include <iostream>

#include "cpu/core/ARM7TDI.hpp"

int main(int argc, char* argv[]) {
	GBA::memory::Bus bus;
	GBA::cpu::ARM7TDI proc(&bus);

	proc.EnterException(GBA::cpu::ExceptionCode::UNDEF);
	proc.RestorePreviousMode(0x0);

	std::cin.get();
}