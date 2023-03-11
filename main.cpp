#include <iostream>

#include "./cpu/core/Register.hpp"
#include "./cpu/core/CPSR.hpp"

int main(int argc, char* argv[]) {
	GBA::cpu::RegisterManager regs{};

	regs.SetReg(0, 1);
	regs.SetReg(8, 1);
	regs.SetReg(13, 1);
	regs.SetReg(14, 1);
	regs.SetReg(15, 10);

	regs.SwitchMode(GBA::cpu::Mode::FIQ);

	regs.SetReg(8, 2);

	regs.SwitchMode(GBA::cpu::Mode::SYS);
	regs.SwitchMode(GBA::cpu::Mode::SWI);
	regs.SwitchMode(GBA::cpu::Mode::ABRT);
	regs.SwitchMode(GBA::cpu::Mode::IRQ);
	regs.SwitchMode(GBA::cpu::Mode::UND);

	regs.SetReg(13, 3);
	regs.SetReg(8, 4);

	regs.SwitchMode(GBA::cpu::Mode::User);

	regs.SwitchMode(GBA::cpu::Mode::FIQ);

	std::cin.get();
}