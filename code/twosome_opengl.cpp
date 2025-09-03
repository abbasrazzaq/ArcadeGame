#include "twosome_render_group.h"
#include "twosome_asset.h"


#define GL_CLAMP_TO_EDGE                  0x812F
#define GL_TEXTURE0                       0x84C0

#define GL_ARRAY_BUFFER                   0x8892
#define GL_STREAM_DRAW                    0x88E0
#define GL_STATIC_DRAW                    0x88E4
#define GL_DYNAMIC_DRAW                   0x88E8

#define GL_COMPILE_STATUS                 0x8B81
#define GL_INFO_LOG_LENGTH                0x8B84
#define GL_LINK_STATUS                    0x8B82

#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31

#define GL_SHADING_LANGUAGE_VERSION       0x8B8C

#define GL_BLEND_DST_ALPHA                0x80CA
#define GL_BLEND_SRC_ALPHA                0x80CB

#define GL_MAJOR_VERSION                  0x821B
#define GL_MINOR_VERSION                  0x821C

#define MULTILINE_STRING(...) #__VA_ARGS__

// NOTE: Required versions of GL and GLSL
// TODO: Can we drop to earlier non-core profile to allow for 
// immediate mode drawing for debug visuals?
#define required_gl_major_version 3
#define required_gl_minor_version 2
#define required_glsl_major_version 1
#define required_glsl_minor_version 5

enum Vertex_Attribute_Index
{
    vertex_attribute_index_position = 1,
    vertex_attribute_index_colour = 2,
    vertex_attribute_index_uv = 3
};

struct Mesh
{
    GLuint vao;
    GLuint vbo;
    
    int num_vertices;
    int stride;
    int vertex_flags;
    
    GLenum draw_mode;
    GLenum buffer_type;
};

struct Shader_Vertex_Soup
{
    // NOTE: No uniforms
};

struct Shader_Colour
{
    GLint world_view_proj;
    GLint colour;
};

struct Shader_Text
{
    GLint world_view_proj;
    GLint colour;
    GLint texture;
};

struct Shader
{
    b32 loaded;
    GLuint program;
    
    union
    {
        Shader_Vertex_Soup vertex_soup;
        Shader_Colour colour;
        Shader_Text text;
    };
};

#if TWOSOME_INTERNAL
struct DEBUG_Text_Data
{
    b32 initialized;
    Mesh textured_quad;
    GLuint temp_texture;
    
};
#endif

struct Open_GL
{
    Shader shaders[shader_types_count];
    Mesh vertex_soup_mesh;
    
#if TWOSOME_INTERNAL
    DEBUG_Text_Data DEBUG_text_data;
#endif
};

global_variable Open_GL open_gl;

internal GLuint generate_texture(GLint filtering, GLint wrapping)
{
    GLuint texture;
    glGenTextures(1, &texture);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filtering);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filtering);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapping);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapping);
    
    return texture;
}

internal void fill_texture_data(uint8 *data, int width, int height, GLint format)
{
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
}

internal void bind_texture(int texture_object, int texture_slot)
{
    glActiveTexture(GL_TEXTURE0 + texture_slot);
    glBindTexture(GL_TEXTURE_2D, texture_object);
}

internal void bind_vertex_buffer(GLuint buffer_object)
{
    glBindBuffer(GL_ARRAY_BUFFER, buffer_object);
}

internal void set_shader_uniform_4fv(int uniform_location, float *value)
{
    glUniform4fv(uniform_location, 1, value);
}

internal void set_shader_uniform_matrix4fv(int uniform_location, float *value)
{
    glUniformMatrix4fv(uniform_location, 1, GL_FALSE, value);
}

internal void bind_shader_vertex(int vertex_location, int num_vertices, Memory_Index offset, int stride)
{
    glEnableVertexAttribArray(vertex_location);
    glVertexAttribPointer(vertex_location, num_vertices, GL_FLOAT, GL_FALSE, stride, (GLvoid *)offset);
}

internal void fill_vertex_buffer(f32 *data, GLsizeiptr data_size, GLenum buffer_usage)
{    
    glBufferData(GL_ARRAY_BUFFER, data_size, data, buffer_usage);
}

internal void partially_fill_vertex_buffer(float *data, int buffer_offset, int data_size)
{
    glBufferSubData(GL_ARRAY_BUFFER, buffer_offset, data_size, data);
}

internal b32 check_gl_and_glsl_compatibility()
{
    b32 supports_required_gl_version = false;
    b32 supports_required_glsl_version = false;
    char *gl_version = (char *)glGetString(GL_VERSION);
    
    // NOTE: Need at least 3 characters in version string.
    // First three characters should be "[major_version][.][minor_version]"
    if(string_length(gl_version) > 2)
    {
        char required_gl_major_version_char = number_to_char(required_gl_major_version);
        char required_gl_minor_version_char = number_to_char(required_gl_minor_version);
        
        char actual_gl_major_version = gl_version[0];
        char actual_gl_minor_version = gl_version[2];
        
        if(actual_gl_major_version > required_gl_major_version_char
           || (actual_gl_major_version == required_gl_major_version_char
               && actual_gl_minor_version >= required_gl_minor_version_char)
           )
        {
            supports_required_gl_version = true;
            
            const GLubyte *shading_language_version = glGetString(GL_SHADING_LANGUAGE_VERSION);
            char *actual_shader_language_version = (char *)shading_language_version;
            
            // NOTE: Need at least 3 characters in version string.
            // First three characters should be "[major_version][.][minor_version]"
            if(string_length(actual_shader_language_version) > 2)
            {
                char required_glsl_major_version_char = number_to_char(required_glsl_major_version);
                char required_glsl_minor_version_char = number_to_char(required_glsl_minor_version);
                
                char actual_glsl_major_version = actual_shader_language_version[0];
                char actual_glsl_minor_version = actual_shader_language_version[2];
                
                if(actual_glsl_major_version > required_glsl_major_version_char
                   || (actual_glsl_major_version == required_glsl_major_version_char
                       && actual_glsl_minor_version >= required_glsl_minor_version_char)
                   )
                {
                    supports_required_glsl_version = true;
                }
            }
        }
    }    
    
    b32 result = (supports_required_gl_version && supports_required_glsl_version);
    return result;
}

internal GLuint load_shader(char *shader_source, GLenum shader_type, Memory_Arena *temp_arena)
{    
    GLuint shader = glCreateShader(shader_type);
    
#define shader_source_version "150"
    // NOTE: Check if need to update shader version
    assert(shader_source_version[0] == number_to_char(required_glsl_major_version)
           && shader_source_version[1] == number_to_char(required_glsl_minor_version));
    
    char *glsl_version = "#version " shader_source_version " core";
    
    const GLchar *shader_strings[] =
    {
        glsl_version,
        "\n",
        shader_source
    };
    
    GLint shader_string_lengths[] =
    {
        -1,
        -1,
        -1
    };
    
    assert(array_count(shader_strings) == array_count(shader_string_lengths));
    
    GLsizei shader_strings_count = array_count(shader_strings);
    glShaderSource(shader, shader_strings_count, shader_strings, shader_string_lengths);
    glCompileShader(shader);
    
#if TWOSOME_INTERNAL
    GLint compilation_result = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compilation_result);
    if(compilation_result == GL_FALSE)
    {
        // NOTE: Failed to compile shader
        GLint log_size = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_size);
        
        Temporary_Memory temp_mem = begin_temporary_memory(temp_arena);
        
        char *compile_log = (char *)push_size(temp_arena, log_size);
        GLint returned_log_size;
        glGetShaderInfoLog(shader, log_size, &returned_log_size, compile_log);
        
        log_error(compile_log);
        
        end_temporary_memory(temp_mem);
        
        return 0;
    }
#endif
    
    return shader;
}

internal GLuint create_shader_program(char *vs_code, char *fs_code, Memory_Arena *temp_arena)
{
    GLuint vs_shader = load_shader(vs_code, GL_VERTEX_SHADER, temp_arena);
    GLuint fs_shader = load_shader(fs_code, GL_FRAGMENT_SHADER, temp_arena);
    
    GLuint program = glCreateProgram();
    
    glAttachShader(program, vs_shader);
    glAttachShader(program, fs_shader);
    
    glBindAttribLocation(program, vertex_attribute_index_position, "position");
    glBindAttribLocation(program, vertex_attribute_index_colour, "colour");
#if TWOSOME_INTERNAL
    glBindAttribLocation(program, vertex_attribute_index_uv, "uv");
#endif
    
    glLinkProgram(program);
    
#if TWOSOME_INTERNAL
    GLint link_status = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &link_status);
    if(!link_status)
    {
        GLint log_length = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
        
        if(log_length > 0)
        {
            Temporary_Memory temp_mem = begin_temporary_memory(temp_arena);
            
            char *info_log = (char *)push_size(temp_arena, log_length);
            glGetProgramInfoLog(program, log_length, 0, info_log);
            log_error("Failed to link shader: %s", info_log);
            
            end_temporary_memory(temp_mem);
        }
    }
#endif
    
    return program;
}

internal void destroy_texture(int texture_object)
{
    glDeleteTextures(1, (GLuint *)&texture_object);
}

internal void modify_texture_data(int x, int y, int width, int height, GLint format, uint8 *data)
{
    glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height, format, GL_UNSIGNED_BYTE, data);
}

internal void create_mesh(Mesh *m, int vertex_flags, GLenum vertex_draw_mode, GLenum buffer_type)
{
    zero_object(Mesh, *m);
    
    m->draw_mode = vertex_draw_mode;
    m->buffer_type = buffer_type;
    m->vertex_flags = vertex_flags;
    
    int num_values_per_vertex = get_values_per_vertex(vertex_flags);        
    m->stride = num_values_per_vertex * sizeof(f32);
    
    glGenVertexArrays(1, &m->vao);
    glGenBuffers(1, &m->vbo);
    
    glBindVertexArray(m->vao);
    bind_vertex_buffer(m->vbo);
    
    GLsizei vertex_offset = 0;
    if(m->vertex_flags & vertex_flag_xy)
    {
        bind_shader_vertex(vertex_attribute_index_position, 2, vertex_offset, m->stride);
        vertex_offset += 2 * sizeof(f32);
    }
    if(m->vertex_flags & vertex_flag_xyz)
    {
        bind_shader_vertex(vertex_attribute_index_position, 3, vertex_offset, m->stride);
        vertex_offset += 3 * sizeof(f32);
    }
    if(m->vertex_flags & vertex_flag_uv)
    {
        bind_shader_vertex(vertex_attribute_index_uv, 2, vertex_offset, m->stride);
        vertex_offset += 2 * sizeof(f32);
    }
    if(m->vertex_flags & vertex_flag_rgba)
    {
        bind_shader_vertex(vertex_attribute_index_colour, 4, vertex_offset, m->stride);
        vertex_offset += 4 * sizeof(f32);
    }
    
    glBindVertexArray(0);
    bind_vertex_buffer(0);
}

internal void set_mesh_data(Mesh *m, float *vertex_data, int num_vertices)
{
    m->num_vertices = num_vertices;
    assert(m->stride);
    GLsizeiptr data_size = m->stride * num_vertices;
    
    assert(m->vbo && m->buffer_type && vertex_data);
    bind_vertex_buffer(m->vbo);
    
    glBufferData(GL_ARRAY_BUFFER, data_size, vertex_data, m->buffer_type);
    
}

internal void create_mesh_and_set_data(Mesh *m, float *vertex_data, int num_vertices, int vertex_flags, GLenum vertex_draw_mode, GLenum buffer_type)
{
    create_mesh(m, vertex_flags, vertex_draw_mode, buffer_type);
    set_mesh_data(m, vertex_data, num_vertices);
}

internal void free_mesh(Mesh *m)
{
    if(m->vao)
    {
        glDeleteVertexArrays(1, (GLuint *)&m->vao);
    }
    
    if(m->vbo)
    {
        glDeleteBuffers(1, (GLuint *)&m->vbo);
    }
}

internal void draw_mesh(Mesh *m)
{
    glBindVertexArray(m->vao);
    glDrawArrays(m->draw_mode, 0, m->num_vertices);
}

internal GLint get_uniform_location(GLuint program, char *name)
{
    GLint uniform_location = glGetUniformLocation(program, name);
    assert(uniform_location >= 0);
    
    return uniform_location;
}

internal Shader *load_opengl_shader(int shader_type, Memory_Arena *temp_arena)
{
    Shader *shader = &open_gl.shaders[shader_type];
    assert(shader->loaded == false);
    
    switch(shader_type)
    {
        //
        // NOTE: Vertex Soup Shader
        //
        case shader_type_vertex_soup:
        {
            char *vertex_shader_code =
                MULTILINE_STRING(                                               
                in vec2 position;
                in vec4 colour;
                
                out vec4 frag_colour;
                
                void main()
                {
                gl_Position = vec4(position, 0, 1);
                frag_colour = colour;
                }
                );
            
            char *fragment_shader_code =
                MULTILINE_STRING(                                               
                in vec4 frag_colour;
                out vec4 out_colour;
                
                void main()
                {
                out_colour = frag_colour;
                }
                );
            
            shader->program = create_shader_program(vertex_shader_code, fragment_shader_code, temp_arena);
            
            Shader_Vertex_Soup *vs = &shader->vertex_soup;
            
        } break;
        
        //
        // NOTE: Colour Shader
        //
        case shader_type_colour:
        {
            char *vertex_shader_code =
                MULTILINE_STRING(
                in vec2 position;
                
                uniform mat4 world_view_proj;
                uniform vec4 colour;
                
                out vec4 frag_colour;
                
                void main()
                {
                gl_Position = world_view_proj * vec4(position, 0, 1);
                frag_colour = colour;
                };
                );
            
            char *fragment_shader_code = 
                MULTILINE_STRING(
                in vec4 frag_colour;
                out vec4 out_colour;
                
                void main()
                {
                out_colour = frag_colour;
                }
                );
            
            shader->program = create_shader_program(vertex_shader_code, fragment_shader_code, temp_arena);
            Shader_Colour *cs = &shader->colour;
            cs->world_view_proj = get_uniform_location(shader->program, "world_view_proj");
            cs->colour = get_uniform_location(shader->program, "colour");
            
        } break;
        
#if TWOSOME_INTERNAL            
        //
        // NOTE: Text Shader
        //
        case DEBUG_shader_type_text:
        {
            char *vertex_shader_code =
                MULTILINE_STRING(                                      
                in vec2 position;
                in vec2 uv;
                
                out vec2 frag_uv;
                
                uniform mat4 world_view_proj;
                
                void main()
                {
                gl_Position = world_view_proj * vec4(position, 0, 1);
                
                frag_uv = uv;
                }
                );
            
            char *fragment_shader_code =
                MULTILINE_STRING(                                      
                in vec2 frag_uv;
                
                out vec4 out_colour;
                
                uniform sampler2D texture_sampler;
                uniform vec4 colour;
                
                void main()
                {
                float t = texture(texture_sampler, vec2(frag_uv.x, 1.0 - frag_uv.y)).r;
                
                float alpha = colour.a * t;
                
                out_colour = vec4(colour.r * alpha, colour.g * alpha, colour.b * alpha, alpha);
                }
                );
            
            shader->program = create_shader_program(vertex_shader_code, fragment_shader_code, temp_arena);
            Shader_Text *ts = &shader->text;
            
            ts->world_view_proj = get_uniform_location(shader->program, "world_view_proj");
            ts->colour = get_uniform_location(shader->program, "colour");
            ts->texture = get_uniform_location(shader->program, "texture_sampler");
            
        } break;
#endif
        
        invalid_default_case;
    }    
    
    shader->loaded = true;   
    
    return shader;
}

internal Shader *get_opengl_shader(int shader_type)
{
    Shader *shader = &open_gl.shaders[shader_type];
    assert(shader->loaded);
    
    return shader;
}

internal void opengl_load_resources(Memory_Arena *temp_arena)
{    
#if TWOSOME_INTERNAL
    assert(open_gl.DEBUG_text_data.initialized == false);
    // NOTE: Only use this for text rendering
    {
        glGenTextures(1, &open_gl.DEBUG_text_data.temp_texture);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, open_gl.DEBUG_text_data.temp_texture);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);        
    }
    
    {
        Vertex_XY_UV vertices[] =
        {
            { 0, 0,    0, 0 },
            { 1, 0,    1, 0 },
            { 0, 1,    0, 1 },
            { 0, 1,    0, 1 },
            { 1, 0,    1, 0 },
            { 1, 1,    1, 1 }
        };
        create_mesh_and_set_data(&open_gl.DEBUG_text_data.textured_quad, (f32 *)vertices, array_count(vertices), vertex_flag_xy | vertex_flag_uv, GL_TRIANGLES, GL_STATIC_DRAW);
    }   
#endif
    
    // NOTE: Preload shaders
    for(u32 shader_type = 0; shader_type < shader_types_count; ++shader_type)
    {
        load_opengl_shader(shader_type, temp_arena);
    }
    
    // NOTE: Vertex Soup Mesh
    create_mesh(&open_gl.vertex_soup_mesh, vertex_flag_xy | vertex_flag_rgba, GL_TRIANGLES, GL_DYNAMIC_DRAW);
}

internal void opengl_draw_colour(Mesh *m, Vec2 position, Vec2 scale, Vec4 colour, Mat4 view_proj)
{
    Shader *shader = get_opengl_shader(shader_type_colour);
    glUseProgram(shader->program);
    
    Mat4 world_view_proj = view_proj * translation_matrix(vec3(position, 0.0f)) * scaling_matrix(vec3(scale, 0.0f));
    
    set_shader_uniform_matrix4fv(shader->colour.world_view_proj, world_view_proj.e);
    set_shader_uniform_4fv(shader->colour.colour, colour.e);
    
    draw_mesh(m);
}

internal void opengl_execute_render_commands(Game_Render_Commands *commands, Game_Render_Prep *prep, Memory_Arena *temp_arena)
{
    glEnable(GL_BLEND);
    //glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glBlendFunc(GL_ONE, GL_ONE);
    
    /* Additive:
    glBlendFunc(src, dest)
    glBendFunc(GL_SRC_ALPHA
    
    */
    
    //glEnable(GL_DEPTH_TEST);
    //glDepthFunc(GL_LEQUAL);
    
    glViewport(commands->view.x, commands->view.y, commands->view.width, commands->view.height);
    
    glClearColor(commands->clear_colour.r, commands->clear_colour.g, commands->clear_colour.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    Mat4 screen_orthographic_matrix = orthographic_projection_matrix(0.0f, (f32)commands->view.width, 0.0f, (f32)commands->view.height, 0.0f, 1.0f);
    
    assert(commands->push_buffer_at >= commands->push_buffer_base);
    
    u8 *push_buffer_end = (commands->push_buffer_base + commands->push_buffer_size);    
    
    Temporary_Memory temp_mem = begin_temporary_memory(temp_arena);
    
    Vertex_XY_RGBA *vertex_soup_base = (Vertex_XY_RGBA *)push_size(temp_arena, prep->vertex_soup_data_size);
    Vertex_XY_RGBA *vs = vertex_soup_base;
    
    for(u32 sorted_render_entry_index = 0; sorted_render_entry_index < commands->render_entries_count; ++sorted_render_entry_index)
    {
        Memory_Index render_entry_header_cursor = prep->sorted_render_entry_header_cursors[sorted_render_entry_index];
        
        Render_Entry_Header *header = (Render_Entry_Header *)(push_buffer_end - render_entry_header_cursor);
        
        u8 *entry = ((uint8 *)header - header->entry_size);
        switch(header->type)
        {            
            case render_entry_type_Render_Entry_Vertices:
            {
                Render_Entry_Vertices *v = (Render_Entry_Vertices *)entry;
#if 1
                // NOTE: Collect all vertices into one big block of vertex data
                memcpy(vs, v->vertices, v->vertices_count * sizeof(*vs));
                vs += v->vertices_count;
                
                // NOTE: If hit last vertex then issue the draw call to draw all of it in one batch
                assert(((u8*)vs - (u8*)vertex_soup_base) <= (ptrdiff)prep->vertex_soup_data_size);
                if(prep->last_vertex_soup_render_entry_header_cursor == render_entry_header_cursor)
                {
                    Shader *shader = get_opengl_shader(header->shader_type);
                    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                    
                    glUseProgram(shader->program);
                    
                    assert(((u8*)vs - (u8*)vertex_soup_base) == (ptrdiff)prep->vertex_soup_data_size);
                    set_mesh_data(&open_gl.vertex_soup_mesh, (f32 *)vertex_soup_base, (vs - vertex_soup_base));
                    draw_mesh(&open_gl.vertex_soup_mesh);
                    
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                }
                
#else
#if TWOSOME_INTERNAL
                // NOTE: Draw each vertex entry separately, guess it's handy for gfx debugging?
                Shader *shader = get_opengl_shader(header->shader_type);
                glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                Mesh m;
                create_mesh(&m, v->vertices, v->vertices_count, v->vertex_flags, GL_TRIANGLES, GL_DYNAMIC_DRAW);
                
                Mat4 world_view_proj = identity_mat4();
                glUseProgram(shader->program);
                
                set_shader_uniform_matrix4fv(shader->vertex_colour.world_view_proj, world_view_proj.e);
                
                draw_mesh(&m);
                free_mesh(&m);
                
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
#endif                
                
            } break;
            
#if TWOSOME_INTERNAL
            case render_entry_type_DEBUG_Render_Entry_Text:
            {
                DEBUG_Render_Entry_Text *t = (DEBUG_Render_Entry_Text *)entry;
                {
                    GLint last_blend_src_alpha;
                    GLint last_blend_dest_alpha;
                    glGetIntegerv(GL_BLEND_DST_ALPHA, &last_blend_dest_alpha);
                    glGetIntegerv(GL_BLEND_SRC_ALPHA, &last_blend_src_alpha);
                    
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                    {
                        Shader *shader = get_opengl_shader(header->shader_type);
                        glUseProgram(shader->program);
                        bind_texture(open_gl.DEBUG_text_data.temp_texture, 0);
                        
                        fill_texture_data((u8 *)t->texture_data, t->width, t->height, GL_RED);
                        
                        //Mat4 world_view_proj = screen_orthographic_matrix * translation_matrix(vec3((r32)t->x, (r32)t->y, 0.0f)) * scaling_matrix(vec3((r32)t->width, (r32)t->height, 0.0f));
                        Mat4 world_view_proj = commands->view.screen_orthographic_matrix * translation_matrix(vec3((r32)t->x, (r32)t->y, 0.0f)) * scaling_matrix(vec3((r32)t->width, (r32)t->height, 0.0f));
                        
                        glUniform1i(shader->text.texture, 0);
                        set_shader_uniform_matrix4fv(shader->text.world_view_proj, world_view_proj.e);
                        set_shader_uniform_4fv(shader->text.colour, t->colour.e);
                        
                        draw_mesh(&open_gl.DEBUG_text_data.textured_quad);
                    }
                    glBlendFunc(last_blend_src_alpha, last_blend_dest_alpha);
                }
                
            } break;
#endif
            
            invalid_default_case;
        };
    }
    
    end_temporary_memory(temp_mem);
    
}
