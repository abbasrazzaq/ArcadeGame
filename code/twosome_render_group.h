#ifndef TWOSOME_RENDER_GROUP_H
#define TWOSOME_RENDER_GROUP_H

#if TWOSOME_INTERNAL

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_assert
#include <stb_truetype.h>

struct DEBUG_Loaded_Font
{
    unsigned char *font_file_buffer;
    stbtt_fontinfo stb_font;
};

#endif

enum Shader_Types
{
    shader_type_vertex_soup = 0,
    shader_type_colour,
#if TWOSOME_INTERNAL
    DEBUG_shader_type_text,
#endif
    shader_types_count
};

enum Render_Entry_Type
{
    // NOTE: These all follow the format render_entry_type_{Type_Struct_Name}
    render_entry_type_DEBUG_Render_Entry_Text,
    render_entry_type_Render_Entry_Vertices
};

struct Render_Entry_Header
{
    uint16 type;
    // NOTE: Total entry size
    Memory_Index entry_size;
    // NOTE: Bytes that the entry requested for data not contained
    // inside entry struct (used for vertex / texture data)
    Memory_Index entry_extra_bytes;
    s32 draw_order;
    s32 shader_type;    
};

enum Render_Transform_Flags
{
    render_transform_flag_right_anchor_number          = 1 << 0,
    render_transform_flag_wrap_line_points             = 1 << 1,
    render_transform_flag_additive_blend               = 1 << 2,
    // NOTE: Indents the corners so that it is half of the line thickness
    render_transform_flag_half_line_corner_indent      = 1 << 3,
    render_transform_flag_hexagon_lattice_outline      = 1 << 4,
    // NOTE; When drawing outline of shape, puts gaps between the shape's points
    render_transform_flag_outline_gaps                 = 1 << 5,
    render_transform_flag_rect_no_center               = 1 << 6,
    // NOTE: This makes a nice outline with the lines nicely joined together
    render_transform_flag_joined_outline               = 1 << 7,
    // NOTE: Reverses the order in which blocky blend pieces would draw
    // if wanting to draw only some of the pieces
    render_transform_flag_reverse_blend_draw_order     =  1 << 8,
    
    render_transform_flag_blend_exclude_first_colour   = 1 << 9,
    render_transform_flag_blend_exclude_second_colour  = 1 << 10,
    
    render_transform_flag_screen_coords                = 1 << 11,
    
    render_transform_flag_do_world_transform        = 1 << 12,
    render_transform_flag_do_local_rotation         = 1 << 14,
    render_transform_flag_do_world_rotation         = 1 << 15,
    
    
    render_transform_flags_blend_exclude_colours = render_transform_flag_blend_exclude_first_colour | render_transform_flag_blend_exclude_second_colour
};

struct Render_Transform
{
    Vec2 rotation_pt;
    f32 rotation;
    
    f32 world_rotation;
    Vec2 world_rotation_pt;
    
    s32 draw_order;
    f32 line_thickness;
    f32 corner_indent;
    
    u32 flags;
    
    // NOTE: Normalized value to indicate to blocky blend draw function
    // what the center is (for using centroid on triangle)
    Vec2 blocky_blend_center;
    
    s32 blocky_blend_pieces_total_count;
    // NOTE: How many of the pieces of a blocky blend shape
    // to draw goes from outer bit to inner bit
    s32 blocky_blend_pieces_to_draw;
    
    Vec3 hexagon_lattice_outline_colour;
};

enum Vertex_Flags
{
    vertex_flag_xy       = 1 << 0,
    vertex_flag_xyz      = 1 << 1,
    vertex_flag_uv       = 1 << 2,
    vertex_flag_rgba     = 1 << 3,
};

struct Vertex_XY_RGBA
{
    Vec2 position;
    Vec4 colour;
};

union Vertex_XYZ_RGBA
{
    Vec3 position;
    Vec4 colour;
};

internal Vertex_XY_RGBA vertex_xy_rgba(Vec2 xy, Vec4 rgba)
{
    Vertex_XY_RGBA result = { xy, rgba };
    return result;
}

internal Vertex_XY_RGBA vertex_xy_rgba(f32 x, f32 y, Vec4 rgba)
{
    Vertex_XY_RGBA result = { x, y, rgba };
    return result;
}

union Vertex_XY_UV
{
    struct
    {
        Vec2 position;
        Vec2 uv;
    };
    
    r32 e[4];
};

struct Render_Entry_Vertices
{    
    f32 *vertices;
    int32 vertices_count;
    int vertex_flags;
};

struct DEBUG_Render_Entry_Text
{
    void *texture_data;    
    Vec4 colour;
    
    // NOTE: In screen units
    s32 x, y;
    s32 width, height;
};

internal s32 get_values_per_vertex(s32 vertex_flags)
{
    s32 result = 0;
    
    if(vertex_flags & vertex_flag_xy)
    {
        result += 2;
    }
    if(vertex_flags & vertex_flag_xyz)
    {
        result += 3;
    }    
    if(vertex_flags & vertex_flag_uv)
    {
        result += 2;
    }
    if(vertex_flags & vertex_flag_rgba)
    {
        result += 4;
    }
    
    return result;
}

struct Blocky_Blend_T
{
    f32 t;
    f32 next_t;
    f32 c_t;    
};

#endif
