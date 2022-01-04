#include <cpymo_backend_image.h>
#include <cpymo_engine.h>
#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>

static float offset_3d(enum cpymo_backend_image_draw_type type)
{
    switch(type) {
        case cpymo_backend_image_draw_type_bg: return -10.0f;
        case cpymo_backend_image_draw_type_chara: return -5.0f;
        default: return 0.0f;
    }
}

const extern float render_3d_offset;

static float game_width, game_height;
static float viewport_width, viewport_height;
static float offset_x, offset_y;

void cpymo_backend_image_init(float game_w, float game_h)
{
    game_width = game_w;
    game_height = game_h;

    float ratio_w = game_w / 400;
    float ratio_h = game_h / 240;

    if(ratio_w > ratio_h) {
        viewport_width = 400;
        viewport_height = game_h / ratio_w;
    } 
    else {
        viewport_width = game_w / ratio_h;
        viewport_height = 240;
    }

    offset_x = 400 / 2 - viewport_width / 2;
    offset_y = 240 / 2 - viewport_height / 2;
}

static void trans_size(float *w, float *h) {
    *w = *w / game_width * viewport_width;
    *h = *h / game_height * viewport_height;
}

static void trans_pos(float *x, float *y) {
    *x = *x / game_width * viewport_width + offset_x;
    *y = *y / game_height * viewport_height + offset_y;
}

void cpymo_backend_image_fill_rects(
	const float *xywh, size_t count,
	cpymo_color color, float alpha,
	enum cpymo_backend_image_draw_type draw_type)
{
    for(size_t i = 0; i < count; ++i) {
        float x = xywh[i * 4];
        float y = xywh[i * 4 + 1];
        float w = xywh[i * 4 + 2];
        float h = xywh[i * 4 + 3];

        trans_pos(&x, &y);
        trans_size(&w, &h);

        x += offset_3d(draw_type) * render_3d_offset;
        C2D_DrawRectSolid(x, y, 0.0, w, h, C2D_Color32(color.r, color.g, color.b, (u8)(255 * alpha)));
    }
}
