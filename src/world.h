#ifndef WORLD_H
#define WORLD_H

#include <stdint.h>

typedef struct Grid {
	int columns;
	int rows;
	uint8_t *cells;
} Grid;

int grid_init(Grid *grid, int columns, int rows);
int grid_resize(Grid *grid, int columns, int rows);
void grid_destroy(Grid *grid);
void grid_clear(Grid *grid);
int grid_index(const Grid *grid, int x, int y);
uint8_t grid_get(const Grid *grid, int x, int y);
void grid_set(Grid *grid, int x, int y, uint8_t value);

#endif