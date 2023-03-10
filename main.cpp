#include <iostream>

#include "./cpu/core/Register.hpp"
#include "./cpu/core/CPSR.h"

int main(int argc, char* argv[]) {
	std::cout << sizeof(GBA::cpu::CPSR);

	std::cin.get();
}