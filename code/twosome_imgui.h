#ifndef IMGUI_H
#define IMGUI_H

typedef Memory_Index UI_Id;


struct UI_Context
{
    UI_Id last_hot_or_active;
    
    UI_Id hot;
    UI_Id active;

    s32 mouse_x, mouse_y;
    b32 mouse_is_down;
    b32 mouse_went_up;

    f32 time;

    b32 mouse_inside_element;

    f32 time_entered_last_hot;

#if TWOSOME_SLOW
    UI_Id id_records[512];
    u32 id_records_count;
#endif
};

enum Menu_Button_Icon
{
    menu_button_icon_null = 0,
    menu_button_icon_resume_game,
    menu_button_icon_go_fullscreen,
    menu_button_icon_go_windowed,
    menu_button_icon_yes_confirm,
    menu_button_icon_no_confirm,
    menu_button_icon_restart_game,
    menu_button_icon_quit_game,
    menu_button_icon_quit_level,
    menu_button_icon_volume_speaker,
    menu_button_icon_volume_speaker_muted,
};

#define menu_button_size 100.0f
#define menu_button_half_size (menu_button_size/2.0f)
#define menu_button_spacing menu_button_half_size

global_variable Vec2 menu_button_scale = vec2(menu_button_size);

#endif
