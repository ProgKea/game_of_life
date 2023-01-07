CC=clang
LIBS=$(shell pkg-config sdl2 --cflags --libs)
FLAGS=-Wextra -Wall -pedantic -O2

gol: main.c
	${CC} ${FLAGS} -o $@ $< ${LIBS}
