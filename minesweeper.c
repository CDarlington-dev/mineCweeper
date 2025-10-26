#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#define GRID_WIDTH 16
#define GRID_HEIGHT 16
#define MINE_COUNT 40
#define CELL_SIZE 30
#define WINDOW_WIDTH (GRID_WIDTH * CELL_SIZE + 40)
#define WINDOW_HEIGHT (GRID_HEIGHT * CELL_SIZE + 100)

typedef enum {
    CELL_HIDDEN,
    CELL_REVEALED,
    CELL_FLAGGED
} CellState;

typedef struct {
    bool is_mine;
    CellState state;
    int adjacent_mines;
} Cell;

typedef enum {
    GAME_PLAYING,
    GAME_WON,
    GAME_LOST
} GameStatus;

typedef struct {
    Cell grid[GRID_HEIGHT][GRID_WIDTH];
    GameStatus status;
    int flags_remaining;
    bool first_click;
} GameState;

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
GameState game;

void init_game() {
    game.status = GAME_PLAYING;
    game.flags_remaining = MINE_COUNT;
    game.first_click = true;
    
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            game.grid[y][x].is_mine = false;
            game.grid[y][x].state = CELL_HIDDEN;
            game.grid[y][x].adjacent_mines = 0;
        }
    }
}

void place_mines(int avoid_x, int avoid_y) {
    int mines_placed = 0;
    
    while (mines_placed < MINE_COUNT) {
        int x = rand() % GRID_WIDTH;
        int y = rand() % GRID_HEIGHT;
        
        if (!game.grid[y][x].is_mine && !(x == avoid_x && y == avoid_y)) {
            game.grid[y][x].is_mine = true;
            mines_placed++;
        }
    }
    
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            if (game.grid[y][x].is_mine) continue;
            
            int count = 0;
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    int nx = x + dx;
                    int ny = y + dy;
                    if (nx >= 0 && nx < GRID_WIDTH && ny >= 0 && ny < GRID_HEIGHT) {
                        if (game.grid[ny][nx].is_mine) count++;
                    }
                }
            }
            game.grid[y][x].adjacent_mines = count;
        }
    }
}

int main(int argc, char* argv[]) {
    srand((unsigned int)time(NULL));
    
    window = SDL_CreateWindow("Minesweeper", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    
    init_game();
    
    bool running = true;
    SDL_Event event;
    
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }
        
        SDL_SetRenderDrawColor(renderer, 192, 192, 192, 255);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
    
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}