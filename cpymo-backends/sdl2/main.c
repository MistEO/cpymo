﻿#include <stdlib.h>
#include <stdio.h>
#include <cpymo_error.h>
#include <SDL.h>
#include <cpymo_engine.h>
#include <cpymo_interpreter.h>
#include <string.h>
#include <cpymo_backend_text.h>

#define STBI_NO_PSD
#define STBI_NO_TGA
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define FASTEST_FILTER STBIR_FILTER_BOX
#define STBIR_DEFAULT_FILTER_DOWNSAMPLE  FASTEST_FILTER
#define STBIR_DEFAULT_FILTER_UPSAMPLE    FASTEST_FILTER
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#if _WIN32 && !NDEBUG
#include <crtdbg.h>
#endif

#ifdef __APPLE__
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include "posix_win32.h"

SDL_Window *window;
SDL_Renderer *renderer;
cpymo_engine engine;

extern error_t cpymo_backend_font_init(const char *gamedir);
extern void cpymo_backend_font_free();

static void set_window_icon(const char *gamedir) 
{
	int w, h, channel;
	char *icon_path = (char *)alloca(strlen(gamedir) + 10);
	if (icon_path == NULL) return;

	sprintf(icon_path, "%s/icon.png", gamedir);

	stbi_uc *icon = stbi_load(icon_path, &w, &h, &channel, 4);
	//free(icon_path);
	if (icon == NULL) return;

	SDL_Surface *surface =
		SDL_CreateRGBSurfaceFrom(icon, w, h, channel * 8, channel * w, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);

	if (surface == NULL) {
		stbi_image_free(icon);
		return;
	}

	SDL_SetWindowIcon(window, surface);
	SDL_FreeSurface(surface);
	stbi_image_free(icon);
}

static void ensure_save_dir(const char *gamedir)
{
	char *save_dir = (char *)alloca(strlen(gamedir) + 8);
	sprintf(save_dir, "%s/save", gamedir);
	mkdir(save_dir, 0777);
}

int main(int argc, char **argv) 
{
	int ret = 0;
	const char *gamedir = "./";

	if (argc == 2) {
		gamedir = argv[1];
	}

	ensure_save_dir(gamedir);

	error_t err = cpymo_engine_init(&engine, gamedir);

	if (err != CPYMO_ERR_SUCC) {
		SDL_Log("[Error] cpymo_engine_init (%s)", cpymo_error_message(err));
		return -1;
	}

	// SDL2 has memory leak in alloc id 113 and 114(or 112)!!!
	if (SDL_Init(
		SDL_INIT_EVENTS |
		SDL_INIT_AUDIO |
		SDL_INIT_VIDEO) != 0) {
		cpymo_engine_free(&engine);
		SDL_Log("[Error] Unable to initialize SDL: %s", SDL_GetError());
		return -1;
	}

	SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
	SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");
	SDL_SetHint(SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "0");

#if !NDEBUG
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
#endif

	if (SDL_CreateWindowAndRenderer(
		engine.gameconfig.imagesize_w,
		engine.gameconfig.imagesize_h,
		SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE,
		&window,
		&renderer) != 0) {
		cpymo_engine_free(&engine);
		SDL_Log("[Error] Can not create window and renderer: %s", SDL_GetError());
		SDL_Quit();
		return -1;
	}

	SDL_SetWindowTitle(window, engine.gameconfig.gametitle);
	set_window_icon(gamedir);
	
	if (SDL_RenderSetLogicalSize(renderer, engine.gameconfig.imagesize_w, engine.gameconfig.imagesize_h) != 0) {
		SDL_Log("[Error] Can not set logical size: %s", SDL_GetError());
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		cpymo_engine_free(&engine);
		SDL_Quit();
		return -1;
	}

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

	if (cpymo_backend_font_init(gamedir) != CPYMO_ERR_SUCC) {
		SDL_Log("[Error] Can not find font file, try put default.ttf into <gamedir>/system/.");
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		cpymo_engine_free(&engine);
		SDL_Quit();
		return -1;
	}

	Uint32 prev_ticks = SDL_GetTicks();
	SDL_Event event;
	while (1) {
		bool redraw_by_event = false;

		extern float mouse_wheel;
		mouse_wheel = 0;

		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT)
				goto EXIT;
			else if (event.type == SDL_WINDOWEVENT)
				redraw_by_event = true;
			else if (event.type == SDL_MOUSEWHEEL) {
				mouse_wheel = (float)event.wheel.y;
				if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED)
					mouse_wheel *= -1;
			}
		}

		bool need_to_redraw = false;

		Uint32 ticks = SDL_GetTicks();
		err = cpymo_engine_update(
			&engine, 
			(float)(ticks - prev_ticks) * 0.001f, 
			&need_to_redraw);

		switch (err) {
		case CPYMO_ERR_SUCC: break;
		case CPYMO_ERR_NO_MORE_CONTENT: goto EXIT;
		default: {
			SDL_Log("[Error] %s.", cpymo_error_message(err));
			ret = -1;
			goto EXIT;
		}
		}

		prev_ticks = ticks;

		if (need_to_redraw || redraw_by_event) {
			SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
			SDL_RenderClear(renderer);
			cpymo_engine_draw(&engine);
			SDL_RenderPresent(renderer);
		} else SDL_Delay(16);
	}

	EXIT:
	cpymo_engine_free(&engine);
	cpymo_backend_font_free();

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	SDL_Quit();


	#if _WIN32 && !NDEBUG
	_CrtDumpMemoryLeaks();
	#endif

	return ret;
}
