#include "world.h"

#include <stdlib.h>
#include <string.h>

int grid_init(Grid *grid, int columns, int rows)
{
	size_t cell_count;

	grid->columns = 0;
	grid->rows = 0;
	grid->cells = NULL;

	if (columns <= 0 || rows <= 0) {
		return 0;
	}

	cell_count = (size_t)columns * (size_t)rows;
	grid->cells = (uint8_t *)calloc(cell_count, sizeof(*grid->cells));
	if (grid->cells == NULL) {
		return 0;
	}

	grid->columns = columns;
	grid->rows = rows;
	return 1;
}

int grid_resize(Grid *grid, int columns, int rows)
{
	Grid resized;

	if (!grid_init(&resized, columns, rows)) {
		return 0;
	}

	grid_destroy(grid);
	*grid = resized;
	return 1;
}

void grid_destroy(Grid *grid)
{
	free(grid->cells);
	grid->cells = NULL;
	grid->columns = 0;
	grid->rows = 0;
}

void grid_clear(Grid *grid)
{
	if (grid->cells != NULL) {
		memset(grid->cells, 0, (size_t)grid->columns * (size_t)grid->rows);
	}
}

int grid_index(const Grid *grid, int x, int y)
{
	return y * grid->columns + x;
}

uint8_t grid_get(const Grid *grid, int x, int y)
{
	return grid->cells[grid_index(grid, x, y)];
}

void grid_set(Grid *grid, int x, int y, uint8_t value)
{
	grid->cells[grid_index(grid, x, y)] = (uint8_t)(value != 0u);
}