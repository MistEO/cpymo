#include "cpymo_say.h"
#include "cpymo_engine.h"
#include <assert.h>

#define DISABLE_TEXTBOX(SAY) \
	if (SAY->textbox_usable) { \
		cpymo_textbox_free(&(SAY)->textbox); \
		SAY->textbox_usable = false; \
	}

#define ENABLE_TEXTBOX(SAY, X, Y, W, H, FONT_SIZE, COL, TEXT, ERR) \
	{ \
		DISABLE_TEXTBOX(SAY); \
		ERR = cpymo_textbox_init( \
			&(SAY)->textbox, X, Y, W, H, FONT_SIZE, COL, TEXT); \
		if (ERR == CPYMO_ERR_SUCC) SAY->textbox_usable = true; \
	}

#define RESET_NAME(SAY) \
	if (SAY->name) { cpymo_backend_text_free(SAY->name); SAY->name = NULL; }

static void cpymo_say_lazy_init(cpymo_say *out, cpymo_assetloader *loader)
{
	if (out->lazy_init == false) {
		out->lazy_init = true;

		error_t err;
		err = cpymo_say_load_msgbox_image(out, cpymo_parser_stream_span_pure("message"), loader);
		if (err != CPYMO_ERR_SUCC) out->msgbox = NULL;

		err = cpymo_say_load_namebox_image(out, cpymo_parser_stream_span_pure("name"), loader);
		if (err != CPYMO_ERR_SUCC) out->namebox = NULL;

		err = cpymo_assetloader_load_system_image(
			&out->msg_cursor,
			&out->msg_cursor_w,
			&out->msg_cursor_h,
			cpymo_parser_stream_span_pure("message_cursor"),
			"png",
			loader,
			cpymo_gameconfig_is_symbian(loader->game_config));

		if (err != CPYMO_ERR_SUCC) out->msg_cursor = NULL;
	}
}

void cpymo_say_init(cpymo_say *out)
{
	out->msgbox = NULL;
	out->namebox = NULL;
	out->msg_cursor = NULL;
	out->active = false;
	out->lazy_init = false;
	out->textbox_usable = false;
	out->name = NULL;
}

void cpymo_say_free(cpymo_say *say)
{
	DISABLE_TEXTBOX(say);
	RESET_NAME(say);
	if (say->msgbox) cpymo_backend_image_free(say->msgbox);
	if (say->namebox) cpymo_backend_image_free(say->namebox);
	if (say->msg_cursor) cpymo_backend_image_free(say->msg_cursor);
}

void cpymo_say_draw(const struct cpymo_engine *e)
{
	if (e->say.active && !e->input.hide_window) {
		float ratio = (float)e->gameconfig.imagesize_w / (float)e->say.msgbox_w;
		float msg_h = (float)e->say.msgbox_h * ratio;
		float y = (float)e->gameconfig.imagesize_h - msg_h;
		float offx = (float)e->gameconfig.nameboxorg_x / 540.0f * (float)e->gameconfig.imagesize_w;
		float offy = (float)e->gameconfig.nameboxorg_y / 360.0f * (float)e->gameconfig.imagesize_h;

		float fontsize = cpymo_gameconfig_font_size(&e->gameconfig);
		float namebox_h = fontsize * 1.4f;
		float namebox_w = (float)e->say.namebox_w / ((float)e->say.namebox_h / namebox_h);

		float namebox_x;
		switch (e->gameconfig.namealign) {
		case 0:
			namebox_x = (float)(e->gameconfig.imagesize_w - namebox_w) / 2;
			break;
		case 2:
			namebox_x = (float)(e->gameconfig.imagesize_w - namebox_w);
			break;
		default:
			namebox_x = 0;
			break;
		};

		float namebox_y = y - namebox_h;

		namebox_x += offx;
		namebox_y -= offy;

		if (e->say.name && e->say.namebox)
			cpymo_backend_image_draw(
				namebox_x, namebox_y, namebox_w, namebox_h,
				e->say.namebox, 0, 0, e->say.namebox_w, e->say.namebox_h, 1.0f,
				cpymo_backend_image_draw_type_textbox);

		if (e->say.msgbox) {
			cpymo_backend_image_draw(
				0, y, (float)e->gameconfig.imagesize_w, msg_h,
				e->say.msgbox, 0, 0, e->say.msgbox_w, e->say.msgbox_h,
				1.0f, cpymo_backend_image_draw_type_textbox);
		}

		if (e->say.name) {
			float name_x =
				namebox_w / 2 - (float)e->say.name_width / 2 + namebox_x;

			cpymo_backend_text_draw(
				e->say.name,
				name_x, namebox_y + cpymo_gameconfig_font_size(&e->gameconfig),
				e->gameconfig.textcolor, 1.0f, cpymo_backend_image_draw_type_text_say);
		}

		if (e->say.textbox_usable) {
			cpymo_textbox_draw(e, &e->say.textbox, cpymo_backend_image_draw_type_text_say);
		}
	}
}

error_t cpymo_say_load_msgbox_image(cpymo_say *say, cpymo_parser_stream_span name, cpymo_assetloader *l)
{
	cpymo_say_lazy_init(say, l);

	if (say->msgbox) cpymo_backend_image_free(say->msgbox);
	say->msgbox = NULL;

	error_t err = cpymo_assetloader_load_system_image(
		&say->msgbox,
		&say->msgbox_w,
		&say->msgbox_h,
		name,
		"png",
		l,
		cpymo_gameconfig_is_symbian(l->game_config));

	if (err != CPYMO_ERR_SUCC) say->msgbox = NULL;

	return err;
}

error_t cpymo_say_load_namebox_image(cpymo_say *say, cpymo_parser_stream_span name, cpymo_assetloader *l)
{
	cpymo_say_lazy_init(say, l);

	if (say->namebox) cpymo_backend_image_free(say->namebox);
	say->namebox = NULL;

	error_t err = cpymo_assetloader_load_system_image(
		&say->namebox,
		&say->namebox_w,
		&say->namebox_h,
		name,
		"png",
		l,
		cpymo_gameconfig_is_symbian(l->game_config));

	if (err != CPYMO_ERR_SUCC) say->namebox = NULL;

	return err;
}

/** Mermaid Flow Chart 

graph TB
	A(START) --> cpymo_say_wait_text_fadein
	cpymo_say_wait_text_fadein --> cpymo_say_wait_text_fadein_callback
	cpymo_say_wait_text_fadein_callback --> cpymo_say_wait_text_reading
	cpymo_say_wait_text_reading --> cpymo_say_wait_text_read
	cpymo_say_wait_text_read --> cpymo_say_wait_text_read_callback
	cpymo_say_wait_text_read_callback --> cpymo_say_wait_text_fadein
	cpymo_say_wait_text_read_callback --> E(END)

 **/

static error_t cpymo_say_wait_text_fadein_callback(cpymo_engine *e);

static bool cpymo_say_wait_text_reading(cpymo_engine *e, float dt)
{
	assert(e->say.textbox_usable);
	return cpymo_textbox_wait_text_reading(e, dt, &e->say.textbox);
}

static bool cpymo_say_wait_text_fadein(cpymo_engine *e, float dt)
{
	assert(e->say.textbox_usable);
	return cpymo_textbox_wait_text_fadein(e, dt, &e->say.textbox);
}

static error_t cpymo_say_wait_text_read_callback(cpymo_engine *e)
{
	cpymo_say *say = &e->say;

	assert(say->textbox_usable);

	if (!cpymo_textbox_all_finished(&say->textbox)) {
		cpymo_textbox_clear_page(&say->textbox);
		cpymo_wait_register_with_callback(
			&e->wait,
			&cpymo_say_wait_text_fadein,
			&cpymo_say_wait_text_fadein_callback);
		cpymo_engine_request_redraw(e);
	}
	else {
		DISABLE_TEXTBOX(say);
		say->active = false;
	}

	return CPYMO_ERR_SUCC;
}

static error_t cpymo_say_wait_text_fadein_callback(cpymo_engine *e)
{
	cpymo_wait_register_with_callback(
		&e->wait, 
		&cpymo_say_wait_text_reading,
		&cpymo_say_wait_text_read_callback);
	return CPYMO_ERR_SUCC;
}

error_t cpymo_say_start(struct cpymo_engine *e, cpymo_parser_stream_span name, cpymo_parser_stream_span text)
{
	cpymo_say_lazy_init(&e->say, &e->assetloader);

	float fontsize = cpymo_gameconfig_font_size(&e->gameconfig);

	// Create name box
	cpymo_say *say = &e->say;
	RESET_NAME(say);

	cpymo_parser_stream_span_trim(&name);
	if (name.len > 0) {
		error_t err = cpymo_backend_text_create(&say->name, &say->name_width, name, fontsize);
		if (err != CPYMO_ERR_SUCC) say->name = NULL;
	}

	// Create say message text
	float msglr_l = (float)e->gameconfig.msglr_l * e->gameconfig.imagesize_w / 540.0f;
	float msglr_r = (float)e->gameconfig.msglr_r * e->gameconfig.imagesize_w / 540.0f;
	float msgtb_t = (float)e->gameconfig.msgtb_t * e->gameconfig.imagesize_h / 360.0f;
	float msgtb_b = (float)e->gameconfig.msgtb_b * e->gameconfig.imagesize_h / 360.0f;

	float ratio = (float)e->gameconfig.imagesize_w / (float)e->say.msgbox_w;
	float msg_h = (float)e->say.msgbox_h * ratio;

	float x = msglr_l;
	float w = (float)e->gameconfig.imagesize_w - msglr_l - msglr_r;
	float y = (float)e->gameconfig.imagesize_h - msg_h + msgtb_t;
	float h = msg_h - msgtb_t - msgtb_b;

	error_t err;
	ENABLE_TEXTBOX(say, x, y, w, h, fontsize, e->gameconfig.textcolor, text, err);

	cpymo_engine_request_redraw(e);

	e->say.active = true;

	cpymo_wait_register_with_callback(
		&e->wait, 
		&cpymo_say_wait_text_fadein, 
		&cpymo_say_wait_text_fadein_callback);

	return err;
}
