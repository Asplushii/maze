#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "version.h"

#define SCREEN_WIDTH 1080
#define SCREEN_HEIGHT 1080

int CELLS_X = 20;
int CELLS_Y = 20;
int CELL_SIZE;

const char* get_build_timestamp() {
    return __DATE__ " " __TIME__;
}

typedef enum {
    UP,
    DOWN,
    LEFT,
    RIGHT
}
Direction;

typedef struct {
    int x, y;
    int visited;
    int walls[4];
}
Cell;

void initGrid(Cell ** grid, int cellsX, int cellsY);
void freeGrid(Cell ** grid, int cellsX);
void removeWalls(Cell * current, Cell * next);
void drawGrid(SDL_Renderer * renderer, Cell ** grid, int cellsX, int cellsY);
Cell * getNeighbour(Cell ** grid, Cell * current, int cellsX, int cellsY);

int main(int argc, char * argv[]) {

    int steps = 1;

    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            if (strcmp(argv[i], "--steps") == 0 || strcmp(argv[i], "-s") == 0) {
                if (i + 1 < argc) {
                    if (strcmp(argv[i + 1], "instant") == 0) {
                        steps = CELLS_X * CELLS_Y - 1;
                    } else {
                        steps = atoi(argv[i + 1]);
                    }
                }
            } else if (strcmp(argv[i], "--cells") == 0 || strcmp(argv[i], "-c") == 0) {
                if (i + 2 < argc) {
                    CELLS_X = atoi(argv[i + 1]);
                    CELLS_Y = atoi(argv[i + 2]);
                }
            } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
                printf("\nOptions:\n");
                printf("  -s, --steps <steps>    Number of steps to perform (default: 1)\n");
                printf("                         Use 'instant' to generate complete maze at once\n");
                printf("  -c, --cells <x> <y>    Number of cells along X and Y axes (default: 20x20)\n");
                printf("  -v, --version          Show current version and build timestamp of the program\n");
                printf("  -h, --help             Show this help message\n");
                printf("\n");
                return 0; 
            } else if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0) {
                printf("Maze v%s\n", VERSION_STRING);
                printf("Build Timestamp: %s\n", get_build_timestamp());
                return 0;
            }
        }
    }

    CELL_SIZE = (SCREEN_WIDTH < SCREEN_HEIGHT ? SCREEN_WIDTH : SCREEN_HEIGHT) / CELLS_X;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window * window = SDL_CreateWindow("Maze", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer * renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    Cell ** grid = calloc(CELLS_X, sizeof(Cell * ));
    if (!grid) {
        printf("Failed to allocate memory for grid\n");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    for (int i = 0; i < CELLS_X; ++i) {
        grid[i] = calloc(CELLS_Y, sizeof(Cell));
        if (!grid[i]) {
            printf("Failed to allocate memory for grid[%d]\n", i);
            for (int j = 0; j < i; ++j) {
                free(grid[j]);
            }
            free(grid);
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            SDL_Quit();
            return 1;
        }
    }

    initGrid(grid, CELLS_X, CELLS_Y);

    Cell * current = & grid[0][0];
    current -> visited = 1;
    Cell * stack[CELLS_X * CELLS_Y];
    int stackSize = 0;
    srand(time(0));

    int quit = 0;
    SDL_Event e;

    int mazeGenerated = 0;

    while (!quit) {
        while (SDL_PollEvent( & e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = 1;
            }
        }

        if (!mazeGenerated) {
            int remainingSteps = steps;
            while (remainingSteps--> 0) {
                Cell * next = getNeighbour(grid, current, CELLS_X, CELLS_Y);
                if (next != NULL) {
                    next -> visited = 1;
                    stack[stackSize++] = current;
                    removeWalls(current, next);
                    current = next;
                } else if (stackSize > 0) {
                    current = stack[--stackSize];
                } else {
                    mazeGenerated = 1;
                    break;
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
        SDL_RenderClear(renderer);

        drawGrid(renderer, grid, CELLS_X, CELLS_Y);

        SDL_SetRenderDrawColor(renderer, 0x00, 0xFF, 0x00, 0xFF);
        SDL_Rect startRect = {
            0,
            0,
            CELL_SIZE,
            CELL_SIZE
        };
        SDL_RenderFillRect(renderer, & startRect);

        SDL_SetRenderDrawColor(renderer, 0xFF, 0x00, 0x00, 0xFF);
        SDL_Rect endRect = {
            (CELLS_X - 1) * CELL_SIZE,
            (CELLS_Y - 1) * CELL_SIZE,
            CELL_SIZE,
            CELL_SIZE
        };
        SDL_RenderFillRect(renderer, & endRect);

        SDL_RenderPresent(renderer);
    }
    
    freeGrid(grid, CELLS_X);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

void initGrid(Cell ** grid, int cellsX, int cellsY) {
    for (int x = 0; x < cellsX; x++) {
        for (int y = 0; y < cellsY; y++) {
            grid[x][y].x = x;
            grid[x][y].y = y;
            grid[x][y].visited = 0;
            memset(grid[x][y].walls, 1, sizeof(grid[x][y].walls));
        }
    }
}

void freeGrid(Cell ** grid, int cellsX) {
    for (int x = 0; x < cellsX; x++) {
        free(grid[x]);
    }
    free(grid);
}

void removeWalls(Cell * current, Cell * next) {
    int dx = next -> x - current -> x;
    int dy = next -> y - current -> y;

    if (dx == 1) {
        current -> walls[RIGHT] = 0;
        next -> walls[LEFT] = 0;
    } else if (dx == -1) {
        current -> walls[LEFT] = 0;
        next -> walls[RIGHT] = 0;
    }

    if (dy == 1) {
        current -> walls[DOWN] = 0;
        next -> walls[UP] = 0;
    } else if (dy == -1) {
        current -> walls[UP] = 0;
        next -> walls[DOWN] = 0;
    }
}

void drawGrid(SDL_Renderer * renderer, Cell ** grid, int cellsX, int cellsY) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF);

    int offsetX = (SCREEN_WIDTH - cellsX * CELL_SIZE) / 2;
    int offsetY = (SCREEN_HEIGHT - cellsY * CELL_SIZE) / 2;

    for (int x = 0; x < cellsX; x++) {
        for (int y = 0; y < cellsY; y++) {
            int x1 = offsetX + x * CELL_SIZE;
            int y1 = offsetY + y * CELL_SIZE;
            int x2 = x1 + CELL_SIZE;
            int y2 = y1 + CELL_SIZE;

            if (grid[x][y].walls[UP]) {
                SDL_RenderDrawLine(renderer, x1, y1, x2, y1);
            }
            if (grid[x][y].walls[DOWN]) {
                SDL_RenderDrawLine(renderer, x1, y2, x2, y2);
            }
            if (grid[x][y].walls[LEFT]) {
                SDL_RenderDrawLine(renderer, x1, y1, x1, y2);
            }
            if (grid[x][y].walls[RIGHT]) {
                SDL_RenderDrawLine(renderer, x2, y1, x2, y2);
            }
        }
    }
}

Cell * getNeighbour(Cell ** grid, Cell * current, int cellsX, int cellsY) {
    Cell * neighbours[4];
    int n = 0;

    if (current -> y > 0 && !grid[current -> x][current -> y - 1].visited) {
        neighbours[n++] = & grid[current -> x][current -> y - 1];
    }
    if (current -> y < cellsY - 1 && !grid[current -> x][current -> y + 1].visited) {
        neighbours[n++] = & grid[current -> x][current -> y + 1];
    }
    if (current -> x > 0 && !grid[current -> x - 1][current -> y].visited) {
        neighbours[n++] = & grid[current -> x - 1][current -> y];
    }
    if (current -> x < cellsX - 1 && !grid[current -> x + 1][current -> y].visited) {
        neighbours[n++] = & grid[current -> x + 1][current -> y];
    }

    return (n > 0) ? neighbours[rand() % n] : NULL;
}