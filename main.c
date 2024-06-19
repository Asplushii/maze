#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
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

const char * get_build_timestamp() {
    return __DATE__ " "
    __TIME__;
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
int saveMazeAsPNG(SDL_Renderer * renderer,
    const char * filePath);

void generateMazeEasy(Cell ** grid, int cellsX, int cellsY, int steps);
void generateMazeMedium(Cell ** grid, int cellsX, int cellsY);
void generateMazeHard(Cell ** grid, int cellsX, int cellsY);

int main(int argc, char * argv[]) {
    int steps = 1215752192;
    int save = 0;
    char * difficulty = "easy";

    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            if (strcmp(argv[i], "--cells") == 0 || strcmp(argv[i], "-c") == 0) {
                if (i + 2 < argc) {
                    CELLS_X = atoi(argv[i + 1]);
                    CELLS_Y = atoi(argv[i + 2]);
                }
            } else if (strcmp(argv[i], "--difficulty") == 0 || strcmp(argv[i], "-d") == 0) {
                if (i + 1 < argc) {
                    difficulty = argv[i + 1];
                }
            } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
                printf("\nOptions:\n");
                printf("  -c, --cells <x> <y>          Number of cells along X and Y axes (default: 20x20)\n");
                printf("  -d, --difficulty <level>     Difficulty level: easy, medium, hard (default: easy)\n");
                printf("  -v, --version                Show current version and build timestamp of the program\n");
                printf("  -h, --help                   Show this help message\n");
                printf("  --save                       Save the maze as a PNG file\n");
                printf("\n");
                return 0;
            } else if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0) {
                printf("Maze v%s\n", VERSION_STRING);
                printf("Build Timestamp: %s\n", get_build_timestamp());
                return 0;
            } else if (strcmp(argv[i], "--save") == 0) {
                save = 1;
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

    srand(time(0));

    if (strcmp(difficulty, "easy") == 0) {
        generateMazeEasy(grid, CELLS_X, CELLS_Y, steps);
    } else if (strcmp(difficulty, "medium") == 0) {
        generateMazeMedium(grid, CELLS_X, CELLS_Y);
    } else if (strcmp(difficulty, "hard") == 0) {
        generateMazeHard(grid, CELLS_X, CELLS_Y);
    } else {
        printf("Unknown difficulty level: %s\n", difficulty);
        freeGrid(grid, CELLS_X);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    int quit = 0;
    SDL_Event e;

    while (!quit) {
        while (SDL_PollEvent( & e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = 1;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
        SDL_RenderClear(renderer);

        drawGrid(renderer, grid, CELLS_X, CELLS_Y);
        SDL_RenderPresent(renderer);
    }

    if (save) {
        time_t t = time(NULL);
        char filePath[64];
        snprintf(filePath, sizeof(filePath), "%ld.png", t);
        if (saveMazeAsPNG(renderer, filePath) != 0) {
            printf("Failed to save the maze as a PNG file\n");
        } else {
            printf("Maze saved as %s\n", filePath);
        }
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
    int offsetX = (SCREEN_WIDTH - cellsX * CELL_SIZE) / 2;
    int offsetY = (SCREEN_HEIGHT - cellsY * CELL_SIZE) / 2;

    for (int x = 0; x < cellsX; x++) {
        for (int y = 0; y < cellsY; y++) {
            int x1 = offsetX + x * CELL_SIZE;
            int y1 = offsetY + y * CELL_SIZE;
            int x2 = x1 + CELL_SIZE;
            int y2 = y1 + CELL_SIZE;

            if (x == 0 && y == 0) {
                SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
                SDL_Rect rect = {
                    x1,
                    y1,
                    CELL_SIZE,
                    CELL_SIZE
                };
                SDL_RenderFillRect(renderer, & rect);
            } else if (x == cellsX - 1 && y == cellsY - 1) {
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                SDL_Rect rect = {
                    x1,
                    y1,
                    CELL_SIZE,
                    CELL_SIZE
                };
                SDL_RenderFillRect(renderer, & rect);
            }

            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

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

void generateMazeEasy(Cell ** grid, int cellsX, int cellsY, int steps) {
    Cell * current = & grid[0][0];
    current -> visited = 1;
    Cell * stack[cellsX * cellsY];
    int stackSize = 0;

    int remainingSteps = steps;
    while (remainingSteps--> 0) {
        Cell * next = getNeighbour(grid, current, cellsX, cellsY);
        if (next != NULL) {
            next -> visited = 1;
            stack[stackSize++] = current;
            removeWalls(current, next);
            current = next;
        } else if (stackSize > 0) {
            current = stack[--stackSize];
        } else {
            break;
        }
    }
}

void generateMazeMedium(Cell ** grid, int cellsX, int cellsY) {
    Cell * current = & grid[rand() % cellsX][rand() % cellsY];
    current -> visited = 1;

    while (1) {
        Cell * next = getNeighbour(grid, current, cellsX, cellsY);
        if (next != NULL) {
            removeWalls(current, next);
            next -> visited = 1;
            current = next;
        } else {
            int found = 0;
            for (int x = 0; x < cellsX; ++x) {
                for (int y = 0; y < cellsY; ++y) {
                    if (!grid[x][y].visited && getNeighbour(grid, & grid[x][y], cellsX, cellsY) != NULL) {
                        current = & grid[x][y];
                        Cell * next = getNeighbour(grid, current, cellsX, cellsY);
                        removeWalls(current, next);
                        next -> visited = 1;
                        current = next;
                        found = 1;
                        break;
                    }
                }
                if (found) break;
            }
            if (!found) break;
        }
    }
}

typedef struct {
    int x1, y1, x2, y2;
}
Edge;

int find(int * parent, int i) {
    while (parent[i] != i)
        i = parent[i];
    return i;
}

void unionSets(int * parent, int * rank, int x, int y) {
    int rootX = find(parent, x);
    int rootY = find(parent, y);
    if (rank[rootX] < rank[rootY]) {
        parent[rootX] = rootY;
    } else if (rank[rootX] > rank[rootY]) {
        parent[rootY] = rootX;
    } else {
        parent[rootY] = rootX;
        rank[rootX]++;
    }
}

void generateMazeHard(Cell ** grid, int cellsX, int cellsY) {
    int numCells = cellsX * cellsY;
    int * parent = malloc(numCells * sizeof(int));
    int * rank = malloc(numCells * sizeof(int));
    for (int i = 0; i < numCells; ++i) {
        parent[i] = i;
        rank[i] = 0;
    }

    Edge edges[numCells * 4];
    int edgeCount = 0;
    for (int x = 0; x < cellsX; ++x) {
        for (int y = 0; y < cellsY; ++y) {
            if (x > 0) edges[edgeCount++] = (Edge) {
                x,
                y,
                x - 1,
                y
            };
            if (y > 0) edges[edgeCount++] = (Edge) {
                x,
                y,
                x,
                y - 1
            };
        }
    }

    for (int i = 0; i < edgeCount; ++i) {
        int j = rand() % edgeCount;
        Edge temp = edges[i];
        edges[i] = edges[j];
        edges[j] = temp;
    }

    for (int i = 0; i < edgeCount; ++i) {
        int x1 = edges[i].x1;
        int y1 = edges[i].y1;
        int x2 = edges[i].x2;
        int y2 = edges[i].y2;
        int set1 = find(parent, x1 * cellsY + y1);
        int set2 = find(parent, x2 * cellsY + y2);
        if (set1 != set2) {
            removeWalls( & grid[x1][y1], & grid[x2][y2]);
            unionSets(parent, rank, set1, set2);
        }
    }

    free(parent);
    free(rank);
}

int saveMazeAsPNG(SDL_Renderer * renderer,
    const char * filePath) {
    int width, height;
    SDL_GetRendererOutputSize(renderer, & width, & height);

    SDL_Surface * surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA32);
    if (!surface) {
        printf("Failed to create surface: %s\n", SDL_GetError());
        return -1;
    }

    if (SDL_RenderReadPixels(renderer, NULL, surface -> format -> format, surface -> pixels, surface -> pitch) != 0) {
        printf("Failed to read pixels: %s\n", SDL_GetError());
        SDL_FreeSurface(surface);
        return -1;
    }

    if (IMG_SavePNG(surface, filePath) != 0) {
        printf("Failed to save PNG: %s\n", IMG_GetError());
        SDL_FreeSurface(surface);
        return -1;
    }

    SDL_FreeSurface(surface);
    return 0;
}