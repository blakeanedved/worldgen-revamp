#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <thread>
#include <functional>
#include <chrono>
#include "vendor/include/SDL2/SDL.h"
#include "vendor/include/noise/noise.h"


class ImageColor {
	public:
		Uint8 r, g, b, a;
		ImageColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a);
		ImageColor(int r, int g, int b, int a);
		ImageColor(double r, double g, double b, double a);
};

ImageColor::ImageColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a){
	this->r = r;
	this->g = g;
	this->b = b;
	this->a = a;
}

ImageColor::ImageColor(int r, int g, int b, int a) {
	this->r = static_cast<Uint8>(r);
	this->g = static_cast<Uint8>(g);
	this->b = static_cast<Uint8>(b);
	this->a = static_cast<Uint8>(a);
}

ImageColor::ImageColor(double r, double g, double b, double a) {
	this->r = static_cast<Uint8>(static_cast<int>(r));
	this->g = static_cast<Uint8>(static_cast<int>(g));
	this->b = static_cast<Uint8>(static_cast<int>(b));
	this->a = static_cast<Uint8>(static_cast<int>(a));
}

class Image {
	public:
		bool rendering;
		std::function<double(unsigned int)> scaleX;
		std::function<double(unsigned int)> scaleY;
		std::function<ImageColor(double)> color;
		float noiseX, noiseY, noiseZ;
		std::function<void(double)> OnRender;

	private:
		std::shared_ptr<noise::module::Module> noiseSampler;
		SDL_Window* window;
		SDL_Renderer* renderer;
		SDL_GLContext context;
		SDL_Event event;
		unsigned int width, height;
		float fps;
		std::thread thread;

		auto internal_render() -> void;

	public:
		Image(unsigned int width, unsigned int height);
		~Image();
		auto InitSDL() -> void;
		auto SetSampler(std::shared_ptr<noise::module::Module> noiseSampler) -> void;
		auto SetFPS(float fps) -> void;
		auto GetFPS() -> float;
		auto PollEvents() -> bool;
		auto StartRenderer() -> void;
		auto StopRenderer() -> void;
		auto Save(std::string filename) -> void;
};

Image::Image(unsigned int width, unsigned int height) {
	this->width = width;
	this->height = height;
	
	// Not currently rendering on an interval
	this->rendering = false;

	// Default FPS
	this->fps = 60.0;

	// Explicitly initialize window and renderer to nullptr's
	this->window = nullptr;
	this->renderer = nullptr;

	// Initialize the scale functions to 1 to 1
	this->scaleX = [](unsigned int x) -> double { return static_cast<double>(x); };
	this->scaleY = [](unsigned int y) -> double { return static_cast<double>(y); };
	
	// Initialize color function to output black -> white
	this->color = [](double noiseval) -> ImageColor { return {
		255.0 * (1.0 + noiseval) / 2.0,
		255.0 * (1.0 + noiseval) / 2.0,
		255.0 * (1.0 + noiseval) / 2.0,
		255.0
	};};

	// Initialize the OnRender function (dt*=2 is just to get the warning to go away)
	this->OnRender = [](double dt){dt*=2;};

	// Initialize the internal noise position offset to 0 for all
	this->noiseX = 0.0;
	this->noiseY = 0.0;
	this->noiseZ = 0.0;
}

Image::~Image() {
	// Dispose of rendering thread
	this->StopRenderer();
	
	// Destroy the renderer if it exists
	if (this->renderer != nullptr){
		SDL_DestroyRenderer(this->renderer);
	}

	// Destroy the window if it exists
	if (this->window != nullptr){
		SDL_DestroyWindow(this->window);
		SDL_Quit();
	}
}

auto Image::InitSDL() -> void {
	// Initialize the window and events
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
	this->window = SDL_CreateWindow("libnoise", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, this->width, this->height, SDL_WINDOW_RESIZABLE);

	// Create the opengl context and set tell it not to make this thread the current owner
	this->context = SDL_GL_GetCurrentContext();
	SDL_GL_MakeCurrent(this->window, nullptr);
}

auto Image::SetSampler(std::shared_ptr<noise::module::Module> noiseSampler) -> void {
	this->noiseSampler = noiseSampler;
}

auto Image::SetFPS(float fps) -> void {
	this->fps = fps;
}

auto Image::GetFPS() -> float {
	return this->fps;
}

auto Image::PollEvents() -> bool {
	// https://wiki.libsdl.org/SDL_WindowEvent

	SDL_PollEvent(&this->event);

	switch (this->event.type){

		case SDL_QUIT:
			this->StopRenderer();
			return false;

		case SDL_WINDOWEVENT:
			if (this->event.window.event == SDL_WINDOWEVENT_RESIZED || this->event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED){
				this->width = event.window.data1;
				this->height = event.window.data2;
			}
			break;

	}
	return true;
}

auto Image::internal_render() -> void {
	// Make this thread the owner of the opengl rendering context
	SDL_GL_MakeCurrent(this->window, this->context);
	this->renderer = SDL_CreateRenderer(this->window, -1, SDL_RENDERER_SOFTWARE);
	
	std::chrono::high_resolution_clock timer;
	while (this->rendering){
		auto start = timer.now();

		for (unsigned int x = 0; x < this->width; x++){
			for (unsigned int y = 0; y < this->height; y++){
				auto c = this->color(this->noiseSampler->GetValue(this->noiseX + this->scaleX(x), this->noiseY + this->scaleY(y), this->noiseZ));
				
				SDL_SetRenderDrawColor(this->renderer, c.r, c.g, c.b, c.a);
				SDL_RenderDrawPoint(this->renderer, x, y);
			}
		}

		// Force the renderer to show its changes in the window
		SDL_RenderPresent(this->renderer);

		// Force the thread to wait up to the maximum length of a frame
		//SDL_Delay(1000.0 / this->fps);
		auto stop = timer.now();
		double dt = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() / 1000.0;
		this->OnRender(dt);
	}
}

auto Image::StartRenderer() -> void {
	if (this->window != nullptr){
		if (this->noiseSampler != nullptr){
			// create rendering thread
			this->rendering = true;
			this->thread = std::thread(&Image::internal_render, this);
		} else {
			std::cout << "Internal noiseSampler has not been initialized, please call Image::SetSampler() with a std::shared_ptr<noise::module::Module> before calling Image::StartRenderer()" << std::endl;
		}
	} else {
		std::cout << "SDL has not been initialized, please initialize SDL before calling Image::StartRenderer()" << std::endl;
	}
}

auto Image::StopRenderer() -> void {
	if (this->rendering){
		// stop the thread and wait for it to join
		this->rendering = false;
		this->thread.join();
	}
}

auto Image::Save(std::string filename) -> void {
	std::cout << "Writing " << filename << std::endl;
}
