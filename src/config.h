#ifndef CONFIG_H
#define CONFIG_H

#include <stddef.h>

#define GOL_PATH_CAPACITY 512

typedef struct SimulationConfig {
	int columns;
	int rows;
	int cell_size;
	int step_ms;
	int density;
	unsigned int seed;
	int wrap_edges;
	int headless;
	int max_steps;
	int save_every_step;
	char config_path[GOL_PATH_CAPACITY];
	char initial_world_path[GOL_PATH_CAPACITY];
	char save_dir[GOL_PATH_CAPACITY];
} SimulationConfig;

typedef enum ConfigParseResult {
	CONFIG_PARSE_OK,
	CONFIG_PARSE_HELP,
	CONFIG_PARSE_ERROR
} ConfigParseResult;

void config_set_defaults(SimulationConfig *config);
ConfigParseResult config_parse_args(int argc, char **argv, SimulationConfig *config, char *error, size_t error_size);
void config_print_usage(const char *program_name);

#endif