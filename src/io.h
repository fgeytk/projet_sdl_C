#ifndef IO_H
#define IO_H

#include "world.h"

#include <stddef.h>

int io_load_world(const char *path, Grid *grid, char *error, size_t error_size);
int io_save_world(const char *path, const Grid *grid, unsigned long long generation, char *error, size_t error_size);
int io_save_generation(const char *directory, const Grid *grid, unsigned long long generation, char *error, size_t error_size);

#endif