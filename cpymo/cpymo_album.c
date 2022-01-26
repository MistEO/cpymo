#include <stb_image_resize.h>
#include <stb_image_write.h>
#include <stb_image.h>
#include <memory.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include "cpymo_error.h"
#include "cpymo_parser.h"
#include <cpymo_backend_image.h>
#include "cpymo_assetloader.h"
#include "cpymo_engine.h"

static error_t cpymo_album_generate_album_ui_image(
	cpymo_backend_image *out_image, 
	cpymo_parser_stream_span album_list_text, 
	cpymo_parser_stream_span output_cache_ui_file_name,
	size_t page, 
	cpymo_assetloader* loader,
	size_t *ref_w, size_t *ref_h)
{
	stbi_uc *pixels = NULL;

	{
		char *image_buf = NULL;
		size_t image_buf_size = 0;
		error_t err = cpymo_assetloader_load_system(&image_buf, &image_buf_size, "albumbg", "png", loader);
		if (err == CPYMO_ERR_SUCC) {
			int w2, h2, channels;
			pixels = stbi_load_from_memory((stbi_uc *)image_buf, (int)image_buf_size, &w2, &h2, &channels, 3);
			free(image_buf);

			if (pixels != NULL) {
				*ref_w = (size_t)w2;
				*ref_h = (size_t)h2;
			}
		}

		if (pixels == NULL) {
			pixels = (stbi_uc *)malloc(*ref_w * *ref_h * 3);
			if (pixels == NULL) return CPYMO_ERR_OUT_OF_MEM;
			memset(pixels, 0, *ref_w * *ref_h * 3);
		}
	}

	cpymo_parser album_list_parser;
	cpymo_parser_init(&album_list_parser, album_list_text.begin, album_list_text.len);

	size_t next_id = 0;

	const size_t thumb_width = (size_t)(0.17 * (double)*ref_w);
	const size_t thumb_height = (size_t)(0.17 * (double)*ref_h);

	do {
		cpymo_parser_stream_span page_str = cpymo_parser_curline_pop_commacell(&album_list_parser);
		cpymo_parser_stream_span_trim(&page_str);

		if (page_str.len <= 0) continue;
		if (cpymo_parser_stream_span_atoi(page_str) != (int)page + 1) continue;

		const size_t id = next_id++;

		cpymo_parser_stream_span thumb_count_str = 
			cpymo_parser_curline_pop_commacell(&album_list_parser);
		cpymo_parser_stream_span_trim(&thumb_count_str);
		if (thumb_count_str.len <= 0) continue;
		const int thumb_count = cpymo_parser_stream_span_atoi(thumb_count_str);

		if (thumb_count < 1) continue;

		cpymo_parser_curline_pop_commacell(&album_list_parser);	// discard CG name.

		const size_t col = id % 5;
		const size_t row = id / 5;
		const size_t thumb_left_top_x = (size_t)ceil((0.03 + 0.19 * col) * (double)*ref_w);
		const size_t thumb_left_top_y = (size_t)ceil((0.02 + 0.19 * row) * (double)*ref_h);
		const size_t thumb_right_bottom_x = thumb_left_top_x + thumb_width;
		const size_t thumb_right_bottom_y = thumb_left_top_y + thumb_height;

		if (thumb_right_bottom_x >= *ref_w || thumb_right_bottom_y >= *ref_h) continue;

		int cg_w, cg_h;
		stbi_uc *thumb_pixels = NULL;
		for (int i = 0; i < thumb_count; ++i) {
			// Try load ONE thumb.
			char bgname[64];
			cpymo_parser_stream_span bgname_span = 
				cpymo_parser_curline_pop_commacell(&album_list_parser);
			cpymo_parser_stream_span_trim(&bgname_span);
			if (bgname_span.len <= 0) continue;
			cpymo_parser_stream_span_copy(bgname, sizeof(bgname), bgname_span);

			char *buf = NULL;
			size_t buf_size = 0;
			error_t err = cpymo_assetloader_load_bg(&buf, &buf_size, bgname, loader);
			if (err != CPYMO_ERR_SUCC) continue;

			int cg_channels;
			thumb_pixels = stbi_load_from_memory((stbi_uc *)buf, (int)buf_size, &cg_w, &cg_h, &cg_channels, 3);
			free(buf);

			if (thumb_pixels != NULL) break;
		}

		if (thumb_pixels == NULL) continue;

		stbir_resize_uint8(
			thumb_pixels, cg_w, cg_h, cg_w * 3, 
			pixels + 3 * thumb_left_top_y * *ref_w + 3 * thumb_left_top_x, 
			(int)thumb_width, (int)thumb_height, (int)*ref_w * 3, 3);
		free(thumb_pixels);
	} while (cpymo_parser_next_line(&album_list_parser));

	if (cpymo_backend_image_album_ui_writable()) {
		char *path = (char *)malloc(strlen(loader->gamedir) + output_cache_ui_file_name.len + 32);
		if (path != NULL) {
			strcpy(path, loader->gamedir);
			strcat(path, "/system/");
			strncat(path, output_cache_ui_file_name.begin, output_cache_ui_file_name.len);
			strcat(path, "_");
			char page_str[4];
			snprintf(page_str, sizeof(page_str), "%d", (int)page);
			strcat(path, page_str);
			strcat(path, ".png");
			stbi_write_png(path, (int)*ref_w, (int)*ref_h, 3, pixels, (int)*ref_w * 3);
			free(path);
		}
	}

	error_t err = cpymo_backend_image_load(out_image, pixels, (int)*ref_w, (int)*ref_h, cpymo_backend_image_format_rgb);
	if (err != CPYMO_ERR_SUCC) {
		free(pixels);
		return err;
	}

	return CPYMO_ERR_SUCC;
}

static error_t cpymo_album_load_ui_image(
	cpymo_backend_image *out_image,
	cpymo_parser_stream_span album_list_text,
	cpymo_parser_stream_span ui_file_name,
	size_t page,
	cpymo_assetloader *loader,
	size_t *ref_w, size_t *ref_h) 
{
	char *assetname = (char *)malloc(ui_file_name.len + 16);
	if (assetname == NULL) return CPYMO_ERR_OUT_OF_MEM;

	cpymo_parser_stream_span_copy(assetname, ui_file_name.len + 16, ui_file_name);
	char page_str[4];
	snprintf(page_str, sizeof(page_str), "%d", (int)page);
	strcat(assetname, "_");
	strcat(assetname, page_str);

	char *buf = NULL;
	size_t buf_size = 0;
	error_t err = cpymo_assetloader_load_system(&buf, &buf_size, assetname, "png", loader);
	free(assetname);

	if (err != CPYMO_ERR_SUCC) {
		GENERATE:
		return cpymo_album_generate_album_ui_image(
			out_image, album_list_text, ui_file_name, page, loader, ref_w, ref_h);
	}
	else {
		int w, h, c;
		stbi_uc *px = stbi_load_from_memory((stbi_uc *)buf, (int)buf_size, &w, &h, &c, 3);
		free(buf);

		if (px == NULL) goto GENERATE;
		else {
			*ref_w = (size_t)w;
			*ref_h = (size_t)h;
			return cpymo_backend_image_load(out_image, px, w, h, cpymo_backend_image_format_rgb);
		}
	}

}

typedef struct {
	cpymo_backend_image current_ui;
	size_t current_ui_w, current_ui_h;

	size_t current_page, page_count, cg_count;
	cpymo_parser_stream_span album_list_name, album_ui_name;

	int current_cg_selection;

	char *album_list_text;
	size_t album_list_text_size;

	float mouse_wheel_sum;
} cpymo_album;


static error_t cpymo_album_load_page(cpymo_engine *e, cpymo_album *a)
{
	cpymo_engine_request_redraw(e);

	if (a->current_ui != NULL) {
		cpymo_backend_image_free(a->current_ui);
		a->current_ui = NULL;
	}

	a->current_ui_w = e->gameconfig.imagesize_w;
	a->current_ui_h = e->gameconfig.imagesize_h;

	a->cg_count = 0;
	a->current_cg_selection = 0;

	cpymo_parser album_list;
	cpymo_parser_init(&album_list, a->album_list_text, a->album_list_text_size);
	do {
		cpymo_parser_stream_span page_str = cpymo_parser_curline_pop_commacell(&album_list);
		cpymo_parser_stream_span_trim(&page_str);

		if (page_str.len <= 0) continue;
		if (cpymo_parser_stream_span_atoi(page_str) != (int)a->current_page + 1) continue;

		a->cg_count++;

		cpymo_parser_stream_span thumb_count_str =
			cpymo_parser_curline_pop_commacell(&album_list);
		cpymo_parser_stream_span_trim(&thumb_count_str);

		if (thumb_count_str.len <= 0) continue;
		const int thumb_count = cpymo_parser_stream_span_atoi(thumb_count_str);

		if (thumb_count < 1) continue;

		/*cpymo_parser_stream_span cg_name =
			cpymo_parser_curline_pop_commacell(&album_list);*/


	} while (cpymo_parser_next_line(&album_list));

	cpymo_parser_stream_span span;
	span.begin = a->album_list_text;
	span.len = a->album_list_text_size;

	error_t err = cpymo_album_load_ui_image(
		&a->current_ui,
		span,
		a->album_ui_name,
		a->current_page,
		&e->assetloader,
		&a->current_ui_w,
		&a->current_ui_h);

	return err;
}

static error_t cpymo_album_next_page(cpymo_engine *e, cpymo_album *a)
{
	size_t prev_page = a->current_page;
	a->current_page = (a->current_page + 1) % a->page_count;
	if (prev_page != a->current_page) 
		return cpymo_album_load_page(e, a);
	else {
		a->current_cg_selection = 0;
		return CPYMO_ERR_SUCC;
	}
}

static error_t cpymo_album_prev_page(cpymo_engine *e, cpymo_album *a)
{
	size_t prev_page = a->current_page;
	if (a->current_page <= 0) a->current_page = a->page_count - 1;
	else a->current_page--;

	if (prev_page != a->current_page) return cpymo_album_load_page(e, a);
	else return CPYMO_ERR_SUCC;
}

static error_t cpymo_album_update(cpymo_engine *e, void *a, float dt)
{
	if (CPYMO_INPUT_JUST_PRESSED(e, cancel)) {
		cpymo_ui_exit(e);
		return CPYMO_ERR_SUCC;
	}

	cpymo_album *album = (cpymo_album *)a;

	if (CPYMO_INPUT_JUST_PRESSED(e, left)) return cpymo_album_prev_page(e, album);
	if (CPYMO_INPUT_JUST_PRESSED(e, right)) return cpymo_album_next_page(e, album);

	if (CPYMO_INPUT_JUST_PRESSED(e, down)) {
		album->current_cg_selection++;
		cpymo_engine_request_redraw(e);

		if (album->current_cg_selection >= (int)album->cg_count)
			return cpymo_album_next_page(e, album);
		return CPYMO_ERR_SUCC;
	}

	if (CPYMO_INPUT_JUST_PRESSED(e, up)) {
		cpymo_engine_request_redraw(e);
		album->current_cg_selection--;
		if (album->current_cg_selection < 0) {
			error_t err = cpymo_album_prev_page(e, album);
			album->current_cg_selection = (int)album->cg_count - 1;
			return err;
		}
		return CPYMO_ERR_SUCC;
	}

	album->mouse_wheel_sum += e->input.mouse_wheel_delta;

	if (album->mouse_wheel_sum >= 1.0f) {
		album->mouse_wheel_sum -= 1.0f;
		return cpymo_album_prev_page(e, album);
	}

	if (album->mouse_wheel_sum <= -1.0f) {
		album->mouse_wheel_sum += 1.0f;
		return cpymo_album_next_page(e, album);
	}

	if (e->prev_input.mouse_position_useable && e->input.mouse_position_useable) {
		if (e->prev_input.mouse_x != e->input.mouse_x || e->prev_input.mouse_y != e->input.mouse_y) {
			for (size_t i = 0; i < album->cg_count; ++i) {
				int row = i / 5;
				int col = i % 5;
				float 
					x = (float)e->gameconfig.imagesize_w * (0.03f + 0.19f * col),
					y = (float)e->gameconfig.imagesize_h * (0.02f + 0.19f * row),
					w = (float)e->gameconfig.imagesize_w * 0.17f,
					h = (float)e->gameconfig.imagesize_h * 0.17f,
					mx = e->input.mouse_x,
					my = e->input.mouse_y;

				float x2 = x + w, y2 = y + h;

				if (mx >= x && mx <= x2 && my >= y && my <= y2) {
					cpymo_engine_request_redraw(e);
					album->current_cg_selection = (int)i;
					break;
				}
			}
		}
	}

	return CPYMO_ERR_SUCC;
}

static void cpymo_album_draw(const cpymo_engine *e, const void *_a)
{
	const cpymo_album *a = (const cpymo_album *)_a;
	
	if (a->current_ui) {
		cpymo_backend_image_draw(
			0, 0, e->gameconfig.imagesize_w, e->gameconfig.imagesize_h,
			a->current_ui, 0, 0, (int)a->current_ui_w, (int)a->current_ui_h,
			1.0f, cpymo_backend_image_draw_type_bg);
	}

	// Draw Selection Rect
	int row = a->current_cg_selection / 5;
	int col = a->current_cg_selection % 5;
	float xywh[] = {
		(float)e->gameconfig.imagesize_w * (0.03f + 0.19f * col),
		(float)e->gameconfig.imagesize_h * (0.02f + 0.19f * row),
		(float)e->gameconfig.imagesize_w * 0.17f,
		(float)e->gameconfig.imagesize_h * 0.17f
	};
	cpymo_backend_image_fill_rects(xywh, 1, cpymo_color_white, 0.5f, cpymo_backend_image_draw_type_bg);

}

static void cpymo_album_deleter(cpymo_engine *e, void *a)
{
	cpymo_album *album = (cpymo_album *)a;
	if (album->album_list_text) free(album->album_list_text);
	if (album->current_ui) cpymo_backend_image_free(album->current_ui);
}

error_t cpymo_album_enter(
	cpymo_engine *e, 
	cpymo_parser_stream_span album_list_name, 
	cpymo_parser_stream_span album_ui_name,
	size_t page)
{
	cpymo_album *album = NULL;
	error_t err = cpymo_ui_enter(
		(void **)&album, 
		e, 
		sizeof(cpymo_album),
		&cpymo_album_update,
		&cpymo_album_draw, 
		&cpymo_album_deleter);
	CPYMO_THROW(err);

	album->album_list_name = album_list_name;
	album->album_ui_name = album_ui_name;
	album->current_ui = NULL;
	album->current_page = page;
	album->page_count = 0;
	album->mouse_wheel_sum = 0;

	char script_name[128];
	cpymo_parser_stream_span_copy(script_name, sizeof(script_name), album_list_name);

	album->album_list_text = NULL;
	err = cpymo_assetloader_load_script(
		&album->album_list_text,
		&album->album_list_text_size, 
		script_name, &e->assetloader);

	cpymo_parser parser;
	cpymo_parser_init(&parser, album->album_list_text, album->album_list_text_size);
	do {
		cpymo_parser_stream_span span = cpymo_parser_curline_pop_commacell(&parser);
		cpymo_parser_stream_span_trim(&span);
		if (span.len > 0) {
			size_t page_id = (size_t)cpymo_parser_stream_span_atoi(span);
			if (page_id > album->page_count) album->page_count = page_id;
		}
	} while (cpymo_parser_next_line(&parser));

	CPYMO_THROW(err);

	return cpymo_album_load_page(e, album);
}
