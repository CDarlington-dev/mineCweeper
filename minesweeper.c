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
TTF_Font* font = NULL;
TTF_Font* title_font = NULL;
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

TTF_Font* load_font(int size) {
    TTF_Font* loaded_font = NULL;
    
    const char* font_paths[] = {
        "font.ttf",
        "./font.ttf",
        "C:\\Windows\\Fonts\\arial.ttf",
        "C:\\Windows\\Fonts\\Arial.ttf",
        "C:\\Windows\\Fonts\\calibri.ttf",
        "C:\\Windows\\Fonts\\verdana.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/System/Library/Fonts/Helvetica.ttc"
    };
    
    for (int i = 0; i < sizeof(font_paths) / sizeof(font_paths[0]); i++) {
        loaded_font = TTF_OpenFont(font_paths[i], size);
        if (loaded_font) break;
    }
    
    return loaded_font;
}

void draw_text(const char* text, int x, int y, SDL_Color color, TTF_Font* use_font) {
    if (!use_font) return;
    SDL_Surface* surface = TTF_RenderText_Solid(use_font, text, color);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect rect = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void draw_game() {
    SDL_SetRenderDrawColor(renderer, 192, 192, 192, 255);
    SDL_RenderClear(renderer);
    
    if (title_font) {
        char status_text[64];
        snprintf(status_text, sizeof(status_text), "Mines: %d", game.flags_remaining);
        SDL_Color black = {0, 0, 0, 255};
        draw_text(status_text, 20, 20, black, title_font);
    }
    
    if (font) {
        if (game.status == GAME_WON) {
            SDL_Color green = {0, 200, 0, 255};
            draw_text("YOU WIN! Click to restart", WINDOW_WIDTH / 2 - 100, 45, green, font);
        } else if (game.status == GAME_LOST) {
            SDL_Color red = {200, 0, 0, 255};
            draw_text("GAME OVER! Click to restart", WINDOW_WIDTH / 2 - 110, 45, red, font);
        }
    }
    
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
                if (font) {
                    SDL_Color red = {255, 0, 0, 255};
                    draw_text("F", px + 8, py + 5, red, font);
                }
            } else if (cell->state == CELL_REVEALED) {
                if (cell->is_mine) {
                    SDL_SetRenderDrawColor(renderer, 255, 100, 100, 255);
                    SDL_RenderFillRect(renderer, &rect);
                    if (font) {
                        SDL_Color dark = {100, 0, 0, 255};
                        draw_text("*", px + 8, py + 5, dark, font);
                    }
                } else {
                    SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255);
                    SDL_RenderFillRect(renderer, &rect);
                    
                    if (cell->adjacent_mines > 0 && font) {
                        char num[2];
                        snprintf(num, sizeof(num), "%d", cell->adjacent_mines);
                        SDL_Color colors[] = {
                            {0, 0, 255, 255},
                            {0, 128, 0, 255},
                            {255, 0, 0, 255},
                            {0, 0, 128, 255},
                            {128, 0, 0, 255},
                            {0, 128, 128, 255},
                            {0, 0, 0, 255},
                            {128, 128, 128, 255}
                        };
                        draw_text(num, px + 8, py + 5, colors[cell->adjacent_mines - 1], font);
                    }
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
    
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    
    window = SDL_CreateWindow("Minesweeper", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    
    font = load_font(18);
    title_font = load_font(24);
    
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
    
    if (font) TTF_CloseFont(font);
    if (title_font) TTF_CloseFont(title_font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    
    return 0;
}