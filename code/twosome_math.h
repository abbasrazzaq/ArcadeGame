#ifndef TWOSOME_MATH_H
#define TWOSOME_MATH_H


#define PI 3.14159265359f
#define degrees_to_radians(degrees) ((degrees) * (float)(PI / 180.0f))


union Vec2
{
    struct
    {
        f32 x, y;
    };
    
    struct
    {
        f32 u, v;
    };
    
    f32 e[2];

    f32 &operator[](int index)
    {
        assert(index > -1 && index < array_count(e));
        return e[index];
    }
};

union Vec3
{
    struct
    {
        f32 x, y, z;
    };
    struct
    {
        f32 r, g, b;
    };
    struct
    {
        Vec2 xy;
        f32 _dud_z;
    };
    
    f32 e[3];
    
    f32 &operator[](int index)
    {
        assert(index > -1 && index < array_count(e));
        return e[index];
    }    
};

union Vec4
{
    struct
    {
        f32 x, y, z, w;
    };
    struct
    {
        f32 r, g, b, a;
    };
    struct
    {
        Vec2 xy;
        f32 _dud_z;
        f32 _dud_w;
    };
    struct
    {
        Vec3 xyz;
        f32 _dud_w_2;
    };
    struct
    {
        Vec3 rgb;
        f32 _dud_a;
    };

    f32 e[4];

    f32 &operator[](int index)
    {
        assert(index > -1 && index < array_count(e));
        return e[index];
    }
};

union Mat3
{
    struct
    {
        f32 _11, _21, _31;
        f32 _12, _22, _32;
        f32 _13, _23, _33;
    };

    f32 e[9];
};

union Mat4
{
    struct
    {
        f32 _11, _21, _31, _41;
        f32 _12, _22, _32, _42;
        f32 _13, _23, _33, _43;
        f32 _14, _24, _34, _44;
    };
    
    f32 e[16];
};


//
// NOTE: Vec2
//
internal Vec2 vec2(f32 v)
{
    Vec2 result;
    result.x = v;
    result.y = v;
    return result;
}

internal Vec2 vec2(f32 x, r32 y)
{
    Vec2 result;
    result.x = x;
    result.y = y;
    return result;
}

internal Vec2 vec2i(s32 x, s32 y)
{
    Vec2 result = vec2((f32)x, (f32)y);
    return result;
}

internal Vec2 operator+(Vec2 lhs, Vec2 rhs)
{
    Vec2 result;
    result.x = lhs.x + rhs.x;
    result.y = lhs.y + rhs.y;
    
    return result;
}

internal Vec2 operator-(Vec2 lhs, Vec2 rhs)
{
    Vec2 result;
    result.x = lhs.x - rhs.x;
    result.y = lhs.y - rhs.y;

    return result;
}

internal Vec2 operator-(Vec2 v)
{
    Vec2 result;
    result.x = -v.x;
    result.y = -v.y;
    return result;
}

internal Vec2 operator+(Vec2 lhs, r32 rhs)
{
    Vec2 result;
    result.x = lhs.x + rhs;
    result.y = lhs.y + rhs;
    return result;
}

internal Vec2 operator-(Vec2 lhs, r32 rhs)
{
    Vec2 result;
    result.x = lhs.x - rhs;
    result.y = lhs.y - rhs;
    return result;
}

internal Vec2 operator*(Vec2 lhs, r32 rhs)
{
    Vec2 result;
    result.x = lhs.x * rhs;
    result.y = lhs.y * rhs;
    return result;
}

internal Vec2 operator*(f32 lhs, Vec2 rhs)
{
    Vec2 result = rhs * lhs;
    return result;
}

internal Vec2 operator/(Vec2 lhs, r32 rhs)
{
    Vec2 result;
    result.x = lhs.x / rhs;
    result.y = lhs.y / rhs;
    return result;
}

internal Vec2 &operator+=(Vec2 &lhs, Vec2 rhs)
{
    lhs = lhs + rhs;    
    return lhs;
}

internal Vec2 &operator-=(Vec2 &lhs, Vec2 rhs)
{
    lhs = lhs - rhs;
    return lhs;
}

internal Vec2 &operator+=(Vec2 &lhs, r32 rhs)
{
    lhs = lhs + rhs;
    return lhs;
}

internal Vec2 &operator-=(Vec2 &lhs, r32 rhs)
{
    lhs = lhs - rhs;
    return lhs;
}

internal Vec2 &operator*=(Vec2 &lhs, r32 rhs)
{
    lhs = lhs * rhs;
    return lhs;
}

internal Vec2 &operator/=(Vec2 &lhs, r32 rhs)
{
    lhs = lhs / rhs;
    return lhs;
}

internal f32 magnitude(Vec2 v)
{
    f32 result = sqrtf((v.x*v.x) + (v.y*v.y));
    return result;
}

internal f32 distance(Vec2 a, Vec2 b)
{
    f32 result = magnitude(a - b);
    return result;
}

internal Vec2 normalize(Vec2 v)
{
    f32 mag = magnitude(v);

    Vec2 result = vec2(v.x / mag, v.y / mag);
    return result;
}

internal Vec2 normalize_or_zero(Vec2 v)
{
    Vec2 result = vec2(0);
    f32 mag = magnitude(v);
    if(mag > 0.0f)
    {
        result = v / mag;
    }
    return result;
}

internal r32 dot_product(Vec2 a, Vec2 b)
{
    r32 result = (a.x * b.x) + (a.y * b.y);
    return result;
}

internal r32 angle_between(Vec2 a, Vec2 b, bool normalize_vectors)
{
    if(normalize_vectors)
    {
        a = normalize(a);
        b = normalize(b);
    }

    r32 result = acosf(dot_product(a, b));
    return result;
}

internal Vec2 clamp_v2(Vec2 v, Vec2 min, Vec2 max)
{
    Vec2 result = v;
    result.x = clamp(result.x, min.x, max.x);
    result.y = clamp(result.y, min.y, max.y);
    
    return result;
}

//
// NOTE: Vec3
//
internal Vec3 vec3(r32 v)
{
    Vec3 result;
    result.x = v;
    result.y = v;
    result.z = v;
    
    return result;
}

internal Vec3 vec3(Vec2 v, r32 z)
{
    Vec3 result;
    result.x = v.x;
    result.y = v.y;
    result.z = z;

    return result;
}

internal Vec3 vec3(r32 x, r32 y, r32 z)
{
    Vec3 result;
    result.x = x;
    result.y = y;
    result.z = z;
    
    return result;
}

internal Vec3 operator+(Vec3 lhs, Vec3 rhs)
{
    Vec3 result;
    result.x = lhs.x + rhs.x;
    result.y = lhs.y + rhs.y;
    result.z = lhs.z + rhs.z;
    
    return result;
}

internal Vec3 operator-(Vec3 lhs, Vec3 rhs)
{
    Vec3 result;
    result.x = lhs.x - rhs.x;
    result.y = lhs.y - rhs.y;
    result.z = lhs.z - rhs.z;

    return result;
}

internal Vec3 operator+(Vec3 lhs, r32 rhs)
{
    Vec3 result;
    result.x = lhs.x + rhs;
    result.y = lhs.y + rhs;
    result.z = lhs.z + rhs;
    
    return result;
}

internal Vec3 operator-(Vec3 lhs, r32 rhs)
{
    Vec3 result;
    result.x = lhs.x - rhs;
    result.y = lhs.y - rhs;
    result.z = lhs.z - rhs;
    
    return result;
}

internal Vec3 operator*(Vec3 lhs, r32 rhs)
{
    Vec3 result;
    result.x = lhs.x * rhs;
    result.y = lhs.y * rhs;
    result.z = lhs.z * rhs;
    
    return result;
}

internal Vec3 operator*(f32 lhs, Vec3 rhs)
{
    Vec3 result = rhs * lhs;
    return result;
}

internal Vec3 operator/(Vec3 lhs, r32 rhs)
{
    Vec3 result;
    result.x = lhs.x / rhs;
    result.y = lhs.y / rhs;
    result.z = lhs.z / rhs;
    
    return result;
}

internal Vec3 &operator+=(Vec3 &lhs, Vec3 rhs)
{
    lhs = lhs + rhs;
    return lhs;
}

internal Vec3 &operator-=(Vec3 &lhs, Vec3 rhs)
{
    lhs = lhs - rhs;
    return lhs;
}

internal Vec3 &operator+=(Vec3 &lhs, r32 rhs)
{
    lhs = lhs + rhs;
    return lhs;
}

internal Vec3 &operator-=(Vec3 &lhs, r32 rhs)
{
    lhs = lhs - rhs;
    return lhs;
}

internal Vec3 &operator*=(Vec3 &lhs, r32 rhs)
{
    lhs = lhs * rhs;
    return lhs;
}

internal Vec3 &operator/=(Vec3 &lhs, r32 rhs)
{
    lhs = lhs / rhs;
    return lhs;
}

internal Vec3 operator*(Mat3 lhs, Vec3 rhs)
{
    Vec3 result;
    result.x = ( (lhs._11 * rhs.x) + (lhs._12 * rhs.y) + (lhs._13 * rhs.z) );
    result.y = ( (lhs._21 * rhs.x) + (lhs._22 * rhs.y) + (lhs._23 * rhs.z) );
    result.z = ( (lhs._31 * rhs.x) + (lhs._32 * rhs.y) + (lhs._33 * rhs.z) );

    return result;
}

internal r32 magnitude(Vec3 v)
{
    r32 result = sqrtf((v.x*v.x) + (v.y*v.y) + (v.z*v.z));
    return result;
}

internal Vec3 normalize(Vec3 v)
{
    r32 mag = magnitude(v);

    Vec3 result = vec3(v.x / mag, v.y / mag, v.z / mag);
    return result;
}

internal Vec3 normalize_or_zero(Vec3 v)
{
    Vec3 result = vec3(0);
    r32 mag = magnitude(v);
    if(mag > 0.0f)
    {
        result = v / mag;
    }
    return result;
}

internal r32 dot_product(Vec3 a, Vec3 b)
{
    r32 result = (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
    return result;
}

internal Vec3 cross_product(Vec3 a, Vec3 b)
{
    Vec3 result = vec3( (a.y * b.z) - (a.z * b.y), (a.x * b.z) - (a.z * b.x), (a.x * b.y) - (a.y * b.x) );
    return result;
}

internal Vec3 clamp_v3(Vec3 v, Vec3 min, Vec3 max)
{
    Vec3 result;
    result.x = clamp(v.x, min.x, max.x);
    result.y = clamp(v.y, min.y, max.y);
    result.z = clamp(v.z, min.z, max.z);

    return result;
}

internal Vec3 fabs_v3(Vec3 v)
{
    Vec3 result;
    result.x = fabs(v.x);
    result.y = fabs(v.y);
    result.z = fabs(v.z);

    return result;
}

//
// NOTE: Vec4
//
internal Vec4 vec4(r32 v)
{
    Vec4 result;
    result.x = v;
    result.y = v;
    result.z = v;
    result.w = v;
    
    return result;
}

internal Vec4 vec4(Vec2 v, f32 z, f32 w)
{
    Vec4 result;
    result.x = v.x;
    result.y = v.y;
    result.z = z;
    result.w = w;

    return result;
}

internal Vec4 vec4(Vec3 v, r32 w)
{
    Vec4 result;
    result.x = v.x;
    result.y = v.y;
    result.z = v.z;
    result.w = w;

    return result;
}

internal Vec4 vec4(r32 x, r32 y, r32 z, r32 w)
{
    Vec4 result;
    result.x = x;
    result.y = y;
    result.z = z;
    result.w = w;
    
    return result;
}

internal Vec4 operator+(Vec4 lhs, Vec4 rhs)
{
    Vec4 result;
    result.x = lhs.x + rhs.x;
    result.y = lhs.y + rhs.y;
    result.z = lhs.z + rhs.z;
    result.w = lhs.w + rhs.w;
    
    return result;
}

internal Vec4 operator-(Vec4 lhs, Vec4 rhs)
{
    Vec4 result;
    result.x = lhs.x - rhs.x;
    result.y = lhs.y - rhs.y;
    result.z = lhs.z - rhs.z;
    result.w = lhs.w - rhs.w;

    return result;
}

internal Vec4 operator+(Vec4 lhs, r32 rhs)
{
    Vec4 result;
    result.x = lhs.x + rhs;
    result.y = lhs.y + rhs;
    result.z = lhs.z + rhs;
    result.w = lhs.w + rhs;
    
    return result;
}

internal Vec4 operator-(Vec4 lhs, r32 rhs)
{
    Vec4 result;
    result.x = lhs.x - rhs;
    result.y = lhs.y - rhs;
    result.z = lhs.z - rhs;
    result.w = lhs.w - rhs;
    
    return result;
}

internal Vec4 operator*(Vec4 lhs, r32 rhs)
{
    Vec4 result;
    result.x = lhs.x * rhs;
    result.y = lhs.y * rhs;
    result.z = lhs.z * rhs;
    result.w = lhs.w * rhs;
    
    return result;
}

internal Vec4 operator*(f32 lhs, Vec4 rhs)
{
    Vec4 result = rhs * lhs;
    return result;
}

internal Vec4 operator/(Vec4 lhs, r32 rhs)
{
    Vec4 result;
    result.x = lhs.x / rhs;
    result.y = lhs.y / rhs;
    result.z = lhs.z / rhs;
    result.w = lhs.w / rhs;
    
    return result;
}

internal Vec4 &operator+=(Vec4 &lhs, Vec4 rhs)
{
    lhs = lhs + rhs;
    return lhs;
}

internal Vec4 &operator-=(Vec4 &lhs, Vec4 rhs)
{
    lhs = lhs - rhs;
    return lhs;
}

internal Vec4 &operator+=(Vec4 &lhs, r32 rhs)
{
    lhs = lhs + rhs;
    return lhs;
}

internal Vec4 &operator-=(Vec4 &lhs, r32 rhs)
{
    lhs = lhs - rhs;
    return lhs;
}

internal Vec4 &operator*=(Vec4 &lhs, r32 rhs)
{
    lhs = lhs * rhs;
    return lhs;
}

internal Vec4 &operator/=(Vec4 &lhs, r32 rhs)
{
    lhs = lhs / rhs;
    return lhs;
}

internal Vec4 operator*(Mat4 lhs, Vec4 rhs)
{
    Vec4 result;
    result.x = ((lhs._11 * rhs.x) + (lhs._12 * rhs.y) + (lhs._13 * rhs.z) + (lhs._14 * rhs.w));
    result.y = ((lhs._21 * rhs.x) + (lhs._22 * rhs.y) + (lhs._23 * rhs.z) + (lhs._24 * rhs.w));
    result.z = ((lhs._31 * rhs.x) + (lhs._32 * rhs.y) + (lhs._33 * rhs.z) + (lhs._34 * rhs.w));
    result.w = ((lhs._41 * rhs.x) + (lhs._42 * rhs.y) + (lhs._43 * rhs.z) + (lhs._44 * rhs.w));

    return result;
}

//
// NOTE: Mat3
//
internal Mat3 mat3(f32 m11, f32 m12, f32 m13,
                   f32 m21, f32 m22, f32 m23,
                   f32 m31, f32 m32, f32 m33)
{
    Mat3 m;
    m._11 = m11; m._12 = m12; m._13 = m13;
    m._21 = m21; m._22 = m22; m._23 = m23;
    m._31 = m31; m._32 = m32; m._33 = m33;

    return m;
}

internal Mat3 identity_mat3(void)
{
    Mat3 m = mat3(1.0f, 0, 0,
                  0, 1.0f, 0,
                  0, 0, 1.0f);
    
    return m;
}

internal Mat3 operator*(Mat3 lhs, Mat3 rhs)
{
    f32 m11 = (lhs._11 * rhs._11) + (lhs._12 * rhs._21) + (lhs._13 * rhs._31);
    f32 m12 = (lhs._11 * rhs._12) + (lhs._12 * rhs._22) + (lhs._13 * rhs._32);
    f32 m13 = (lhs._11 * rhs._13) + (lhs._12 * rhs._23) + (lhs._13 * rhs._33);

    f32 m21 = (lhs._21 * rhs._11) + (lhs._22 * rhs._21) + (lhs._23 * rhs._31);
    f32 m22 = (lhs._21 * rhs._12) + (lhs._22 * rhs._22) + (lhs._23 * rhs._32);
    f32 m23 = (lhs._21 * rhs._13) + (lhs._22 * rhs._23) + (lhs._23 * rhs._33);

    f32 m31 = (lhs._31 * rhs._11) + (lhs._32 * rhs._21) + (lhs._33 * rhs._31);
    f32 m32 = (lhs._31 * rhs._12) + (lhs._32 * rhs._22) + (lhs._33 * rhs._32);
    f32 m33 = (lhs._31 * rhs._13) + (lhs._32 * rhs._23) + (lhs._33 * rhs._33);

    Mat3 result = mat3(m11, m12, m13,
                       m21, m22, m23,
                       m31, m32, m33);

    return result;
}

internal Mat3 &operator*=(Mat3 &lhs, Mat3 rhs)
{
    lhs = lhs * rhs;
    return lhs;
}

internal Mat3 translation_mat3(Vec2 v)
{
    Mat3 m = identity_mat3();
    
    m._13 = v.x;
    m._23 = v.y;

    return m;
}

internal Mat3 scaling_mat3(Vec2 v)
{
    Mat3 m = identity_mat3();
    
    m._11 = v.x;
    m._22 = v.y;

    return m;
}

internal Mat3 roll_mat3(f32 angle)
{
    Mat3 m = identity_mat3();

    m._11 = cosf(angle);
    m._12 = -sinf(angle);
    m._21 = sinf(angle);
    m._22 = cosf(angle);    

    return m;
}

//
// NOTE: Mat4
//
internal Mat4 mat4(r32 m11, r32 m12, r32 m13, r32 m14,
                   r32 m21, r32 m22, r32 m23, r32 m24,
                   r32 m31, r32 m32, r32 m33, r32 m34,
                   r32 m41, r32 m42, r32 m43, r32 m44)
{
    Mat4 m;
    m._11 = m11; m._12 = m12; m._13 = m13; m._14 = m14;
    m._21 = m21; m._22 = m22; m._23 = m23; m._24 = m24;
    m._31 = m31; m._32 = m32; m._33 = m33; m._34 = m34;
    m._41 = m41; m._42 = m42; m._43 = m43; m._44 = m44;

    return m;
}

internal Mat4 identity_mat4(void)
{
    Mat4 m = mat4(1.0f, 0, 0, 0,
                  0, 1.0f, 0, 0,
                  0, 0, 1.0f, 0,
                  0, 0, 0, 1.0f);
    
    return m;
}

internal Mat4 operator*(Mat4 lhs, Mat4 rhs)
{
    f32 m11 = (lhs._11 * rhs._11) + (lhs._12 * rhs._21) + (lhs._13 * rhs._31) + (lhs._14 * rhs._41);
    f32 m12 = (lhs._11 * rhs._12) + (lhs._12 * rhs._22) + (lhs._13 * rhs._32) + (lhs._14 * rhs._42);
    f32 m13 = (lhs._11 * rhs._13) + (lhs._12 * rhs._23) + (lhs._13 * rhs._33) + (lhs._14 * rhs._43);
    f32 m14 = (lhs._11 * rhs._14) + (lhs._12 * rhs._24) + (lhs._13 * rhs._34) + (lhs._14 * rhs._44);

    f32 m21 = (lhs._21 * rhs._11) + (lhs._22 * rhs._21) + (lhs._23 * rhs._31) + (lhs._24 * rhs._41);
    f32 m22 = (lhs._21 * rhs._12) + (lhs._22 * rhs._22) + (lhs._23 * rhs._32) + (lhs._24 * rhs._42);
    f32 m23 = (lhs._21 * rhs._13) + (lhs._22 * rhs._23) + (lhs._23 * rhs._33) + (lhs._24 * rhs._43);
    f32 m24 = (lhs._21 * rhs._14) + (lhs._22 * rhs._24) + (lhs._23 * rhs._34) + (lhs._24 * rhs._44);

    f32 m31 = (lhs._31 * rhs._11) + (lhs._32 * rhs._21) + (lhs._33 * rhs._31) + (lhs._34 * rhs._41);
    f32 m32 = (lhs._31 * rhs._12) + (lhs._32 * rhs._22) + (lhs._33 * rhs._32) + (lhs._34 * rhs._42);
    f32 m33 = (lhs._31 * rhs._13) + (lhs._32 * rhs._23) + (lhs._33 * rhs._33) + (lhs._34 * rhs._43);
    f32 m34 = (lhs._31 * rhs._14) + (lhs._32 * rhs._24) + (lhs._33 * rhs._34) + (lhs._34 * rhs._44);

    f32 m41 = (lhs._41 * rhs._11) + (lhs._42 * rhs._21) + (lhs._43 * rhs._31) + (lhs._44 * rhs._41);
    f32 m42 = (lhs._41 * rhs._12) + (lhs._42 * rhs._22) + (lhs._43 * rhs._32) + (lhs._44 * rhs._42);
    f32 m43 = (lhs._41 * rhs._13) + (lhs._42 * rhs._23) + (lhs._43 * rhs._33) + (lhs._44 * rhs._43);
    f32 m44 = (lhs._41 * rhs._14) + (lhs._42 * rhs._24) + (lhs._43 * rhs._34) + (lhs._44 * rhs._44);

    Mat4 result = mat4(m11, m12, m13, m14,
                       m21, m22, m23, m24,
                       m31, m32, m33, m34,
                       m41, m42, m43, m44);

    return result;
}

internal Mat4 &operator*=(Mat4 &lhs, Mat4 rhs)
{
    lhs = lhs * rhs;
    return lhs;
}

internal Mat4 translation_matrix(Vec3 v)
{
    Mat4 m = identity_mat4();
    
    m._14 = v.x;
    m._24 = v.y;
    m._34 = v.z;

    return m;
}

internal Mat4 scaling_matrix(Vec3 v)
{
    Mat4 m = identity_mat4();
    
    m._11 = v.x;
    m._22 = v.y;
    m._33 = v.z;

    return m;
}

internal Mat4 pitch_matrix(r32 angle)
{
    Mat4 m = identity_mat4();

    m._22 = cosf(angle);
    m._23 = -sinf(angle);
    m._32 = sinf(angle);
    m._33 = cosf(angle);

    return m;
}

internal Mat4 yaw_matrix(r32 angle)
{
    Mat4 m = identity_mat4();

    m._11 = cosf(angle);
    m._13 = sinf(angle);
    m._31 = -sinf(angle);
    m._33 = cosf(angle);

    return m;
}

internal Mat4 roll_matrix(r32 angle)
{
    Mat4 m = identity_mat4();

    m._11 = cosf(angle);
    m._12 = -sinf(angle);
    m._21 = sinf(angle);
    m._22 = cosf(angle);

    return m;
}

internal Mat4 orthographic_projection_matrix(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far)
{
    Mat4 m = identity_mat4();

    m._11 = 2.0f / (right - left);    
    m._14 = -1.0f * ((right + left) / (right - left));

    m._22 = 2.0f / (top - bottom);
    m._24 = -1.0f * ((top + bottom) / (top - bottom));

    m._33 = (-2.0f / (far - near));
    m._34 = -1.0f;

    return m;
}

internal Mat4 perspective_projection_matrix(int screen_width, int screen_height, r32 fov_y, r32 near_plane, r32 far_plane)
{
    r32 top = tanf( fov_y / 2.0f ) * near_plane;
    r32 bottom = -1.0f * top;

    r32 right = top * ((r32)screen_width / (r32)screen_height);
    r32 left = -1.0f * right;

    Mat4 m = identity_mat4();

    m._11 = (2.0f * near_plane) / (right - left);
    m._13 = (right + left) / (right - left);

    m._22 = (2.0f * near_plane) / (top - bottom);
    m._23 = (top + bottom) / (top - bottom);

    m._33 = -1.0f * ((far_plane + near_plane) / (far_plane - near_plane));
    m._34 = -1.0f * ((2.0f * far_plane * near_plane) / (far_plane - near_plane));

    m._43 = -1.0f;
    m._44 = 0.0f;

    return m;
}

internal Mat4 build_world_matrix_world_rotate(Vec2 position, Vec2 scale, r32 rotation, Vec2 local_rotation_pt)
{
    Mat4 world = translation_matrix(vec3(position + local_rotation_pt, 0.0f)) * roll_matrix(rotation) * translation_matrix(vec3(local_rotation_pt * -1.0f, 0.0f)) * scaling_matrix(vec3(scale, 0.0f));

    return world;
}

#endif
