#include <SDL2/SDL.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_render.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define FOREGROUND_COLOR 255, 255, 255, 255
#define BACKGROUND_COLOR 0, 0, 0, 255
#define PAUSED_COLOR 50, 50, 50, 255

#define CELL_ROWS 20
#define CELL_COLS 20
#define CELL_SIZE 50
#define CELL_COUNT CELL_ROWS *CELL_COLS

#define DEFAULT_WINDOW_WIDTH CELL_ROWS *CELL_SIZE
#define DEFAULT_WINDOW_HEIGHT CELL_COLS *CELL_SIZE

#define RETURN_ERROR(msg)                                 \
    do {                                                  \
        fprintf(stderr, "%s\n%s\n", msg, SDL_GetError()); \
        return EXIT_FAILURE;                              \
    } while (0)

typedef struct {
    uint32_t row;
    uint32_t col;
    bool dead;
} Cell;

uint32_t get_index(uint32_t row, uint32_t col)
{
    return row + col * CELL_COLS;
}

uint32_t count_neighbors(Cell *grid, uint32_t row, uint32_t col)
{
    uint32_t n = 0;

    for (int32_t i = row - 1; i <= (int32_t)row + 1; i++) {
        for (int32_t j = col - 1; j <= (int32_t)col + 1; j++) {
            if (i == (int32_t)row && j == (int32_t)col) continue;
            if (i >= 0 && i < CELL_ROWS && j >= 0 && j < CELL_COLS && !grid[get_index(i, j)].dead)
                n++;
        }
    }

    return n;
}

void render_grid(Cell *grid, SDL_Renderer *renderer, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    for (uint32_t i = 0; i < CELL_COUNT; i++) {
        SDL_SetRenderDrawColor(renderer, r, g, b, a);
        if (grid[i].dead) {
            SDL_RenderDrawRect(renderer, &(SDL_Rect){.w = CELL_SIZE, .h = CELL_SIZE, .x = grid[i].row * CELL_SIZE, .y = grid[i].col * CELL_SIZE});
        } else {
            SDL_RenderFillRect(renderer, &(SDL_Rect){.w = CELL_SIZE, .h = CELL_SIZE, .x = grid[i].row * CELL_SIZE, .y = grid[i].col * CELL_SIZE});
        }
    }
}

void create_grid(Cell *grid)
{
    for (uint32_t row = 0; row < CELL_ROWS; row++) {
        for (uint32_t col = 0; col < CELL_COLS; col++) {
            grid[get_index(row, col)] = (Cell){row, col, true};
        }
    }
}

void update_grid(Cell *grid)
{
    static Cell grid_backup[CELL_COUNT] = {0};
    memcpy(grid_backup, grid, CELL_COUNT * sizeof(Cell));

    for (int32_t row = 0; row < CELL_ROWS; row++) {
        for (int32_t col = 0; col < CELL_COLS; col++) {
            Cell *current_cell = &grid[get_index(row, col)];
            uint32_t neighbors = count_neighbors(grid_backup, row, col);

            if (current_cell->dead) {
                if (neighbors == 3) {
                    current_cell->dead = false;
                }
            } else {
                if (!(neighbors == 2 || neighbors == 3)) {
                    current_cell->dead = true;
                }
            }
        }
    }
}

static Cell grid[CELL_COUNT];

int32_t main(void)
{
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        RETURN_ERROR("SDL_Init failed.");
    }

    SDL_Window *win = SDL_CreateWindow("Game of life",
                                       SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                       DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT,
                                       SDL_WINDOW_SHOWN);
    if (!win) {
        RETURN_ERROR("Window creation failed.");
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        SDL_DestroyRenderer(renderer);
        SDL_Quit();
        RETURN_ERROR("Renderer creation failed.");
    }

    create_grid(grid);
    bool quit = false;
    bool paused = true;
    SDL_Event e;
    int32_t mousex, mousey;
    while (!quit) {
        SDL_PollEvent(&e);
        switch (e.type) {
        case SDL_QUIT:
            quit = true;
            break;
        case SDL_MOUSEBUTTONDOWN:
            SDL_GetMouseState(&mousex, &mousey);
            int32_t row = mousex / CELL_SIZE % CELL_ROWS;
            int32_t col = mousey / CELL_SIZE % CELL_COLS;
            int32_t idx = get_index(row, col);
            if (idx >= 0) grid[idx].dead ^= true;
            break;
        case SDL_KEYDOWN:
            switch (e.key.keysym.sym) {
            case SDLK_ESCAPE:
                quit = true;
                break;
            case SDLK_SPACE:
                paused = !paused;
                break;
            case SDLK_r:
                create_grid(grid);
                break;
            }
            break;
        }

        SDL_SetRenderDrawColor(renderer, BACKGROUND_COLOR);
        SDL_RenderClear(renderer);

        if (!paused) {
            update_grid(grid);
            render_grid(grid, renderer, FOREGROUND_COLOR);
        } else {
            render_grid(grid, renderer, PAUSED_COLOR);
        }

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return EXIT_SUCCESS;
}

// TODO: make it wrap
// TODO: read file to generate the grid or let the user click cells in order to toggle them
// TODO: add zoom function
// TODO: make it slower
