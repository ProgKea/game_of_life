CC?=clang
LIBS=$(shell pkg-config sdl2 --cflags --libs) -I./include
FLAGS=-std=c99 -Wextra -Wall -Wshadow -Wstrict-aliasing -Wstrict-overflow -pedantic -O2

gol: main.c
	${CC} ${FLAGS} -o $@ $< ${LIBS}
