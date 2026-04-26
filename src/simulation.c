#include "simulation.h"

#include "io.h"

#include <stdio.h>
#include <string.h>

static uint32_t next_random(uint32_t *state)
{
	uint32_t value = *state;

	if (value == 0u) {
		value = 0x6D2B79F5u;
	}

	value ^= value << 13;
	value ^= value >> 17;
	value ^= value << 5;
	*state = value;
	return value;
}

static void set_error(char *error, size_t error_size, const char *message)
{
	if (error != NULL && error_size > 0) {
		snprintf(error, error_size, "%s", message);
	}
}

static void randomize_world(Simulation *simulation, uint32_t seed)
{
	int total_cells = simulation->current.columns * simulation->current.rows;
	int index;
	uint32_t state = seed;

	simulation->active_seed = seed;
	simulation->generation = 0;

	for (index = 0; index < total_cells; ++index) {
		simulation->current.cells[index] = (uint8_t)((next_random(&state) % 100u) < (uint32_t)simulation->config.density);
		simulation->next.cells[index] = 0;
	}
}

static int count_neighbors(const Simulation *simulation, int x, int y)
{
	int dx;
	int dy;
	int neighbors = 0;

	for (dy = -1; dy <= 1; ++dy) {
		for (dx = -1; dx <= 1; ++dx) {
			int sample_x = x + dx;
			int sample_y = y + dy;

			if (dx == 0 && dy == 0) {
				continue;
			}

			if (simulation->config.wrap_edges) {
				sample_x = (sample_x + simulation->current.columns) % simulation->current.columns;
				sample_y = (sample_y + simulation->current.rows) % simulation->current.rows;
				neighbors += grid_get(&simulation->current, sample_x, sample_y) != 0u;
			} else if (sample_x >= 0 && sample_x < simulation->current.columns && sample_y >= 0 && sample_y < simulation->current.rows) {
				neighbors += grid_get(&simulation->current, sample_x, sample_y) != 0u;
			}
		}
	}

	return neighbors;
}

static int load_initial_world_if_needed(Simulation *simulation, char *error, size_t error_size)
{
	if (simulation->config.initial_world_path[0] == '\0') {
		randomize_world(simulation, simulation->config.seed);
		return 1;
	}

	if (!io_load_world(simulation->config.initial_world_path, &simulation->current, error, error_size)) {
		return 0;
	}

	if (!grid_resize(&simulation->next, simulation->current.columns, simulation->current.rows)) {
		set_error(error, error_size, "Allocation impossible pour la grille secondaire");
		return 0;
	}

	grid_clear(&simulation->next);
	simulation->config.columns = simulation->current.columns;
	simulation->config.rows = simulation->current.rows;
	simulation->active_seed = simulation->config.seed;
	simulation->generation = 0;
	return 1;
}

int simulation_init(Simulation *simulation, const SimulationConfig *config, char *error, size_t error_size)
{
	memset(simulation, 0, sizeof(*simulation));
	simulation->config = *config;
	simulation->active_seed = config->seed;

	if (!grid_init(&simulation->current, config->columns, config->rows)) {
		set_error(error, error_size, "Impossible d'allouer la grille principale");
		return 0;
	}

	if (!grid_init(&simulation->next, config->columns, config->rows)) {
		grid_destroy(&simulation->current);
		set_error(error, error_size, "Impossible d'allouer la grille secondaire");
		return 0;
	}

	if (!load_initial_world_if_needed(simulation, error, error_size)) {
		simulation_destroy(simulation);
		return 0;
	}

	return 1;
}

void simulation_destroy(Simulation *simulation)
{
	grid_destroy(&simulation->current);
	grid_destroy(&simulation->next);
	memset(simulation, 0, sizeof(*simulation));
}

void simulation_step(Simulation *simulation)
{
	int y;

	for (y = 0; y < simulation->current.rows; ++y) {
		int x;
		for (x = 0; x < simulation->current.columns; ++x) {
			int neighbors = count_neighbors(simulation, x, y);
			uint8_t alive = grid_get(&simulation->current, x, y);
			grid_set(&simulation->next, x, y, (uint8_t)(neighbors == 3 || (alive != 0u && neighbors == 2)));
		}
	}

	{
		Grid swap = simulation->current;
		simulation->current = simulation->next;
		simulation->next = swap;
	}

	++simulation->generation;
}

void simulation_clear(Simulation *simulation)
{
	grid_clear(&simulation->current);
	grid_clear(&simulation->next);
	simulation->generation = 0;
}

int simulation_restart(Simulation *simulation, char *error, size_t error_size)
{
	if (simulation->config.initial_world_path[0] != '\0') {
		if (!io_load_world(simulation->config.initial_world_path, &simulation->current, error, error_size)) {
			return 0;
		}

		if (!grid_resize(&simulation->next, simulation->current.columns, simulation->current.rows)) {
			set_error(error, error_size, "Impossible de redimensionner la grille secondaire");
			return 0;
		}

		simulation->config.columns = simulation->current.columns;
		simulation->config.rows = simulation->current.rows;
		grid_clear(&simulation->next);
		simulation->generation = 0;
		return 1;
	}

	randomize_world(simulation, simulation->active_seed);
	return 1;
}

int simulation_generate_next_world(Simulation *simulation, char *error, size_t error_size)
{
	++simulation->active_seed;
	if (simulation->config.initial_world_path[0] != '\0') {
		simulation->config.seed = simulation->active_seed;
		return simulation_restart(simulation, error, error_size);
	}

	randomize_world(simulation, simulation->active_seed);
	return 1;
}

int simulation_save_snapshot(const Simulation *simulation, char *error, size_t error_size)
{
	return io_save_generation(simulation->config.save_dir, &simulation->current, (unsigned long long)simulation->generation, error, error_size);
}

void simulation_set_cell(Simulation *simulation, int x, int y, uint8_t value)
{
	if (x < 0 || x >= simulation->current.columns || y < 0 || y >= simulation->current.rows) {
		return;
	}

	grid_set(&simulation->current, x, y, value);
}

uint8_t simulation_get_cell(const Simulation *simulation, int x, int y)
{
	return grid_get(&simulation->current, x, y);
}