#include <iostream>
#include "NoiseLang.hpp"
#include "vendor/include/noise/noise.h"

#define WINDOW_SIZE 500

auto main() -> int {
	
	auto interpreter = std::make_unique<NoiseLang::Interpreter>();

	interpreter->StartReading();

	return 0;
}
