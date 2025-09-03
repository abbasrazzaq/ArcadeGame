#ifndef TWOSOME_GAME_LEVEL_H
#define TWOSOME_GAME_LEVEL_H


#define player_life_lost_shake_time 1.0f
#define player_level_start_lives 3
#define player_size 25.0f
#define player_life_display_size (player_size*0.95f)
#define player_life_display_glow_size (player_life_display_size*1.75f)

#define life_loss_lifespan_time 2.0f

// NOTE: These numbers should be divisible by two so can get equal numbers of black and white connectors
#define max_connectors 50
#define high_connectors_count max_connectors
#define medium_connectors_count 24
#define low_connectors_count 10

// NOTE: Connector Scales
#define low_connector_count_scale_min_size 100.0f
#define medium_connector_count_scale_min_size 30.0f
#define high_connector_count_scale_min_size 20.0f
#define connector_default_thickness 3.5f
#define connector_touch_max_thickness 5.5f
#define connector_thickness_swell_time 1.0f
#define connector_scale_max_size 200.0f

#define connector_max_speed 60.0f
#define connector_speed_variance 5.0f
#define connector_connect_time 15.0f
#define connector_connect_score_increase_time 1.0f
#define connector_connected_score_increase 1
#define connector_connected_in_shield_score_increase (connector_connected_score_increase * 10)
#define min_score_notification_number_size 2
#define max_score_notification_number_size 5

#define spikes_barrier_haze_width 100.0f
#define spikes_barrier_speed 85.0f
#define spikes_barrier_safe_width 15.0f
#define spikes_barrier_triangle_spacing 4.0f
#define spikes_barrier_safe_height 60.0f
#define spikes_barrier_spawn_time 1.0f

#define collectable_size 35.0f
#define collectable_activated_lifetime 15.0f
#define collectable_deactivated_time 15.0f

#define max_spinner_count 2
//#define spinner_size 25.0f
#define spinner_size 37.5f
#define time_for_spinner_laser_activation 2.0f
// NOTE: The time between laser activations
#define spinner_laser_to_activate_time 5.0f
#define spinner_total_laser_count 4
#define spinner_fully_active_time (time_for_spinner_laser_activation * (spinner_total_laser_count - 1) + time_for_spinner_laser_activation)
#define spinner_laser_thickness 25.0f
#define spinner_laser_width ((max(virtual_screen_width, virtual_screen_height)) * 2.0f)
#define player_laser_sound_cutoff_dist 400.0f

#define shield_active_time 30.0f
#define shield_min_radius (player_size*1.5f)
#define shield_max_radius (virtual_screen_height/2.0f)

#define safe_element_colour vec3(1.0f, 0.84f, 0.0f)
#define danger_element_colour vec3(1.0f, 0.0f, 1.0f)

#define tutorial_colour_changes_player_makes_to_progress 3
#define tutorial_time_before_collectable_appears 1.0f

#define time_from_end_for_sun 10.0f

#define active_particles_soft_max 256

#define background_blob_center_square_size 25.0f
#define background_blob_active_time 4.0f

#define end_of_level_score_fill_sound_time_interval 0.05f

enum Entity_Draw_Orders
{
    entity_draw_order_background,
    entity_draw_order_background_blob,
    entity_draw_order_background_timeline_bit,
    entity_draw_order_particles,
    entity_draw_order_meter_glow,
    entity_draw_order_shield,
    entity_draw_order_background_timeline_blob,
    entity_draw_order_connection_change,
    entity_draw_order_deployed_shield_time_life,
    entity_draw_order_shield_around_entity,
    entity_draw_order_cursor_axis,
    entity_draw_order_connecting_line,
    entity_draw_order_spikes_barrier_haze,
    entity_draw_order_laser_spinner,
    entity_draw_order_expanding_shield,
    
    entity_draw_order_default,
    
    entity_draw_order_spikes_barrier,
    entity_draw_order_corner_score_meter,
    entity_draw_order_face,
    entity_draw_order_life_lost_flash,
    entity_draw_order_lives_display,
    entity_draw_order_game_over_tint,
    entity_draw_order_life_lost,
    entity_draw_order_floating_score_meter,
    entity_draw_order_volume_ui,
    entity_draw_order_ui,
    entity_draw_order_transition_tint,
    entity_draw_order_loading_screen,
#if TWOSOME_INTERNAL
    entity_draw_order_DEBUG_text
#endif
    
};

struct Level_Background_Blob
{
    Vec2 position;
    f32 initial_scale;
    
    Vec3 colour;
    
    f32 life;
    f32 life_velocity;
};

struct Connector
{
    Vec2 position;
    Vec2 scale;
    
    Vec2 velocity;
    int32 connect_colour;
    bool32 connected;
    f32 connected_time;
    
    AABB aabb;
    OBB obb;
    
    f32 last_score_increase_time;
    
    f32 speed;
    b32 touching_shield;
    b32 touching_spinner_laser;
    
    f32 initial_speed;
    
    f32 swell_until_time;
    
    Play_Sound_Result last_connection_sound_played;
    u32 connection_count_level_at_last_connection;
    
    Vec2 last_connection_change;
    Vec3 last_connection_bkg_colour;
};

struct Spikes_Barrier
{
    b32 active;
    
    f32 x;
    f32 direction;
    f32 safe_spikes_y;
    s32 safe_cursor_colour;
    f32 spawn_time;
    
    AABB danger_bottom_aabb;
    AABB danger_top_aabb;
    AABB safe_spikes_aabb;
    
    Vec2 last_hint_point_offsets[4];
    Vec2 hint_point_offsets[4];
    
    f32 last_points_change_time;
    
    f32 last_particles_spawn_time;
    
    b32 hint_spawned;
    
    Spikes_Barrier *next;
};

struct Spikes_Barrier_Death
{
    f32 x;
    f32 top_parting_y;
    f32 bottom_parting_y;
    f32 spawn_time;
    
    b32 killed_player;
    b32 killed_by_shield;
    Vec3 safe_colour;
    
    Spikes_Barrier_Death *next;
};

struct Spinner_Laser
{
    b32 active;
    f32 spawn_time;
    OBB obb;
    f32 rotation;
};

struct Spinner
{
    Vec2 position;
    Vec2 velocity;
    f32 rotation;
    f32 rotation_direction;
    b32 active;
    f32 active_time;
    f32 speed;
    b32 touching_shield;
    s32 total_lasers;
    f32 inner_ball_rotation;
    
    AABB aabb;
    
    s32 max_lasers;
    Spinner_Laser lasers[spinner_total_laser_count];
};

struct Connection_Change
{
    Vec2 position;
    Vec2 scale;
    
    Vec2 ray_scale;
    f32 time_spawned;
    
    b32 lost_connection;
    
    // NOTE: For morphing quad
    f32 last_point_angles[4];
    f32 next_point_angles[4];    
    f32 last_point_change_time;
    
    Connection_Change *next;
};

struct Life_Loss
{
    Vec2 position;
    Vec2 scale;
    Vec2 large_scale;
    
    Vec3 colour;
    
    f32 time_life_left;
    
    Life_Loss *next;
};

struct Shield
{
    bool32 active;
    Vec2 center;
    f32 radius;
    f32 activated_time;
    f32 time_left;
    f32 normalized_time_left;
    
    Flashing_Value flash;
    
    f32 rotation;
    
    f32 last_beat_time;
    int beat;
    b32 played_deactivate_sound;
    
    Shield *next;
};

struct Collectable
{
    Vec2 position;
    bool32 active;
    f32 activated_time;
    f32 deactivated_time;
    f32 rotation;
    
    f32 last_beat_time;
};

struct Score_Notification
{
    real32 start_time;
    Vec2 position;
    int32 amount;
    
    Score_Notification *next;
};

struct Particle
{
    Vec2 position;
    Vec2 velocity;
    Vec3 colour;
    f32 size;
    f32 life;
    f32 start_life;
    
    f32 corner_offset;
    
    Particle *next;
};

struct Timeline_Event
{
    bool32 enabled;
    
    bool32 activated;
    real32 activate_time;
    
    union
    {
        // NOTE: Spikes Barrier
        struct
        {
            b32 spawn_from_both_sides;
        };
        // NOTE: Spinner
        struct
        {
            r32 rotation_direction;
            s32 max_lasers;
        };        
    };    
};

enum Tutorial_Type
{
    tutorial_type_null = 0,
    tutorial_type_changing_colour,
    tutorial_type_using_shield
};

enum Control_Hint_Type
{
    control_hint_type_none = 0,
    control_hint_type_colour_change,
    control_hint_type_shield_activate,
};

struct Tutorial
{
    union
    {
        // NOTE: Changing Colour Tutorial
        struct
        {
            s32 colour_changes_count;
            b32 connectors_spawned;
        };
        // NOTE: Using Shield Tutorial
        struct
        {
            b32 got_shield;
            f32 got_shield_time;
            b32 activated_shield;
        };
    };
    
    f32 control_hint_flash_time;
    b32 control_hint_flash_on;
};

enum Spawn_Side
{
    spawn_side_left,
    spawn_side_right,
    spawn_side_bottom,
    spawn_side_top,
    spawn_side_count
};

struct Stage_Metadata
{
    // NOTE: This is the current tutorial type state and
    // is turned off as soon as tutorial finishes
    s32 tutorial_type;
    
    s32 tutorial_stage_type;
    f32 stage_length_time;
    
    s32 total_connectors;
    Timeline_Event spikes_barrier_left;
    Timeline_Event spikes_barrier_right;
    
    Timeline_Event spinners[max_spinner_count];
};

struct Player
{
    Vec2 position;
    Vec2 scale;
    int32 connect_colour;
    
    f32 last_time_changed_colour;
    b32 touching_connector;
    f32 shake_until_time;
    b32 touching_spinner_laser;
    
    AABB aabb;
    OBB obb;
};

enum Connect_Colour
{
    connect_colour_black,
    connect_colour_white,
    connect_colour_count
};

enum Level_Mode
{
    level_mode_playing,
    level_mode_game_over,    
};

enum Level_Sound_Echo_Mode
{
    level_sound_echo_mode_default = 0,
    level_sound_echo_mode_regular_beat,
    level_sound_echo_mode_irregular_beat,
    level_sound_echo_mode_loop,
};

struct Level_Sound_Echo
{
    Play_Sound_Result sound_info;
    
    f32 speed;
    f32 time_to_start;
    f32 volume;
    
    s32 mode;
    u32 playing_beat;
    
    Level_Sound_Echo *next;
};

enum Connection_Count_Level
{
    connection_count_level_null = 0,
    connection_count_level_low,
    connection_count_level_medium,
    connection_count_level_high
};

struct Level_Stage
{
    Tutorial tutorial;
    
    f32 time;
    // NOTE: This doesn't include the tutorial time
    f32 active_time;
    
    int32 num_connectors;
    Connector connectors[max_connectors];
    // NOTE: There's one for each connector, and then one for player
    Level_Background_Blob background_blobs[max_connectors + 1];
    
    s32 num_connections;
    
    int spawn_connector_colours[max_connectors];
    f32 connectors_scale;
    
    Connection_Change *first_active_connection_change;
    Connection_Change *first_free_connection_change;
    
    Life_Loss *first_active_life_loss;
    Life_Loss *first_free_life_loss;
    
    Player player;
    
    Captured_Sounds_Arena captured_sounds;
    
    Level_Sound_Echo *first_active_sound_echo;
    Level_Sound_Echo *first_free_sound_echo;
    
    Playing_Sound *spinner_laser_sound;
    
    f32 life_lost_flash_start;
    Stage_Metadata meta;
    
    int32 mode;
    
    Score_Notification *first_visible_score_notif;
    Score_Notification *first_free_score_notif;
    
    int32 connector_following_player_index;
    int32 num_spinners;
    Spinner spinners[max_spinner_count];
    
    Spikes_Barrier *first_active_spikes_barrier;
    Spikes_Barrier *first_free_spikes_barrier;
    f32 last_spikes_barrier_spawned_direction;
    
    Shield *first_active_shield;
    Shield *first_free_shield;
    // NOTE: This is used by all entites that are covered by shield
    f32 shield_cover_rotation;
    Flashing_Value expanding_shield_flash;
    
    Spikes_Barrier_Death *first_active_spikes_barrier_death;
    Spikes_Barrier_Death *first_free_spikes_barrier_death;
    
    Particle *first_active_particle;
    u32 active_particles_count;
    Particle *first_free_particle;
    
    Collectable collectable;
    
    f32 nearest_player_laser_dist;
    
    b32 stages_in_transition;
    b32 transition_sound_played;
};

#define player_background_blob_index (array_count( ((Level_Stage *)0)->background_blobs ) - 1)

enum Game_Level_Type
{
    game_level_type_connectors = 0,
    game_level_type_spikes_barrier,
    game_level_type_spinners,
    game_level_type_everything,
    game_level_type_shield_intro,
    game_level_type_shield_everything,
    game_level_type_count
};

struct Game_Level
{
    // NOTE: Gets zero-ed out at the start of every stage
    Level_Stage stage;
    
    s32 type;
    
    s32 lives;
    
    u32 score;
    u32 score_to_get;
    u32 last_best_score;
    
    Stage_Metadata stage_metadatas[3];
    int32 stage_metadatas_count;
    
    s32 current_stage;
    
    f32 gametime;
    
    f32 game_over_time;
    Flashing_Value game_over_goal_flash;
    b32 played_goal_achieved_sound;
    f32 time_played_goal_achieved_sound;
    
    Flashing_Value awaiting_resume_flash;
    
    f32 time_last_spawned_life_loss_on_game_over;
    
    // NOTE: Used by level stage init, to know what kind of level echo sound to add
    Play_Sound_Result last_level_stage_transition_play_sound_result;
    
    f32 current_end_of_level_score_filling;
    
    f32 last_end_of_level_score_filling;
    f32 last_end_of_level_score_filling_sound_played_time;
    
    Memory_Arena *mode_arena;
    
    struct Cursor *cursor;
    Game_Audio *audio;
    
    RNG *rng;
};

#endif
