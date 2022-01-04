#ifndef INCLUDE_CPYMO_ENGINE
#define INCLUDE_CPYMO_ENGINE

#include <cpymo_backend_input.h>
#include "cpymo_assetloader.h"
#include "cpymo_gameconfig.h"
#include "cpymo_error.h"
#include "cpymo_interpreter.h"
#include "cpymo_vars.h"
#include "cpymo_wait.h"

struct cpymo_engine {
	cpymo_gameconfig gameconfig;
	cpymo_assetloader assetloader;
	cpymo_vars vars;
	cpymo_interpreter *interpreter;
	cpymo_input prev_input, input;
	cpymo_wait wait;
	bool skipping;
	char *title;
};

typedef struct cpymo_engine cpymo_engine;

error_t cpymo_engine_init(cpymo_engine *out, const char *gamedir);
void cpymo_engine_free(cpymo_engine *engine);
error_t cpymo_engine_update(cpymo_engine *engine, float delta_time_sec, bool *redraw);
void cpymo_engine_draw(cpymo_engine *engine);

static inline bool cpymo_engine_skipping(cpymo_engine *engine)
{
	return engine->input.skip || engine->skipping;
}

#define CPYMO_INPUT_JUST_PRESSED(PENGINE, KEY) \
	(!PENGINE->prev_input.KEY && PENGINE->input.KEY)

#endif

