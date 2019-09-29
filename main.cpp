#include <iostream>
#include "Image.hpp"
#include "vendor/include/noise/noise.h"

#define WINDOW_SIZE 500

auto main() -> int {
	
	auto image = std::make_unique<Image>(WINDOW_SIZE, WINDOW_SIZE);
	image->InitSDL();

	//auto perlin = noise::module::Voronoi();
	auto sampler = std::make_shared<noise::module::Voronoi>();
	//sampler->SetSourceModule(0, perlin);

	image->scaleX = [](int x) -> double {
		return static_cast<double>(x) / 100.0;
	};

	image->scaleY = [](int y) -> double {
		return static_cast<double>(y) / 100.0;
	};

	image->SetSampler(sampler);
	image->StartRenderer();

	while (image->PollEvents()){
		image->noiseZ += 0.000003;
	}

	return 0;
}
