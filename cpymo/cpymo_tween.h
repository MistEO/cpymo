#ifndef INCLUDE_CPYMO_TWEEN
#define INCLUDE_CPYMO_TWEEN

#include <stdbool.h>
#include "cpymo_utils.h"

typedef struct {
	float all_time, current_time;
	float begin_value, end_value;
} cpymo_tween;

static inline cpymo_tween cpymo_tween_create_value(float all_time, float begin_value, float end_value)
{
	cpymo_tween t;
	t.all_time = all_time;
	t.current_time = 0;
	t.begin_value = begin_value;
	t.end_value = end_value;
	return t;
}

static inline cpymo_tween cpymo_tween_create(float all_time)
{ return cpymo_tween_create_value(all_time, 0, 1); }

static inline void cpymo_tween_update(cpymo_tween *tween, float time)
{ tween->current_time += time; }

static inline float cpymo_tween_progress(const cpymo_tween *tween)
{ return cpymo_utils_clampf(tween->current_time / tween->all_time, 0.0f, 1.0f); }

static inline float cpymo_tween_value(const cpymo_tween *tween)
{ return cpymo_utils_lerp(tween->begin_value, tween->end_value, cpymo_tween_progress(tween)); }

static inline void cpymo_tween_to(cpymo_tween *tween, float target, float time)
{ *tween = cpymo_tween_create_value(time, cpymo_tween_value(tween), target); }

static inline bool cpymo_tween_finished(const cpymo_tween *tween)
{ return tween->current_time >= tween->all_time; }

static inline void cpymo_tween_finish(cpymo_tween *tween)
{ tween->current_time = tween->all_time; }

#endif