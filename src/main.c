#include <SDL3/SDL_main.h>

#include "app.h"
#include "config.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
	SimulationConfig config;
	ConfigParseResult result;
	char error[256];

	config_set_defaults(&config);
	result = config_parse_args(argc, argv, &config, error, sizeof(error));

	if (result == CONFIG_PARSE_HELP) {
		config_print_usage(argv[0]);
		return EXIT_SUCCESS;
	}

	if (result == CONFIG_PARSE_ERROR) {
		if (error[0] != '\0') {
			fprintf(stderr, "%s\n", error);
		}
		config_print_usage(argv[0]);
		return EXIT_FAILURE;
	}

	return app_run(&config);
}
