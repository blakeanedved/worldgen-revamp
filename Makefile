all: main

main: main.cpp
	g++ -std=c++17 main.cpp -Lvendor/lib -Ivendor/include -lSDL2 -lnoise

debug: main.cpp
	g++ -std=c++17 main.cpp -g -Lvendor/lib -Ivendor/include -lSDL2 -lnoise
