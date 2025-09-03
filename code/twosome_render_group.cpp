#include "twosome_render_group.h"


#define push_render_element(commands, type, shader_type, transform) (type *)push_render_element_(commands, sizeof(type), 0, shader_type, render_entry_type_##type, transform)
#define push_render_element_padded(commands, type, shader_type, entry_extra_bytes, transform) (type *)push_render_element_(commands, (sizeof(type) + (entry_extra_bytes)), (entry_extra_bytes), shader_type, render_entry_type_##type, transform)

internal void *push_render_element_(Game_Render_Commands *commands, Memory_Index entry_size, Memory_Index entry_extra_bytes, int shader_type, uint16 type, Render_Transform transform)
{        
    commands->push_buffer_at -= sizeof(Render_Entry_Header);
    Render_Entry_Header *header = (Render_Entry_Header *)commands->push_buffer_at;
    
    ++commands->render_entries_count;
    
    header->shader_type = shader_type;
    
    header->type = type;
    header->draw_order = transform.draw_order;
    header->entry_size = entry_size;
    header->entry_extra_bytes = entry_extra_bytes;
    
    commands->push_buffer_at -= entry_size;
    
    return commands->push_buffer_at;
}

internal Vec2 equilateral_centroid(Vec2 scale)
{
    // NOTE: Find midpoint along base, then find point that sits two-thirds of the way *from* the opposite vertex (i.e. the top)
    Vec2 result = vec2(scale.x*0.5f, (f32)(scale.y - (scale.y * (2.0/3.0))) );
    return result;
}

// 
// NOTE: Functions for making Render_Transform, you can chain these together
// by calling another transform function for the input of the last default parameter
//
internal Render_Transform default_transform()
{
    Render_Transform transform = {};
    transform.line_thickness = 1.0f;
    transform.draw_order = entity_draw_order_default;
    transform.blocky_blend_center = vec2(0.5f);
    
    transform.blocky_blend_pieces_total_count = 6;
    transform.blocky_blend_pieces_to_draw = transform.blocky_blend_pieces_total_count;
    
    return transform;
}

internal Render_Transform rotation_transform(f32 rotation, Vec2 rotation_pt = vec2(0.0f), Render_Transform transform = default_transform())
{
    transform.rotation = rotation;
    transform.rotation_pt = rotation_pt;
    
    transform.flags |= render_transform_flag_do_local_rotation;
    
    return transform;
}

internal Render_Transform world_rotation_transform(f32 rotation, Vec2 rotation_pt, Render_Transform transform = default_transform())
{
    transform.world_rotation = rotation;
    transform.world_rotation_pt = rotation_pt;
    
    transform.flags |= render_transform_flag_do_world_rotation;
    
    return transform;
}

internal Render_Transform draw_order_transform(s32 draw_order, Render_Transform transform = default_transform())
{
    //assert(draw_order >= entity_draw_order_min && draw_order <= entity_draw_order_max);
    transform.draw_order = draw_order;
    
    return transform;
}

internal Render_Transform thickness_transform(f32 thickness, Render_Transform transform = default_transform())
{
    transform.line_thickness = thickness;
    
    return transform;
}

internal Render_Transform corner_indent_transform(f32 corner_indent, Render_Transform transform = default_transform())
{
    transform.corner_indent = corner_indent;
    
    return transform;
}

internal Render_Transform transform_flags(u32 render_transform_flags, Render_Transform transform = default_transform())
{
    transform.flags |= render_transform_flags;
    
    return transform;
}

internal Render_Transform blocky_blend_center_transform(Vec2 blocky_blend_center, Render_Transform transform = default_transform())
{
    transform.blocky_blend_center = blocky_blend_center;
    
    return transform;
}

internal Render_Transform blocky_blend_pieces_to_draw_transform(s32 blocky_blend_pieces_to_draw, Render_Transform transform = default_transform())
{
    transform.blocky_blend_pieces_to_draw = blocky_blend_pieces_to_draw;
    
    return transform;
}

internal Render_Transform blocky_blend_pieces_total_count_transform(s32 blocky_blend_pieces_total_count, Render_Transform transform = default_transform())
{
    transform.blocky_blend_pieces_total_count = blocky_blend_pieces_total_count;
    
    return transform;
}

internal Render_Transform hexagon_lattice_outline_transform(Vec3 outline_colour, Render_Transform transform = default_transform())
{
    transform.flags |= render_transform_flag_hexagon_lattice_outline;
    transform.hexagon_lattice_outline_colour = outline_colour;
    
    return transform;
}

internal void clamp_line_thickness(Render_Transform *transform, Vec2 scale)
{
    transform->line_thickness = min(transform->line_thickness, (scale.x*0.5f));
    transform->line_thickness = min(transform->line_thickness, (scale.y*0.5f));
    
    assert(transform->line_thickness <= (scale.x*0.5f) && transform->line_thickness <= (scale.y*0.5f));
}

internal f32 clamp_corner_indent(f32 indent, Vec2 scale)
{
    indent = min(indent, scale.x*0.5f);
    indent = min(indent, scale.y*0.5f);
    
    return indent;
}

// NOTE: You can pass in same array of vertices for transformed vertices
internal void transform_vertices(Vec2 *verts, Vec2 *transformed_verts, u32 verts_count, Vec2 position, Vec2 scale)
{
    TIMED_BLOCK();
    
    for(u32 vert_index = 0; vert_index < verts_count; ++vert_index)
    {
        Vec3 v = translation_mat3(position) * scaling_mat3(scale) * vec3(verts[vert_index], 1.0f);
        transformed_verts[vert_index] = v.xy;
    }
}

internal void transform_vertices(Vec2 *verts, Vec2 *transformed_verts, u32 verts_count, Vec2 position, Vec2 scale, f32 rotation, Vec2 rotation_pt)
{
    TIMED_BLOCK();
    
    for(u32 vert_index = 0; vert_index < verts_count; ++vert_index)
    {
        Vec3 v = translation_mat3(position + rotation_pt) * roll_mat3(rotation) * translation_mat3(rotation_pt * -1.0f) * scaling_mat3(scale) * vec3(verts[vert_index], 1.0f);
        transformed_verts[vert_index] = v.xy;
    }
}

internal void _push_vertices(Game_Render_Commands *commands, f32 *vertices, int32 vertices_count, s32 vertex_flags_in, Vec2 position, Vec2 scale, Vec4 colour, Render_Transform transform = default_transform())
{
    TIMED_BLOCK();
    
    assert((vertices_count % 3) == 0);
    
    Temporary_Memory temp_mem = begin_temporary_memory(commands->temp_arena);
    
    s32 vertex_flags = vertex_flag_xy | vertex_flag_rgba;
    s32 values_per_vertex = get_values_per_vertex(vertex_flags);
    assert(values_per_vertex == 6);    
    
    s32 shader_type = shader_type_vertex_soup;
    
    Memory_Index vertex_buffer_size = vertices_count * (values_per_vertex * sizeof(*vertices));
    Render_Entry_Vertices *v = push_render_element_padded(commands, Render_Entry_Vertices, shader_type, vertex_buffer_size, transform);
    if(v)
    {
        v->vertices_count = vertices_count;
        v->vertex_flags = vertex_flags;
        
        v->vertices = (f32 *)(commands->push_buffer_at + sizeof(*v));
        
        Mat4 view_proj = identity_mat4();
        if(transform.flags & render_transform_flag_screen_coords)
        {
            view_proj = commands->view.screen_orthographic_matrix;
        }
        else
        {
            view_proj = commands->view.orthographic_matrix;            
        }
        
        Mat4 world_view_proj;
        
        b32 skip_world_transform = (transform.flags & render_transform_flag_do_world_transform) == false;
        b32 skip_world_rotation = (transform.flags & render_transform_flag_do_world_rotation) == false;
        b32 skip_local_rotation = (transform.flags & render_transform_flag_do_local_rotation) == false;
        
        assert(skip_world_transform == false || (position.x == 0.0f && position.y == 0.0f && scale.x == 1.0f && scale.y == 1.0f));
        assert(skip_world_rotation == false || (transform.world_rotation == 0.0f));
        assert(skip_local_rotation == false || (transform.rotation == 0.0f));
        
        if(skip_world_transform && skip_world_rotation && skip_local_rotation)
        {
            world_view_proj = view_proj;            
        }
        else
        {
            Mat3 world_mat3;
            if(skip_local_rotation && skip_world_rotation)
            {
                world_mat3 = translation_mat3(position) * scaling_mat3(scale);
            }
            else
            {
                Mat3 model = translation_mat3(transform.rotation_pt) *
                    roll_mat3(transform.rotation) *
                    translation_mat3(transform.rotation_pt * -1.0f) *
                    scaling_mat3(scale);
                
                world_mat3 = translation_mat3(position) *
                    translation_mat3(transform.world_rotation_pt) *
                    roll_mat3(transform.world_rotation) *
                    translation_mat3(transform.world_rotation_pt * -1.0f) *
                    model;
            }
            
            Mat4 world = identity_mat4();
            world._11 = world_mat3._11;
            world._21 = world_mat3._21;
            
            world._12 = world_mat3._12;                        
            world._22 = world_mat3._22;                      
            
            world._14 = world_mat3._13;
            world._24 = world_mat3._23;
            
            world_view_proj = view_proj * world;
        }
        
        if(vertex_flags_in & vertex_flag_rgba)
        {
            Vertex_XY_RGBA *vertices_in = (Vertex_XY_RGBA *)vertices;
            Vertex_XY_RGBA *vertices_out = (Vertex_XY_RGBA *)v->vertices;
            
            if((transform.flags & render_transform_flag_additive_blend))
            {
                for(s32 vertex_index = 0; vertex_index < vertices_count; ++vertex_index)
                {
                    vertices_out->position.x = ((world_view_proj._11 * vertices_in->position.x) + (world_view_proj._12 * vertices_in->position.y) + world_view_proj._14);
                    vertices_out->position.y = ((world_view_proj._21 * vertices_in->position.x) + (world_view_proj._22 * vertices_in->position.y) + world_view_proj._24);
                    
                    vertices_out->colour = vec4(vertices_in->colour.rgb, 0.0f);
                    
                    ++vertices_in;
                    ++vertices_out;
                }                
            }
            else
            {
                for(s32 vertex_index = 0; vertex_index < vertices_count; ++vertex_index)
                {
                    vertices_out->position.x = ((world_view_proj._11 * vertices_in->position.x) + (world_view_proj._12 * vertices_in->position.y) + world_view_proj._14);
                    vertices_out->position.y = ((world_view_proj._21 * vertices_in->position.x) + (world_view_proj._22 * vertices_in->position.y) + world_view_proj._24);
                    
                    vertices_out->colour = vec4(vertices_in->colour.rgb*vertices_in->colour.a, vertices_in->colour.a);
                    
                    ++vertices_in;
                    ++vertices_out;
                }
            }
        }
        else
        {            
            Vec4 premultiplied_colour;
            if(transform.flags & render_transform_flag_additive_blend)
            {
                premultiplied_colour = vec4(colour.rgb, 0.0f);
            }
            else
            {
                premultiplied_colour = vec4(colour.rgb*colour.a, colour.a);
            }
            
            Vec2 *vertices_in = (Vec2 *)vertices;
            Vertex_XY_RGBA *vertices_out = (Vertex_XY_RGBA *)v->vertices;
            for(s32 vertex_index = 0; vertex_index < vertices_count; ++vertex_index)
            {
                vertices_out->position.x = ((world_view_proj._11 * vertices_in->x) + (world_view_proj._12 * vertices_in->y) + world_view_proj._14);
                vertices_out->position.y = ((world_view_proj._21 * vertices_in->x) + (world_view_proj._22 * vertices_in->y) + world_view_proj._24);
                
                vertices_out->colour = premultiplied_colour;
                
                ++vertices_in;
                ++vertices_out;
            }
        }
    }
    
    end_temporary_memory(temp_mem);
}

internal void push_vertices(Game_Render_Commands *commands, f32 *vertices, int32 vertices_count, s32 vertex_flags_in, Vec2 position, Vec2 scale, Vec4 colour, Render_Transform transform = default_transform())
{
    // NOTE: If we don't need to apply translation and scaling then use other push_vertices
    assert(position.x != 0.0f || position.y != 0.0f || scale.x != 1.0f || scale.y != 1.0f);
    
    transform = transform_flags(render_transform_flag_do_world_transform, transform);
    
    _push_vertices(commands, vertices, vertices_count, vertex_flags_in, position, scale, colour, transform);
}

internal void push_vertices(Game_Render_Commands *commands, f32 *vertices, int32 vertices_count, s32 vertex_flags_in, Vec4 colour, Render_Transform transform = default_transform())
{
    _push_vertices(commands, vertices, vertices_count, vertex_flags_in, vec2(0.0f), vec2(1.0f), colour, transform);
}

internal Blocky_Blend_T make_blocky_blend_t(s32 blend_piece_index, Render_Transform transform)
{
    Blocky_Blend_T result;
    
    b32 full_first_colour = !(transform.flags & render_transform_flag_blend_exclude_first_colour);
    b32 full_second_colour = !(transform.flags & render_transform_flag_blend_exclude_second_colour);
    
    result.c_t = 0.0f;
    if(full_first_colour && full_second_colour)
    {
        result.c_t = ( (f32)blend_piece_index / (f32)(transform.blocky_blend_pieces_total_count - 1) );
    }
    else if(full_first_colour)
    {
        result.c_t = ( (f32)(blend_piece_index) / (f32)(transform.blocky_blend_pieces_total_count) );
    }
    else if(full_second_colour)
    {
        result.c_t = ( (f32)(blend_piece_index + 1) / (f32)(transform.blocky_blend_pieces_total_count) );
    }
    else
    {
        result.c_t = ( (f32)(blend_piece_index + 1) / (f32)(transform.blocky_blend_pieces_total_count + 1) );
    }
    
    result.t = (1.0f / transform.blocky_blend_pieces_total_count) * blend_piece_index;
    result.next_t = (1.0f / transform.blocky_blend_pieces_total_count) * (blend_piece_index + 1);
    
    // NOTE: If drawing the pieces inside out then need to reverse t values
    if(transform.flags & render_transform_flag_reverse_blend_draw_order)
    {
        result.t = 1.0f - result.t;
        result.next_t = 1.0f - result.next_t;
        result.c_t = 1.0f - result.c_t;
    }
    
    return result;    
}

internal Vertex_XY_RGBA *add_blocky_blend_verts(Vertex_XY_RGBA *v, Vec4 start_colour, Vec4 end_colour, Vec2 start_pt_0, Vec2 end_pt_0, Vec2 start_pt_1, Vec2 end_pt_1, Render_Transform transform = default_transform())
{        
    for(s32 blend_piece_index = 0; blend_piece_index < transform.blocky_blend_pieces_to_draw; ++blend_piece_index)
    {        
        Blocky_Blend_T blend = make_blocky_blend_t(blend_piece_index, transform);
        
        Vec4 c = interpolate(start_colour, end_colour, blend.c_t);
        
        Vec2 bl = interpolate(start_pt_0, end_pt_0, blend.t);
        Vec2 br = interpolate(start_pt_0, end_pt_0, blend.next_t);
        Vec2 tl = interpolate(start_pt_1, end_pt_1, blend.t);
        Vec2 tr = interpolate(start_pt_1, end_pt_1, blend.next_t);
        
        *v++ = vertex_xy_rgba(bl, c);
        *v++ = vertex_xy_rgba(br, c);
        *v++ = vertex_xy_rgba(tl, c);
        
        *v++ = vertex_xy_rgba(tl, c);
        *v++ = vertex_xy_rgba(br, c);
        *v++ = vertex_xy_rgba(tr, c);
    }
    
    return v;
}

internal void push_blocky_blend(Game_Render_Commands *commands, Vec2 position, Vec2 scale, Vec4 start_colour, Vec4 end_colour, Vec2 start_pt_0, Vec2 end_pt_0, Vec2 start_pt_1, Vec2 end_pt_1, Render_Transform transform = default_transform())
{
    Temporary_Memory temp_mem = begin_temporary_memory(commands->temp_arena);
    
    s32 total_quads = transform.blocky_blend_pieces_total_count;
    s32 total_verts = total_quads*6;
    
    Vertex_XY_RGBA *verts = push_array(commands->temp_arena, Vertex_XY_RGBA, total_verts);
    Vertex_XY_RGBA *v = add_blocky_blend_verts(verts, start_colour, end_colour, start_pt_0, end_pt_0, start_pt_1, end_pt_1, transform);
    
    assert((v - verts) <= total_verts);
    
    push_vertices(commands, (f32 *)verts, (v - verts), vertex_flag_xy | vertex_flag_rgba, position, scale, vec4(1.0f), transform);
    
    end_temporary_memory(temp_mem);
}

internal void get_shape_dimensions(Vec2 *points, u32 points_count, Vec2 *bl, Vec2 *tr)
{
    *bl = points[0];
    *tr = points[0];
    
    for(u32 point_index = 1; point_index < points_count; ++point_index)
    {
        Vec2 pt = points[point_index];
        
        if(pt.x < bl->x)
        {
            bl->x = pt.x;
        }
        if(pt.y < bl->y)
        {
            bl->y = pt.y;
        }
        
        if(pt.x > tr->x)
        {
            tr->x = pt.x;
        }
        if(pt.y > tr->y)
        {
            tr->y = pt.y;
        }
    }
}

internal void push_shape_blocky_blend(Game_Render_Commands *commands, Vec2 position, Vec2 scale, Vec4 inner_colour, Vec4 outer_colour, Vec2 *shape_points_local, s32 shape_points_count, Render_Transform transform = default_transform())
{
    if(transform.blocky_blend_pieces_to_draw > 0 && shape_points_count > 0)
    {
        Temporary_Memory temp_mem = begin_temporary_memory(commands->temp_arena);
        
        s32 total_verts_count = (transform.blocky_blend_pieces_total_count * (shape_points_count * 6));
        Vertex_XY_RGBA *verts = push_array(commands->temp_arena, Vertex_XY_RGBA, total_verts_count);
        Vertex_XY_RGBA *v = verts;
        
        Vec2 *shape_points = push_array(commands->temp_arena, Vec2, shape_points_count);
        transform_vertices(shape_points_local, shape_points, shape_points_count, position, scale);
        
        Vec2 bl, tr;
        get_shape_dimensions(shape_points, shape_points_count, &bl, &tr);
        
        Vec2 center = {};
        Vec2 size = (tr - bl);
        center = bl + vec2(size.x * transform.blocky_blend_center.x, size.y * transform.blocky_blend_center.y);
        
        for(s32 shape_index = 0; (shape_index < transform.blocky_blend_pieces_total_count && shape_index < transform.blocky_blend_pieces_to_draw); ++shape_index)
        {
            Blocky_Blend_T blend = make_blocky_blend_t(shape_index, transform);
            
            Vec4 colour = interpolate(outer_colour, inner_colour, blend.c_t);
            
            for(s32 quad_index = 0; quad_index < shape_points_count; ++quad_index)
            {
                Vec2 outer_pt1 = interpolate(shape_points[quad_index], center, blend.t);
                Vec2 outer_pt2 = interpolate(shape_points[ (quad_index + 1) % shape_points_count ], center, blend.t);
                
                Vec2 inner_pt1 = interpolate(shape_points[quad_index], center, blend.next_t);
                Vec2 inner_pt2 = interpolate(shape_points[ (quad_index + 1) % shape_points_count ], center, blend.next_t);
                
                *v++ = vertex_xy_rgba(outer_pt1, colour);
                *v++ = vertex_xy_rgba(outer_pt2, colour);
                *v++ = vertex_xy_rgba(inner_pt1, colour);
                
                *v++ = vertex_xy_rgba(inner_pt1, colour);
                *v++ = vertex_xy_rgba(outer_pt2, colour);
                *v++ = vertex_xy_rgba(inner_pt2, colour);
            }
        }
        
        // NOTE: Because all the points are in world space, we need to add offset the rotation point
        transform.rotation_pt += position;
        assert( (v - verts) <= total_verts_count );
        
        push_vertices(commands, (f32 *)verts, (v - verts), vertex_flag_xy | vertex_flag_rgba, vec4(1.0f), transform);
        
        end_temporary_memory(temp_mem);
    }        
}

internal void push_triangle_blocky_blend(Game_Render_Commands *commands, Vec2 position, Vec2 scale, Vec4 inner_colour, Vec4 outer_colour, Render_Transform transform = default_transform())
{
    Vec2 points[] =
    {
        vec2(0.0f, 0.0f),
        vec2(1.0f, 0.0f),
        vec2(0.5f, 1.0f)
    };
    
    push_shape_blocky_blend(commands, position, scale, inner_colour, outer_colour, points, array_count(points), blocky_blend_center_transform(equilateral_centroid(vec2(1.0f)), transform));
}

internal void push_blocky_blend_rect(Game_Render_Commands *commands, Vec2 position, Vec2 scale, Vec4 left_side_colour, Vec4 right_side_colour, Render_Transform transform = default_transform())
{
    push_blocky_blend(commands, position, scale, left_side_colour, right_side_colour, vec2(0.0f, 0.0f), vec2(1.0f, 0.0f), vec2(0.0f, 1.0f), vec2(1.0f, 1.0f), transform);
}

// NOTE: This function can render a full solid rect or an outline, and can do both with indented corners
internal void _push_rect(Game_Render_Commands *commands, Vec2 position, Vec2 scale, Vec4 colour, Render_Transform transform = default_transform())
{
    // NOTE: If the rectangle requires the corners to be indented, or we're drawing the outline with
    // some thickness then need to do something more complicated with vertices
    if(transform.corner_indent > 0.0f || transform.line_thickness > 0.0f)
    {
        assert(transform.line_thickness > 0.0f);
        
        // NOTE: When there's a corner indent we draw the center part and then and four borders which
        // push-in in the right places
        Vec2 center_bl, center_tr, bottom_bl, bottom_tr, top_bl, top_tr, left_bl, left_tr, right_bl, right_tr;
        {
            // NOTE: Work out where 5 rectangles should go (1 center, and 4 borders)
            Vec2 center_scale = scale - transform.corner_indent*2.0f;
            Vec2 center_pos = position + transform.corner_indent;    
            
            Vec2 bottom_pos = vec2(position.x + transform.corner_indent, position.y);
            Vec2 bottom_scale = vec2(scale.x - transform.corner_indent*2.0f, transform.line_thickness);
            
            Vec2 top_pos = vec2(position.x + transform.corner_indent, position.y + scale.y - transform.line_thickness);
            Vec2 top_scale = vec2(scale.x - transform.corner_indent*2.0f, transform.line_thickness);
            
            Vec2 left_pos = vec2(position.x, position.y + transform.corner_indent);
            Vec2 left_scale = vec2(transform.line_thickness, scale.y - transform.corner_indent*2.0f);
            
            Vec2 right_pos = vec2(position.x + scale.x - transform.line_thickness, position.y + transform.corner_indent);
            Vec2 right_scale = vec2(transform.line_thickness, scale.y - transform.corner_indent*2.0f);
            
            
            center_bl = vec2(center_pos.x, center_pos.y);
            center_tr = center_pos + center_scale;
            
            bottom_bl = bottom_pos;
            bottom_tr = bottom_pos + bottom_scale;
            
            top_bl = top_pos;
            top_tr = top_pos + top_scale;
            
            left_bl = left_pos;
            left_tr = left_pos + left_scale;
            
            right_bl = right_pos;
            right_tr = right_pos + right_scale;
            
        }
        
        Vec2 verts[6*5];
        u32 vi = 0;
        // NOTE: Center
        if(!(transform.flags & render_transform_flag_rect_no_center))
        {
            verts[vi++] = center_bl;
            verts[vi++] = vec2(center_tr.x, center_bl.y);
            verts[vi++] = vec2(center_bl.x, center_tr.y);
            verts[vi++] = vec2(center_bl.x, center_tr.y);
            verts[vi++] = vec2(center_tr.x, center_bl.y);
            verts[vi++] = center_tr;   
        }
        
        // NOTE: Bottom border
        verts[vi++] = bottom_bl;
        verts[vi++] = vec2(bottom_tr.x, bottom_bl.y);
        verts[vi++] = vec2(bottom_bl.x, bottom_tr.y);
        verts[vi++] = vec2(bottom_tr.x, bottom_bl.y);
        verts[vi++] = vec2(bottom_bl.x, bottom_tr.y);
        verts[vi++] = bottom_tr;
        // NOTE: Top border
        verts[vi++] = top_bl;
        verts[vi++] = vec2(top_tr.x, top_bl.y);
        verts[vi++] = vec2(top_bl.x, top_tr.y);
        verts[vi++] = vec2(top_bl.x, top_tr.y);
        verts[vi++] = vec2(top_tr.x, top_bl.y);
        verts[vi++] = top_tr;
        // NOTE: Left border
        verts[vi++] = left_bl;
        verts[vi++] = vec2(left_tr.x, left_bl.y);
        verts[vi++] = vec2(left_bl.x, left_tr.y);       
        verts[vi++] = vec2(left_bl.x, left_tr.y);
        verts[vi++] = vec2(left_tr.x, left_bl.y);
        verts[vi++] = left_tr;
        // NOTE: Right border
        verts[vi++] = right_bl;
        verts[vi++] = vec2(right_tr.x, right_bl.y);
        verts[vi++] = vec2(right_bl.x, right_tr.y);
        verts[vi++] = vec2(right_bl.x, right_tr.y);
        verts[vi++] = vec2(right_tr.x, right_bl.y);
        verts[vi++] = right_tr;
        assert(vi <= array_count(verts));
        
        Render_Transform center_transform = transform;
        // NOTE: Because all the points are in world space, we need to add offset the rotation point
        center_transform.rotation_pt += position;
        
        push_vertices(commands, (f32 *)verts, vi, vertex_flag_xy, colour, center_transform);
    }
    else
    {
        // NOTE: Just draw simple rectangle if no indenting required
        Vec2 verts[] =
        {
            vec2(0.0f, 0.0f),
            vec2(1.0f, 0.0f),
            vec2(0.0f, 1.0f),
            vec2(0.0f, 1.0f),
            vec2(1.0f, 0.0f),
            vec2(1.0f, 1.0f),
        };
        
        push_vertices(commands, (f32 *)verts, array_count(verts), vertex_flag_xy, position, scale, colour, transform);
    }   
}

internal void push_rect(Game_Render_Commands *commands, Vec2 position, Vec2 scale, Vec4 colour, Render_Transform transform = default_transform())
{
    transform.corner_indent = clamp_corner_indent(transform.corner_indent, scale);
    transform.line_thickness = transform.corner_indent;
    _push_rect(commands, position, scale, colour, transform);
}

internal void push_rect_outline(Game_Render_Commands *commands, Vec2 position, Vec2 scale, Vec4 colour, Render_Transform transform = default_transform())
{
    clamp_line_thickness(&transform, scale);
    
    if(transform.flags & render_transform_flag_half_line_corner_indent)
    {
        // NOTE: Want the outline to join each other at half the line thickness
        transform.corner_indent = transform.line_thickness*0.5f;
    }
    else
    {
#if 0
        if(transform.corner_indent == 0.0f)
        {
            transform.corner_indent = transform.line_thickness;            
        }
#endif
    }
    
    _push_rect(commands, position, scale, colour, transform_flags(render_transform_flag_rect_no_center, transform));
}

internal Vec2 screen_to_ortho(Game_Render_View view, s32 x, s32 y)
{
    Vec2 result;
    result.x = ((f32)x / (f32)view.width) * (f32)virtual_screen_width;
    result.y = ((f32)y / (f32)view.height) * (f32)virtual_screen_height;
    
    return result;
}

internal Vec2 ortho_to_screen(Game_Render_View view, Vec2 pt)
{
    Vec2 result;
    result.x = ((pt.x / (f32)virtual_screen_width) * (f32)view.width);
    result.y = ((pt.y / (f32)virtual_screen_height) * (f32)view.height);
    
    return result;
}

#define number_block_col_count 4
#define number_block_row_count 5
#define number_total_block_count (number_block_col_count * number_block_row_count)

struct Number_Blocks
{
    u8 fills[number_total_block_count];
};

global_variable Number_Blocks number_blocks[10] =
{
    // 0
    {
        0, 1, 1, 0,
        1, 0, 0, 1,
        1, 0, 0, 1,
        1, 0, 0, 1,
        0, 1, 1, 0
    },
    // 1
    {
        0, 0, 1, 0,
        0, 1, 1, 0,
        0, 0, 1, 0,
        0, 0, 1, 0,
        0, 1, 1, 1
    },
    // 2
    {
        0, 1, 1, 0,
        1, 0, 0, 1,
        0, 0, 1, 0,
        0, 1, 0, 0,
        1, 1, 1, 1
    },
    // 3
    {
        0, 1, 1, 1,
        0, 0, 0, 1,
        0, 0, 1, 1,
        0, 0, 0, 1,
        0, 1, 1, 1
    },
    // 4
    {
        1, 0, 0, 0,
        1, 0, 1, 0,
        1, 1, 1, 1,
        0, 0, 1, 0,
        0, 0, 1, 0
    },
    // 5
    {
        1, 1, 1, 0,
        1, 0, 0, 0,
        1, 1, 1, 0,
        0, 0, 0, 1,
        1, 1, 1, 0
    },
    // 6
    {
        0, 1, 1, 0,
        1, 0, 0, 0,
        1, 1, 1, 0,
        1, 0, 0, 1,
        0, 1, 1, 0
    },
    // 7
    {
        1, 1, 1, 1,
        0, 0, 0, 1,
        0, 0, 1, 0,
        0, 1, 0, 0,
        0, 1, 0, 0
    },
    // 8
    {
        0, 1, 1, 0,
        1, 0, 0, 1,
        0, 1, 1, 0,
        1, 0, 0, 1,
        0, 1, 1, 0
    },
    // 9
    {
        0, 1, 1, 0,
        1, 0, 0, 1,
        0, 1, 1, 1,
        0, 0, 0, 1,
        0, 1, 1, 0
    }
};

struct Blocky_Number_Info
{
    // NOTE: Work out the number of digits required by saying number of digits increase at every 3 bits-ish
    // Then just adding 3 for luck :-)
    u32 digits[ ((sizeof(u32)*8) / 3) + 3 ];
    u32 digits_count;
    
    s32 scaled_font_size;
    
    // NOTE: In ortho co-ords
    Vec2 total_size;
    
    // NOTE: In screen co-ords
    s32 total_screen_width;
    s32 total_screen_height;
};

internal Blocky_Number_Info get_blocky_number_info(Game_Render_View view, u32 value, f32 raw_font_size)
{
    Blocky_Number_Info result = {};
    
    // NOTE: Extract each digit of the number
    if(value > 9)
    {
        u32 number = value;
        while(number > 0)
        {
            result.digits[result.digits_count++] = (number % 10);
            number /= 10;
        }
    }
    else
    {
        result.digits[result.digits_count++] = value;
    }
    assert(result.digits_count <= array_count(result.digits));
    
    result.scaled_font_size = (s32)round_up( ((f32)raw_font_size * ((f32)view.height / (f32)working_screen_height)) );
    result.scaled_font_size = max(1, result.scaled_font_size);
    
    result.total_screen_width = (s32)( ((result.scaled_font_size*(number_block_col_count)) * result.digits_count) + (result.scaled_font_size * (result.digits_count - 1)));
    result.total_screen_height = (s32)((f32)result.scaled_font_size * (f32)number_block_row_count);
    
    result.total_size = screen_to_ortho(view, result.total_screen_width, result.total_screen_height);
    
    return result;
}

internal f32 push_blocky_number(Game_Render_Commands *render_commands, u32 value, f32 raw_size, f32 ox, f32 oy, Vec4 colour, Render_Transform transform = default_transform())
{
    Vec2 pt = ortho_to_screen(render_commands->view, vec2(ox, oy));
    
    s32 x = (s32)pt.x;
    s32 y = (s32)pt.y;
    
    Blocky_Number_Info info = get_blocky_number_info(render_commands->view, value, raw_size);
    if(transform.flags & render_transform_flag_right_anchor_number)
    {
        x -= info.total_screen_width;
    }
    
    s32 size = info.scaled_font_size;
    for(int digit_index = (info.digits_count - 1); digit_index > -1; --digit_index)
    {
        Vertex_XY_RGBA verts[number_total_block_count * 6];
        Vertex_XY_RGBA *v = verts;
        
        s32 digit_x = x;
        s32 digit_y = y;
        
        for(u32 row = 0; row < number_block_row_count; ++row)
        {
            for(u32 col = 0; col < number_block_col_count; ++col)
            {
                int index = number_total_block_count - (number_block_col_count * row);
                index -= number_block_col_count;
                index += col;
                
                b32 fill = number_blocks[info.digits[digit_index]].fills[index];
                
                Vec4 actual_colour = colour;
                if(!fill)
                {
                    actual_colour.a = 0.0f;
                }
                
                *v++ = vertex_xy_rgba((f32)digit_x, (f32)digit_y, actual_colour);
                *v++ = vertex_xy_rgba((f32)(digit_x + size), (f32)digit_y, actual_colour);
                *v++ = vertex_xy_rgba((f32)digit_x, (f32)(digit_y + size), actual_colour);
                
                *v++ = vertex_xy_rgba((f32)digit_x, (f32)(digit_y + size), actual_colour);
                *v++ = vertex_xy_rgba((f32)(digit_x + size), (f32)digit_y, actual_colour);
                *v++ = vertex_xy_rgba((f32)(digit_x + size), (f32)(digit_y + size), actual_colour);
                
                digit_x += size;
            }
            
            digit_x = x;
            digit_y += size;
            
        }
        
        push_vertices(render_commands, (f32 *)verts, array_count(verts), vertex_flag_xy | vertex_flag_rgba, vec4(1.0f), transform_flags(render_transform_flag_screen_coords, transform));        
        
        x += size * (number_block_col_count + 1);
    }
    
    f32 right_side = screen_to_ortho(render_commands->view, x, 0).x;
    return right_side;
}

struct Line_Quad
{
    union
    {
        Vec2 points[4];
        
        struct
        {
            Vec2 bl;
            Vec2 br;
            Vec2 tr;
            Vec2 tl;
        };
    };
    
};

internal Line_Quad line_quad_points(Vec2 start_pos, Vec2 end_pos, Render_Transform transform = default_transform())
{
    Line_Quad result;
    
    Vec2 dir = normalize_or_zero(end_pos - start_pos);
    Vec2 normal = vec2(-dir.y, dir.x);
    Vec2 offset = normal * (transform.line_thickness * 0.5f);
    
    result.bl = start_pos - offset;
    result.br = end_pos - offset;
    result.tr = end_pos + offset;
    result.tl = start_pos + offset;
    
    return result;
}

internal void push_line(Game_Render_Commands *commands, Vec2 start_pos, Vec4 start_colour, Vec2 end_pos, Vec4 end_colour, Render_Transform transform = default_transform())
{
    Line_Quad line = line_quad_points(start_pos, end_pos, transform);
    
    Vertex_XY_RGBA verts[] =
    {
        { line.bl, start_colour },
        { line.br, end_colour },
        { line.tl, start_colour },
        
        { line.tl, start_colour },
        { line.br, end_colour },
        { line.tr, end_colour }
    };
    
    push_vertices(commands, (r32 *)verts, array_count(verts), vertex_flag_xy | vertex_flag_rgba, vec4(1.0f), transform);
}

internal void push_line(Game_Render_Commands *commands, Vec2 start_pos, Vec2 end_pos, Vec4 colour, Render_Transform transform = default_transform())
{
    push_line(commands, start_pos, colour, end_pos, colour, transform);
}

internal void push_line_batch(Game_Render_Commands *commands, Vec2 *points, u32 points_count, Vec4 colour, Render_Transform transform = default_transform())
{
    Temporary_Memory temp_mem = begin_temporary_memory(commands->temp_arena);
    assert((points_count % 2) == 0);
    
    Vec2 *verts = push_array(commands->temp_arena, Vec2, points_count);
    u32 vert_index = 0;
    for(u32 point_index = 0; point_index < (points_count - 1); point_index += 2)
    {
        Vec2 start_pos = points[point_index];
        Vec2 end_pos = points[point_index + 1];
        
        Vec2 dir = normalize_or_zero(end_pos - start_pos);
        Vec2 normal = vec2(-dir.y, dir.x);
        Vec2 offset = normal * (transform.line_thickness * 0.5f);
        
        Vec2 bl = start_pos - offset;
        Vec2 br = end_pos - offset;
        Vec2 tr = end_pos + offset;
        Vec2 tl = start_pos + offset;
        
        verts[vert_index++] = bl;
        verts[vert_index++] = br;
        verts[vert_index++] = tl;
        verts[vert_index++] = tl;
        verts[vert_index++] = br;
        verts[vert_index++] = tr;
    }
    
    push_vertices(commands, (f32 *)verts, vert_index, vertex_flag_xy, colour, transform);
    
    end_temporary_memory(temp_mem);
}

internal void push_line_points(Game_Render_Commands *commands, Vec2 *points, u32 points_count, Vec4 colour, Render_Transform transform = default_transform())
{
    // NOTE: Create a quad for each line
    assert(points_count > 1);
    
    for(u32 point_index = 0; point_index < (points_count - 1); ++point_index)
    {
        push_line(commands, points[point_index], colour, points[point_index + 1], colour, transform);
    }
    
    if(transform.flags & render_transform_flag_wrap_line_points)
    {
        push_line(commands, points[(points_count - 1)], colour, points[0], colour, transform);
    }
}

struct Bendy_Quad_Points
{
    Vec2 points[8];
};

internal Bendy_Quad_Points get_bendy_quad_points(f32 corner_indent)
{
    Bendy_Quad_Points result;
    
    Vec2 bl = vec2(0.0f);
    Vec2 br = vec2(1.0f, 0.0f);
    Vec2 tr = vec2(1.0f);
    Vec2 tl = vec2(0.0f, 1.0f);
    Vec2 center = vec2(0.5f);
    
    f32 bottom_y = bl.y;
    f32 left_x = bl.x;
    f32 right_x = tr.x;
    f32 top_y = tr.y;
    
    f32 offset = corner_indent * 0.5f;
    bl += (vec2(1.0f, 1.0f)) * offset;
    br += (vec2(-1.0f, 1.0f)) * offset;
    tr += (vec2(-1.0f, -1.0f)) * offset;
    tl += (vec2(1.0f, -1.0f)) * offset;
    
    Vec2 *p = result.points;
    
    *p++ = bl;
    *p++ = vec2(center.x, bottom_y);
    *p++ = br;
    *p++ = vec2(right_x, center.y);
    *p++ = tr;
    *p++ = vec2(center.x, top_y);
    *p++ = tl;
    *p++ = vec2(left_x, center.y);
    
    assert( (p - result.points) == array_count(result.points) );
    
    return result;
}

internal void push_bendy_quad(Game_Render_Commands *commands, Vec2 position, Vec2 scale, Vec4 corner_colour, Vec4 center_colour, f32 corner_indent, Render_Transform transform = default_transform())
{
    Bendy_Quad_Points bendy_quad = get_bendy_quad_points(corner_indent);
    transform_vertices(bendy_quad.points, bendy_quad.points, array_count(bendy_quad.points), position, scale);
    
    Vec2 center = position + scale/2.0f;
    
    Vertex_XY_RGBA verts[array_count(bendy_quad.points)*3];    
    for(u32 point_index = 0; point_index < array_count(bendy_quad.points); ++point_index)
    {
        Vec2 point_a = bendy_quad.points[point_index];
        Vec2 point_b = bendy_quad.points[ ((point_index + 1) % array_count(bendy_quad.points)) ];
        
        verts[point_index*3 + 0] = vertex_xy_rgba(center, center_colour);
        verts[point_index*3 + 1] = vertex_xy_rgba(point_a, corner_colour);
        verts[point_index*3 + 2] = vertex_xy_rgba(point_b, corner_colour);
    }
    
    push_vertices(commands, (f32 *)verts, array_count(verts), vertex_flag_xy | vertex_flag_rgba, vec4(1.0f), transform);
}

internal void push_shape_points(Game_Render_Commands *commands, Vec2 *points, u32 points_count, Vec4 colour, Render_Transform transform = default_transform())
{
    Temporary_Memory temp_mem = begin_temporary_memory(commands->temp_arena);
    
    assert(points_count >= 3);
    
    Vec2 interior_point = (points[0] + points[1] + points[2]) / 3.0f;
    
    u32 verts_count = points_count*3;
    Vec2 *verts = push_array(commands->temp_arena, Vec2, verts_count);
    for(u32 point_index = 0; point_index < points_count; ++point_index)
    {
        Vec2 point_a = points[point_index];
        // NOTE: We'll need to loop around to first point for last triangle
        Vec2 point_b = points[(point_index + 1) % points_count];
        
        verts[point_index*3 + 0] = interior_point;
        verts[point_index*3 + 1] = point_a;
        verts[point_index*3 + 2] = point_b;
    }
    
    push_vertices(commands, (f32*)verts, verts_count, vertex_flag_xy, colour, transform);
    
    end_temporary_memory(temp_mem);
}

internal void push_shape_points(Game_Render_Commands *commands, Vec2 *points, u32 points_count, Vec2 position, Vec2 scale, Vec4 colour, Render_Transform transform = default_transform())
{
    Temporary_Memory temp_mem = begin_temporary_memory(commands->temp_arena);
    
    Vec2 *transformed_points = push_array(commands->temp_arena, Vec2, points_count);    
    transform_vertices(points, transformed_points, points_count, position, scale);
    
    // NOTE: Because all the points are in world space, we need to add offset the rotation point
    transform.rotation_pt += position;
    
    push_shape_points(commands, transformed_points, points_count, colour, transform);
    
    end_temporary_memory(temp_mem);
}

struct Indented_Triangle_World_Points
{
    // NOTE: Points are in world space
    Vec2 points[6];
    
    Render_Transform transform;
};

internal Indented_Triangle_World_Points get_indented_triangle_world_points(Vec2 position, Vec2 scale, Render_Transform transform = default_transform())
{
    Indented_Triangle_World_Points result;
    
    // NOTE: Need to make triangle out of two quads to get "flat" corners
    f32 corner_indent = transform.corner_indent;
    
    Vec2 original_position = position;
    position.x -= transform.corner_indent;
    scale += vec2(transform.corner_indent*2.0f, transform.corner_indent);
    
    Vec2 tm = position + vec2(scale.x/2.0f, scale.y);
    Vec2 bl = position;
    Vec2 br = vec2(position.x + scale.x, position.y);
    
    Vec2 left_side_dir = normalize_or_zero(tm - bl);
    Vec2 right_side_dir = normalize_or_zero(tm - br);
    
    Vec2 bottom_tri_top_left = vec2(bl.x + corner_indent, position.y + corner_indent);
    Vec2 bottom_tri_top_right = vec2(br.x - corner_indent, position.y + corner_indent);
    
    Vec2 bottom_tri_bottom_left = vec2(bl.x + corner_indent, bl.y);    
    Vec2 bottom_tri_bottom_right = vec2(br.x - corner_indent, br.y);
    
    Vec2 top_tri_top_left = tm - vec2(corner_indent*0.5f, corner_indent);
    Vec2 top_tri_top_right = tm + vec2(corner_indent*0.5f, -corner_indent);
    
    Vec2 *points = result.points;
    
    *points++ = bottom_tri_bottom_left;
    *points++ = bottom_tri_bottom_right;
    *points++ = bottom_tri_top_right;
    *points++ = top_tri_top_right;
    *points++ = top_tri_top_left;
    *points++ = bottom_tri_top_left;
    
    transform_vertices(result.points, result.points, array_count(result.points), vec2(0.0f), vec2(1.0f), transform.rotation, transform.rotation_pt + original_position);
    
    result.transform = transform;
    // NOTE: Points already in world space, so don't need to do any rotation
    result.transform.rotation = 0.0f; 
    
    return result;
}

internal void push_triangle(Game_Render_Commands *commands, Vec2 position, Vec2 scale, Vec4 colour, Render_Transform transform = default_transform())
{
    if(transform.corner_indent > 0.0f)
    {
        Indented_Triangle_World_Points indented_triangle = get_indented_triangle_world_points(position, scale, transform);
        
        push_shape_points(commands, indented_triangle.points, array_count(indented_triangle.points), colour, indented_triangle.transform);
    }
    else
    {
        Vec2 verts[] =
        {
            vec2(0.0f, 0.0f),
            vec2(1.0f, 0.0f),
            vec2(0.5f, 1.0f)
        };
        
        push_vertices(commands, (f32 *)verts, array_count(verts), vertex_flag_xy, position, scale, colour, transform);
    }    
}

internal void push_triangle_outline(Game_Render_Commands *commands, Vec2 position, Vec2 scale, Vec4 colour, Render_Transform transform = default_transform())
{
    Vec2 line_points[] =
    {
        position,
        vec2(position.x + scale.x, position.y),
        vec2(position.x + (scale.x*0.5f), position.y + scale.y),
        position
    };
    
    // NOTE: Because all the points are in world space, we need to add offset the rotation point
    transform.rotation_pt += position;
    push_line_points(commands, line_points, array_count(line_points), colour, transform);
}

struct Star_Points
{
    Vec2 points[8];
};
internal Star_Points get_star_points(void)
{
    f32 one_third = 1.0f / 3.0f;
    f32 two_thirds = 2.0f / 3.0f;
    
    Star_Points result;
    
    // NOTE: Builds the points from the bottom-center and goes clockwise
    result.points[0] = vec2(0.5f, 0.0f);
    result.points[1] = vec2(one_third, one_third);
    result.points[2] = vec2(0.0f, 0.5f);
    result.points[3] = vec2(one_third, two_thirds);
    result.points[4] = vec2(0.5f, 1.0f);
    result.points[5] = vec2(two_thirds, two_thirds);
    result.points[6] = vec2(1.0f, 0.5f);
    result.points[7] = vec2(two_thirds, one_third);
    
    return result;
}

#define dodecagon_triangle_angle_radians degrees_to_radians(30.0f)
struct Dodecagon_Points
{
    Vec2 points[12];
};

internal Dodecagon_Points get_dodecagon_points(void)
{
    Dodecagon_Points result;
    // NOTE: This line is just here to stop compiler complaining about "possible uninitialized variable"
    result.points[0].x = 0.0f;
    
    for(u32 point_index = 0; point_index < array_count(result.points); ++point_index)
    {
        f32 rot = point_index * dodecagon_triangle_angle_radians;
        result.points[point_index] = vec2(cosf(rot), sinf(rot));
        
        // NOTE: Transform from [-1 to 1] to [0, 1]
        result.points[point_index] = (result.points[point_index] + 1.0f) / 2.0f;
    }
    
    return result;
};

#define hexagon_triangle_angle_radians degrees_to_radians(60.0f)
struct Hexagon_Points
{
    Vec2 points[6];
};
internal Hexagon_Points get_hexagon_points(void)
{
    Hexagon_Points result;
    // NOTE: This line is just here to stop compiler complaining about "possible uninitialized variable"
    result.points[0].x = 0.0f;
    
    for(u32 point_index = 0; point_index < array_count(result.points); ++point_index)
    {
        f32 rot = point_index * hexagon_triangle_angle_radians;
        result.points[point_index] = vec2(cosf(rot), sinf(rot));
        
        // NOTE: Transform from [-1, 1] to [0, 1]
        result.points[point_index] = (result.points[point_index] + 1.0f) / 2.0f;
    }
    
    return result;
}

internal u32 build_joined_outline_vertices_NEW(Vec2 *points, u32 points_count, Vec2 *verts, Render_Transform transform)
{
    // NOTE: Using proper joined up quads for outline rather than adjacent lines
    
    u32 points_used = 0;
    u32 increment = 1;
    
    if(transform.flags & render_transform_flag_outline_gaps)
    {
        increment = 2;
    }
    
    Vec2 center = vec2(0.0f);
    for(u32 point_index = 0; point_index < points_count; ++point_index)
    {
        center += points[point_index];
    }
    center /= (f32)points_count;
    
    for(u32 point_index = 0; point_index < points_count; (point_index += increment))
    {
        Vec2 point_a = points[point_index];
        Vec2 point_b = points[((point_index + 1) % points_count)];
        
        Vec2 point_a_to_center = point_a + (normalize_or_zero(center - point_a) * transform.line_thickness);
        Vec2 point_b_to_center = point_b + (normalize_or_zero(center - point_b) * transform.line_thickness);
        
        u32 vi = points_used*6;
        verts[vi++] = point_a_to_center;
        verts[vi++] = point_a;
        verts[vi++] = point_b_to_center;
        
        verts[vi++] = point_b_to_center;
        verts[vi++] = point_a;
        verts[vi++] = point_b;
        
        ++points_used;
    }
    
    return points_used;
}

internal u32 build_joined_outline_vertices(Vec2 position, Vec2 scale, Vec2 *points, u32 points_count, Vec2 *verts, Render_Transform transform)
{
    transform_vertices(points, points, points_count, position, scale);
    
    // NOTE: Using proper joined up quads for outline rather than adjacent lines
    
    u32 points_used = 0;
    u32 increment = 1;
    
    if(transform.flags & render_transform_flag_outline_gaps)
    {
        increment = 2;
    }
    
    Vec2 center = (position + scale/2.0f);
    
    for(u32 point_index = 0; point_index < points_count; (point_index += increment))
    {
        Vec2 point_a = points[point_index];
        Vec2 point_b = points[((point_index + 1) % points_count)];
        
        Vec2 point_a_to_center = point_a + (normalize_or_zero(center - point_a) * transform.line_thickness);
        Vec2 point_b_to_center = point_b + (normalize_or_zero(center - point_b) * transform.line_thickness);
        
        u32 vi = points_used*6;
        verts[vi++] = point_a_to_center;
        verts[vi++] = point_a;
        verts[vi++] = point_b_to_center;
        
        verts[vi++] = point_b_to_center;
        verts[vi++] = point_a;
        verts[vi++] = point_b;
        
        ++points_used;
    }
    
    return points_used;
}

internal void push_hexagon_outline(Game_Render_Commands *commands, Vec2 position, Vec2 scale, Vec4 colour, Render_Transform transform = default_transform())
{
    Hexagon_Points hexagon = get_hexagon_points();
    
    // NOTE: Because all the points are in world space, we need to add offset the rotation point
    transform.rotation_pt += position;
    
    if(transform.flags & render_transform_flag_joined_outline)
    {
        Vec2 verts[array_count(hexagon.points)*6];
        u32 points_used = build_joined_outline_vertices(position, scale, hexagon.points, array_count(hexagon.points), verts, transform);
        
        push_vertices(commands, (f32 *)verts, points_used * 6, vertex_flag_xy, colour, transform);        
    }
    else
    {
        transform_vertices(hexagon.points, hexagon.points, array_count(hexagon.points), position, scale);
        
        push_line_points(commands, hexagon.points, array_count(hexagon.points), colour, transform_flags(render_transform_flag_wrap_line_points, transform));   
    }
    
}

internal void push_shape_points_outline(Game_Render_Commands *commands, Vec2 *points, u32 points_count, Vec4 colour, Render_Transform transform = default_transform())
{
    Temporary_Memory temp_mem = begin_temporary_memory(commands->temp_arena);
    
    if(transform.flags & render_transform_flag_joined_outline)
    {
        u32 verts_count = points_count*6;
        Vec2 *verts = push_array(commands->temp_arena, Vec2, verts_count);
        
        u32 points_used = build_joined_outline_vertices_NEW(points, points_count, verts, transform);
        
        push_vertices(commands, (f32 *)verts, points_used * 6, vertex_flag_xy, colour, transform);        
    }
    else
    {
        push_line_points(commands, points, points_count, colour, transform_flags(render_transform_flag_wrap_line_points, transform));   
    }
    
    end_temporary_memory(temp_mem);
}

internal void push_shape_points_outline(Game_Render_Commands *commands, Vec2 *points, u32 points_count, Vec2 position, Vec2 scale, Vec4 colour, Render_Transform transform = default_transform())
{
    Temporary_Memory temp_mem = begin_temporary_memory(commands->temp_arena);
    
    // NOTE: Because all the points are in world space, we need to add offset the rotation point
    transform.rotation_pt += position;
    
    Vec2 *transformed_points = push_array(commands->temp_arena, Vec2, points_count);    
    transform_vertices(points, transformed_points, points_count, position, scale);
    
    push_shape_points_outline(commands, transformed_points, points_count, colour, transform);
    
    end_temporary_memory(temp_mem);
}

internal void push_dodecagon_outline(Game_Render_Commands *commands, Vec2 position, Vec2 scale, Vec4 colour, Render_Transform transform = default_transform())
{
    Dodecagon_Points dodecagon = get_dodecagon_points();
    // NOTE: Because all the points are in world space, we need to add offset the rotation point
    transform.rotation_pt += position;
    
    if(transform.flags & render_transform_flag_joined_outline)
    {
        Vec2 verts[array_count(dodecagon.points) * 6] = {};
        
        u32 points_used = build_joined_outline_vertices(position, scale, dodecagon.points, array_count(dodecagon.points), verts, transform);
        
        push_vertices(commands, (f32 *)verts, points_used * 6, vertex_flag_xy, colour, transform);        
    }
    else
    {
        transform_vertices(dodecagon.points, dodecagon.points, array_count(dodecagon.points), position, scale);
        
        push_line_points(commands, dodecagon.points, array_count(dodecagon.points), colour, transform_flags(render_transform_flag_wrap_line_points, transform));   
    }    
    
}

internal void push_dodecagon_blocky_blend(Game_Render_Commands *commands, Vec2 position, Vec2 scale, Vec4 inner_colour, Vec4 outer_colour, Render_Transform transform = default_transform())
{
    Dodecagon_Points points = get_dodecagon_points();
    
    push_shape_blocky_blend(commands, position, scale, inner_colour, outer_colour, points.points, array_count(points.points), transform);
}

internal void push_dodecagon(Game_Render_Commands *commands, Vec2 position, Vec2 scale, Vec4 colour, Render_Transform transform = default_transform())
{
    Dodecagon_Points dodecagon_points = get_dodecagon_points();
    
    Vec2 verts[array_count(dodecagon_points.points)*3];
    for(u32 point_index = 0; point_index < array_count(dodecagon_points.points); ++point_index)
    {
        Vec2 point_a = dodecagon_points.points[point_index];
        Vec2 point_b = dodecagon_points.points[(point_index + 1) % array_count(dodecagon_points.points)];
        
        verts[point_index*3 + 0] = vec2(0.5f);
        verts[point_index*3 + 1] = point_a;
        verts[point_index*3 + 2] = point_b;
    }
    
    push_vertices(commands, (f32 *)verts, array_count(verts), vertex_flag_xy, position, scale, colour, transform);
}

internal void push_shape_points_hollow(Game_Render_Commands *commands, Vec2 *points, u32 points_count, Vec4 edge_colour, Vec4 center_colour, Vec2 center_t, Render_Transform transform = default_transform())
{
    Temporary_Memory temp_mem = begin_temporary_memory(commands->temp_arena);
    
    u32 verts_count = points_count*3;
    Vertex_XY_RGBA *verts = push_array(commands->temp_arena, Vertex_XY_RGBA, verts_count);
    
    center_t = ((center_t + 1.0f) / 2.0f);
    
    Vec2 bl, tr;
    get_shape_dimensions(points, points_count, &bl, &tr);
    Vec2 scale = (tr - bl);
    Vec2 center = bl + vec2(scale.x*center_t.x, scale.y*center_t.y);
    
    for(u32 point_index = 0; point_index < points_count; ++point_index)
    {
        Vec2 point_a = points[point_index];
        Vec2 point_b = points[(point_index + 1) % points_count];
        
        verts[point_index*3 + 0] = vertex_xy_rgba(center, center_colour);
        verts[point_index*3 + 1] = vertex_xy_rgba(point_a, edge_colour);
        verts[point_index*3 + 2] = vertex_xy_rgba(point_b, edge_colour);
    }
    
    push_vertices(commands, (f32 *)verts, verts_count, vertex_flag_xy | vertex_flag_rgba, vec4(1.0f), transform);
    
    end_temporary_memory(temp_mem);
}

internal void push_dodecagon_hollow(Game_Render_Commands *commands, Vec2 position, Vec2 scale, Vec4 edge_colour, Vec4 center_colour, Vec2 center_t, Render_Transform transform = default_transform())
{
    Dodecagon_Points dodecagon_points = get_dodecagon_points();
    
    Vertex_XY_RGBA verts[array_count(dodecagon_points.points)*3];
    
    center_t = ((center_t + 1.0f) / 2.0f);
    
    for(u32 point_index = 0; point_index < array_count(dodecagon_points.points); ++point_index)
    {
        Vec2 point_a = dodecagon_points.points[point_index];
        Vec2 point_b = dodecagon_points.points[(point_index + 1) % array_count(dodecagon_points.points)];
        
        verts[point_index*3 + 0] = vertex_xy_rgba(center_t, center_colour);
        verts[point_index*3 + 1] = vertex_xy_rgba(point_a, edge_colour);
        verts[point_index*3 + 2] = vertex_xy_rgba(point_b, edge_colour);
    }
    
    push_vertices(commands, (f32 *)verts, array_count(verts), vertex_flag_xy | vertex_flag_rgba, position, scale, vec4(1.0f), transform);
}

internal void push_hexagon(Game_Render_Commands *commands, Vec2 position, Vec2 scale, Vec4 colour, Render_Transform transform = default_transform())
{
    Hexagon_Points hexagon = get_hexagon_points();
    push_shape_points(commands, hexagon.points, array_count(hexagon.points), position, scale, colour, transform);
}

internal void push_hexagon_lattice(Game_Render_Commands *commands, Vec2 position, Vec2 scale, Vec4 colour, f32 spacing, Render_Transform transform = default_transform())
{    
    Vec2 center = position + scale/2.0f;
    
    f32 offset = (f32)( (1.0 / 3.0) * sqrt(3.0) );
    
    Vec2 s = (scale/2.6f);
    
    Vec2 p = center - s/2.0f;
    Vec4 c = vec4(1.0f);
    
    f32 magic_hack_spacing = 0.606f * (s.x / 10.0f);
    
    //f32 spacing = scale.x - ((scale.x / s.x) * s.x);
    //f32 spacing = (f32)((offset * (1.0f/3.0f)) * s.x);
    //f32 spacing = (f32)((offset/8.0f) * scale.x);
    //f32 spacing = ((scale.x - s.x)/3.0f) * offset;
    
    //f32 spacing = 5.0f + (10.0f * ((sinf(rot_t*10.0f) + 1.0f) / 2.0f));
    
    
    // Center
    push_hexagon(commands, p, s, colour);
    if(transform.flags & render_transform_flag_hexagon_lattice_outline)
    {
        push_hexagon_outline(commands, p, s, vec4(transform.hexagon_lattice_outline_colour, 1.0f), transform);
    }
    //push_rect_outline(render_commands, p, s, vec4(1.0f));
    
    for(u32 border_index = 0; border_index < 6; ++border_index)
    {
        f32 rot = (hexagon_triangle_angle_radians*0.5f) + (border_index * hexagon_triangle_angle_radians);
        
        Vec2 orientation = normalize(vec2(cosf(rot), sinf(rot)));
        
        Vec2 sc = s + magic_hack_spacing + spacing;
        f32 size = magnitude(sc);// + magnitude(vec2(spacing));
        Vec2 pt = p + (orientation * (size * offset));
        
        push_hexagon(commands, pt, s, colour);
        if(transform.flags & render_transform_flag_hexagon_lattice_outline)
        {
            push_hexagon_outline(commands, pt, s, vec4(transform.hexagon_lattice_outline_colour, 1.0f), transform);
        }
        //push_rect_outline(render_commands, pt, s, vec4(1.0f));
    }
    
    //push_rect_outline(commands, center - scale/2.0f, scale, vec4(1.0f));
}

internal void push_rect_bordered(Game_Render_Commands *commands, Vec2 position, Vec2 scale, Vec4 inner_colour, Vec4 border_colour, Render_Transform transform = default_transform())
{
    push_rect(commands, position, scale, inner_colour);
    
    transform.line_thickness = min(transform.line_thickness, scale.x);
    transform.line_thickness = min(transform.line_thickness, scale.y);
    // NOTE: Top and bottom
    push_rect(commands, position + vec2(0.0f, scale.y - transform.line_thickness), vec2(scale.x, transform.line_thickness), border_colour);
    push_rect(commands, position, vec2(scale.x, transform.line_thickness), border_colour);
    // NOTE: Left and right
    push_rect(commands, position, vec2(transform.line_thickness, scale.y), border_colour);
    push_rect(commands, position + vec2(scale.x - transform.line_thickness, 0.0f), vec2(transform.line_thickness, scale.y), border_colour);
}

internal void push_curved_line(Game_Render_Commands *commands, u32 points_count, Vec2 start_pos, Vec4 start_colour, Vec2 end_pos, Vec4 end_colour, Vec2 control_point, Render_Transform transform = default_transform(), f32 end_thickness = -1.0f)
{
    Vec2 start = start_pos;
    Vec2 end = end_pos;
    Vec2 control = control_point;
    
    if(end_thickness < 0.0f)
    {
        end_thickness = transform.line_thickness;
    }
    
    Vec2 last_point = start_pos;
    Vec4 last_colour = start_colour;
    f32 last_thickness = transform.line_thickness;
    for(u32 points_index = 0; points_index < points_count; ++points_index)
    {
        f32 t = (1.0f / (f32)(points_count - 1)) * (f32)points_index;
        // NOTE: Bezier interpolation: ((1-t)^2)a + 2(1 - t)tb + (t^2)c
        Vec2 current_point = (square(1.0f - t) * start) + ((2.0f * (1.0f - t)) * t * control) + (square(t) * end);
        Vec4 current_colour = start_colour*(1.0f - t) + end_colour*t;
        f32 current_thickness = (1.0f - t)*transform.line_thickness + end_thickness*t;
        
        if(points_index > 0)
        {
            Render_Transform current_transform = transform;
            current_transform.line_thickness = current_thickness;
            push_line(commands, last_point, last_colour, current_point, current_colour, current_transform);
        }
        
        last_point = current_point;
        last_colour = current_colour;
        last_thickness = current_thickness;
    }
}

#if TWOSOME_INTERNAL

struct DEBUG_Glyph_Stats
{
    int ix0, iy0;
    int ix1, iy1;
    int bitmap_w, bitmap_h;
    
    int advance;
    int lsb;
    
    DEBUG_Glyph_Stats *next;
};

internal Asset *DEBUG_load_font(Game_Assets *assets, uint32 font_id);

internal void DEBUG_push_text(Game_Render_Commands *commands, char *text, f32 raw_ft_height, int x, int y, Vec4 colour)
{
    //NOTE: Actual font height
    f32 ft_height = (f32)((s32)(raw_ft_height * (commands->view.height / (f32)working_screen_height)));
    
    // NOTE: If screen_width/screen_height goes to 0 the font size goes to 0 and breaks font rendering, but
    // we don't care coz this is DEBUG code!
    if(ft_height > 0.0f)
    {
        size_t text_buffer_size = strlen(text) + 1;
        if(text_buffer_size > 0)
        {
            Temporary_Memory temp_mem = begin_temporary_memory(commands->temp_arena);
            
            Asset *asset = DEBUG_load_font(commands->assets, asset_DEBUG_font);
            DEBUG_Loaded_Font *font = &asset->DEBUG_font;
            if(font->font_file_buffer)
            {
                
                u32 texture_width = 0;
                u32 texture_height = 0;
                
                float scale = stbtt_ScaleForPixelHeight(&font->stb_font, ft_height);
                int ascent, descent, line_gap;
                stbtt_GetFontVMetrics(&font->stb_font, &ascent, &descent, &line_gap);
                int baseline = (int)(ascent * scale);
                DEBUG_Glyph_Stats *first_stat = push_struct(commands->temp_arena, DEBUG_Glyph_Stats);
                DEBUG_Glyph_Stats *current_stat = first_stat;
                
                //
                // NOTE: Calculate texture size
                //
                {
                    s32 ch = 0;
                    
                    s32 line_width = 0;
                    s32 longest_line = 0;
                    s32 num_lines = 0;
                    s32 last_line_below_baseline = 0;    
                    
                    // NOTE: Work out how big texture needs to be for this piece of text
                    while(text[ch])
                    {
                        if(text[ch] != '\n')
                        {
                            stbtt_GetCodepointHMetrics(&font->stb_font, text[ch], &current_stat->advance, &current_stat->lsb);
                            
                            stbtt_GetCodepointBitmapBox(&font->stb_font, text[ch], scale, scale, &current_stat->ix0, &current_stat->iy0, &current_stat->ix1, &current_stat->iy1);
                            
                            current_stat->bitmap_w = current_stat->ix1 - current_stat->ix0;
                            current_stat->bitmap_h = current_stat->iy1 - current_stat->iy0;
                            
                            int below_baseline = current_stat->bitmap_h + current_stat->iy0;
                            if(below_baseline > last_line_below_baseline)
                            {
                                last_line_below_baseline = below_baseline;
                            }
                            
                            line_width += (int)(current_stat->advance * scale);
                            
                            current_stat->next = push_struct(commands->temp_arena, DEBUG_Glyph_Stats);
                            current_stat = current_stat->next;
                        }
                        else
                        {
                            ++num_lines;
                            if(line_width > longest_line)
                            {
                                longest_line = line_width;
                            }
                            line_width = 0;
                            last_line_below_baseline = 0;
                        }
                        
                        ++ch;
                    }
                    
                    texture_height += ((num_lines + 1) * (int)((float)(ascent - (descent + line_gap)) * scale)) + last_line_below_baseline;
                    texture_width = (line_width > longest_line ? line_width : longest_line) + 1;
                }
                
                Memory_Index texture_data_size = texture_width * texture_height;        
                DEBUG_Render_Entry_Text *t = push_render_element_padded(commands, DEBUG_Render_Entry_Text, DEBUG_shader_type_text, texture_data_size, draw_order_transform(entity_draw_order_DEBUG_text));
                if(t)
                {
                    //
                    // NOTE: Rendering glyph bitmaps into the text
                    //
                    void *texture_data = commands->push_buffer_at + sizeof(*t);
                    {
                        u32 cursor_x = 0;
                        u32 cursor_y = baseline;
                        
                        Memory_Index text_length_inc_null_terminator = 1;
                        
                        u8 *data = (u8 *)texture_data;
                        zero_buffer(data, texture_data_size);
                        
                        current_stat = first_stat;
                        s32 ch = 0;
                        while(text[ch])
                        {        
                            if(text[ch] != '\n')
                            {
                                unsigned char *bitmap = (unsigned char *)push_size(commands->temp_arena, current_stat->bitmap_w * current_stat->bitmap_h);
                                
                                int stride = current_stat->bitmap_w;
                                stbtt_MakeCodepointBitmap(&font->stb_font, bitmap, current_stat->bitmap_w, current_stat->bitmap_h, current_stat->bitmap_w, scale, scale, text[ch]);
                                
                                assert(cursor_x < texture_width && cursor_y < texture_height);
                                
                                // NOTE: Copy glyph bitmap into big texture data: one row at a time...
                                for(s32 row_index = 0; row_index < current_stat->bitmap_h; ++row_index)
                                {
                                    int x = cursor_x + (int)(current_stat->lsb * scale);
                                    int y = (int)(cursor_y + current_stat->iy0);
                                    
                                    u32 bitmap_offset = (((y + row_index) * texture_width) + x);
                                    u8 *bitmap_dest = data + bitmap_offset;
                                    assert((bitmap_offset + current_stat->bitmap_w) < (texture_width * texture_height));
                                    memcpy(bitmap_dest, (bitmap + (row_index * current_stat->bitmap_w)), current_stat->bitmap_w);
                                }
                                
                                cursor_x += (int)(current_stat->advance * scale);
                                current_stat = current_stat->next;
                            }
                            else
                            {
                                cursor_x = 0;
                                cursor_y += (int)((float)(ascent - (descent + line_gap)) * scale);
                            }
                            
                            ++ch;
                            ++text_length_inc_null_terminator;
                        }
                    }
                    
                    t->x = (s32)((r32)x * ((r32)commands->view.width / (r32)virtual_screen_width));
                    t->y = (s32)((r32)y * ((r32)commands->view.height / (r32)virtual_screen_height));
                    t->colour = colour;
                    t->width = texture_width;
                    t->height = texture_height;
                    t->texture_data = texture_data;
                }   
            }
            
            end_temporary_memory(temp_mem);
        }   
    }
}

#endif // NOTE: TWOSOME_INTERNAL
