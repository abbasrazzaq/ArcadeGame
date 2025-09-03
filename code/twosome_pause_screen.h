#ifndef TWOSOME_PAUSE_SCREEN_H
#define TWOSOME_PAUSE_SCREEN_H


#define pause_screen_confirmation_icon_flash_time 1.0f
#define pause_screen_second_confirmation_icon_flash_time 0.75f

enum Pause_Screen_Update_Action
{
    pause_screen_update_action_null = 0,
    pause_screen_update_action_toggle_fullscreen,
    pause_screen_update_action_quit_game,
    pause_screen_update_action_restart_game,
};

enum Pause_Screen_Mode
{
    pause_mode_main = 0,
    pause_mode_quit_level_confirmation,
    pause_mode_restart_game_confirmation,
    pause_mode_restart_game_second_confirmation,
    pause_mode_quit_game_confirmation,
    pause_mode_awaiting_game_level_resume,
    pause_mode_count
};

enum Pause_Center_Button
{
    pause_center_button_resume,
    pause_center_button_toggle_fullscreen,
    pause_center_button_quit,
    pause_center_buttons_count
};

enum Pause_Screen_Pre_Update_Toggle_Action
{
    pause_screen_pre_update_toggle_action_null = 0,
    pause_screen_pre_update_toggle_action_resume,
    pause_screen_pre_update_toggle_action_quit_level,
};

struct Pause_Screen
{
    b32 active;
    s32 mode;

    Playing_Sound *master_volume_test_sound;
    f32 master_volume_setting;

    Flashing_Value confirmation_icon_flash;

    f32 volume_slider_cursor_y;

    Vec2 cursor_position_before_pause;
    
    u32 center_buttons[pause_center_buttons_count];

    // NOTE: When we're toggling pause from the pause screen, we kinda want it to happen
    // the next frame, before it's started rendering the pause screen so we use this
    u32 pre_update_toggle_action;
};

struct Volume_Slider
{
    Vec2 button_position;
    Vec2 button_scale;
    Vec2 slider_scale;
    Vec2 cursor_scale;

    // NOTE: The spine part, with markings for low, medium and hight volume
    Vec2 slider_bar_pos;
    Vec2 slider_bar_scale;
    f32 slider_bar_mark_height;
    f32 slider_bar_mark_max_width;

    Vec2 border_scale;
    Vec2 border_position;
};

#endif
