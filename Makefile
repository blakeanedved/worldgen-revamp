all: main

main: main.cpp NoiseLang.hpp
	g++ -std=c++17  -Lvendor/lib -Ivendor/include -lSDL2 -lnoise main.cpp

debug: main.cpp NoiseLang.hpp
	g++ -std=c++17 -g -Lvendor/lib -Ivendor/include -lSDL2 -lnoise main.cpp
