#ifndef TWOSOME_COLLISION_H
#define TWOSOME_COLLISION_H


struct AABB
{
    Vec2 center;
    Vec2 last_center;
    Vec2 extents;
};

struct OBB
{
    Vec2 center;
    Vec2 last_center;
    
    Vec2 extents;

    float rotation;
    float last_rotation;
    
    Vec2 basis[2];
    Vec2 last_basis[2];
};

#endif
