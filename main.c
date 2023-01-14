#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include "common.h"

#define make_rgba(r, g, b, a) \
    (rgba)                    \
    {                         \
        r, g, b, a            \
    }

#define unwrap_rgba(rgba) rgba.r, rgba.g, rgba.b, rgba.a

#define FOREGROUND_COLOR make_rgba(225, 225, 225, 255)
#define BACKGROUND_COLOR make_rgba(0, 0, 0, 255)
#define HOVER_COLOR make_rgba(100, 100, 100, 255)
#define PAUSED_COLOR make_rgba(50, 50, 50, 255)

#define CELL_ROWS 30
#define CELL_COLS 30
#define CELL_SIZE 35
#define CELL_COUNT (CELL_ROWS * CELL_COLS)
#define CELL_UPDATE 1
// #define MAX_CELLS 900

#define DEFAULT_WINDOW_WIDTH (CELL_ROWS * CELL_SIZE)
#define DEFAULT_WINDOW_HEIGHT (CELL_COLS * CELL_SIZE)

#define return_error(msg)                                 \
    do {                                                  \
        fprintf(stderr, "%s\n%s\n", msg, SDL_GetError()); \
        return 1;                                         \
    } while (0)

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} rgba;

typedef struct {
    uint32_t row;
    uint32_t col;
    bool dead;
    rgba fill_color;
    rgba border_color;
} Cell;

void zoom(int zoom)
{
    UNUSED(zoom);
    UNIMPLEMENTED("Zoom function");
}

uint32_t get_index(uint32_t row, uint32_t col)
{
    return row + col * CELL_COLS;
}

void remove_newlines(char *s)
{
    size_t i, di = 0;
    for (i = 0; i < strlen(s); i++) {
        if (s[i] == '\n') {
            di++;
            continue;
        }
        s[i - di] = s[i];
    }
    s[i - di + 1] = '\0';
}

void set_grid_str(Cell *grid, char *s)
{
    remove_newlines(s);

    uint32_t di = 0;

    for (uint32_t i = 0; i < CELL_COUNT; i++) {
        if (s[i] == '\n') {
            di++;
            continue;
        }
        grid[i + di].dead = s[i + di] == '0';
    }
}

Errno save_grid_to_file(Cell *grid, const char *file_path)
{
    int result = 0;
    FILE *file = NULL;

    {
        file = fopen(file_path, "wb");
        if (!file) return_defer(errno);

        for (uint32_t col = 0; col < CELL_COLS; col++) {
            for (uint32_t row = 0; row < CELL_ROWS; row++) {
                fprintf(file, "%d", !grid[get_index(row, col)].dead);
                if (ferror(file)) return_defer(errno);
            }
            fprintf(file, "\n");
            if (ferror(file)) return_defer(errno);
        }
    }

defer:
    if (file) fclose(file);
    return result;
}

char *slurp_file(const char *file_path)
{
    FILE *file = NULL;
    char *buffer = NULL;

    {
        file = fopen(file_path, "rb");
        if (!file) goto error;

        if (fseek(file, 0, SEEK_END) < 0) goto error;

        long file_size = ftell(file);
        if (file_size < 0) goto error;
        buffer = malloc((size_t)file_size + 1);
        if (!buffer) goto error;
        if (fseek(file, 0, SEEK_SET) < 0) goto error;

        UNUSED(fread(buffer, 1, (size_t)file_size, file));
        if (ferror(file)) goto error;
        buffer[file_size] = '\0';
    }

    fclose(file);
    return buffer;

error:
    if (file) fclose(file);
    return NULL;
}

uint32_t count_neighbors(Cell *grid, uint32_t row, uint32_t col)
{
    uint32_t n = 0;

    for (int i = row - 1; i <= (int)row + 1; i++) {
        for (int j = col - 1; j <= (int)col + 1; j++) {
            if (i == (int)row && j == (int)col) continue;
            int x = i % CELL_ROWS;
            int y = j % CELL_COLS;
            if (x < 0) x = CELL_ROWS - 1;
            if (y < 0) y = CELL_COLS - 1;
            if (!grid[get_index(x, y)].dead) n++;
        }
    }

    return n;
}

void render_grid(Cell *grid, SDL_Renderer *renderer)
{
    for (uint32_t i = 0; i < CELL_COUNT; i++) {
        SDL_SetRenderDrawColor(renderer, unwrap_rgba(grid[i].fill_color));
        SDL_RenderFillRect(renderer,
                           &(SDL_Rect){
                               .w = CELL_SIZE,
                               .h = CELL_SIZE,
                               .x = grid[i].row * CELL_SIZE,
                               .y = grid[i].col * CELL_SIZE});
        SDL_SetRenderDrawColor(renderer, unwrap_rgba(grid[i].border_color));
        SDL_RenderDrawRect(renderer,
                           &(SDL_Rect){
                               .w = CELL_SIZE,
                               .h = CELL_SIZE,
                               .x = grid[i].row * CELL_SIZE,
                               .y = grid[i].col * CELL_SIZE});
        SDL_SetRenderDrawColor(renderer, unwrap_rgba(BACKGROUND_COLOR));
    }
}

void create_grid(Cell *grid)
{
    for (uint32_t row = 0; row < CELL_ROWS; row++) {
        for (uint32_t col = 0; col < CELL_COLS; col++) {
            grid[get_index(row, col)] = (Cell){row, col, true, BACKGROUND_COLOR, FOREGROUND_COLOR};
        }
    }
}

void swap_cells(Cell *a, Cell *b)
{
    Cell *t = a;
    a->dead = b->dead;
    b->dead = t->dead;
}

void update_grid(Cell *grid)
{
    static Cell old_grid[CELL_COUNT] = {0};
    memcpy(old_grid, grid, CELL_COUNT * sizeof(Cell));

    for (int row = 0; row < CELL_ROWS; row++) {
        for (int col = 0; col < CELL_COLS; col++) {
            Cell *current_cell = &grid[get_index(row, col)];
            uint32_t neighbors = count_neighbors(old_grid, row, col);

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

int main()
{
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        return_error("SDL_Init failed.");
    }

    SDL_Window *win = SDL_CreateWindow("Game of life",
                                       SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                       DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT,
                                       SDL_WINDOW_SHOWN);
    if (!win) {
        return_error("Window creation failed.");
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        SDL_DestroyRenderer(renderer);
        SDL_Quit();
        return_error("Renderer creation failed.");
    }

    create_grid(grid);
    bool quit = false;
    bool paused = true;
    SDL_Event e;
    int mousex, mousey;
    int mouse_cell_idx = 0;
    int update_frames = 0;

    char *txt_grid = slurp_file("gol.txt");
    set_grid_str(grid, txt_grid);

    while (!quit) {
        SDL_PollEvent(&e);
        switch (e.type) {
        case SDL_QUIT:
            quit = true;
            break;
        case SDL_MOUSEMOTION:
            SDL_GetMouseState(&mousex, &mousey);
            int mouse_row = mousex / CELL_SIZE % CELL_ROWS;
            int mouse_col = mousey / CELL_SIZE % CELL_COLS;
            mouse_cell_idx = get_index(mouse_row, mouse_col);
            break;
        case SDL_MOUSEBUTTONDOWN:
            grid[mouse_cell_idx].dead ^= true;
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
            case SDLK_s:
                save_grid_to_file(grid, "gol.txt");
                break;
            }
            break;
        }

        SDL_RenderClear(renderer);

        // FIXME: This code is not really that clean
        for (uint32_t i = 0; i < CELL_COUNT; i++) {
            if (!grid[i].dead)
                grid[i].fill_color = FOREGROUND_COLOR;
            else
                grid[i].fill_color = BACKGROUND_COLOR;

            if (paused)
                grid[i].border_color = PAUSED_COLOR;
            else
                grid[i].border_color = FOREGROUND_COLOR;
        }
        if (grid[mouse_cell_idx].dead) grid[mouse_cell_idx].fill_color = HOVER_COLOR;

        if (!paused) {
            if (update_frames >= CELL_UPDATE) {
                update_grid(grid);
                update_frames = 0;
            }
            render_grid(grid, renderer);
        } else {
            render_grid(grid, renderer);
        }

        SDL_RenderPresent(renderer);
        update_frames++;
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}

// TODO: add zoom function
// TODO: fix the rows and cols being mixed up in the code
// TODO: find a way to make CELL_ROWS and CELL_COLS variable
// TODO: Set a maximum amount of cells and let the user decide the cells
