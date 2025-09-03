#ifndef Game_View_h
#define Game_View_h

#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>
// TODO: SHould we be loading opengl function ourselves?
#import <OpenGL/gl3.h>
#import <Carbon/Carbon.h>

#import "../twosome_platform.h"
#import "../twosome_shared.h"
#import "../twosome_render.cpp"

struct Game_Code
{
    Game_Update_And_Render *update_and_render;
    Game_Get_Sound_Samples *get_sound_samples;
};


struct Macosx_Sound_Output
{
    // TODO: Make use of these
    s32 samples_per_second;
    s32 bytes_per_sample;
    
    // TODO: Should these be in here, maybe need
    // separate struct for callback user_data parameter
    Game_Memory *game_memory;
    Game_Code *game;
};


struct Macosx_Platform_File_Handle
{
    NSStream *stream;
    s32 access_mode;
};

struct Macosx_State
{
    // NOTE: Paths end with forward
    NSString *exe_directory_path;
    NSString *app_output_path;
    
    Memory_Arena temp_arena;
};

enum Macosx_Input_Event_Type
{
    macosx_input_event_type_null           = 0,
    macosx_input_event_type_keyboard       = 1,
    macosx_input_event_type_mouse_clicked  = 2,
    macosx_input_event_type_mouse_moved    = 3,
};

enum Macosx_Mouse_Event_Button
{
    macosx_mouse_event_button_null      = 0,
    macosx_mouse_event_button_left      = 1,
    macosx_mouse_event_button_right     = 2,
};

struct Macosx_Input_Event
{
    union
    {
        // NOTE: Keyboard
        struct
        {
            unsigned short key_code;
            b32 key_down;
        };
        // NOTE: Mouse Click
        struct
        {
            u32 mouse_button;
            b32 mouse_button_down;
        };
        struct
        {
            Vec2 mouse_moved;
        };
    };
    
    u32 type;
};

struct Macosx_Input_Events_Buffer
{
    Macosx_Input_Event events[256];
    u32 events_count;
};

@interface Game_View : NSView
{
    NSOpenGLContext *opengl_context;
    NSOpenGLPixelFormat *pixel_format;
    
    CVDisplayLinkRef display_link;
    double game_update_hz;
    
    Platform_API platform_api;
    
    Game_Memory game_memory;
    
    Game_Input input[2];
    Game_Input *new_input;
    Game_Input *old_input;
    
    Game_Settings game_settings;
    
    s16 *samples;
    Memory_Index samples_buffer_size;
    
    Memory_Index render_commands_push_buffer_size;
    void *render_commands_push_buffer;
    
    Game_Code game;
    
    // NOTE: We ignore the mouse moved after we've made the cursor move
    s32 cursor_centering_jump_delta_offset_count;
    
    Macosx_Sound_Output sound_output;
}

- (void) app_activeness_changed:(b32)active;

@end


#endif
