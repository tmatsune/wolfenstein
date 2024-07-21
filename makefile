# Compiler flags
CFLAGS = -std=c17 -Wall -Wextra -Werror `sdl2-config --cflags`

# Linker flags
LDFLAGS = `sdl2-config --libs` -lSDL2_image

# Targets
all: main

main: main.o utils.o
	gcc main.o utils.o -o main $(LDFLAGS)

main.o: main.c
	gcc $(CFLAGS) -c main.c -o main.o

utils.o: utils/utils.c utils/utils.h
	gcc $(CFLAGS) -c utils/utils.c -o utils.o

clean:
	rm -f main main.o utils.o
