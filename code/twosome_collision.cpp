#include "twosome_collision.h"


internal bool32 point_vs_circle(Vec2 point, Vec2 center, real32 radius)
{
    bool32 result = (distance(center, point) <= radius);
    return result;
}

internal bool32 point_vs_rect(Vec2 point, Vec2 bl, Vec2 scale)
{
    bool32 result = (point.x >= bl.x && point.x <= (bl.x + scale.x) && point.y >= bl.y && point.y <= (bl.y + scale.y));
    return result;
}

internal b32 point_vs_polygon(Vec2 test_point, Vec2 *polygon_points, u32 polygon_points_count)
{
    b32 result = true;
    
    assert(polygon_points_count >= 3);

    // NOTE: This function assumes anti-clockwise winding, if assumption turns out wrong then
    // uncomment the interior_point bits
    
    //Vec2 interior_point = (polygon_points[0] + polygon_points[1] + polygon_points[2]) / 3.0f;

    for(u32 polygon_point_index = 0; polygon_point_index < polygon_points_count; ++polygon_point_index)
    {
        Vec2 edge = (polygon_points[ (polygon_point_index + 1) % polygon_points_count ] - polygon_points[polygon_point_index]);

        //Vec2 edge_to_interior_point = (interior_point - polygon_points[polygon_point_index]);
        Vec2 edge_to_test_point = (test_point - polygon_points[polygon_point_index]);
        
        Vec2 edge_normal = vec2(-edge.y, edge.x);

        f32 same_side_test = /*dot_product(edge_normal, edge_to_interior_point) * */dot_product(edge_normal, edge_to_test_point);
        if(same_side_test < 0.0f)
        {
            result = false;
            break;
        }
    }

    return result;
}

internal b32 point_vs_polygon(Vec2 test_point, Vec2 *polygon_points, u32 polygon_points_count, Vec2 position, Vec2 scale)
{
    b32 result = false;

    // NOTE: Do simple rect test first
    if(point_vs_rect(test_point, position, scale))
    {
        if(point_vs_polygon(test_point, polygon_points, polygon_points_count))
        {
            result = true;
        }
    }

    return result;
}

internal b32 same_side(Vec3 p1, Vec3 p2, Vec3 a, Vec3 b)
{
    Vec3 edge = (b - a);
    Vec3 cp1 = cross_product(edge, (p1 - a));
    Vec3 cp2 = cross_product(edge, (p2 - a));

    b32 result = (dot_product(cp1, cp2) >= 0.0f);

    return result;
}

internal b32 point_vs_triangle(Vec2 pt, Vec2 a, Vec2 b, Vec2 c)
{
    b32 result = false;
    
    Vec3 a_v3 = vec3(a, 0.0f);
    Vec3 b_v3 = vec3(b, 0.0f);
    Vec3 c_v3 = vec3(c, 0.0f);

    Vec3 pt_v3 = vec3(pt, 0.0f);
    
    if(same_side(pt_v3, a_v3, b_v3, c_v3))
    {
        if(same_side(pt_v3, b_v3, a_v3, c_v3))
        {
            if(same_side(pt_v3, c_v3, a_v3, b_v3))
            {
                result = true;
            }
        }
    }

    return result;
}

internal OBB make_obb_t(OBB a, float t)
{
    OBB obb = a;
    obb.center = a.last_center*(1.0f - t) + a.center*t;
    obb.basis[0] = normalize(a.last_basis[0]*(1.0f - t) + a.basis[0]*t);
    obb.basis[1] = normalize(a.last_basis[1]*(1.0f - t) + a.basis[1]*t);

    return obb;
}

internal OBB make_obb(Vec2 last_position, Vec2 position, float last_rotation, float rotation, Vec2 local_rotation_pt, Vec2 scale)
{
    OBB obb;
    obb.extents = scale / 2.0f;

    obb.rotation = rotation;
    obb.last_rotation = last_rotation;

    float last_cos_r = cosf(last_rotation);
    float last_sin_r = sinf(last_rotation);
    float cos_r = cosf(rotation);
    float sin_r = sinf(rotation);
    
    obb.last_basis[0] = normalize(vec2(last_cos_r, last_sin_r));
    obb.last_basis[1] = normalize(vec2(-last_sin_r, last_cos_r));
    obb.basis[0] = normalize(vec2(cos_r, sin_r));
    obb.basis[1] = normalize(vec2(-sin_r, cos_r));

    //Work out where the center is based on rotation
    Vec2 last_center_vector = obb.last_basis[0]*obb.extents.x + obb.last_basis[1]*obb.extents.y;
    Vec2 center_vector = obb.basis[0]*obb.extents.x + obb.basis[1]*obb.extents.y;

    Vec2 last_rotation_offset = local_rotation_pt - (obb.last_basis[0]*local_rotation_pt.x + obb.last_basis[1]*local_rotation_pt.y);

    Vec2 rotation_offset = local_rotation_pt - (obb.basis[0]*local_rotation_pt.x + obb.basis[1]*local_rotation_pt.y);
    
    obb.last_center = last_position + last_center_vector + last_rotation_offset;
    obb.center = position + center_vector + rotation_offset;
    
    return obb;
}

internal OBB make_obb(Vec2 position, Vec2 scale, float rotation, Vec2 rotation_pt)
{
    OBB obb = make_obb(position, position, rotation, rotation, rotation_pt, scale);
    return obb;
}

internal OBB make_obb(AABB a)
{
    OBB obb = make_obb(a.center - a.extents, a.extents * 2.0f, 0, vec2(0));
    return obb;
}

internal AABB make_aabb(Vec2 last_bl, Vec2 bl, Vec2 scale)
{
    AABB a;
    a.extents = scale / 2.0f;
    a.last_center = last_bl + a.extents;
    a.center = bl + a.extents;

    return a;
}

internal AABB make_aabb(Vec2 bl, Vec2 scale)
{
    AABB a = make_aabb(bl, bl, scale);
    return a;
}

internal float aabb_min(AABB a, int axis_index)
{
    float result = a.center[axis_index] - a.extents[axis_index];
    return result;
}

internal float aabb_max(AABB a, int axis_index)
{
    float result = a.center[axis_index] + a.extents[axis_index];
    return result;
}

internal bool aabb_overlap(AABB a, AABB b)
{
    Vec2 t = b.center - a.center;
    bool result = fabs(t.x) <= (a.extents.x + b.extents.x) && fabs(t.y) <= (a.extents.y + b.extents.y);
    return result;
}

internal b32 obb_overlap(OBB a, OBB b, Vec2 *i0 = 0, Vec2 *i1 = 0)
{
    b32 result = true;

    Vec2 v = b.center - a.center;
    // NOTE: Translation, in A's frame 
    Vec2 translation_wrt_a = vec2(dot_product(v, a.basis[0]), dot_product(v, a.basis[1]));
    // NOTE: Basis in A's frame of reference. So A's bases become x and y axis for B's bases to be projected onto.
    Vec2 projs_on_a[2] =
    {
        // NOTE: Component projection of B1 onto A1, and B2 onto A1 (i.e. onto x bases)
        vec2( dot_product(a.basis[0], b.basis[0]), dot_product(a.basis[0], b.basis[1]) ),
        // NOTE: Component projection of B1 onto A2, and B2 onto A2 (i.e. onto y bases)
        vec2( dot_product(a.basis[1], b.basis[0]), dot_product(a.basis[1], b.basis[1]) )
    };
    
    /* NOTE: Use separating axis test for the 4 potential separating axis.
             If a separating axis couldn't be found, the two boxes overlap. */
    for(int basis_axis_index = 0; basis_axis_index < 2; ++basis_axis_index)
    {
        r32 ra = a.extents[basis_axis_index];
        r32 rb = (b.extents.x * fabsf(projs_on_a[basis_axis_index].x)) + (b.extents.y * fabsf(projs_on_a[basis_axis_index].y));
        r32 t = fabsf(translation_wrt_a[basis_axis_index]);
        
        if(t > (ra + rb))
        {
            result = false;
        }
#if 0
        else
        {
            if(basis_axis_index == 0 && i0 && i1)
            {
                Vec2 st = translation_wrt_a;
                i0->x = (sign_real32(st.x) * a.extents.x);
                i0->y = (st.y - (sign_real32(st.y) * a.extents.y));

                *i0 = a.center + vec2(dot_product(*i0, -a.basis[0]), dot_product(*i0, -a.basis[1]));
            }
        }
#endif
    }

    for(int basis_axis_index = 0; basis_axis_index < 2; ++basis_axis_index)
    {
        r32 ra = a.extents.x*fabsf(projs_on_a[0][basis_axis_index]) + a.extents.y*fabsf(projs_on_a[1][basis_axis_index]);
        r32 rb = b.extents[basis_axis_index];
        r32 t = fabsf(translation_wrt_a.x*projs_on_a[0][basis_axis_index] + translation_wrt_a[1]*projs_on_a[1][basis_axis_index]);

        if(t > (ra + rb))
        {
            result = false;
        }
    }
    
    return result;
}

internal bool obb_vs_obb(OBB a, OBB b)
{
    if(obb_overlap(a, b))
    {
        return true;
    }
    
    // NOTE: We are sampling positions in between the amount moved, in increments of the smallest edge of the largest obb, not great but this is only way I know how to do some kind of continuous collision with OBBs
    
    Vec2 va = a.center - a.last_center;
    r32 va_length = magnitude(va);
    Vec2 vb = b.center - b.last_center;
    r32 vb_length = magnitude(vb);

    r32 displacement = va_length > vb_length ? va_length : vb_length;

    //Use larger obb
    OBB *larger_obb = &a;
    if((b.extents[0] + b.extents[1]) > (larger_obb->extents[0] + larger_obb->extents[1]))
    {
        larger_obb = &b;
    }

    r32 smaller_extent = min(larger_obb->extents[0], larger_obb->extents[1]);

    if(displacement >= smaller_extent)
    {
         r32 t_step = smaller_extent / displacement;

         int num_steps = (int)(displacement / smaller_extent);
         if(num_steps > 4)
         {
             assert(!"Doing over 4 overlap checks for obb displacement");
         }
         
         float t = 0.0f;
         while( 1 )
         {
             OBB t_a = make_obb_t(a, t);
             OBB t_b = make_obb_t(b, t);

             if(obb_overlap(t_a, t_b))
             {
                 return true;
             }

             if(t >= 1.0f)
             {
                 break;
             }
        
             t += t_step;
             if(t > 1.0f)
             {
                 t = 1.0f;
             }        
         }   
    }

    return false;
}

internal bool aabb_vs_aabb(AABB a, AABB b, float *u0)
{
    /*
     The displacement vector (v) (in A's frame of reference) of the two objects is checked to see if it crossed all the extents (min, max) of the rect between the two aabbs. ux = Xmax,a - Xmin,b / v.x gives the normalized times it took for x-extents and y-extents to overlap. The extents of the rect between two rects is used for u0 and extents of rect that includes both rects is used for u1. u0 needs to be less than or equal to u1.
    */
    
    *u0 = 0.0f;
    
    AABB prev_a = a;
    prev_a.center = prev_a.last_center;
    AABB prev_b = b;
    prev_b.center = prev_b.last_center;

    //First check if was overlapping in previous frame
    if(aabb_overlap(prev_a, prev_b))
    {
        return true;
    }
    
    Vec2 va = a.center - a.last_center;
    Vec2 vb = b.center - b.last_center;
    //In A's frame of reference
    Vec2 vba = vb - va;

    Vec2 u0_overlap = vec2(-1.0f);
    Vec2 u1_overlap = vec2(2.0f);
    for(int axis = 0; axis < 2; ++axis)
    {
        float a_min = aabb_min(prev_a, axis);
        float a_max = aabb_max(prev_a, axis);
        float b_min = aabb_min(prev_b, axis);
        float b_max = aabb_max(prev_b, axis);
        float v = vba[axis];

        if(a_max < b_min && v < 0.0f)
        {
            u0_overlap[axis] = (a_max - b_min) / v;   
        }
        else if(b_max < a_min && v > 0.0f)
        {
            u0_overlap[axis] = (a_min - b_max) / v;
        }
        else if((a_min >= b_min && a_min <= b_max) || (a_max >= b_min && a_max <= b_max) 
                || (b_min >= a_min && b_min <= a_max))
        {
            u0_overlap[axis] = 0.0f;
        }

        if(a_min < b_max && v < 0.0f)
        {
            u1_overlap[axis] = (a_min - b_max) / v;
        }
        else if(b_min < a_max && v > 0.0f)
        {
            u1_overlap[axis] = (a_max - b_min) / v;
        }
        else if((a_min >= b_min && a_min <= b_max) || (a_max >= b_min && a_max <= b_max)
                || (b_min >= a_min && b_min <= a_max))
        {
            u1_overlap[axis] = 1.0f;
        }

    } 

    if(u0_overlap.x >= 0.0f && u0_overlap.x <= 1.0f && u0_overlap.y >= 0.0f && u0_overlap.y <= 1.0f)
      //if(u0_overlap.x > 0.0f && u0_overlap.x < 1.0f && u0_overlap.y > 0.0f && u0_overlap.y < 1.0f)
    {
        *u0 = max(u0_overlap.x, u0_overlap.y);
        float u1 = min(u1_overlap.x, u1_overlap.y);

        bool result = *u0 <= u1;
        
        return result;
    } 

    return false;
}

internal b32 aabb_vs_aabb(AABB a, AABB b)
{
    r32 dud_u0;
    b32 result = aabb_vs_aabb(a, b, &dud_u0);
    return result;
}

internal bool aabb_circle_overlap(AABB aa, float circle_radius, Vec2 center, float *distance)
{
    *distance = 0.0f;
    
    //Find square of distance from sphere to the box
    for(int axis_index = 0; axis_index < 2; ++axis_index)
    {
        float c = center[axis_index];
        if(c < aabb_min(aa, axis_index))
        {
            float s = c - aabb_min(aa, axis_index);
            *distance += s*s;
        }
        else if(c > aabb_max(aa, axis_index))
        {
            float s = c - aabb_max(aa, axis_index);
            *distance += s*s;
        }
    }

    if(*distance <= circle_radius*circle_radius)
    {
        *distance = sqrtf(*distance);
        return true;
    }

    return false;
}

internal bool aabb_circle_overlap(AABB aa, float circle_radius, Vec2 center)
{
    float dud;
    bool result = aabb_circle_overlap(aa, circle_radius, center, &dud);
    return result;
}

internal bool line_vs_line(Vec2 a_start, Vec2 a_end, Vec2 b_start, Vec2 b_end)
{
    float denom = ((b_end.y - b_start.y) * (a_end.x - a_start.x)) - ((b_end.x - b_start.x) * (a_end.y - a_start.y));

    float epsilon = 0.0001f;
    if(fabs(denom) < epsilon)
    {
        return false;
    }

    float t_a = (((b_end.x - b_start.x) * (a_start.y - b_start.y)) - ((b_end.y - b_start.y) * (a_start.x - b_start.x))) / denom;
    if(t_a < 0.0f || t_a > 1.0f)
    {
        return false;
    }
        
    float t_b = (((a_end.x - a_start.x) * (a_start.y - b_start.y)) - ((a_end.y - a_start.y) * (a_start.x - b_start.x))) / denom;
    if(t_b < 0.0f || t_b > 1.0f)
    {
        return false;
    }
    
    return true;
}

internal bool line_vs_rect(Vec2 pt1, Vec2 pt2, Vec2 bl, Vec2 tr)
{
    bool result = true;

    if(!line_vs_line(pt1, pt2, vec2(bl.x, tr.y), tr))
    {
        if(!line_vs_line(pt1, pt2, bl, vec2(tr.x, bl.y)))
        {
            if(!line_vs_line(pt1, pt2, bl, vec2(bl.x, tr.y)))
            {
                if(!line_vs_line(pt1, pt2, vec2(tr.x, bl.y), tr))
                {
                    result = false;
                }
            }
        }
    }

    return result;
}

internal b32 circle_vs_circle(Vec2 a_center, r32 a_radius, Vec2 b_center, r32 b_radius)
{
    b32 result = magnitude(a_center - b_center) <= (a_radius + b_radius);
    return result;
}

internal bool circle_vs_line(Vec2 circle_center, float circle_radius, Vec2 line_start, Vec2 line_end)
{
    //Line points relative to circle center
    Vec2 local_start = vec2(line_start.x - circle_center.x, line_start.y - circle_center.y);
    Vec2 local_end = vec2(line_end.x - circle_center.x, line_end.y - circle_center.y);

    Vec2 line_delta = vec2(local_end.x - local_start.x, local_end.y - local_start.y);

    float a = (line_delta.x * line_delta.x) + (line_delta.y * line_delta.y);
    float b = 2.0f * ((local_start.x * line_delta.x) + (local_start.y * line_delta.y));
    float c = (local_start.x * local_start.x) + (local_start.y * local_start.y) - (circle_radius * circle_radius);

    float epsilon = 0.0001f;

    float t_1 = -1.0f;
    float t_2 = -1.0f;
    
    float delta = (b * b) - (4 * a * c);
    if(delta < 0.0f)
    {
        return false;
    }
    else if(delta < epsilon)
    {
        t_1 = -b / (2 * a);
    }
    else
    {
        float sqrt_delta = sqrtf(delta);

        t_1 = (-b + sqrt_delta) / (2.0f * a);
        t_2 = (-b - sqrt_delta) / (2.0f * a);
    }

    bool result = false;

    bool i_1 = (t_1 >= 0.0f && t_1 <= 1.0f);
    bool i_2 = (t_2 >= 0.0f && t_2 <= 1.0f);

    return (i_1 || i_2);
    
}
