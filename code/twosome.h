#ifndef TWOSOME_H
#define TWOSOME_H


/*   
   TODO:
   
   ================================================ Win32 ========================================
   //
   // Bugs:
   //
   = On laptop, if you fullscreen, you can't alt-tab away, or press windows key to see taskbar.
   
   = Title bar icon disappears if started in fullscreen and then go to windowed. Not fixing: to do with when we're
   setting fullscreen (if set after ShowWindow then it works, but get weird expanding window thing)
   = Look into this error: "Keyboard input came in through non-dispatch message!"
   
   = Happens omsetimes with ui active/hotness, where the cursor is not over ui element but is shown as hot: made it happen by alt-tabbing to game
   -- REPLICATION: Turn off constraining, hover over level select, then click (off window), and ui will stay
   hot even when moving cursor off.   
   
   //
   // Tests:
   //
   = What happens when dsound failed to initialize (just run reaper first)
    -- Does the getting of sound samples make sense?
    
   =================================================================================================
   
   //
   // Bugs:
   //
   
   //
   // Polish:
   //
   = Add readme
    -- Section on what to do for startup failed
    
   //
   // Non-Win32 Platforms:
   //
   = Will need to draw different control hints based on device it's on (PC & Mac vs iOS & Android)
    - No right click on Mac, maybe switch to SPACE bar activating shield
 */


#include "twosome_platform.h"
#include "twosome_debug.h"
#include "twosome_shared.h"

#include "twosome_math.h"
#include "twosome_render_group.h"
#include "twosome_imgui.h"
#include "twosome_asset.h"
#include "twosome_audio.h"
#include "twosome_collision.h"
#include "twosome_game_level.h"
#include "twosome_animation.h"
#include "twosome_game_hub.h"
#include "twosome_pause_screen.h"

#define ui_window_edge_offset 10.0f
#define stage_transition_sound_fade_duration 0.5f

#define game_mode_transition_time 0.375f
#define game_mode_transition_fade_out_direction -1
#define game_mode_transition_fade_in_direction 1

#define game_clear_colour vec3(0.1f)

enum Game_Mode_Result_Action
{
    game_mode_result_action_null = 0,
    game_mode_result_action_save,
    // NOTE: The game is saved on mode switch too
    game_mode_result_action_switch_mode,
};

struct Game_Mode_Result
{
    s32 action;
    
    // NOTE: Hub to Level & Back:
    s32 level_type;
    // NOTE: Hub to Level
    u32 level_score_to_get;
};

enum Game_Mode
{
    game_mode_loading_assets,
    game_mode_hub,
    game_mode_playing_level
};

#pragma pack(push, 1)

// NOTE: In case we had to support different save versions
struct Game_Save_Header
{
    u16 version;
    // NOTE: Indicates whether the save file is a dummy one
    // (used for clearing out game save on restart)
    b32 filled;
};

struct Game_Save
{
    Game_Save_Header header;
    
    u32 best_level_scores[number_of_levels];
    
    s32 levels_unlocked;
    s32 levels_completed;
    
    s32 hub_state;
    Vec3 girl_side_background_colour;
    
    b32 boy_has_sang;
    Vec2 boy_face_position;
    u32 boy_idle_anim;
    b32 boy_flashing;
    f32 boy_flash_time_for_one_flash;
    
    Vec2 girl_face_position;
    f32 girl_face_rot_t;
    u32 girl_face_movement_flags;
    b32 girl_flashing;
    f32 girl_flash_time_for_one_flash;
};
#pragma pack(pop)

struct Cursor
{
    union
    {
        struct
        {
            f32 x, y;
        };
        
        Vec2 position;
    };
    
    Vec3 colour;
};

struct Game_Progress
{
    u32 best_level_scores[number_of_levels];
    s32 levels_unlocked;
    s32 levels_completed;
};

struct Game_State
{ 
    bool32 is_initialized;
    
    Memory_Arena mode_arena;
    Memory_Arena audio_arena;
    
    Game_Audio audio;
    Cursor cursor;
    
    int game_mode;
    Pause_Screen pause_screen;
    Game_Level level;
    Game_Hub hub;
    
    Game_Progress progress;
    
    UI_Context ui_context;
    
    RNG rng;
    
    u64 total_frame_count;
    u64 playing_frame_count;
    f32 gametime;
    
    f32 game_mode_transition_t;
    s32 game_mode_transition_direction;
    f32 game_loading_transition_colour_t;
    
    Game_Mode_Result game_mode_transition_mode_result;
    
#if TWOSOME_INTERNAL
    Playing_Sound *test_sound;
    Overlapping_Sound *test_overlapping_sound;
#endif
};

struct Task_With_Memory
{
    b32 being_used;
    Memory_Arena arena;
    
    Temporary_Memory memory_flush;
};

struct Transient_State
{
    bool32 is_initialized;
    
    Game_Assets *assets;
    
    Memory_Arena tran_arena;
    
    Task_With_Memory tasks[4];    
    
    Platform_Work_Queue *work_queue;
};

// NOTE: Something for iniitlization an link list item
#if 0
#define something(Type, item, first_active, first_free)  \
first_free = first_free->next; \
zero_object(Type, *item); \
item->next = first_active; \
first_active = item; \

#endif

#define free_link_list(Type, first_active, first_free) \
for(Type **active_ptr = &first_active; *active_ptr;)   \
{ \
    Type *active = *active_ptr; \
    *active_ptr = active->next; \
    active->next = first_free; \
    first_free = active; \
} \

global_variable Platform_API platform;

internal Task_With_Memory *begin_task_with_memory(Transient_State *tran_state);
internal void end_task_with_memory(Task_With_Memory *task);

#endif
