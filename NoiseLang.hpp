#pragma once

#include <iostream>
#include <string>
#include <algorithm>
#include <functional>
#include <chrono>
#include <fstream>
#include <sstream>
#include <deque>
#include <map>
#include <regex>
#include <thread>
#include <vector>

#include "vendor/include/noise/noise.h"
#include "vendor/include/noise/noiseutils.h"
#include "vendor/include/SDL2/SDL.h"

namespace NoiseLang {

	int Ok = 0;
	int Error = 1;

	class ImageColor {
		public:
			Uint8 r, g, b, a;
			ImageColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a){
				this->r = r;
				this->g = g;
				this->b = b;
				this->a = a;
			}
			ImageColor(int r, int g, int b, int a){
				this->r = static_cast<Uint8>(r);
				this->g = static_cast<Uint8>(g);
				this->b = static_cast<Uint8>(b);
				this->a = static_cast<Uint8>(a);
			}
			ImageColor(double r, double g, double b, double a){
				this->r = static_cast<Uint8>(static_cast<int>(r));
				this->g = static_cast<Uint8>(static_cast<int>(g));
				this->b = static_cast<Uint8>(static_cast<int>(b));
				this->a = static_cast<Uint8>(static_cast<int>(a));
			}
	};
	
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
			SDL_Texture* texture;
			unsigned int width, height;
			float fps;
			std::thread thread;
			bool is_dead;

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
			auto IsDead() -> bool;
	};

	class Argument {
		public:
			std::string identifier;
			double number;

			Argument(std::string arg){
				std::regex number_re("-?\\d*\\.\\d+");
				std::regex identifier_re("([a-zA-z]{1}[a-zA-z0-9]*)");

				if (std::regex_match(arg, number_re)){
					std::stringstream ss(arg);
					ss >> this->number;
					this->identifier = "";
				} else if (std::regex_match(arg, identifier_re)){
					this->number = 0;
					this->identifier = arg;
				}
			}

			auto GetType() -> std::string {

				return this->identifier == "" ? "number" : "identifier";

			}
	};

	class Assignment {
		public:
			std::string identifier;
			std::string module;
			std::vector<std::string> arguments;
	};

	class Method {
		public:
			std::string identifier;
			std::string method;
			std::vector<NoiseLang::Argument> arguments;

			auto Print() -> void {
				std::cout << "(IDENTIFIER: \"" << this->identifier << "\") -> (METHOD: \"" << this->method << "\") (ARGUMENTS: [";
				for (auto& arg : this->arguments){
					if (arg.GetType() == "number")
						std::cout << arg.number << ", ";
					else
						std::cout << "\"" << arg.identifier << "\", ";
				}
				std::cout << "]);" << std::endl;
			}
	};

	class Out {
		public:
			std::string identifier;
	};

	class Save {
		public:
			std::string filename;
	};

	class Load {
		public:
			std::string filename;
	};

	class Show {
		public:
			int width;
			int height;
	};

	class Interpreter {
		
		private:
			std::map<std::string, std::pair<std::string, std::shared_ptr<noise::module::Module>>> modules;
			std::string output_module = "";
			std::deque<std::string> errors;
			std::regex assignment;
			std::regex method;
			std::regex out;
			std::regex save;
			std::regex load;
			std::regex show;
			std::regex exit;

			std::regex assignment_iter;
			std::regex method_iter;
			std::regex out_iter;
			std::regex save_load_iter;
			std::regex show_iter;

			std::vector<std::string> lines;

			int reading_status;
			std::shared_ptr<std::thread> reading_thread = nullptr;

			std::unique_ptr<NoiseLang::Image> image = nullptr;

			std::map<std::string, std::vector<std::pair<std::string, std::vector<std::string>>>> methods = {
				{"abs", {{"SetSourceModule", {"number", "identifier"}}}},
				{"add", {{"SetSourceModule", {"number", "identifier"}}}},
				{"billow", {{"SetFrequency", {"number"}}, {"SetLacunarity", {"number"}}, {"SetNoiseQuality", {"number"}}, {"SetOctaveCount", {"number"}}, {"SetPersistence", {"number"}}, {"SetSeed", {"number"}}}},
				{"blend", {{"SetSourceModule", {"number", "identifier"}}, {"SetControlModule", {"identifier"}}}},
				{"cache", {{"SetSourceModule", {"number", "identifier"}}}},
				{"checkerboard", {}},
				{"clamp", {{"SetSourceModule", {"number", "identifier"}}, {"SetBounds", {"number", "number"}}}},
				{"const", {{"SetConstValue", {"number"}}}},
				{"curve", {{"SetSourceModule", {"number", "identifier"}}, {"AddControlPoint", {"number", "number"}}, {"ClearAllControlPoints", {}}}},
				{"cylinders", {{"SetFrequency", {"number"}}}},
				{"displace", {{"SetSourceModule", {"number", "identifier"}}, {"SetDisplaceModules", {"identifier", "identifier", "identifier"}}, {"SetXDisplaceModule", {"identifier"}}, {"SetYDisplaceModule", {"identifier"}}, {"SetZDisplaceModule", {"identifier"}}}},
				{"exponent", {{"SetSourceModule", {"number", "identifier"}}, {"SetExponent", {"number"}}}},
				{"invert", {{"SetSourceModule", {"number", "identifier"}}}},
				{"max", {{"SetSourceModule", {"number", "identifier"}}}},
				{"min", {{"SetSourceModule", {"number", "identifier"}}}},
				{"multiply", {{"SetSourceModule", {"number", "identifier"}}}},
				{"perlin", {{"SetFrequency", {"number"}}, {"SetLacunarity", {"number"}}, {"SetNoiseQuality", {"number"}}, {"SetOctaveCount", {"number"}}, {"SetPersistence", {"number"}}, {"SetSeed", {"number"}}}},
				{"power", {{"SetSourceModule", {"number", "identifier"}}}},
				{"ridgedmulti", {{"SetFrequency", {"number"}}, {"SetLacunarity", {"number"}}, {"SetNoiseQuality", {"number"}}, {"SetOctaveCount", {"number"}}, {"SetSeed", {"number"}}}},
				{"rotatepoint", {{"SetSourceModule", {"number", "identifier"}}, {"SetAngles", {"number", "number", "number"}}, {"SetXAngle", {"number"}}, {"SetYAngle", {"number"}}, {"SetZAngle", {"number"}}}},
				{"scalebias", {{"SetSourceModule", {"number", "identifier"}}, {"SetBias", {"number"}}, {"SetScale", {"number"}}}},
				{"scalepoint", {{"SetSourceModule", {"number", "identifier"}}, {"SetScale", {"number", "number", "number"}}, {"SetXScale", {"number"}}, {"SetYScale", {"number"}}, {"SetZScale", {"number"}}}},
				{"select", {{"SetSourceModule", {"number", "identifier"}}, {"SetControlModule", {"identifier"}}, {"SetBounds", {"number", "number"}}, {"SetEdgeFalloff", {"number"}}}},
				{"spheres", {{"SetFrequency", {"number"}}}},
				{"terrace", {{"SetSourceModule", {"number", "identifier"}}, {"InvertTerraces", {}}, {"MakeControlPoints", {"number"}}, {"AddControlPoint", {"number"}}, {"ClearAllControlPoints", {}}}},
				{"translatepoint", {{"SetSourceModule", {"number", "identifier"}}, {"SetTranslation", {"number", "number", "number"}}, {"SetXTranslation", {"number"}}, {"SetYTranslation", {"number"}}, {"SetZTranslation", {"number"}}}},
				{"turbulence", {{"SetSourceModule", {"number", "identifier"}}, {"SetFrequency", {"number"}}, {"SetPower", {"number"}}, {"SetRoughness", {"number"}}, {"SetSeed", {"number"}}}},
				{"voronoi", {{"EnableDistance", {}}, {"DisableDistance", {}}, {"SetFrequency", {"number"}}, {"SetDisplacement", {"number"}}, {"SetSeed", {"number"}}}},
			};

		public:
			Interpreter();
			~Interpreter();

			auto RunLine(const std::string& line, bool saveline = true) -> int;

			auto Run(const std::string& filename, bool savelines = true) -> void;
			auto Reset() -> void;

			auto GetOutModule() -> std::shared_ptr<noise::module::Module>;

			auto GetError() -> std::string;

			auto GetDefaultOutModule() -> std::shared_ptr<noise::module::Module>;

			auto StartReading() -> void;
			auto StopReading() -> void;

		private:
			auto AddError(std::string errorMessage) -> void;

			auto CheckIdentifierArgs(std::vector<std::string> args) -> bool;

			auto ParseAssignment(const std::string& line) -> NoiseLang::Assignment;
			auto ParseMethod(const std::string& line) -> NoiseLang::Method;
			auto ParseOut(const std::string& line) -> NoiseLang::Out;
			auto ParseSave(const std::string& line) -> NoiseLang::Save;
			auto ParseLoad(const std::string& line) -> NoiseLang::Load;
			auto ParseShow(const std::string& show) -> NoiseLang::Show;

			auto InternalRead() -> void;
			auto InternalThreadedRead() -> void;

	};

}

NoiseLang::Interpreter::Interpreter() {
	this->assignment = std::regex("^([a-zA-z]{1}[a-zA-z0-9]*)([ \t]*)(=)([ \t]*)(abs|add|billow|blend|cache|checkerboard|clamp|const|curve|cylinders|displace|exponent|invert|max|min|multiply|perlin|power|ridgedmulti|rotatepoint|scalebias|scalepoint|select|spheres|terrace|translatepoint|turbulence|voronoi)(\\()(([a-zA-z]{1}[a-zA-z0-9]*([ \t]*),([ \t]*))*([a-zA-z]{1}[a-zA-z0-9]*([ \t]*)){1})?(\\))$");
	this->method = std::regex("^([a-zA-Z]{1}[a-zA-Z0-9]*)(->)([A-Z]{1}[a-zA-z]*)(\\()((((-?\\d*\\.?\\d+)|([a-zA-Z]{1}[a-zA-Z0-9]*))([ \t]*),([ \t]*))*((-?\\d*\\.?\\d+)|([a-zA-Z]{1}[a-zA-Z0-9]*)){1})(\\))$");
	this->out = std::regex("^(out)([ \t]+)([a-zA-Z]{1}[a-zA-Z0-9]*)$");
	this->save = std::regex("^(save)([ \t]+)([a-zA-z0-9]*\\.?[a-zA-z0-9]+)$");
	this->load = std::regex("^(load)([ \t]+)([a-zA-z0-9]*\\.?[a-zA-z0-9]+)$");
	this->show = std::regex("^(show)([ \t]+)(\\d{1,4})x(\\d{1,4})$");
	this->exit = std::regex("^(exit)([ \t]*)$");

	this->assignment_iter = std::regex("([a-zA-z]{1}[a-zA-z0-9]*)|(abs|add|billow|blend|cache|checkerboard|clamp|const|curve|cylinders|displace|exponent|invert|max|min|multiply|perlin|power|ridgedmulti|rotatepoint|scalebias|scalepoint|select|spheres|terrace|translatepoint|turbulence|voronoi)");
	this->method_iter = std::regex("([a-zA-Z]{1}[a-zA-Z0-9]*)|([A-Z]{1}[a-zA-z]*)|(-?\\d*\\.?\\d+)");
	this->out_iter = std::regex("([a-zA-Z]{1}[a-zA-Z0-9]*)$");
	this->save_load_iter = std::regex("([a-zA-Z0-9]*\\.[a-zA-Z0-9]+)$");
	this->show_iter = std::regex("(\\d{1,4})");

	this->output_module = "";
}

NoiseLang::Interpreter::~Interpreter() {
	this->modules.clear();
}

auto NoiseLang::Interpreter::AddError(std::string errorMessage) -> void {
	this->errors.push_front(std::move(errorMessage));
}

auto NoiseLang::Interpreter::GetError() -> std::string {
	if (this->errors.size() > 0){
		std::string error = this->errors.front();
		this->errors.pop_front();
		return error;
	} else {
		return "";
	}
}

auto NoiseLang::Interpreter::ParseAssignment(const std::string& line) -> NoiseLang::Assignment {
	auto a = NoiseLang::Assignment();

	auto begin = std::sregex_iterator(line.begin(), line.end(), this->assignment_iter);
	auto end = std::sregex_iterator();
	
	int token = 0;

	for (;begin != end; ++begin){
		std::smatch match = *begin;
		std::string s = match.str();

		switch (token){
			case 0: // Identifier
				a.identifier = s;
				break;
			case 1: // Module
				a.module = s;
				break;
			default: // Anything after Module (arguments)
				a.arguments.push_back(s);
				break;
		}

		token++;
	}

	return a;
}

auto NoiseLang::Interpreter::ParseMethod(const std::string& line) -> NoiseLang::Method {
	auto m = NoiseLang::Method();

	auto begin = std::sregex_iterator(line.begin(), line.end(), this->method_iter);
	auto end = std::sregex_iterator();
	
	int token = 0;

	for (;begin != end; ++begin){
		std::smatch match = *begin;
		std::string s = match.str();

		switch (token){
			case 0: // Identifier
				m.identifier = s;
				break;
			case 1: // Module
				m.method = s;
				break;
			default: // Anything after Module (arguments)
				m.arguments.push_back({s});
				break;
		}

		token++;
	}

	return m;
}

auto NoiseLang::Interpreter::ParseOut(const std::string& line) -> NoiseLang::Out {
	auto o = NoiseLang::Out();

	auto begin = std::sregex_iterator(line.begin(), line.end(), this->out_iter);
	auto end = std::sregex_iterator();
	
	int token = 0;

	for (;begin != end; ++begin){
		std::smatch match = *begin;
		std::string s = match.str();

		switch (token){
			case 0: // Identifier
				o.identifier = s;
				break;
		}

		token++;
	}

	return o;
}

auto NoiseLang::Interpreter::ParseSave(const std::string& line) -> NoiseLang::Save {
	auto sa = NoiseLang::Save();

	auto begin = std::sregex_iterator(line.begin(), line.end(), this->save_load_iter);
	auto end = std::sregex_iterator();
	
	int token = 0;

	for (;begin != end; ++begin){
		std::smatch match = *begin;
		std::string s = match.str();

		switch (token){
			case 0: // Filename
				sa.filename = s;
				break;
		}

		token++;
	}

	return sa;
}

auto NoiseLang::Interpreter::ParseLoad(const std::string& line) -> NoiseLang::Load {
	auto l = NoiseLang::Load();

	auto begin = std::sregex_iterator(line.begin(), line.end(), this->save_load_iter);
	auto end = std::sregex_iterator();
	
	int token = 0;

	for (;begin != end; ++begin){
		std::smatch match = *begin;
		std::string s = match.str();

		switch (token){
			case 0: // Filename
				l.filename = s;
				break;
		}

		token++;
	}

	return l;
}

auto NoiseLang::Interpreter::ParseShow(const std::string& line) -> NoiseLang::Show {
	auto s = NoiseLang::Show();

	auto begin = std::sregex_iterator(line.begin(), line.end(), this->show_iter);
	auto end = std::sregex_iterator();
	
	int token = 0;

	for (;begin != end; ++begin){
		std::smatch match = *begin;
		std::string str = match.str();

		std::stringstream ss(str);

		switch (token){
			case 0: // Width
				ss >> s.width;
				break;
			case 1: // Height
				ss >> s.height;
				break;
		}

		token++;
	}

	return s;
}

auto NoiseLang::Interpreter::CheckIdentifierArgs(std::vector<std::string> args) -> bool {
	
	for (unsigned int i = 0; i < args.size(); i++){
		auto it = this->modules.find(args[i]);
		if (it == this->modules.end())
			return false;
	}

	return true;
}

auto NoiseLang::Interpreter::RunLine(const std::string& line, bool saveline) -> int {

	// Grammars found in NoiseLangGrammar.txt

	if (saveline)
		this->lines.push_back(line);

	int status = NoiseLang::Ok;

	if (std::regex_match(line, this->assignment)){

		// Line is a <assignment> grammar
		auto a = this->ParseAssignment(line);

		if (auto it = this->modules.find(a.identifier); it == this->modules.end()){

			// {{{ Module checking
			if (a.module == "abs" && a.arguments.size() == 1 && this->CheckIdentifierArgs(a.arguments)){
				this->modules.insert(std::make_pair(a.identifier, std::make_pair("abs", std::make_shared<noise::module::Abs>())));
				this->modules[a.identifier].second->SetSourceModule(0, *this->modules[a.arguments[0]].second);
			} else if (a.module == "add" && a.arguments.size() == 2 && this->CheckIdentifierArgs(a.arguments)){
				this->modules.insert(std::make_pair(a.identifier, std::make_pair("add", std::make_shared<noise::module::Add>())));
				this->modules[a.identifier].second->SetSourceModule(0, *this->modules[a.arguments[0]].second);
				this->modules[a.identifier].second->SetSourceModule(1, *this->modules[a.arguments[1]].second);
			} else if (a.module == "billow" && a.arguments.size() == 0 && this->CheckIdentifierArgs(a.arguments)){
				this->modules.insert(std::make_pair(a.identifier, std::make_pair("billow", std::make_shared<noise::module::Billow>())));
			} else if (a.module == "blend" && a.arguments.size() == 3 /* maybe 2 */ && this->CheckIdentifierArgs(a.arguments)){
				this->modules.insert(std::make_pair(a.identifier, std::make_pair("blend", std::make_shared<noise::module::Blend>())));
				this->modules[a.identifier].second->SetSourceModule(0, *this->modules[a.arguments[0]].second);
				this->modules[a.identifier].second->SetSourceModule(1, *this->modules[a.arguments[1]].second);
				this->modules[a.identifier].second->SetSourceModule(2, *this->modules[a.arguments[2]].second);
			} else if (a.module == "cache" && a.arguments.size() == 1 && this->CheckIdentifierArgs(a.arguments)){
				this->modules.insert(std::make_pair(a.identifier, std::make_pair("cache", std::make_shared<noise::module::Cache>())));
				this->modules[a.identifier].second->SetSourceModule(0, *this->modules[a.arguments[0]].second);
			} else if (a.module == "checkerboard" && a.arguments.size() == 0 && this->CheckIdentifierArgs(a.arguments)){
				this->modules.insert(std::make_pair(a.identifier, std::make_pair("checkerboard", std::make_shared<noise::module::Checkerboard>())));
			} else if (a.module == "clamp" && a.arguments.size() == 1 && this->CheckIdentifierArgs(a.arguments)){
				this->modules.insert(std::make_pair(a.identifier, std::make_pair("clamp", std::make_shared<noise::module::Clamp>())));
				this->modules[a.identifier].second->SetSourceModule(0, *this->modules[a.arguments[0]].second);
			} else if (a.module == "const" && a.arguments.size() == 0 && this->CheckIdentifierArgs(a.arguments)){
				this->modules.insert(std::make_pair(a.identifier, std::make_pair("const", std::make_shared<noise::module::Const>())));
			} else if (a.module == "curve" && a.arguments.size() == 1 && this->CheckIdentifierArgs(a.arguments)){
				this->modules.insert(std::make_pair(a.identifier, std::make_pair("curve", std::make_shared<noise::module::Curve>())));
				this->modules[a.identifier].second->SetSourceModule(0, *this->modules[a.arguments[0]].second);
			} else if (a.module == "cylinders" && a.arguments.size() == 0 && this->CheckIdentifierArgs(a.arguments)){
				this->modules.insert(std::make_pair(a.identifier, std::make_pair("cylinders", std::make_shared<noise::module::Cylinders>())));
			} else if (a.module == "displace" && a.arguments.size() == 4 /* maybe 1 */ && this->CheckIdentifierArgs(a.arguments)){
				this->modules.insert(std::make_pair(a.identifier, std::make_pair("displace", std::make_shared<noise::module::Displace>())));
				this->modules[a.identifier].second->SetSourceModule(0, *this->modules[a.arguments[0]].second);
				this->modules[a.identifier].second->SetSourceModule(1, *this->modules[a.arguments[1]].second);
				this->modules[a.identifier].second->SetSourceModule(2, *this->modules[a.arguments[2]].second);
				this->modules[a.identifier].second->SetSourceModule(3, *this->modules[a.arguments[3]].second);
			} else if (a.module == "exponent" && a.arguments.size() == 1 && this->CheckIdentifierArgs(a.arguments)){
				this->modules.insert(std::make_pair(a.identifier, std::make_pair("exponent", std::make_shared<noise::module::Exponent>())));
				this->modules[a.identifier].second->SetSourceModule(0, *this->modules[a.arguments[0]].second);
			} else if (a.module == "invert" && a.arguments.size() == 1 && this->CheckIdentifierArgs(a.arguments)){
				this->modules.insert(std::make_pair(a.identifier, std::make_pair("invert", std::make_shared<noise::module::Invert>())));
				this->modules[a.identifier].second->SetSourceModule(0, *this->modules[a.arguments[0]].second);
			} else if (a.module == "max" && a.arguments.size() == 2 && this->CheckIdentifierArgs(a.arguments)){
				this->modules.insert(std::make_pair(a.identifier, std::make_pair("max", std::make_shared<noise::module::Max>())));
				this->modules[a.identifier].second->SetSourceModule(0, *this->modules[a.arguments[0]].second);
				this->modules[a.identifier].second->SetSourceModule(1, *this->modules[a.arguments[1]].second);
			} else if (a.module == "min" && a.arguments.size() == 2 && this->CheckIdentifierArgs(a.arguments)){
				this->modules.insert(std::make_pair(a.identifier, std::make_pair("min", std::make_shared<noise::module::Min>())));
				this->modules[a.identifier].second->SetSourceModule(0, *this->modules[a.arguments[0]].second);
				this->modules[a.identifier].second->SetSourceModule(1, *this->modules[a.arguments[1]].second);
			} else if (a.module == "multiply" && a.arguments.size() == 2 && this->CheckIdentifierArgs(a.arguments)){
				this->modules.insert(std::make_pair(a.identifier, std::make_pair("multiply", std::make_shared<noise::module::Multiply>())));
				this->modules[a.identifier].second->SetSourceModule(0, *this->modules[a.arguments[0]].second);
				this->modules[a.identifier].second->SetSourceModule(1, *this->modules[a.arguments[1]].second);
			} else if (a.module == "perlin" && a.arguments.size() == 0 && this->CheckIdentifierArgs(a.arguments)){
				this->modules.insert(std::make_pair(a.identifier, std::make_pair("perlin", std::make_shared<noise::module::Perlin>())));
			} else if (a.module == "power" && a.arguments.size() == 2 && this->CheckIdentifierArgs(a.arguments)){
				this->modules.insert(std::make_pair(a.identifier, std::make_pair("power", std::make_shared<noise::module::Power>())));
				this->modules[a.identifier].second->SetSourceModule(0, *this->modules[a.arguments[0]].second);
				this->modules[a.identifier].second->SetSourceModule(1, *this->modules[a.arguments[1]].second);
			} else if (a.module == "ridgedmulti" && a.arguments.size() == 0 && this->CheckIdentifierArgs(a.arguments)){
				this->modules.insert(std::make_pair(a.identifier, std::make_pair("ridgedmulti", std::make_shared<noise::module::RidgedMulti>())));
			} else if (a.module == "rotatepoint" && a.arguments.size() == 1 && this->CheckIdentifierArgs(a.arguments)){
				this->modules.insert(std::make_pair(a.identifier, std::make_pair("rotatepoint", std::make_shared<noise::module::RotatePoint>())));
				this->modules[a.identifier].second->SetSourceModule(0, *this->modules[a.arguments[0]].second);
			} else if (a.module == "scalebias" && a.arguments.size() == 1 && this->CheckIdentifierArgs(a.arguments)){
				this->modules.insert(std::make_pair(a.identifier, std::make_pair("scalebias", std::make_shared<noise::module::ScaleBias>())));
				this->modules[a.identifier].second->SetSourceModule(0, *this->modules[a.arguments[0]].second);
			} else if (a.module == "scalepoint" && a.arguments.size() == 1 && this->CheckIdentifierArgs(a.arguments)){
				this->modules.insert(std::make_pair(a.identifier, std::make_pair("scalepoint", std::make_shared<noise::module::ScalePoint>())));
				this->modules[a.identifier].second->SetSourceModule(0, *this->modules[a.arguments[0]].second);
			} else if (a.module == "select" && a.arguments.size() == 3 /* maybe 2 */ && this->CheckIdentifierArgs(a.arguments)){
				this->modules.insert(std::make_pair(a.identifier, std::make_pair("select", std::make_shared<noise::module::Select>())));
				this->modules[a.identifier].second->SetSourceModule(0, *this->modules[a.arguments[0]].second);
				this->modules[a.identifier].second->SetSourceModule(1, *this->modules[a.arguments[1]].second);
				this->modules[a.identifier].second->SetSourceModule(2, *this->modules[a.arguments[2]].second);
			} else if (a.module == "spheres" && a.arguments.size() == 0 && this->CheckIdentifierArgs(a.arguments)){
				this->modules.insert(std::make_pair(a.identifier, std::make_pair("spheres", std::make_shared<noise::module::Spheres>())));
			} else if (a.module == "terrace" && a.arguments.size() == 1 && this->CheckIdentifierArgs(a.arguments)){
				this->modules.insert(std::make_pair(a.identifier, std::make_pair("terrace", std::make_shared<noise::module::Terrace>())));
				this->modules[a.identifier].second->SetSourceModule(0, *this->modules[a.arguments[0]].second);
			} else if (a.module == "translatepoint" && a.arguments.size() == 1 && this->CheckIdentifierArgs(a.arguments)){
				this->modules.insert(std::make_pair(a.identifier, std::make_pair("translatepoint", std::make_shared<noise::module::TranslatePoint>())));
				this->modules[a.identifier].second->SetSourceModule(0, *this->modules[a.arguments[0]].second);
			} else if (a.module == "turbulence" && a.arguments.size() == 1 && this->CheckIdentifierArgs(a.arguments)){
				this->modules.insert(std::make_pair(a.identifier, std::make_pair("turbulence", std::make_shared<noise::module::Turbulence>())));
				this->modules[a.identifier].second->SetSourceModule(0, *this->modules[a.arguments[0]].second);
			} else if (a.module == "voronoi" && a.arguments.size() == 0 && this->CheckIdentifierArgs(a.arguments)){
				this->modules.insert(std::make_pair(a.identifier, std::make_pair("voronoi", std::make_shared<noise::module::Voronoi>())));
			} else {
				status = NoiseLang::Error;
				this->AddError("Invalid number of arguments for module " + a.module);
			}
			// }}}

		} else {

			status = NoiseLang::Error;
			this->AddError("Duplicate identifier `" + a.identifier + "`");

		}

	} else if (std::regex_match(line, this->method)) {

		// Line is a <method>
		auto m = this->ParseMethod(line);
		
		if (auto it = this->modules.find(m.identifier); it != this->modules.end()){

			for (unsigned int i = 0; i < this->methods[it->second.first].size(); i++){

				if (this->methods[it->second.first][i].first == m.method){
					
					if (m.arguments.size() != this->methods[it->second.first][i].second.size()){

						status = NoiseLang::Error;
						this->AddError("Invalid number of arguments to method \"" + m.method + "\"");
						break;

					}

					for (unsigned int j = 0; j < m.arguments.size(); j++){
						
						if (m.arguments[j].GetType() != this->methods[it->second.first][i].second[j]){

							status = NoiseLang::Error;
							this->AddError("Invalid argument type");
							break;

						}

					}

					break;

				}

			}

			if (status == NoiseLang::Ok){

				// Method call is legit
				// auto mod = std::dynamic_pointer_cast<NOISE_MODULE_TYPE>(it->second.second);
				
				m.Print();
				
				if (it->second.first == "abs"){
					auto mod = std::dynamic_pointer_cast<noise::module::Abs>(it->second.second);
					if (m.method == "SetSourceModule") mod->SetSourceModule(static_cast<int>(m.arguments[0].number), *this->modules.find(m.arguments[1].identifier)->second.second);
				} else if (it->second.first == "add"){
					auto mod = std::dynamic_pointer_cast<noise::module::Add>(it->second.second);
					if (m.method == "SetSourceModule") mod->SetSourceModule(static_cast<int>(m.arguments[0].number), *this->modules.find(m.arguments[1].identifier)->second.second);
				} else if (it->second.first == "billow"){
					auto mod = std::dynamic_pointer_cast<noise::module::Billow>(it->second.second);
					if (m.method == "SetFrequency") mod->SetFrequency(m.arguments[0].number);
					else if (m.method == "SetLacunarity") mod->SetLacunarity(m.arguments[0].number);
					//else if (m.method == "SetNoiseQuality") mod->SetNoiseQuality(m.arguments[0].number);
					else if (m.method == "SetOctaveCount") mod->SetOctaveCount(static_cast<int>(m.arguments[0].number));
					else if (m.method == "SetPersistence") mod->SetPersistence(m.arguments[0].number);
					else if (m.method == "SetSeed") mod->SetSeed(static_cast<int>(m.arguments[0].number));
				} else if (it->second.first == "blend"){
					auto mod = std::dynamic_pointer_cast<noise::module::Blend>(it->second.second);
					if (m.method == "SetSourceModule") mod->SetSourceModule(static_cast<int>(m.arguments[0].number), *this->modules.find(m.arguments[1].identifier)->second.second);
					else if (m.method == "SetControlModule") mod->SetControlModule(*this->modules.find(m.arguments[0].identifier)->second.second);
				} else if (it->second.first == "cache"){
					auto mod = std::dynamic_pointer_cast<noise::module::Cache>(it->second.second);
					if (m.method == "SetSourceModule") mod->SetSourceModule(static_cast<int>(m.arguments[0].number), *this->modules.find(m.arguments[1].identifier)->second.second);
				} else if (it->second.first == "checkerboard"){
				} else if (it->second.first == "clamp"){
					auto mod = std::dynamic_pointer_cast<noise::module::Clamp>(it->second.second);
					if (m.method == "SetSourceModule") mod->SetSourceModule(static_cast<int>(m.arguments[0].number), *this->modules.find(m.arguments[1].identifier)->second.second);
					else if (m.method == "SetBounds") mod->SetBounds(m.arguments[0].number, m.arguments[1].number);
				} else if (it->second.first == "const"){
					auto mod = std::dynamic_pointer_cast<noise::module::Const>(it->second.second);
					if (m.method == "SetConstValue") mod->SetConstValue(m.arguments[0].number);
				} else if (it->second.first == "curve"){
					auto mod = std::dynamic_pointer_cast<noise::module::Curve>(it->second.second);
					if (m.method == "SetSourceModule") mod->SetSourceModule(static_cast<int>(m.arguments[0].number), *this->modules.find(m.arguments[1].identifier)->second.second);
					else if (m.method == "AddControlPoint") mod->AddControlPoint(m.arguments[0].number, m.arguments[1].number);
					else if (m.method == "ClearAllControlPoints") mod->ClearAllControlPoints();
				} else if (it->second.first == "cylinders"){
					auto mod = std::dynamic_pointer_cast<noise::module::Cylinders>(it->second.second);
					if (m.method == "SetFrequency") mod->SetFrequency(m.arguments[0].number);
				} else if (it->second.first == "displace"){
					auto mod = std::dynamic_pointer_cast<noise::module::Displace>(it->second.second);
					if (m.method == "SetSourceModule") mod->SetSourceModule(static_cast<int>(m.arguments[0].number), *this->modules.find(m.arguments[1].identifier)->second.second);
					else if (m.method == "SetDisplaceModules") mod->SetDisplaceModules(*this->modules.find(m.arguments[0].identifier)->second.second, *this->modules.find(m.arguments[1].identifier)->second.second, *this->modules.find(m.arguments[2].identifier)->second.second);
					else if (m.method == "SetXDisplaceModule") mod->SetXDisplaceModule(*this->modules.find(m.arguments[0].identifier)->second.second);
					else if (m.method == "SetYDisplaceModule") mod->SetYDisplaceModule(*this->modules.find(m.arguments[0].identifier)->second.second);
					else if (m.method == "SetZDisplaceModule") mod->SetZDisplaceModule(*this->modules.find(m.arguments[0].identifier)->second.second);
				} else if (it->second.first == "exponent"){
					auto mod = std::dynamic_pointer_cast<noise::module::Exponent>(it->second.second);
					if (m.method == "SetSourceModule") mod->SetSourceModule(static_cast<int>(m.arguments[0].number), *this->modules.find(m.arguments[1].identifier)->second.second);
					else if (m.method == "SetExponent") mod->SetExponent(m.arguments[0].number);
				} else if (it->second.first == "invert"){
					auto mod = std::dynamic_pointer_cast<noise::module::Invert>(it->second.second);
					if (m.method == "SetSourceModule") mod->SetSourceModule(static_cast<int>(m.arguments[0].number), *this->modules.find(m.arguments[1].identifier)->second.second);
				} else if (it->second.first == "max"){
					auto mod = std::dynamic_pointer_cast<noise::module::Max>(it->second.second);
					if (m.method == "SetSourceModule") mod->SetSourceModule(static_cast<int>(m.arguments[0].number), *this->modules.find(m.arguments[1].identifier)->second.second);
				} else if (it->second.first == "min"){
					auto mod = std::dynamic_pointer_cast<noise::module::Min>(it->second.second);
					if (m.method == "SetSourceModule") mod->SetSourceModule(static_cast<int>(m.arguments[0].number), *this->modules.find(m.arguments[1].identifier)->second.second);
				} else if (it->second.first == "multiply"){
					auto mod = std::dynamic_pointer_cast<noise::module::Multiply>(it->second.second);
					if (m.method == "SetSourceModule") mod->SetSourceModule(static_cast<int>(m.arguments[0].number), *this->modules.find(m.arguments[1].identifier)->second.second);
				} else if (it->second.first == "perlin"){
					auto mod = std::dynamic_pointer_cast<noise::module::Perlin>(it->second.second);
					if (m.method == "SetFrequency") mod->SetFrequency(m.arguments[0].number);
					else if (m.method == "SetLacunarity") mod->SetLacunarity(m.arguments[0].number);
					//else if (m.method == "SetNoiseQuality") mod->SetNoiseQuality(m.arguments[0].number);
					else if (m.method == "SetOctaveCount") mod->SetOctaveCount(static_cast<int>(m.arguments[0].number));
					else if (m.method == "SetPersistence") mod->SetPersistence(m.arguments[0].number);
					else if (m.method == "SetSeed") mod->SetSeed(static_cast<int>(m.arguments[0].number));
				} else if (it->second.first == "power"){
					auto mod = std::dynamic_pointer_cast<noise::module::Power>(it->second.second);
					if (m.method == "SetSourceModule") mod->SetSourceModule(static_cast<int>(m.arguments[0].number), *this->modules.find(m.arguments[1].identifier)->second.second);
				} else if (it->second.first == "ridgedmulti"){
					auto mod = std::dynamic_pointer_cast<noise::module::RidgedMulti>(it->second.second);
					if (m.method == "SetFrequency") mod->SetFrequency(m.arguments[0].number);
					else if (m.method == "SetLacunarity") mod->SetLacunarity(m.arguments[0].number);
					//else if (m.method == "SetNoiseQuality") mod->SetNoiseQuality(m.arguments[0].number);
					else if (m.method == "SetOctaveCount") mod->SetOctaveCount(static_cast<int>(m.arguments[0].number));
					else if (m.method == "SetSeed") mod->SetSeed(static_cast<int>(m.arguments[0].number));
				} else if (it->second.first == "rotatepoint"){
					auto mod = std::dynamic_pointer_cast<noise::module::RotatePoint>(it->second.second);
					if (m.method == "SetSourceModule") mod->SetSourceModule(static_cast<int>(m.arguments[0].number), *this->modules.find(m.arguments[1].identifier)->second.second);
					else if (m.method == "SetAngles") mod->SetAngles(m.arguments[0].number, m.arguments[1].number, m.arguments[2].number);
					else if (m.method == "SetXAngle") mod->SetXAngle(m.arguments[0].number);
					else if (m.method == "SetYAngle") mod->SetYAngle(m.arguments[0].number);
					else if (m.method == "SetZAngle") mod->SetZAngle(m.arguments[0].number);
				} else if (it->second.first == "scalebias"){
					auto mod = std::dynamic_pointer_cast<noise::module::ScaleBias>(it->second.second);
					if (m.method == "SetSourceModule") mod->SetSourceModule(static_cast<int>(m.arguments[0].number), *this->modules.find(m.arguments[1].identifier)->second.second);
					else if (m.method == "SetBias") mod->SetBias(m.arguments[0].number);
					else if (m.method == "SetScale") mod->SetScale(m.arguments[0].number);
				} else if (it->second.first == "scalepoint"){
					auto mod = std::dynamic_pointer_cast<noise::module::ScalePoint>(it->second.second);
					if (m.method == "SetSourceModule") mod->SetSourceModule(static_cast<int>(m.arguments[0].number), *this->modules.find(m.arguments[1].identifier)->second.second);
					else if (m.method == "SetScale") mod->SetScale(m.arguments[0].number, m.arguments[1].number, m.arguments[2].number);
					else if (m.method == "SetXScale") mod->SetXScale(m.arguments[0].number);
					else if (m.method == "SetYScale") mod->SetYScale(m.arguments[0].number);
					else if (m.method == "SetZScale") mod->SetZScale(m.arguments[0].number);
				} else if (it->second.first == "select"){
					auto mod = std::dynamic_pointer_cast<noise::module::Select>(it->second.second);
					if (m.method == "SetSourceModule") mod->SetSourceModule(static_cast<int>(m.arguments[0].number), *this->modules.find(m.arguments[1].identifier)->second.second);
					else if (m.method == "SetBounds") mod->SetBounds(m.arguments[0].number, m.arguments[1].number);
					else if (m.method == "SetControlModule") mod->SetControlModule(*this->modules.find(m.arguments[1].identifier)->second.second);
					else if (m.method == "SetEdgeFalloff") mod->SetEdgeFalloff(m.arguments[1].number);
				} else if (it->second.first == "spheres"){
					auto mod = std::dynamic_pointer_cast<noise::module::Spheres>(it->second.second);
					if (m.method == "SetFrequency") mod->SetFrequency(m.arguments[0].number);
				} else if (it->second.first == "terrace"){
					auto mod = std::dynamic_pointer_cast<noise::module::Terrace>(it->second.second);
					if (m.method == "SetSourceModule") mod->SetSourceModule(static_cast<int>(m.arguments[0].number), *this->modules.find(m.arguments[1].identifier)->second.second);
					else if (m.method == "AddControlPoint") mod->AddControlPoint(m.arguments[0].number);
					else if (m.method == "ClearAllControlPoints") mod->ClearAllControlPoints();
					else if (m.method == "InvertTerraces") mod->InvertTerraces(!mod->IsTerracesInverted());
					else if (m.method == "MakeControlPoints") mod->MakeControlPoints(static_cast<int>(m.arguments[0].number));
				} else if (it->second.first == "translatepoint"){
					auto mod = std::dynamic_pointer_cast<noise::module::TranslatePoint>(it->second.second);
					if (m.method == "SetSourceModule") mod->SetSourceModule(static_cast<int>(m.arguments[0].number), *this->modules.find(m.arguments[1].identifier)->second.second);
					else if (m.method == "SetTranslation") mod->SetTranslation(m.arguments[0].number, m.arguments[1].number, m.arguments[2].number);
					else if (m.method == "SetXTranslation") mod->SetXTranslation(m.arguments[0].number);
					else if (m.method == "SetYTranslation") mod->SetYTranslation(m.arguments[0].number);
					else if (m.method == "SetZTranslation") mod->SetZTranslation(m.arguments[0].number);
				} else if (it->second.first == "turbulence"){
					auto mod = std::dynamic_pointer_cast<noise::module::Turbulence>(it->second.second);
					if (m.method == "SetSourceModule") mod->SetSourceModule(static_cast<int>(m.arguments[0].number), *this->modules.find(m.arguments[1].identifier)->second.second);
					else if (m.method == "SetFrequency") mod->SetFrequency(m.arguments[0].number);
					else if (m.method == "SetPower") mod->SetPower(m.arguments[0].number);
					else if (m.method == "SetRoughness") mod->SetRoughness(static_cast<int>(m.arguments[0].number));
					else if (m.method == "SetSeed") mod->SetSeed(static_cast<int>(m.arguments[0].number));
				} else if (it->second.first == "voronoi"){
					auto mod = std::dynamic_pointer_cast<noise::module::Voronoi>(it->second.second);
					if (m.method == "EnableDistance") mod->EnableDistance(true);
					else if (m.method == "DisableDistance") mod->EnableDistance(false);
					else if (m.method == "SetDisplacement") mod->SetDisplacement(m.arguments[0].number);
					else if (m.method == "SetFrequency") mod->SetFrequency(m.arguments[0].number);
					else if (m.method == "SetSeed") mod->SetSeed(static_cast<int>(m.arguments[0].number));
				}
			}

		} else {

			status = NoiseLang::Error;
			this->AddError("Identifier " + m.identifier + " does not exist");

		}

	} else if (std::regex_match(line, this->out)) {

		// Line is a <out> grammar
		auto o = this->ParseOut(line);

		if (auto it = this->modules.find(o.identifier); it != this->modules.end()){

			if (this->image != nullptr)
				this->image->SetSampler(it->second.second);
			this->output_module = o.identifier;

		} else {

			status = NoiseLang::Error;
			this->AddError("Identifier " + o.identifier + " does not exist");

		}

	} else if (std::regex_match(line, this->save)) {

		// Line is a <save> grammar
		auto s = this->ParseSave(line);

		this->lines.erase(this->lines.end() - 1);

		std::ofstream outFile;
		outFile.open(s.filename);

		for (auto& l : this->lines){
			outFile << l << std::endl;
		}

		outFile.close();

	} else if (std::regex_match(line, this->load)) {

		// Line is a <load> grammar
		auto l = this->ParseLoad(line);

		this->Run(l.filename, false);

	} else if (std::regex_match(line, this->show)) {

		auto s = this->ParseShow(line);

		this->image = std::make_unique<NoiseLang::Image>(s.width, s.height);
		this->image->InitSDL();
		this->image->OnRender = [this](double dt) {
			dt*=2;
			this->image->noiseZ += (0.01);
		};
		if (this->output_module == "")	
			this->image->SetSampler(this->GetDefaultOutModule());
		else
			this->image->SetSampler(this->modules[this->output_module].second);
		this->image->StartRenderer();
		this->image->PollEvents();
			
		this->reading_status = 2;
		this->reading_thread = std::make_shared<std::thread>(&NoiseLang::Interpreter::InternalThreadedRead, this);

	} else if (std::regex_match(line, this->exit)) {

		this->reading_status = 0;

	} else if (line.size() > 0) {

		// Line is not blank and doesnt match any patterns
		status = NoiseLang::Error;
		this->AddError("Line does not match any patterns");

	}

	return status;
}

auto NoiseLang::Interpreter::Run(const std::string& filename, bool savelines) -> void {
	std::ifstream inFile(filename);

	std::string line;
	while (std::getline(inFile, line)){
		if (this->RunLine(line, savelines) == NoiseLang::Error){
			this->Reset();
			break;
		}
	}

	inFile.close();
}

auto NoiseLang::Interpreter::Reset() -> void {
	this->modules.clear();
}

auto NoiseLang::Interpreter::GetDefaultOutModule() -> std::shared_ptr<noise::module::Module> {
	return std::make_unique<noise::module::Const>();
}

auto NoiseLang::Interpreter::StartReading() -> void {

	this->reading_status = 1;

	while (this->reading_status > 0){

		if (this->image != nullptr){

			this->image->PollEvents();
			if (this->image->IsDead()){
				this->image = nullptr;
				this->reading_status = 1;
				this->reading_thread->join();
			}

		} else {

			this->InternalRead();

		}

	}

	this->StopReading();
	if (this->image != nullptr)
		this->image->StopRenderer();

}

auto NoiseLang::Interpreter::StopReading() -> void {
	this->reading_status = 0;
	if (this->reading_thread != nullptr)
		this->reading_thread->join();
}

auto NoiseLang::Interpreter::InternalRead() -> void {

	std::string line;
	std::cout << ">>> ";
	std::getline(std::cin, line);
	if (line != "" && this->RunLine(line) == NoiseLang::Error){
		std::cout << this->GetError() << std::endl;
	}

}

auto NoiseLang::Interpreter::InternalThreadedRead() -> void {
	while (this->reading_status == 2){
		this->InternalRead();
	}
}

NoiseLang::Image::Image(unsigned int width, unsigned int height) {
	this->width = width;
	this->height = height;
	
	// Not currently rendering on an interval
	this->rendering = false;

	// Default FPS
	this->fps = 60.0;

	// Explicitly initialize window and renderer to nullptr's
	this->window = nullptr;
	this->renderer = nullptr;

	// Initialize texture to a nullptr
	this->texture = nullptr;

	// Initialize the scale functions to 1 to 1
	this->scaleX = [](unsigned int x) -> double { return static_cast<double>(x) / 100.0; };
	this->scaleY = [](unsigned int y) -> double { return static_cast<double>(y) / 100.0; };
	
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

	this->is_dead = false;
}

NoiseLang::Image::~Image() {
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

auto NoiseLang::Image::InitSDL() -> void {
	// Initialize the window and events
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
	this->window = SDL_CreateWindow("libnoise", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, this->width, this->height, SDL_WINDOW_RESIZABLE);

	// Create the opengl context and set tell it not to make this thread the current owner
	this->context = SDL_GL_GetCurrentContext();
	SDL_GL_MakeCurrent(this->window, nullptr);

	SDL_DisplayMode DM;
	SDL_GetCurrentDisplayMode(0, &DM);
	SDL_SetWindowPosition(this->window, DM.w / 2, (DM.h / 2) - (this->height / 2));
}

auto NoiseLang::Image::SetSampler(std::shared_ptr<noise::module::Module> noiseSampler) -> void {
	this->noiseSampler = noiseSampler;
}

auto NoiseLang::Image::SetFPS(float fps) -> void {
	this->fps = fps;
}

auto NoiseLang::Image::GetFPS() -> float {
	return this->fps;
}

auto NoiseLang::Image::PollEvents() -> bool {
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

auto NoiseLang::Image::internal_render() -> void {
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

auto NoiseLang::Image::StartRenderer() -> void {
	if (this->window != nullptr){
		if (this->noiseSampler != nullptr){
			// create rendering thread
			this->rendering = true;
			this->thread = std::thread(&NoiseLang::Image::internal_render, this);
		} else {
			std::cout << "Internal noiseSampler has not been initialized, please call Image::SetSampler() with a std::shared_ptr<noise::module::Module> before calling Image::StartRenderer()" << std::endl;
		}
	} else {
		std::cout << "SDL has not been initialized, please initialize SDL before calling Image::StartRenderer()" << std::endl;
	}
}

auto NoiseLang::Image::StopRenderer() -> void {
	if (this->rendering){
		// stop the thread and wait for it to join
		this->rendering = false;
		this->thread.join();
		this->is_dead = true;
	}
}

auto NoiseLang::Image::IsDead() -> bool {
	return this->is_dead;
}
