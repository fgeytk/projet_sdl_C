#include "app.h"

#include "simulation.h"

#include <SDL3/SDL.h>

#include <stdint.h>
#include <stdio.h>

typedef struct SdlApp {
	Simulation simulation;
	SDL_Window *window;
	SDL_Renderer *renderer;
	uint64_t last_step_ticks;
	int running;
	int paused;
} SdlApp;

static void print_error_and_cleanup(const char *message, Simulation *simulation)
{
	fprintf(stderr, "%s\n", message);
	simulation_destroy(simulation);
}

static void update_title(SdlApp *app)
{
	char title[256];

	SDL_snprintf(title, sizeof(title),
		"Jeu de la vie | gen=%llu | seed=%u | %s | %d ms | %dx%d",
		(unsigned long long)app->simulation.generation,
		app->simulation.active_seed,
		app->paused ? "pause" : "run",
		app->simulation.config.step_ms,
		app->simulation.current.columns,
		app->simulation.current.rows);
	SDL_SetWindowTitle(app->window, title);
}

static void save_if_needed(Simulation *simulation)
{
	char error[256];

	if (!simulation->config.save_every_step) {
		return;
	}

	if (!simulation_save_snapshot(simulation, error, sizeof(error))) {
		fprintf(stderr, "%s\n", error);
	}
}

static void step_and_optionally_save(SdlApp *app)
{
	simulation_step(&app->simulation);
	save_if_needed(&app->simulation);
	update_title(app);
}

static void draw(const SdlApp *app)
{
	SDL_FRect rect;
	int y;

	SDL_SetRenderDrawColor(app->renderer, 12, 16, 22, 255);
	SDL_RenderClear(app->renderer);

	rect.w = (float)app->simulation.config.cell_size;
	rect.h = (float)app->simulation.config.cell_size;
	SDL_SetRenderDrawColor(app->renderer, 133, 244, 121, 255);

	for (y = 0; y < app->simulation.current.rows; ++y) {
		int x;
		rect.y = (float)(y * app->simulation.config.cell_size);
		for (x = 0; x < app->simulation.current.columns; ++x) {
			if (simulation_get_cell(&app->simulation, x, y) == 0u) {
				continue;
			}

			rect.x = (float)(x * app->simulation.config.cell_size);
			SDL_RenderFillRect(app->renderer, &rect);
		}
	}

	if (app->simulation.config.cell_size >= 5) {
		SDL_SetRenderDrawColor(app->renderer, 38, 48, 58, 255);
		for (y = 0; y <= app->simulation.current.columns; ++y) {
			int line_x = y * app->simulation.config.cell_size;
			SDL_RenderLine(app->renderer, (float)line_x, 0.0f, (float)line_x, (float)(app->simulation.current.rows * app->simulation.config.cell_size));
		}

		for (y = 0; y <= app->simulation.current.rows; ++y) {
			int line_y = y * app->simulation.config.cell_size;
			SDL_RenderLine(app->renderer, 0.0f, (float)line_y, (float)(app->simulation.current.columns * app->simulation.config.cell_size), (float)line_y);
		}
	}

	SDL_RenderPresent(app->renderer);
}

static void paint_cell(SdlApp *app, int pixel_x, int pixel_y, uint8_t value)
{
	int grid_x = pixel_x / app->simulation.config.cell_size;
	int grid_y = pixel_y / app->simulation.config.cell_size;

	simulation_set_cell(&app->simulation, grid_x, grid_y, value);
	update_title(app);
}

static void handle_key(SdlApp *app, SDL_Keycode key)
{
	char error[256];

	switch (key) {
	case SDLK_ESCAPE:
		app->running = 0;
		break;
	case SDLK_SPACE:
		app->paused = !app->paused;
		update_title(app);
		break;
	case SDLK_N:
		if (app->paused) {
			step_and_optionally_save(app);
		}
		break;
	case SDLK_G:
		if (!simulation_generate_next_world(&app->simulation, error, sizeof(error))) {
			fprintf(stderr, "%s\n", error);
		}
		update_title(app);
		break;
	case SDLK_R:
		if (!simulation_restart(&app->simulation, error, sizeof(error))) {
			fprintf(stderr, "%s\n", error);
		}
		update_title(app);
		break;
	case SDLK_C:
		simulation_clear(&app->simulation);
		update_title(app);
		break;
	case SDLK_S:
		if (!simulation_save_snapshot(&app->simulation, error, sizeof(error))) {
			fprintf(stderr, "%s\n", error);
		}
		break;
	case SDLK_UP:
		if (app->simulation.config.step_ms > 10) {
			app->simulation.config.step_ms -= 10;
			update_title(app);
		}
		break;
	case SDLK_DOWN:
		if (app->simulation.config.step_ms < 5000) {
			app->simulation.config.step_ms += 10;
			update_title(app);
		}
		break;
	default:
		break;
	}
}

static void handle_events(SdlApp *app)
{
	SDL_Event event;

	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_EVENT_QUIT:
			app->running = 0;
			break;
		case SDL_EVENT_KEY_DOWN:
			if (event.key.repeat == 0) {
				handle_key(app, event.key.key);
			}
			break;
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
			if (event.button.button == SDL_BUTTON_LEFT) {
				paint_cell(app, (int)event.button.x, (int)event.button.y, 1);
			} else if (event.button.button == SDL_BUTTON_RIGHT) {
				paint_cell(app, (int)event.button.x, (int)event.button.y, 0);
			}
			break;
		case SDL_EVENT_MOUSE_MOTION:
			if ((event.motion.state & SDL_BUTTON_MASK(SDL_BUTTON_LEFT)) != 0u) {
				paint_cell(app, (int)event.motion.x, (int)event.motion.y, 1);
			}
			if ((event.motion.state & SDL_BUTTON_MASK(SDL_BUTTON_RIGHT)) != 0u) {
				paint_cell(app, (int)event.motion.x, (int)event.motion.y, 0);
			}
			break;
		default:
			break;
		}
	}
}

static int run_headless(const SimulationConfig *config)
{
	Simulation simulation;
	char error[256];
	int step;

	if (config->max_steps <= 0) {
		fprintf(stderr, "Le mode headless requiert --max-steps strictement positif.\n");
		return 1;
	}

	if (!simulation_init(&simulation, config, error, sizeof(error))) {
		fprintf(stderr, "%s\n", error);
		return 1;
	}

	if (simulation.config.save_every_step && !simulation_save_snapshot(&simulation, error, sizeof(error))) {
		print_error_and_cleanup(error, &simulation);
		return 1;
	}

	for (step = 0; step < simulation.config.max_steps; ++step) {
		simulation_step(&simulation);
		if (simulation.config.save_every_step && !simulation_save_snapshot(&simulation, error, sizeof(error))) {
			print_error_and_cleanup(error, &simulation);
			return 1;
		}
	}

	printf("Simulation terminee: generation=%llu, taille=%dx%d\n",
		(unsigned long long)simulation.generation,
		simulation.current.columns,
		simulation.current.rows);
	simulation_destroy(&simulation);
	return 0;
}

static int run_sdl(const SimulationConfig *config)
{
	SdlApp app;
	char error[256];

	app.window = NULL;
	app.renderer = NULL;
	app.last_step_ticks = 0;
	app.running = 1;
	app.paused = 0;

	if (!simulation_init(&app.simulation, config, error, sizeof(error))) {
		fprintf(stderr, "%s\n", error);
		return 1;
	}

	if (!SDL_Init(SDL_INIT_VIDEO)) {
		fprintf(stderr, "SDL_Init a echoue: %s\n", SDL_GetError());
		simulation_destroy(&app.simulation);
		return 1;
	}

	app.window = SDL_CreateWindow(
		"Jeu de la vie",
		app.simulation.current.columns * app.simulation.config.cell_size,
		app.simulation.current.rows * app.simulation.config.cell_size,
		0);

	if (app.window == NULL) {
		fprintf(stderr, "Creation de la fenetre impossible: %s\n", SDL_GetError());
		SDL_Quit();
		simulation_destroy(&app.simulation);
		return 1;
	}

	app.renderer = SDL_CreateRenderer(app.window, NULL);
	if (app.renderer == NULL) {
		fprintf(stderr, "Creation du renderer impossible: %s\n", SDL_GetError());
		SDL_DestroyWindow(app.window);
		SDL_Quit();
		simulation_destroy(&app.simulation);
		return 1;
	}

	SDL_SetRenderVSync(app.renderer, 1);

	if (app.simulation.config.save_every_step && !simulation_save_snapshot(&app.simulation, error, sizeof(error))) {
		fprintf(stderr, "%s\n", error);
	}

	update_title(&app);
	app.last_step_ticks = SDL_GetTicks();

	while (app.running) {
		uint64_t now = SDL_GetTicks();
		handle_events(&app);

		if (!app.paused && now - app.last_step_ticks >= (uint32_t)app.simulation.config.step_ms) {
			step_and_optionally_save(&app);
			app.last_step_ticks = now;
		}

		draw(&app);
	}

	SDL_DestroyRenderer(app.renderer);
	SDL_DestroyWindow(app.window);
	SDL_Quit();
	simulation_destroy(&app.simulation);
	return 0;
}

int app_run(const SimulationConfig *config)
{
	if (config->headless) {
		return run_headless(config);
	}

	return run_sdl(config);
}