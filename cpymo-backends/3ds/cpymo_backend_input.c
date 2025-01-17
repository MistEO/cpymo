#include <cpymo_backend_input.h>
#include <3ds.h>
#include <stdlib.h>

bool cpymo_input_fast_kill_pressed = false;

cpymo_input cpymo_input_snapshot()
{
    cpymo_input out;
    u32 keys = hidKeysHeld();

    if((keys & KEY_ZL)) cpymo_input_fast_kill_pressed = true;
    
    out.ok = (keys & KEY_A) > 0;
    out.skip = (keys & KEY_R) > 0;
    out.cancel = (keys & KEY_B) > 0;
    out.hide_window = (keys & KEY_L) > 0;
    out.auto_mode = (keys & KEY_Y) > 0;
    out.down = (keys & KEY_DOWN) > 0;
    out.up = (keys & KEY_UP) > 0;
    out.left = (keys & KEY_LEFT) > 0;
    out.right = (keys & KEY_RIGHT) > 0;
    out.mouse_button = false;
    out.mouse_position_useable = false;
    out.mouse_wheel_delta = 0;

    circlePosition pos;
    hidCircleRead(&pos);

    if(pos.dy > 170) out.up = true;
    else if(pos.dy < -170) out.down = true;

    return out;
}
