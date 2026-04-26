#ifndef SIMULATION_H
#define SIMULATION_H

#include "config.h"
#include "world.h"

#include <stddef.h>
#include <stdint.h>

typedef struct Simulation {
	SimulationConfig config;
	Grid current;
	Grid next;
	uint64_t generation;
	uint32_t active_seed;
} Simulation;

int simulation_init(Simulation *simulation, const SimulationConfig *config, char *error, size_t error_size);
void simulation_destroy(Simulation *simulation);
void simulation_step(Simulation *simulation);
void simulation_clear(Simulation *simulation);
int simulation_restart(Simulation *simulation, char *error, size_t error_size);
int simulation_generate_next_world(Simulation *simulation, char *error, size_t error_size);
int simulation_save_snapshot(const Simulation *simulation, char *error, size_t error_size);
void simulation_set_cell(Simulation *simulation, int x, int y, uint8_t value);
uint8_t simulation_get_cell(const Simulation *simulation, int x, int y);

#endif