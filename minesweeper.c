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

void reveal_cell(int x, int y) {
    if (x < 0 || x >= GRID_WIDTH || y < 0 || y >= GRID_HEIGHT) return;
    if (game.grid[y][x].state != CELL_HIDDEN) return;
    
    game.grid[y][x].state = CELL_REVEALED;
    
    if (game.grid[y][x].is_mine) {
        game.status = GAME_LOST;
        for (int row = 0; row < GRID_HEIGHT; row++) {
            for (int col = 0; col < GRID_WIDTH; col++) {
                if (game.grid[row][col].is_mine) {
                    game.grid[row][col].state = CELL_REVEALED;
                }
            }
        }
        return;
    }
    
    if (game.grid[y][x].adjacent_mines == 0) {
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                if (dx != 0 || dy != 0) {
                    reveal_cell(x + dx, y + dy);
                }
            }
        }
    }
}

void check_win() {
    int hidden_count = 0;
    
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            if (game.grid[y][x].state == CELL_HIDDEN || game.grid[y][x].state == CELL_FLAGGED) {
                hidden_count++;
            }
        }
    }
    
    if (hidden_count == MINE_COUNT) {
        game.status = GAME_WON;
    }
}

void handle_left_click(int mouse_x, int mouse_y) {
    if (game.status != GAME_PLAYING) return;
    
    int grid_x = (mouse_x - 20) / CELL_SIZE;
    int grid_y = (mouse_y - 80) / CELL_SIZE;
    
    if (grid_x < 0 || grid_x >= GRID_WIDTH || grid_y < 0 || grid_y >= GRID_HEIGHT) return;
    
    if (game.first_click) {
        place_mines(grid_x, grid_y);
        game.first_click = false;
    }
    
    if (game.grid[grid_y][grid_x].state == CELL_HIDDEN) {
        reveal_cell(grid_x, grid_y);
        check_win();
    }
}

void handle_right_click(int mouse_x, int mouse_y) {
    if (game.status != GAME_PLAYING) return;
    
    int grid_x = (mouse_x - 20) / CELL_SIZE;
    int grid_y = (mouse_y - 80) / CELL_SIZE;
    
    if (grid_x < 0 || grid_x >= GRID_WIDTH || grid_y < 0 || grid_y >= GRID_HEIGHT) return;
    
    Cell* cell = &game.grid[grid_y][grid_x];
    
    if (cell->state == CELL_HIDDEN) {
        cell->state = CELL_FLAGGED;
        game.flags_remaining--;
    } else if (cell->state == CELL_FLAGGED) {
        cell->state = CELL_HIDDEN;
        game.flags_remaining++;
    }
}

void draw_game() {
    SDL_SetRenderDrawColor(renderer, 192, 192, 192, 255);
    SDL_RenderClear(renderer);
    
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            int px = 20 + x * CELL_SIZE;
            int py = 80 + y * CELL_SIZE;
            SDL_Rect rect = {px, py, CELL_SIZE - 2, CELL_SIZE - 2};
            
            Cell* cell = &game.grid[y][x];
            
            if (cell->state == CELL_HIDDEN) {
                SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255);
                SDL_RenderFillRect(renderer, &rect);
                SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255);
                SDL_Rect highlight = {px, py, CELL_SIZE - 2, 2};
                SDL_RenderFillRect(renderer, &highlight);
                highlight.w = 2;
                highlight.h = CELL_SIZE - 2;
                SDL_RenderFillRect(renderer, &highlight);
            } else if (cell->state == CELL_FLAGGED) {
                SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255);
                SDL_RenderFillRect(renderer, &rect);
            } else if (cell->state == CELL_REVEALED) {
                if (cell->is_mine) {
                    SDL_SetRenderDrawColor(renderer, 255, 100, 100, 255);
                    SDL_RenderFillRect(renderer, &rect);
                } else {
                    SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255);
                    SDL_RenderFillRect(renderer, &rect);
                }
            }
            
            SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
            SDL_RenderDrawRect(renderer, &rect);
        }
    }
    
    SDL_RenderPresent(renderer);
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
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                if (game.status != GAME_PLAYING) {
                    init_game();
                } else {
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        handle_left_click(event.button.x, event.button.y);
                    } else if (event.button.button == SDL_BUTTON_RIGHT) {
                        handle_right_click(event.button.x, event.button.y);
                    }
                }
            }
        }
        
        draw_game();
        SDL_Delay(16);
    }
    
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}