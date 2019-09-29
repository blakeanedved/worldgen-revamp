#include <iostream>
#include "Image.hpp"
#include "vendor/include/noise/noise.h"

#define WINDOW_SIZE 500

auto main() -> int {
	
	auto image = std::make_unique<Image>(WINDOW_SIZE, WINDOW_SIZE);
	image->InitSDL();

	auto perlin = noise::module::Perlin();
	auto sampler = std::make_shared<noise::module::Clamp>();
	sampler->SetSourceModule(0, perlin);

	image->scaleX = [](int x) -> double {
		return static_cast<double>(x) / 100.0;
	};

	image->scaleY = [](int y) -> double {
		return static_cast<double>(y) / 100.0;
	};

	image->OnRender = [&image](double dt) {
		dt*=2;
		image->noiseZ += (0.01);
	};

	image->SetSampler(sampler);
	image->StartRenderer();

	while (image->PollEvents()){
	}

	return 0;
}
