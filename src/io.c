#include "io.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#define GOL_MKDIR(path) _mkdir(path)
#else
#include <sys/stat.h>
#include <sys/types.h>
#define GOL_MKDIR(path) mkdir(path, 0777)
#endif

static int is_alive_character(char character)
{
	return character == '#' || character == 'X' || character == 'x' || character == 'O' || character == 'o' || character == '1' || character == '*';
}

static int is_dead_character(char character)
{
	return character == '.' || character == '0' || character == '-' || character == ' ';
}

static int ensure_directory(const char *directory)
{
	char buffer[512];
	size_t length;
	size_t index;

	if (directory == NULL || directory[0] == '\0') {
		return 1;
	}

	length = strlen(directory);
	if (length >= sizeof(buffer)) {
		return 0;
	}

	snprintf(buffer, sizeof(buffer), "%s", directory);
	for (index = 1; index < length; ++index) {
		if (buffer[index] == '/' || buffer[index] == '\\') {
			char saved = buffer[index];
			buffer[index] = '\0';
			if (buffer[0] != '\0') {
				GOL_MKDIR(buffer);
			}
			buffer[index] = saved;
		}
	}

	GOL_MKDIR(buffer);
	return 1;
}

int io_load_world(const char *path, Grid *grid, char *error, size_t error_size)
{
	FILE *file = fopen(path, "r");
	char line[2048];
	int rows = 0;
	int columns = 0;
	Grid loaded;

	if (file == NULL) {
		snprintf(error, error_size, "Impossible d'ouvrir le monde initial: %s", path);
		return 0;
	}

	while (fgets(line, sizeof(line), file) != NULL) {
		size_t length = strcspn(line, "\r\n");
		if (length == 0) {
			continue;
		}
		if (line[0] == ';') {
			continue;
		}
		if ((int)length > columns) {
			columns = (int)length;
		}
		++rows;
	}

	if (rows == 0 || columns == 0) {
		fclose(file);
		snprintf(error, error_size, "Le monde initial est vide: %s", path);
		return 0;
	}

	rewind(file);
	if (!grid_init(&loaded, columns, rows)) {
		fclose(file);
		snprintf(error, error_size, "Allocation impossible pour le monde charge");
		return 0;
	}

	rows = 0;
	while (fgets(line, sizeof(line), file) != NULL) {
		size_t length = strcspn(line, "\r\n");
		int column;

		if (length == 0) {
			continue;
		}

		if (line[0] == ';') {
			continue;
		}

		for (column = 0; column < (int)length; ++column) {
			char current = line[column];
			if (is_alive_character(current)) {
				grid_set(&loaded, column, rows, 1);
			} else if (is_dead_character(current)) {
				grid_set(&loaded, column, rows, 0);
			} else {
				grid_destroy(&loaded);
				fclose(file);
				snprintf(error, error_size, "Caractere invalide '%c' dans %s", current, path);
				return 0;
			}
		}

		++rows;
	}

	fclose(file);
	grid_destroy(grid);
	*grid = loaded;
	return 1;
}

int io_save_world(const char *path, const Grid *grid, unsigned long long generation, char *error, size_t error_size)
{
	FILE *file = fopen(path, "w");
	int y;

	if (file == NULL) {
		snprintf(error, error_size, "Impossible d'ecrire le fichier: %s", path);
		return 0;
	}

	fprintf(file, "; generation=%llu\n", generation);
	fprintf(file, "; size=%dx%d\n", grid->columns, grid->rows);

	for (y = 0; y < grid->rows; ++y) {
		int x;
		for (x = 0; x < grid->columns; ++x) {
			fputc(grid_get(grid, x, y) != 0u ? '#' : '.', file);
		}
		fputc('\n', file);
	}

	fclose(file);
	return 1;
}

int io_save_generation(const char *directory, const Grid *grid, unsigned long long generation, char *error, size_t error_size)
{
	char path[768];

	if (!ensure_directory(directory)) {
		snprintf(error, error_size, "Impossible de creer le dossier de sauvegarde: %s", directory);
		return 0;
	}

	snprintf(path, sizeof(path), "%s/state_%06llu.txt", directory, generation);
	return io_save_world(path, grid, generation, error, error_size);
}