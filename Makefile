COMPILER_FLAGS=-g -Wall -Wextra -pedantic -O3
LIBRARY=-I/usr/local/ -lsfml-graphics -lsfml-window -lsfml-system
VERSION=-std=c++17

a.out : main.cpp
	g++ $(VERSION) $(COMPILER_FLAGS) $(LIBRARY) $< -o$@
clean:
	rm −f a.out
.PHONY: clean