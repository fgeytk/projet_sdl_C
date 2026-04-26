#include "config.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_COLUMNS 160
#define DEFAULT_ROWS 120
#define DEFAULT_CELL_SIZE 6
#define DEFAULT_STEP_MS 100
#define DEFAULT_DENSITY 28
#define DEFAULT_SEED 0x00C0FFEEu
#define DEFAULT_CONFIG_FILE "game_of_life.cfg"

static int ascii_casecmp(const char *left, const char *right)
{
	while (*left != '\0' && *right != '\0') {
		unsigned char a = (unsigned char)*left;
		unsigned char b = (unsigned char)*right;
		if (tolower(a) != tolower(b)) {
			return (int)tolower(a) - (int)tolower(b);
		}
		++left;
		++right;
	}

	return (int)(unsigned char)*left - (int)(unsigned char)*right;
}

static void set_error(char *error, size_t error_size, const char *message)
{
	if (error != NULL && error_size > 0) {
		snprintf(error, error_size, "%s", message);
	}
}

static void copy_string(char *destination, size_t capacity, const char *source)
{
	if (capacity == 0) {
		return;
	}

	snprintf(destination, capacity, "%s", source);
}

static char *trim_whitespace(char *text)
{
	char *end;

	while (*text != '\0' && isspace((unsigned char)*text)) {
		++text;
	}

	end = text + strlen(text);
	while (end > text && isspace((unsigned char)end[-1])) {
		--end;
	}
	*end = '\0';
	return text;
}

static int parse_int_value(const char *name, const char *value, int minimum, int maximum, int *target, char *error, size_t error_size)
{
	char *end = NULL;
	long parsed = strtol(value, &end, 10);

	if (value[0] == '\0' || end == value || *end != '\0' || parsed < minimum || parsed > maximum) {
		snprintf(error, error_size, "Valeur invalide pour %s: %s", name, value);
		return 0;
	}

	*target = (int)parsed;
	return 1;
}

static int parse_uint_value(const char *name, const char *value, unsigned int *target, char *error, size_t error_size)
{
	char *end = NULL;
	unsigned long parsed = strtoul(value, &end, 10);

	if (value[0] == '\0' || end == value || *end != '\0') {
		snprintf(error, error_size, "Valeur invalide pour %s: %s", name, value);
		return 0;
	}

	*target = (unsigned int)parsed;
	return 1;
}

static int parse_bool_value(const char *name, const char *value, int *target, char *error, size_t error_size)
{
	if (ascii_casecmp(value, "1") == 0 || ascii_casecmp(value, "true") == 0 || ascii_casecmp(value, "yes") == 0 || ascii_casecmp(value, "on") == 0) {
		*target = 1;
		return 1;
	}

	if (ascii_casecmp(value, "0") == 0 || ascii_casecmp(value, "false") == 0 || ascii_casecmp(value, "no") == 0 || ascii_casecmp(value, "off") == 0) {
		*target = 0;
		return 1;
	}

	snprintf(error, error_size, "Valeur booleenne invalide pour %s: %s", name, value);
	return 0;
}

static int file_exists(const char *path)
{
	FILE *file = fopen(path, "r");

	if (file == NULL) {
		return 0;
	}

	fclose(file);
	return 1;
}

static int apply_config_entry(SimulationConfig *config, const char *key, const char *value, char *error, size_t error_size)
{
	if (strcmp(key, "columns") == 0) {
		return parse_int_value(key, value, 8, 600, &config->columns, error, error_size);
	}

	if (strcmp(key, "rows") == 0) {
		return parse_int_value(key, value, 8, 400, &config->rows, error, error_size);
	}

	if (strcmp(key, "cell_size") == 0) {
		return parse_int_value(key, value, 2, 30, &config->cell_size, error, error_size);
	}

	if (strcmp(key, "step_ms") == 0) {
		return parse_int_value(key, value, 10, 5000, &config->step_ms, error, error_size);
	}

	if (strcmp(key, "density") == 0) {
		return parse_int_value(key, value, 0, 100, &config->density, error, error_size);
	}

	if (strcmp(key, "seed") == 0) {
		return parse_uint_value(key, value, &config->seed, error, error_size);
	}

	if (strcmp(key, "wrap_edges") == 0) {
		return parse_bool_value(key, value, &config->wrap_edges, error, error_size);
	}

	if (strcmp(key, "headless") == 0) {
		return parse_bool_value(key, value, &config->headless, error, error_size);
	}

	if (strcmp(key, "max_steps") == 0) {
		return parse_int_value(key, value, 0, 1000000, &config->max_steps, error, error_size);
	}

	if (strcmp(key, "save_every_step") == 0) {
		return parse_bool_value(key, value, &config->save_every_step, error, error_size);
	}

	if (strcmp(key, "initial_world") == 0) {
		copy_string(config->initial_world_path, sizeof(config->initial_world_path), value);
		return 1;
	}

	if (strcmp(key, "save_dir") == 0) {
		copy_string(config->save_dir, sizeof(config->save_dir), value);
		return 1;
	}

	snprintf(error, error_size, "Cle de configuration inconnue: %s", key);
	return 0;
}

static int load_config_file(const char *path, SimulationConfig *config, char *error, size_t error_size)
{
	FILE *file = fopen(path, "r");
	char line[1024];
	int line_number = 0;

	if (file == NULL) {
		snprintf(error, error_size, "Impossible d'ouvrir le fichier de configuration: %s", path);
		return 0;
	}

	while (fgets(line, sizeof(line), file) != NULL) {
		char *separator;
		char *key;
		char *value;

		++line_number;
		key = trim_whitespace(line);
		if (*key == '\0' || *key == ';' || (key[0] == '/' && key[1] == '/')) {
			continue;
		}

		separator = strchr(key, '=');
		if (separator == NULL) {
			snprintf(error, error_size, "Ligne invalide dans %s:%d", path, line_number);
			fclose(file);
			return 0;
		}

		*separator = '\0';
		value = trim_whitespace(separator + 1);
		key = trim_whitespace(key);

		if (!apply_config_entry(config, key, value, error, error_size)) {
			char detailed[256];
			snprintf(detailed, sizeof(detailed), "%s (dans %s:%d)", error, path, line_number);
			set_error(error, error_size, detailed);
			fclose(file);
			return 0;
		}
	}

	fclose(file);
	copy_string(config->config_path, sizeof(config->config_path), path);
	return 1;
}

void config_set_defaults(SimulationConfig *config)
{
	config->columns = DEFAULT_COLUMNS;
	config->rows = DEFAULT_ROWS;
	config->cell_size = DEFAULT_CELL_SIZE;
	config->step_ms = DEFAULT_STEP_MS;
	config->density = DEFAULT_DENSITY;
	config->seed = DEFAULT_SEED;
	config->wrap_edges = 1;
	config->headless = 0;
	config->max_steps = 0;
	config->save_every_step = 0;
	copy_string(config->config_path, sizeof(config->config_path), "");
	copy_string(config->initial_world_path, sizeof(config->initial_world_path), "");
	copy_string(config->save_dir, sizeof(config->save_dir), "states");
}

void config_print_usage(const char *program_name)
{
	fprintf(stderr,
		"Usage: %s [--config FILE] [--seed N] [--cols N] [--rows N] [--cell-size N] [--step-ms N]\n"
		"          [--density N] [--load FILE] [--save-dir DIR] [--save-every-step]\n"
		"          [--headless] [--max-steps N] [--no-wrap] [--help]\n",
		program_name);
}

ConfigParseResult config_parse_args(int argc, char **argv, SimulationConfig *config, char *error, size_t error_size)
{
	const char *config_path = NULL;
	int index;

	if (error != NULL && error_size > 0) {
		error[0] = '\0';
	}

	for (index = 1; index < argc; ++index) {
		if (strcmp(argv[index], "--help") == 0) {
			return CONFIG_PARSE_HELP;
		}

		if (strcmp(argv[index], "--config") == 0) {
			if (index + 1 >= argc) {
				set_error(error, error_size, "Option --config sans fichier");
				return CONFIG_PARSE_ERROR;
			}

			config_path = argv[++index];
		}
	}

	if (config_path != NULL) {
		if (!load_config_file(config_path, config, error, error_size)) {
			return CONFIG_PARSE_ERROR;
		}
	} else if (file_exists(DEFAULT_CONFIG_FILE)) {
		if (!load_config_file(DEFAULT_CONFIG_FILE, config, error, error_size)) {
			return CONFIG_PARSE_ERROR;
		}
	}

	for (index = 1; index < argc; ++index) {
		const char *arg = argv[index];

		if (strcmp(arg, "--config") == 0) {
			++index;
			continue;
		}

		if (strcmp(arg, "--seed") == 0 && index + 1 < argc) {
			if (!parse_uint_value(arg, argv[++index], &config->seed, error, error_size)) {
				return CONFIG_PARSE_ERROR;
			}
			continue;
		}

		if (strcmp(arg, "--cols") == 0 && index + 1 < argc) {
			if (!parse_int_value(arg, argv[++index], 8, 600, &config->columns, error, error_size)) {
				return CONFIG_PARSE_ERROR;
			}
			continue;
		}

		if (strcmp(arg, "--rows") == 0 && index + 1 < argc) {
			if (!parse_int_value(arg, argv[++index], 8, 400, &config->rows, error, error_size)) {
				return CONFIG_PARSE_ERROR;
			}
			continue;
		}

		if (strcmp(arg, "--cell-size") == 0 && index + 1 < argc) {
			if (!parse_int_value(arg, argv[++index], 2, 30, &config->cell_size, error, error_size)) {
				return CONFIG_PARSE_ERROR;
			}
			continue;
		}

		if (strcmp(arg, "--step-ms") == 0 && index + 1 < argc) {
			if (!parse_int_value(arg, argv[++index], 10, 5000, &config->step_ms, error, error_size)) {
				return CONFIG_PARSE_ERROR;
			}
			continue;
		}

		if (strcmp(arg, "--density") == 0 && index + 1 < argc) {
			if (!parse_int_value(arg, argv[++index], 0, 100, &config->density, error, error_size)) {
				return CONFIG_PARSE_ERROR;
			}
			continue;
		}

		if (strcmp(arg, "--load") == 0 && index + 1 < argc) {
			copy_string(config->initial_world_path, sizeof(config->initial_world_path), argv[++index]);
			continue;
		}

		if (strcmp(arg, "--save-dir") == 0 && index + 1 < argc) {
			copy_string(config->save_dir, sizeof(config->save_dir), argv[++index]);
			continue;
		}

		if (strcmp(arg, "--max-steps") == 0 && index + 1 < argc) {
			if (!parse_int_value(arg, argv[++index], 0, 1000000, &config->max_steps, error, error_size)) {
				return CONFIG_PARSE_ERROR;
			}
			continue;
		}

		if (strcmp(arg, "--save-every-step") == 0) {
			config->save_every_step = 1;
			continue;
		}

		if (strcmp(arg, "--headless") == 0) {
			config->headless = 1;
			continue;
		}

		if (strcmp(arg, "--no-wrap") == 0) {
			config->wrap_edges = 0;
			continue;
		}

		if (strcmp(arg, "--help") == 0) {
			continue;
		}

		snprintf(error, error_size, "Option inconnue ou incomplete: %s", arg);
		return CONFIG_PARSE_ERROR;
	}

	return CONFIG_PARSE_OK;
}