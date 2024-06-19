#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "version.h"

#define SCREEN_WIDTH 600
#define SCREEN_HEIGHT 600
#define CELL_SIZE 20
#define GRID_WIDTH (SCREEN_WIDTH / CELL_SIZE)
#define GRID_HEIGHT (SCREEN_HEIGHT / CELL_SIZE)

typedef enum { UP, DOWN, LEFT, RIGHT } Direction;

typedef struct {
    int x, y;
    int visited;
    int walls[4];
} Cell;

void initGrid(Cell grid[GRID_WIDTH][GRID_HEIGHT]);
void removeWalls(Cell *current, Cell *next);
void drawGrid(SDL_Renderer *renderer, Cell grid[GRID_WIDTH][GRID_HEIGHT]);
Cell* getNeighbour(Cell grid[GRID_WIDTH][GRID_HEIGHT], Cell *current);

const char* get_build_timestamp() {
    return __DATE__ " " __TIME__;
}

int main(int argc, char* argv[]) {

    int steps = 1;

    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            if (strcmp(argv[i], "--steps") == 0 || strcmp(argv[i], "-s") == 0) {
                if (i + 1 < argc) {
                    if (strcmp(argv[i + 1], "instant") == 0) {
                        steps = GRID_WIDTH * GRID_HEIGHT - 1;
                    } else {
                        steps = atoi(argv[i + 1]);
                    }
                }
            } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
                printf("\nOptions:\n");
                printf("  -s, --steps <steps>    Number of steps to perform (default: 1)\n");
                printf("                         Use 'instant' to generate complete maze at once\n");
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


    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Maze",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        SDL_WINDOW_SHOWN
    );

    if (window == NULL) {
        printf("Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    Cell grid[GRID_WIDTH][GRID_HEIGHT];
    initGrid(grid);

    Cell *current = &grid[0][0];
    current->visited = 1;
    Cell *stack[GRID_WIDTH * GRID_HEIGHT];
    int stackSize = 0;
    srand(time(0));

    int quit = 0;
    SDL_Event e;

    while (!quit) {

        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = 1;
            }
        }

        int remainingSteps = steps;
        while (remainingSteps-- > 0) {
            Cell *next = getNeighbour(grid, current);
            if (next != NULL) {
                next->visited = 1;
                stack[stackSize++] = current;
                removeWalls(current, next);
                current = next;
            } else if (stackSize > 0) {
                current = stack[--stackSize];
            } else {
                break;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
        SDL_RenderClear(renderer);

        drawGrid(renderer, grid);

        SDL_SetRenderDrawColor(renderer, 0x00, 0xFF, 0x00, 0xFF);
        SDL_Rect startRect = {0, 0, CELL_SIZE, CELL_SIZE};
        SDL_RenderFillRect(renderer, &startRect);

        SDL_SetRenderDrawColor(renderer, 0xFF, 0x00, 0x00, 0xFF);
        SDL_Rect endRect = { (GRID_WIDTH - 1) * CELL_SIZE, (GRID_HEIGHT - 1) * CELL_SIZE, CELL_SIZE, CELL_SIZE};
        SDL_RenderFillRect(renderer, &endRect);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

void initGrid(Cell grid[GRID_WIDTH][GRID_HEIGHT]) {
    for (int x = 0; x < GRID_WIDTH; x++) {
        for (int y = 0; y < GRID_HEIGHT; y++) {
            grid[x][y].x = x;
            grid[x][y].y = y;
            grid[x][y].visited = 0;
            grid[x][y].walls[UP] = 1;
            grid[x][y].walls[DOWN] = 1;
            grid[x][y].walls[LEFT] = 1;
            grid[x][y].walls[RIGHT] = 1;
        }
    }
}

void removeWalls(Cell *current, Cell *next) {
    int dx = next->x - current->x;
    int dy = next->y - current->y;

    if (dx == 1) {
        current->walls[RIGHT] = 0;
        next->walls[LEFT] = 0;
    } else if (dx == -1) {
        current->walls[LEFT] = 0;
        next->walls[RIGHT] = 0;
    }

    if (dy == 1) {
        current->walls[DOWN] = 0;
        next->walls[UP] = 0;
    } else if (dy == -1) {
        current->walls[UP] = 0;
        next->walls[DOWN] = 0;
    }
}

void drawGrid(SDL_Renderer *renderer, Cell grid[GRID_WIDTH][GRID_HEIGHT]) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF);

    for (int x = 0; x < GRID_WIDTH; x++) {
        for (int y = 0; y < GRID_HEIGHT; y++) {
            int x1 = x * CELL_SIZE;
            int y1 = y * CELL_SIZE;
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

Cell* getNeighbour(Cell grid[GRID_WIDTH][GRID_HEIGHT], Cell *current) {
    Cell *neighbours[4];
    int n = 0;

    if (current->y > 0 && !grid[current->x][current->y - 1].visited) {
        neighbours[n++] = &grid[current->x][current->y - 1];
    }
    if (current->y < GRID_HEIGHT - 1 && !grid[current->x][current->y + 1].visited) {
        neighbours[n++] = &grid[current->x][current->y + 1];
    }
    if (current->x > 0 && !grid[current->x - 1][current->y].visited) {
        neighbours[n++] = &grid[current->x - 1][current->y];
    }
    if (current->x < GRID_WIDTH - 1 && !grid[current->x + 1][current->y].visited) {
        neighbours[n++] = &grid[current->x + 1][current->y];
    }

    if (n > 0) {
        return neighbours[rand() % n];
    } else {
        return NULL;
    }
}
