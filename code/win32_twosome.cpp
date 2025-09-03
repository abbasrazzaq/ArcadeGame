#include "twosome_platform.h"
#include "twosome_shared.h"
#include "twosome_render.cpp"

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winuser.h>
#include <mmsystem.h>
#include <shlobj.h>
#include <dsound.h>
#include <gl/gl.h>

#include "win32_twosome.h"
#include "win32\icon_id.h"

global_variable bool32 global_running = true;
global_variable int32 global_screen_width = minimum_screen_width;
global_variable int32 global_screen_height = minimum_screen_height;

global_variable int64 global_perf_count_frequency;
global_variable LPDIRECTSOUNDBUFFER global_secondary_buffer;

global_variable Win32_State global_win32_state;


// NOTE: Direct Sound Function
#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPGUID lpGuid, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter);
typedef DIRECT_SOUND_CREATE(Direct_Sound_Create);

//
// NOTE: OpenGL Constants & Functions
//
#define WGL_DRAW_TO_WINDOW_ARB                       0x2001
#define WGL_ACCELERATION_ARB                         0x2003
#define WGL_SUPPORT_OPENGL_ARB                       0x2010
#define WGL_DOUBLE_BUFFER_ARB                        0x2011
#define WGL_PIXEL_TYPE_ARB                           0x2013
#define WGL_COLOR_BITS_ARB                           0x2014
#define WGL_ALPHA_BITS_ARB                           0x201C
#define WGL_DEPTH_BITS_ARB                           0x2022
#define WGL_FULL_ACCELERATION_ARB                    0x2027
#define WGL_TYPE_RGBA_ARB                            0x202B

#define WGL_CONTEXT_MAJOR_VERSION_ARB                0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB                0x2092
#define WGL_CONTEXT_FLAGS_ARB                        0x2094
#define WGL_CONTEXT_DEBUG_BIT_ARB                    0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB       0x0002
#define WGL_CONTEXT_PROFILE_MASK_ARB                 0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB             0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB    0x00000002

#define GL_DEBUG_SOURCE_API               0x8246
#define GL_DEBUG_SOURCE_WINDOW_SYSTEM     0x8247
#define GL_DEBUG_SOURCE_SHADER_COMPILER   0x8248
#define GL_DEBUG_SOURCE_THIRD_PARTY       0x8249
#define GL_DEBUG_SOURCE_APPLICATION       0x824A
#define GL_DEBUG_SOURCE_OTHER             0x824B
#define GL_DEBUG_TYPE_ERROR               0x824C
#define GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR 0x824D
#define GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR  0x824E
#define GL_DEBUG_TYPE_PORTABILITY         0x824F
#define GL_DEBUG_TYPE_PERFORMANCE         0x8250
#define GL_DEBUG_TYPE_OTHER               0x8251
#define GL_MAX_DEBUG_MESSAGE_LENGTH       0x9143
#define GL_MAX_DEBUG_LOGGED_MESSAGES      0x9144
#define GL_DEBUG_LOGGED_MESSAGES          0x9145
#define GL_DEBUG_SEVERITY_HIGH            0x9146
#define GL_DEBUG_SEVERITY_MEDIUM          0x9147
#define GL_DEBUG_SEVERITY_LOW             0x9148
#define GL_DEBUG_TYPE_MARKER              0x8268
#define GL_DEBUG_TYPE_PUSH_GROUP          0x8269
#define GL_DEBUG_TYPE_POP_GROUP           0x826A
#define GL_DEBUG_SEVERITY_NOTIFICATION    0x826B
#define GL_DEBUG_SOURCE_OTHER_ARB         0x824B

#define GL_DEBUGGING_ENABLED (TWOSOME_SLOW && TWOSOME_INTERNAL)

#if GL_DEBUGGING_ENABLED
internal void *DEBUG_win32_get_optional_gl_function(char *name)
{
    void *func = wglGetProcAddress(name);    
    if(!func)
    {
        log_warning("Failed to get GL function: %s", name);
    }
    
    return func;
}
#endif

internal void *win32_get_gl_function(char *name)
{
    assert(global_win32_state.DEBUG_stop_further_opengl_function_getting == false);
    
    void *func = wglGetProcAddress(name);
    assert(func);
    
    if(!func)
    {
        win32_startup_log("Unable to load OpenGL function:");
        win32_startup_log(name);
        
        global_win32_state.failed_to_get_an_opengl_function = true;
    }
    
    return func;
}
// NOTE: Forcing use of win32_get_gl_function
#define wglGetProcAddress(name) use_win32_get_gl_function_instead_of_wglGetProcAddress_see_win32_get_gl_function_for_more_details__18022018



typedef BOOL WINAPI Wgl_Choose_Pixel_Format_Arb (HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);
typedef HGLRC WINAPI Wgl_Create_Context_Attribs_Arb(HDC hdc, HGLRC hShareContext, const int *attribList);
typedef BOOL WINAPI Wgl_Swap_Interval_Ext(int interval);


typedef char GLchar;
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;

typedef void WINAPI Gl_Get_Shader_Info_Log(GLuint shader, GLsizei buffer_size, GLsizei *length, GLchar *source);
typedef GLuint WINAPI Gl_Create_Shader(GLenum shader_type);
typedef void WINAPI Gl_Use_Program(GLuint program);
typedef void WINAPI Gl_Shader_Source(GLuint shader, GLsizei count, const GLchar **string, const GLint *length);
typedef void WINAPI Gl_Compile_Shader(GLuint shader);
typedef void WINAPI Gl_Get_Shader_Iv(GLuint shader, GLenum pname, GLint *params);
typedef GLuint WINAPI Gl_Create_Program(void);
typedef void WINAPI Gl_Attach_Shader(GLuint program, GLuint shader);
typedef void WINAPI Gl_Link_Program(GLuint program);
typedef void WINAPI Gl_Get_Program_Iv(GLuint program, GLenum pname, GLint *params);
typedef void WINAPI Gl_Get_Program_Info_Log(GLuint program, GLsizei max_length, GLsizei *length, GLchar *info_log);
typedef void WINAPI Gl_Bind_Attrib_Location(GLuint program, GLuint index, const GLchar *name);
typedef GLint WINAPI Gl_Get_Uniform_Location(GLuint program, const GLchar *name);
typedef void WINAPI Gl_Vertex_Attrib_Pointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer);
typedef void WINAPI Gl_Uniform_Matrix_4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void WINAPI Gl_Uniform_4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
typedef void WINAPI Gl_Enable_Vertex_Attrib_Array(GLuint index);
typedef void WINAPI Gl_Gen_Vertex_Arrays(GLsizei n, GLuint *arrays);
typedef void WINAPI Gl_Bind_Vertex_Array(GLuint array);
typedef void WINAPI Gl_Gen_Buffers(GLsizei n, GLuint *buffers);
typedef void WINAPI Gl_Bind_Buffer(GLenum target, GLuint buffer);
typedef void WINAPI Gl_Buffer_Data(GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage);
typedef void WINAPI Gl_Active_Texture(GLenum texture);
typedef void WINAPI Gl_Uniform_1i(GLint location, GLint v0);
typedef void WINAPI Gl_Uniform_4fv(GLint location, GLsizei count, const GLfloat *value);
typedef void WINAPI Gl_Delete_Vertex_Arrays(GLsizei n, const GLuint *arrays);
typedef void WINAPI Gl_Delete_Buffers(GLsizei n, const GLuint *buffers);
typedef void WINAPI Gl_Buffer_Sub_Data(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid *data);

#if GL_DEBUGGING_ENABLED
#define GL_DEBUG_PROC(name) void WINAPI name(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, void *user_param)
typedef GL_DEBUG_PROC(Gl_Debug_Proc);
typedef void WINAPI Gl_Debug_Message_Callback(Gl_Debug_Proc *callback, void *user_param);
#endif

global_variable Gl_Get_Shader_Info_Log *glGetShaderInfoLog;
global_variable Gl_Create_Shader *glCreateShader;
global_variable Gl_Use_Program *glUseProgram;    
global_variable Gl_Shader_Source *glShaderSource;    
global_variable Gl_Compile_Shader *glCompileShader;
global_variable Gl_Get_Shader_Iv *glGetShaderiv;
global_variable Gl_Create_Program *glCreateProgram;    
global_variable Gl_Attach_Shader *glAttachShader;    
global_variable Gl_Link_Program *glLinkProgram;    
global_variable Gl_Get_Program_Iv *glGetProgramiv;    
global_variable Gl_Get_Program_Info_Log *glGetProgramInfoLog;
global_variable Gl_Bind_Attrib_Location *glBindAttribLocation;
global_variable Gl_Get_Uniform_Location *glGetUniformLocation;        
global_variable Gl_Vertex_Attrib_Pointer *glVertexAttribPointer;    
global_variable Gl_Uniform_Matrix_4fv *glUniformMatrix4fv;    
global_variable Gl_Uniform_4f *glUniform4f;    
global_variable Gl_Enable_Vertex_Attrib_Array *glEnableVertexAttribArray;
global_variable Gl_Gen_Vertex_Arrays *glGenVertexArrays;
global_variable Gl_Bind_Vertex_Array *glBindVertexArray;
global_variable Gl_Gen_Buffers *glGenBuffers;
global_variable Gl_Bind_Buffer *glBindBuffer;
global_variable Gl_Buffer_Data *glBufferData;
global_variable Gl_Active_Texture *glActiveTexture;
global_variable Gl_Uniform_1i *glUniform1i;
global_variable Gl_Uniform_4fv *glUniform4fv;
global_variable Gl_Delete_Vertex_Arrays* glDeleteVertexArrays;
global_variable Gl_Delete_Buffers *glDeleteBuffers;
global_variable Gl_Buffer_Sub_Data *glBufferSubData;


#include "twosome_opengl.cpp"

global_variable Wgl_Choose_Pixel_Format_Arb *wgl_choose_pixel_format_arb;
global_variable Wgl_Create_Context_Attribs_Arb *wgl_create_context_attribs_arb;
global_variable Wgl_Swap_Interval_Ext *wgl_swap_interval_ext;

#define win32_text_line_ending "\r\n"

internal DEBUG_PLATFORM_LOGGING(win32_put_message_in_log)
{
#if TWOSOME_SLOW
    OutputDebugStringA(message);
    OutputDebugStringA("\n");
    
    if(global_win32_state.log_fp)
    {
        fprintf(global_win32_state.log_fp, "%s\n", message);
        fflush(global_win32_state.log_fp);
    }    
#endif
}

#if TWOSOME_SLOW
internal DEBUG_PLATFORM_SHOW_ASSERT_POPUP(DEBUG_platform_show_assert_popup)
{
    MessageBox(0, message, "Assert Failed!", MB_OK);
    exit(1);
}
#endif

internal Platform_Date_Time win32_systemtime_to_platform_date_time(SYSTEMTIME sys_time)
{
    Platform_Date_Time time;
    
    time.year = sys_time.wYear;
    time.month = sys_time.wMonth;
    time.day = sys_time.wDay;
    time.hour = sys_time.wHour;
    time.minute = sys_time.wMinute;
    time.second = sys_time.wSecond;
    time.milliseconds = sys_time.wMilliseconds;
    
    return time;
}

internal Platform_Date_Time win32_filetime_to_platform_date_time(FILETIME file_time)
{
    SYSTEMTIME sys_time;
    FileTimeToSystemTime(&file_time, &sys_time);
    
    Platform_Date_Time platform_time = win32_systemtime_to_platform_date_time(sys_time);
    return platform_time;
}

internal PLATFORM_GET_CURRENT_TIME(win32_get_current_time)
{
    SYSTEMTIME sys_time;    
    GetLocalTime(&sys_time);
    // NOTE: System time doesn't include daylight savings and what-no so wouldn't want to use UTC?
    //GetSystemTime(&sys_time);
    
    Platform_Date_Time platform_time = win32_systemtime_to_platform_date_time(sys_time);
    return platform_time;
}

#if TWOSOME_SLOW

#define win32_log_last_win32_api_error(win32_func_name) { \
    LPSTR buffer; \
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM, 0, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&buffer, 0, 0); \
    log_error("Win32 %s error: %s", win32_func_name, buffer);           \
    LocalFree(buffer);                                                  \
}

internal char *win32_get_last_gl_error()
{
    char *gl_error_msg = 0;
    
    GLenum gl_error = glGetError();
    if(gl_error != GL_NO_ERROR)
    {
        switch(gl_error)
        {
            case GL_INVALID_ENUM:
            {
                gl_error_msg = "GL_INVALID_ENUM";
            } break;
            
            case GL_INVALID_VALUE:
            {
                gl_error_msg = "GL_INVALID_VALUE";
            } break;
            
            case GL_INVALID_OPERATION:
            {
                gl_error_msg = "GL_INVALID_OPERATION";
            } break;
            
            case GL_OUT_OF_MEMORY:
            {
                gl_error_msg = "GL_OUT_OF_MEMORY";
            } break;
            
            case GL_STACK_UNDERFLOW:
            {
                gl_error_msg = "GL_STACK_UNDERFLOW";
            } break;
            
            case GL_STACK_OVERFLOW:
            {
                gl_error_msg = "GL_STACK_OVERFLOW";
            } break;
            
            invalid_default_case;                
        };        
    }
    
    return gl_error_msg;
}

#define win32_log_last_opengl_error(opengl_func_name) { \
    char *gl_error_msg = win32_get_last_gl_error(); \
    if(gl_error_msg) log_error("OpenGL %s error: %s", opengl_func_name, gl_error_msg); \
}

#else // TWOSOME_SLOW

#define win32_log_last_win32_api_error(win32_func_name)
#define win32_log_last_opengl_error(opengl_func_name)

#endif

internal void *win32_allocate_memory(Memory_Index size, Memory_Index starting_address = 0)
{
    void *result = VirtualAlloc((LPVOID)starting_address, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    assert(result);
    return result;
}

internal void win32_free_memory(void *memory)
{
    if(memory)
    {
        VirtualFree(memory, 0, MEM_RELEASE);
    }
}

internal wchar_t *get_game_directory_filepath(char *filename, s32 game_directory, Memory_Arena *temp_arena)
{
    Memory_Index filename_length = strlen(filename);
    
    wchar_t *filename_wide = (wchar_t *)push_array(temp_arena, wchar_t, filename_length + 1, clear_to_zero());
    mbstowcs(filename_wide, filename, filename_length);
    
    wchar_t *directory_path = 0;
    Memory_Index directory_path_length = 0;
    
    switch(game_directory)
    {
        case platform_directory_type_output:
        {
            directory_path = global_win32_state.app_output_path;
            directory_path_length = global_win32_state.app_output_path_length;
        } break;
        
        case platform_directory_type_app:
        {
            directory_path = global_win32_state.exe_directory_path;
            directory_path_length = global_win32_state.exe_directory_path_length;
        } break;
        
        invalid_default_case;         
    }
    
    assert(directory_path && directory_path_length > 0);
    
    wchar_t *full_path = (wchar_t *)push_array(temp_arena, wchar_t, (directory_path_length + filename_length + 1), clear_to_zero());
    
    wcscat(full_path, directory_path);
    wcscat(full_path, filename_wide);
    
    return full_path;
}

internal PLATFORM_OPEN_FILE(win32_open_file)
{
    Platform_File_Handle result = {};
    
    DWORD desired_access = 0;
    DWORD share_mode = FILE_SHARE_READ;
    DWORD creation_disposition = 0;
    switch(file_access_mode)
    {
        case platform_file_access_mode_write:
        {
            desired_access = GENERIC_WRITE;
            creation_disposition = CREATE_ALWAYS;
#if !TWOSOME_INTERNAL
            // NOTE: Only allowed to write to the output directory in shipping game
            assert(game_directory == platform_directory_type_output);
#endif
        } break;
        
        case platform_file_access_mode_read:
        {
            desired_access = GENERIC_READ;
            creation_disposition = OPEN_EXISTING;
        } break;
        
        invalid_default_case;
    };
    
    HANDLE file_handle;
    {
        Temporary_Memory temp_mem = begin_temporary_memory(&global_win32_state.temp_arena);
        
        wchar_t *filepath = get_game_directory_filepath(filename, game_directory, &global_win32_state.temp_arena);
        
        file_handle = CreateFileW(filepath, desired_access, share_mode, 0, creation_disposition, 0, 0);
        
        end_temporary_memory(temp_mem);
    }
    
    if(file_handle != INVALID_HANDLE_VALUE)
    {
        Win32_Platform_File_Handle *win32_file_handle = (Win32_Platform_File_Handle *)win32_allocate_memory(sizeof(*win32_file_handle));
        
        if(win32_file_handle)
        {
            win32_file_handle->handle = file_handle;
            win32_file_handle->access_mode = file_access_mode;
        }
        
        result.handle = win32_file_handle;
    }
    else
    {
        win32_log_last_win32_api_error("CreateFile");
        log_error("Couldn't find file: '%s'", filename);
    }
    
    return result;
}

internal PLATFORM_CLOSE_FILE(win32_close_file)
{
    Win32_Platform_File_Handle *win32_file_handle = (Win32_Platform_File_Handle *)file->handle;
    CloseHandle(win32_file_handle->handle);
    win32_free_memory(file->handle);
    file->handle = 0;
}

internal PLATFORM_READ_DATA_FROM_FILE(win32_read_data_from_file)
{
    b32 result = false;
    
    Win32_Platform_File_Handle *win32_file_handle = (Win32_Platform_File_Handle *)source->handle;
    assert(win32_file_handle->access_mode == platform_file_access_mode_read);
    
    OVERLAPPED overlapped = {};
    overlapped.Offset = (u32)((offset >> 0) & 0xFFFFFFFF);
    overlapped.OffsetHigh = (u32)((offset >> 32) & 0xFFFFFFFF);
    
    DWORD bytes_to_read = safe_truncate_uint64(size);
    DWORD bytes_read;
    if(ReadFile(win32_file_handle->handle, buffer, bytes_to_read, &bytes_read, &overlapped))
    {
        if((expect_exact_size && bytes_read == bytes_to_read)
           || (!expect_exact_size && bytes_read <= bytes_to_read))
        {
            result = true;
        }
    }
    else
    {
        win32_log_last_win32_api_error("ReadFile");
    }
    
    return result;
}

// NOTE: If nothing passed in for overlapped then just writes at last file position
internal void win32_write_data_to_file(Platform_File_Handle *source, u8 *buffer, u64 size, OVERLAPPED *overlapped = 0)
{
    Win32_Platform_File_Handle *win32_file_handle = (Win32_Platform_File_Handle *)source->handle;
    assert(win32_file_handle->access_mode == platform_file_access_mode_write);
    
    DWORD bytes_to_write = safe_truncate_uint64(size);
    DWORD bytes_written;
    if(WriteFile(win32_file_handle->handle, buffer, bytes_to_write, &bytes_written, overlapped))
    {
        
    }
    else
    {
        win32_log_last_win32_api_error("WriteFile");
    }
}

internal void win32_startup_log(char *message)
{
#if LOGGING_ENABLED
    // NOTE: Not supporting string formatting in startup log
    assert( string_contains_char('%', message) == false );
    log_information(message);
#endif
    
    if(global_win32_state.startup_log_file.handle)
    {
        char *newline = win32_text_line_ending;
        
        win32_write_data_to_file(&global_win32_state.startup_log_file, (u8 *)message, string_length(message));
        win32_write_data_to_file(&global_win32_state.startup_log_file, (u8 *)newline, string_length(newline));
    }
}

internal PLATFORM_WRITE_DATA_TO_FILE(win32_write_data_to_file)
{
    OVERLAPPED overlapped = {};
    overlapped.Offset = ((u32)(offset >> 0) & 0xFFFFFFFF);
    overlapped.OffsetHigh = ((u32)(offset >> 32) & 0xFFFFFFFF);
    
    win32_write_data_to_file(source, buffer, size, &overlapped);    
}

internal PLATFORM_READ_ENTIRE_FILE(win32_read_entire_file)
{
    b32 result = false;
    
    Platform_File_Handle file_handle = win32_open_file(filename, game_directory, platform_file_access_mode_read);
    if(file_handle.handle)
    {
        result = win32_read_data_from_file(&file_handle, (u8 *)memory, 0, memory_size, true);
        win32_close_file(&file_handle);   
    }
    
    return result;
}

internal PLATFORM_WRITE_ENTIRE_FILE_TO_APP_OUTPUT_DIRECTORY(win32_write_entire_file_to_app_output_directory)
{    
    Platform_File_Handle file_handle = win32_open_file(filename, platform_directory_type_output, platform_file_access_mode_write);
    if(file_handle.handle)
    {
        win32_write_data_to_file(&file_handle, (u8 *)memory, 0, memory_size);
        win32_close_file(&file_handle);
    }
}

internal PLATFORM_LOAD_GAME_SAVE_FILE(win32_load_game_save_file)
{
    // NOTE: If save file doesn't exist, or doesn't match the expected size of the save data, try to load the backup.
    b32 result = false;
    if(win32_read_entire_file(save_filename, platform_directory_type_output, save.data, save.data_size))
    {
        result = true;
    }
    else if(win32_read_entire_file(backup_save_filename, platform_directory_type_output, save.data, save.data_size))
    {
        log_warning("Unable to load save file. Succesfully used the backup file.");
        // NOTE: If we had to use the backup file, then the current save is corrupted/missing and needs to be updated with the one that is legit. This is important because if save file is corrupted then backup mechanism could potentially backup a corrupted save file.
        win32_write_entire_file_to_app_output_directory(save_filename, save.data, save.data_size);
        
        result = true;
    }
    
    return result;
}

internal PLATFORM_WRITE_GAME_SAVE_FILE(win32_write_game_save_file)
{
    Temporary_Memory temp_mem = begin_temporary_memory(&global_win32_state.temp_arena);
    
    // NOTE: Backup existing save file before writing to it    
    wchar_t *save_filepath = get_game_directory_filepath(save_filename, platform_directory_type_output, &global_win32_state.temp_arena);
    wchar_t *backup_save_filepath = get_game_directory_filepath(backup_save_filename, platform_directory_type_output, &global_win32_state.temp_arena);
    
    CopyFileW(save_filepath, backup_save_filepath, FALSE);
    
    end_temporary_memory(temp_mem);
    
    win32_write_entire_file_to_app_output_directory(save_filename, save.data, save.data_size);
}

internal void win32_build_exe_directory_filepath(Win32_State *state, wchar_t *filename, int dest_count, wchar_t *dest)
{
    {
        wchar_t *source = state->exe_directory_path;
        size_t source_length = wcslen(state->exe_directory_path);
        for(size_t index = 0; index < source_length; ++index)
        {
            *dest++ = *source++;
        }
    }
    {
        wchar_t *source = filename;
        size_t source_length = wcslen(filename);
        for(size_t index = 0; index < source_length; ++index)
        {
            *dest++ = *source++;
        }
    }
    
    *dest++ = 0;
}

//
// NOTE: TWOSOME_INTERNAL
//
#if TWOSOME_INTERNAL

struct Win32_Debug_Rendering
{
    Mesh quad;
};

global_variable Win32_Debug_Rendering DEBUG_rendering;

internal DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUG_platform_free_file_memory)
{
    if(memory)
    {
        win32_free_memory(memory);
    }
}

internal DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUG_platform_read_entire_file)
{
    Debug_Read_File_Result result = {};
    
    HANDLE file_handle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if(file_handle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER file_size;
        if(GetFileSizeEx(file_handle, &file_size))
        {
            uint32 file_size_32 = safe_truncate_uint64(file_size.QuadPart);
            result.contents = win32_allocate_memory(file_size_32);
            if(result.contents)
            {
                DWORD bytes_read;
                if(ReadFile(file_handle, result.contents, file_size_32, &bytes_read, 0) && (file_size_32 == bytes_read))
                {
                    result.contents_size = file_size_32;
                }
                else
                {
                    DEBUG_platform_free_file_memory(result.contents);
                }
            }
            else
            {
                win32_put_message_in_log("Failed to read entire file.");
            }              
        }
        else
        {
            win32_put_message_in_log("Failed to get file size.");
        }
        
        CloseHandle(file_handle);
    }
    else
    {
        win32_put_message_in_log("Couldn't open file to read.");
    }
    
    return result;    
}

internal DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUG_platform_write_entire_file)
{
    bool32 result = false;
    
    HANDLE file_handle = CreateFileA(filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if(file_handle != INVALID_HANDLE_VALUE)
    {
        DWORD bytes_written;
        if(WriteFile(file_handle, memory, memory_size, &bytes_written, 0))
        {
            result = (bytes_written == memory_size);
        }
        else
        {
            win32_put_message_in_log("Failed to write data to file.");
        }
        
        CloseHandle(file_handle);
    }
    else
    {
        win32_put_message_in_log("Failed to create file to write to.");
    }
    
    
    return result;
}

internal DEBUG_PLATFORM_DELETE_FILE(DEBUG_platform_delete_file)
{
    DeleteFile(filename);
}

internal DEBUG_PLATFORM_COUNT_FILES_IN_DIRECTORY(DEBUG_platform_count_files_in_directory)
{
    uint32 result = 0;
    
    char filename[MAX_PATH];
    strcpy_s(filename, sizeof(filename), directory_path);
    if(filename[strlen(filename) - 1] != '\\')
    {
        strcat_s(filename, sizeof(filename), "\\");
    }
    strcat_s(filename, sizeof(filename), search_pattern);
    
    WIN32_FIND_DATAA find_data;
    HANDLE search_handle = FindFirstFileA(filename, &find_data);
    
    if(search_handle != INVALID_HANDLE_VALUE)
    {
        do
        {
            if(!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                ++result;
            }
        }
        while(FindNextFile(search_handle, &find_data) != 0);
        
        FindClose(search_handle);
    }
    
    return result;
}

internal void win32_get_input_file_location(Win32_State *state, int dest_count, wchar_t *dest)
{
    win32_build_exe_directory_filepath(state, L"loop_edit.hmi", dest_count, dest);
}

internal void win32_begin_recording_game_input(Win32_State *state)
{
    state->replay.recording = true;
    
    wchar_t filename[DEBUG_win32_state_filename_count];
    win32_get_input_file_location(state, array_count(filename), filename);
    
    state->replay.file_handle = CreateFileW(filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    
    DWORD bytes_to_write = (DWORD)state->game_memory_block_size;
    DWORD bytes_written;
    WriteFile(state->replay.file_handle, state->game_memory_block, bytes_to_write, &bytes_written, 0);
    assert(bytes_written == bytes_to_write);
    
    log_information("Started recording input.");
}

internal void win32_end_recording_game_input(Win32_State *state)
{
    CloseHandle(state->replay.file_handle);
    state->replay.recording = false;
    
    log_information("Ended recording input.");
}

internal void win32_begin_game_input_playback(Win32_State *state)
{
    wchar_t filename[DEBUG_win32_state_filename_count];
    win32_get_input_file_location(state, array_count(filename), filename);
    
    state->replay.file_handle = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    
    if(state->replay.file_handle != INVALID_HANDLE_VALUE)
    {
        DWORD bytes_to_read = (DWORD)state->game_memory_block_size;
        assert(state->game_memory_block_size == bytes_to_read);
        DWORD bytes_read;
        ReadFile(state->replay.file_handle, state->game_memory_block, bytes_to_read, &bytes_read, 0);
        
        state->replay.playing_back = true;
        
        log_information("Started input playback.");
    }
    else
    {
        log_warning("No input playback file to start playing.");
    }
}

internal void win32_end_game_input_playback(Win32_State *state)
{
    CloseHandle(state->replay.file_handle);
    state->replay.playing_back = false;
    
    log_information("Ended input playback.");
}

internal void win32_record_input(Win32_State *state, Game_Input *new_input)
{
    DWORD bytes_written;
    WriteFile(state->replay.file_handle, new_input, sizeof(*new_input), &bytes_written, 0);
}

internal void win32_playback_game_input(Win32_State *state, Game_Input *new_input)
{
    DWORD bytes_read = 0;
    
    if(ReadFile(state->replay.file_handle, new_input, sizeof(*new_input), &bytes_read, 0))
    {
        if(bytes_read == 0)
        {
            win32_end_game_input_playback(state);
            win32_begin_game_input_playback(state);
            ReadFile(state->replay.file_handle, new_input, sizeof(*new_input), &bytes_read, 0);
        }
    }
}

internal Platform_Date_Time DEBUG_get_last_write_time_wchar(wchar_t *filepath)
{
    Platform_Date_Time write_timestamp = {};
    
    WIN32_FIND_DATAW find_data;
    HANDLE find_handle = FindFirstFileW(filepath, &find_data);
    if(find_handle != INVALID_HANDLE_VALUE)
    {
        write_timestamp = win32_filetime_to_platform_date_time(find_data.ftLastWriteTime);
        
        FindClose(find_handle);
    }
    
    return write_timestamp;
}

internal DEBUG_PLATFORM_GET_LATEST_FILE_WRITE_TIME_IN_DIRECTORY(DEBUG_platform_get_latest_file_write_time_in_directory)
{
    Platform_Date_Time latest_timestamp = {};
    
    char filename[MAX_PATH];
    strcpy_s(filename, sizeof(filename), directory_path);
    if(filename[strlen(filename) - 1] != '\\')
    {
        strcat_s(filename, sizeof(filename), "\\");
    }
    strcat_s(filename, sizeof(filename), "*");
    
    WIN32_FIND_DATAA find_data;
    HANDLE search_handle = FindFirstFileA(filename, &find_data);
    
    if(search_handle != INVALID_HANDLE_VALUE)
    {
        do
        {            
            if(!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                Platform_Date_Time file_timestamp = win32_filetime_to_platform_date_time(find_data.ftLastWriteTime);
                if(compare_platform_date_times(latest_timestamp, file_timestamp) == platform_date_time_compare_first_earlier)
                {
                    latest_timestamp = file_timestamp;
                }
            }
            else if(!strings_match(find_data.cFileName, ".") && !strings_match(find_data.cFileName, ".."))
            {
                char sub_dir_path[256];
                wsprintf(sub_dir_path, "%s%s", directory_path, find_data.cFileName);
                Platform_Date_Time sub_latest_timestamp = DEBUG_platform_get_latest_file_write_time_in_directory(sub_dir_path);
                
                if(compare_platform_date_times(latest_timestamp, sub_latest_timestamp) == platform_date_time_compare_first_earlier)
                {
                    latest_timestamp = sub_latest_timestamp;
                }
            }
        }
        while(FindNextFile(search_handle, &find_data) != 0);
        
        FindClose(search_handle);
    }
    
    return latest_timestamp;
}

internal DEBUG_PLATFORM_GET_LAST_FILE_WRITE_TIME(DEBUG_get_last_write_time)
{
    Temporary_Memory temp_mem = begin_temporary_memory(&global_win32_state.temp_arena);
    
    // NOTE: Convert to wide char and pass to wide char version
    size_t filepath_length = strlen(filepath);
    wchar_t *filepath_wide = (wchar_t *)push_size(&global_win32_state.temp_arena, sizeof(*filepath_wide) * (filepath_length + 1), clear_to_zero());
    mbstowcs(filepath_wide, filepath, filepath_length);
    
    Platform_Date_Time write_time = DEBUG_get_last_write_time_wchar(filepath_wide);
    
    end_temporary_memory(temp_mem);
    
    return write_time;
}

internal void DEBUG_draw_rect(Game_Render_View *view, Vec2 position, Vec2 scale, Vec3 colour)
{
    opengl_draw_colour(&DEBUG_rendering.quad, position, scale, vec4(colour, 1.0f), view->screen_orthographic_matrix);
}

internal void DEBUG_draw_directsound_visuals(Game_Render_View *view, Win32_DirectSound_Debug_Marker *directsound_debug_markers, u32 directsound_debug_marker_count, Win32_Sound_Output *sound_output)
{
    f32 y = 300.0f;
    
    f32 buffer_width = 100.0f;
    Vec2 scale = vec2(2.0f, 50.0f);
    
    DEBUG_draw_rect(view, vec2(100.0f, 400.0f), vec2(buffer_width, 50.0f), vec3(1.0f, 0.0f, 1.0f));
    
    f32 x = 0.0f;
    for(u32 marker_index = 0; marker_index < directsound_debug_marker_count; ++marker_index)
    {
        x = buffer_width * marker_index;
        
        Win32_DirectSound_Debug_Marker *marker = directsound_debug_markers + marker_index;
        
        DEBUG_draw_rect(view, vec2(x, y + scale.y*0.75f), vec2(scale.x, scale.y + 20.0f), vec3(1.0f, 1.0f, 1.0f));
        
        f32 play_nrm_x = ((f32)marker->output_play_cursor / (f32)sound_output->secondary_buffer_size);
        f32 write_nrm_x = ((f32)marker->output_write_cursor / (f32)sound_output->secondary_buffer_size);
        f32 output_location_nrm_x = ((f32)marker->output_location / (f32)sound_output->secondary_buffer_size);                
        f32 play_cursor_x = x + (play_nrm_x * buffer_width);
        f32 write_cursor_x = x + (write_nrm_x * buffer_width);
        f32 output_location_x = x + (output_location_nrm_x * buffer_width);
        
        DEBUG_draw_rect(view, vec2(play_cursor_x, y), scale, vec3(1.0f, 0.0f, 0.0f));
        DEBUG_draw_rect(view, vec2(write_cursor_x, y), scale, vec3(0.0f, 1.0f, 0.0f));
        
        DWORD output_end = (marker->output_location + marker->output_byte_count);                
        
        if(output_end > sound_output->secondary_buffer_size)
        {                    
            DWORD first = (sound_output->secondary_buffer_size - marker->output_location);
            DWORD second = (output_end % sound_output->secondary_buffer_size);
            
            
            f32 first_width_nrm_x = ((f32)first / (f32)sound_output->secondary_buffer_size);
            f32 first_width = (first_width_nrm_x * buffer_width);
            first_width = max(first_width, scale.x);
            DEBUG_draw_rect(view, vec2(output_location_x, y), vec2(first_width, scale.y), vec3(1.0f, 0.0f, 1.0f));
            
            f32 second_width_nrm_x = ((f32)second / (f32)sound_output->secondary_buffer_size);
            f32 second_width = (second_width_nrm_x * buffer_width);
            second_width = max(second_width, scale.x);
            
            DEBUG_draw_rect(view, vec2(x, y), vec2(second_width, scale.y), vec3(1.0f, 0.0f, 1.0f));
        }
        else
        {
            f32 output_width_nrm_x = ((f32)marker->output_byte_count / (f32)sound_output->secondary_buffer_size);
            f32 output_width = (output_width_nrm_x * buffer_width);
            output_width = max(output_width, scale.x);
            
            DEBUG_draw_rect(view, vec2(output_location_x, y), vec2(output_width, scale.y), vec3(1.0f, 0.0f, 1.0f));    
        }
    }
    
    DEBUG_draw_rect(view, vec2(x + buffer_width, y + scale.y*0.75f), vec2(scale.x, scale.y + 20.0f), vec3(1.0f, 1.0f, 1.0f));
}

#define DEBUG_FRAME_TIMESTAMP(timestamp_name, from_counter, to_counter) { \
    Debug_Frame_Timestamp timestamp;                                  \
    timestamp.name = (timestamp_name);                                \
    timestamp.time = win32_get_milliseconds_elapsed((from_counter), (to_counter)); \
    Debug_Frame_End_Info *info = global_debug_state->frame_end_infos + global_debug_state->snapshot_index; \
    info->timestamps[info->timestamp_count++] = timestamp; \
}

#else //NOTE: TWOSOME_INTERNAL

#define DEBUG_FRAME_TIMESTAMP(timestamp_name, from_counter, to_counter)

#endif

internal void win32_process_keyboard_message(Game_Button_State *new_state, bool32 is_down)
{
    if(new_state->down != is_down)
    {
        new_state->down = is_down;
        ++new_state->half_transition_count;
    }
}

internal void win32_center_cursor_inside_window(HWND window)
{
    RECT clip_rect = {};
    GetWindowRect(window, &clip_rect);
    
    // NOTE: Raymond chen solution for using AdjustWindowRect to shrink window rect to client rect: https://blogs.msdn.microsoft.com/oldnewthing/20131017-00/?p=2903
    {
        RECT rc;
        SetRectEmpty(&rc);
        DWORD style = GetWindowLong(window, GWL_STYLE);
        if(AdjustWindowRect(&rc, style, FALSE))
        {
            clip_rect.left -= rc.left;
            clip_rect.top -= rc.top;
            clip_rect.right -= rc.right;
            clip_rect.bottom -= rc.bottom;
        }                
    }
    
    SetCursorPos(clip_rect.left + (clip_rect.right - clip_rect.left)/2, clip_rect.top + (clip_rect.bottom - clip_rect.top)/2);
}

internal void win32_relinquish_cursor_from_window(HWND window)
{
    if(!global_win32_state.manually_constraining_cursor_in_window)
    {
        if(global_win32_state.stored_previous_window_clip_cursor)
        {
            ClipCursor(&global_win32_state.previous_window_clip_cursor);
            global_win32_state.stored_previous_window_clip_cursor = false;
            
            log_information("Relinquishing Cursor Clip: L:%d, R:%d, B:%d, Top:%d", global_win32_state.previous_window_clip_cursor.left, global_win32_state.previous_window_clip_cursor.right, global_win32_state.previous_window_clip_cursor.bottom, global_win32_state.previous_window_clip_cursor.top);   
        }   
    }
}

internal void win32_constrain_cursor_inside_window(HWND window)
{
    if(!global_win32_state.stored_previous_window_clip_cursor)
    {
        // NOTE: Get previous window's clip cursor so we can relinquish it properly (as per: https://msdn.microsoft.com/en-us/library/windows/desktop/ms648380(v=vs.85).aspx#_win32_Confining_a_Cursor)
        if(GetClipCursor(&global_win32_state.previous_window_clip_cursor))
        {
            global_win32_state.stored_previous_window_clip_cursor = true;
        }
        else
        {
            win32_log_last_win32_api_error("GetClipCursor");
        }
        
        log_information("Recorded Relinquish Cursor Clip At: L:%d, R:%d, B:%d, T:%d", global_win32_state.previous_window_clip_cursor.left, global_win32_state.previous_window_clip_cursor.right, global_win32_state.previous_window_clip_cursor.bottom, global_win32_state.previous_window_clip_cursor.top);   
    }
    
    RECT clip_rect = {};
    GetWindowRect(window, &clip_rect);
    // NOTE: Raymond chen solution for using AdjustWindowRect to shrink window rect to client rect: https://blogs.msdn.microsoft.com/oldnewthing/20131017-00/?p=2903
    {
        RECT rc;
        SetRectEmpty(&rc);
        DWORD style = GetWindowLong(window, GWL_STYLE);
        if(AdjustWindowRect(&rc, style, FALSE))
        {
            clip_rect.left -= rc.left;
            clip_rect.top -= rc.top;
            clip_rect.right -= rc.right;
            clip_rect.bottom -= rc.bottom;
        }
    }
    
    if(ClipCursor(&clip_rect))
    {
        global_win32_state.manually_constraining_cursor_in_window = false;
        SetCursorPos(clip_rect.left + (clip_rect.right - clip_rect.left)/2, clip_rect.top + (clip_rect.bottom - clip_rect.top)/2);
        
        log_information("Clipped cursor At: L:%d, R:%d, B:%d, T:%d", clip_rect.left, clip_rect.right, clip_rect.bottom, clip_rect.top);
    }
    else
    {
        global_win32_state.manually_constraining_cursor_in_window = true;
        
        win32_log_last_win32_api_error("ClipCursor");
        log_warning("ClipCursor failed so will center cursor in window every frame.");
    }
}

internal b32 win32_set_fullscreen_mode(HWND window, b32 go_fullscreen)
{
    b32 is_fullscreen = false;
    
    // NOTE: Ripped from Raymond Chen's Blog: https://blogs.msdn.microsoft.com/oldnewthing/20100412-00/?p=14353
    DWORD dwStyle = GetWindowLong(window, GWL_STYLE);
    if(go_fullscreen)
    {
        assert((dwStyle & WS_OVERLAPPEDWINDOW));
        
        MONITORINFO mi = { sizeof(mi) };
        if (GetWindowPlacement(window, &global_win32_state.previous_window_placement) && GetMonitorInfo(MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY), &mi))
        {
            SetWindowLong(window, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(window, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top, SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
            
            is_fullscreen = true;
        }        
    }
    else
    {
        assert(!(dwStyle & WS_OVERLAPPEDWINDOW));
        
        SetWindowLong(window, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(window, &global_win32_state.previous_window_placement);
        SetWindowPos(window, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        
        is_fullscreen = false;
    }
    
    return is_fullscreen;
}

internal void win32_process_pending_messages(Win32_State *state, Game_Input *input, HWND window, Game_Settings *settings)
{
    MSG message;
    
    while( 1 )
    {
        if(PeekMessageW(&message, 0, 0, 0, PM_REMOVE))
        {
            switch(message.message)
            {
                case WM_QUIT:
                {
                    global_running = false;
                } break;
                
                case WM_SYSKEYDOWN:
                case WM_SYSKEYUP:
                case WM_KEYDOWN:
                case WM_KEYUP:
                {
                    uint32 vk_code = (uint32)message.wParam;
                    
                    bool32 was_down = ((message.lParam & (1 << 30)) != 0);
                    bool32 is_down = ((message.lParam & (1 << 31)) == 0);
                    
                    if(was_down != is_down)
                    {
                        if(vk_code == VK_ESCAPE)
                        {
                            win32_process_keyboard_message(&input->pause_button, is_down);
                        }
#if TWOSOME_INTERNAL
                        else if(vk_code == 'R')
                        {
                            win32_process_keyboard_message(&input->DEBUG_restart_stage_button, is_down);
                        }
                        else if(vk_code == 'Z')
                        {
                            win32_process_keyboard_message(&input->DEBUG_prev_stage_button, is_down);
                        }
                        else if(vk_code == 'X')
                        {
                            win32_process_keyboard_message(&input->DEBUG_next_stage_button, is_down);
                        }
                        else if(vk_code == 'N')
                        {
                            win32_process_keyboard_message(&input->DEBUG_toggle_mute_button, is_down);
                        }                                
                        else if(vk_code == 'T')
                        {
                            global_running = false;
                        }
                        else if(vk_code == 'L')
                        {
                            if(is_down)
                            {
                                if(!state->replay.playing_back)                                  
                                {
                                    if(state->replay.recording)
                                    {
                                        win32_end_recording_game_input(state);
                                    }
                                    else
                                    {
                                        win32_begin_recording_game_input(state);
                                    }
                                }
                            }
                        }
                        else if(vk_code == 'M')
                        {
                            if(is_down)
                            {
                                if(!state->replay.recording)
                                {
                                    if(state->replay.playing_back)
                                    {
                                        win32_end_game_input_playback(state);
                                    }
                                    else
                                    {
                                        win32_begin_game_input_playback(state);
                                    }
                                }
                            }
                        }
                        else if(vk_code == 'O')
                        {
                            if(!is_down)
                            {
                                settings->fullscreen = win32_set_fullscreen_mode(window, !settings->fullscreen);
                            }
                        }
#endif // TWOSOME_NTERNAL
                    }
                    
                } break;
                
                case WM_INPUT:
                {
                    UINT buffer_size_returned;
                    u8 buffer[sizeof(RAWINPUT)];
                    
                    GetRawInputData((HRAWINPUT)message.lParam, RID_INPUT, 0, &buffer_size_returned, sizeof(RAWINPUTHEADER));
                    assert(buffer_size_returned == sizeof(buffer));
                    
                    GetRawInputData((HRAWINPUT)message.lParam, RID_INPUT, buffer, &buffer_size_returned, sizeof(RAWINPUTHEADER));
                    assert(buffer_size_returned == sizeof(buffer));
                    
                    RAWINPUT *raw = (RAWINPUT *)buffer;
                    if(raw->header.dwType == RIM_TYPEMOUSE)
                    {
                        RAWMOUSE *mouse = &raw->data.mouse;
                        input->mouse_x += mouse->lLastX;
                        input->mouse_y -= mouse->lLastY;
                        
                        if(mouse->usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN)
                        {
                            win32_process_keyboard_message(&input->change_colour_button, true);
                        }
                        if(mouse->usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP)
                        {
                            win32_process_keyboard_message(&input->change_colour_button, false);
                        }
                        if(mouse->usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN)
                        {
                            win32_process_keyboard_message(&input->activate_shield_button, true);
                        }
                        if(mouse->usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP)
                        {
                            win32_process_keyboard_message(&input->activate_shield_button, false);
                        }
                        
                    }
                    
                } break;
                
                default:
                {
                    TranslateMessage(&message);
                    DispatchMessage(&message);
                } break;
            };
        }
        else
        {
            break;
        }
    }
}

internal b32 win32_get_app_output_directory_path(Win32_State *win32_state)
{
    b32 result = false;
    
    wchar_t app_data_directory_path[MAX_PATH];
    if(SHGetFolderPathW(0, CSIDL_APPDATA, 0, SHGFP_TYPE_CURRENT, app_data_directory_path) == S_OK)
    {
        // NOTE: Start with backslash as SHGetFolderPath doesn't return a trailing backslash
        wchar_t *app_output_folder_name = L"\\Twosome\\";
        
        size_t app_data_directory_path_length = wcslen(app_data_directory_path);
        size_t app_output_folder_name_length = wcslen(app_output_folder_name);
        size_t app_output_directory_full_length = (app_data_directory_path_length + app_output_folder_name_length);
        
        win32_state->app_output_path = (wchar_t *)win32_allocate_memory(sizeof(*win32_state->app_output_path) * (app_output_directory_full_length + 1));
        
        if(win32_state->app_output_path)
        {
            // NOTE: Build our app output path
            wcscpy(win32_state->app_output_path, app_data_directory_path);
            wcscat(win32_state->app_output_path, app_output_folder_name);
            
            win32_state->app_output_path_length = wcslen(win32_state->app_output_path);            
            assert(win32_state->app_output_path_length == app_output_directory_full_length);
            
            if(CreateDirectoryW(win32_state->app_output_path, 0))
            {
                result = true;
            }
            else
            {
                DWORD create_directory_error_code = GetLastError();
                if(create_directory_error_code == ERROR_ALREADY_EXISTS)
                {
                    result = true;
                }
                else if(create_directory_error_code == ERROR_PATH_NOT_FOUND)
                {
                    win32_startup_log("Application data directory path invalid.");
                }
                else
                {
                    invalid_code_path;
                }
            }
        }
    }
    
    return result;
}

// NOTE: Use this to get a suitable error message for memory allocation failure
#define win32_get_memory_allocation_failed_log_message(memory_for) "Failed to allocate memory for " memory_for ". Try closing some running applications."

internal void win32_fail_startup_and_show_notification(char *message)
{
    // NOTE: Only want to show one message box, but want everything to be logged    
    win32_startup_log(message);    
    
    if(!global_win32_state.startup_failed)
    {
        char *append_message = win32_text_line_ending "(see readme.txt for more help.)";
        u32 total_message_length = string_length(message) + string_length(append_message);
        
        char string_buffer_on_stack[512];
        char *string = 0;
        // NOTE: We use the buffer on the stack if we can (in case memory allocation fails us)
        if(total_message_length < array_count(string_buffer_on_stack))
        {            
            string = string_buffer_on_stack;
        }
        else
        {
            string = (char *)win32_allocate_memory(total_message_length + 1);
        }
        
        char *string_end = string_copy(message, string);
        string_copy(append_message, string_end);
        
        MessageBox(0, string, "Twosome Startup Failed!", MB_OK);
    }    
    
    global_win32_state.startup_failed = true;    
}

internal b32 win32_get_exe_filepath(Win32_State *state)
{
    b32 result = false;
    
    DWORD length_of_exe_path = MAX_PATH;
    DWORD length_filled = 0;
    assert(length_of_exe_path != length_filled);
    state->exe_directory_path = 0;
    
    while( 1 )
    {
        if(state->exe_directory_path)
        {
            win32_free_memory(state->exe_directory_path);
            // NOTE: Increase by 25%
            length_of_exe_path = length_of_exe_path + ((length_of_exe_path >> 2) + 1);
        }
        DWORD size_of_buffer = sizeof(*state->exe_directory_path) * length_of_exe_path;
        state->exe_directory_path = (wchar_t *)win32_allocate_memory(size_of_buffer);
        if(state->exe_directory_path)
        {
            // NOTE: If the buffer was too small then it returns size_of_buffer
            length_filled = GetModuleFileNameW(0, state->exe_directory_path, length_of_exe_path);
            if(length_filled == 0)
            {
                // NOTE: Function failed
                break;
            }
            else if(length_filled < length_of_exe_path)
            {
                // NOTE: Terminate string after last slash
                state->exe_directory_path_length = 0;
                wchar_t *one_past_last_exe_filepath_slash = state->exe_directory_path;
                for(wchar_t *scan = state->exe_directory_path; *scan; ++scan)
                {
                    if(*scan == '\\')
                    {
                        one_past_last_exe_filepath_slash = scan + 1;
                    }
                }
                assert(*(one_past_last_exe_filepath_slash - 1) == '\\');
                *one_past_last_exe_filepath_slash = '\0';
                
                state->exe_directory_path_length = (one_past_last_exe_filepath_slash - state->exe_directory_path);
                assert(state->exe_directory_path && state->exe_directory_path_length);
                
                result = true;
                break;
            }   
        }
        else
        {
            win32_startup_log(win32_get_memory_allocation_failed_log_message("game directory path"));
            break;
        }
    }
    
    return result;
}

#if TWOSOME_INTERNAL

internal Game_Code DEBUG_load_game_code_from_dll(wchar_t *source_dll_path, wchar_t *temp_dll_path)
{
    log_information("Loading game code from dll");
    
    Game_Code result = {};
    
    wchar_t *dll_path = source_dll_path;
    
    result.dll_last_write_time = DEBUG_get_last_write_time_wchar(source_dll_path);
    CopyFileW(source_dll_path, temp_dll_path, FALSE);
    dll_path = temp_dll_path;
    
    result.game_code_dll = LoadLibraryW(dll_path);
    if(result.game_code_dll)
    {
        result.update_and_render = (Game_Update_And_Render *)GetProcAddress(result.game_code_dll, "game_update_and_render");
        result.get_sound_samples = (Game_Get_Sound_Samples *)GetProcAddress(result.game_code_dll, "game_get_sound_samples");
        result.DEBUG_frame_end = (Debug_Game_Frame_End *)GetProcAddress(result.game_code_dll, "DEBUG_game_frame_end");
    }
    
    return result;
}

internal void DEBUG_unload_game_code(Game_Code *game_code)
{
    if(game_code->game_code_dll)
    {
        FreeLibrary(game_code->game_code_dll);
        game_code->game_code_dll = 0;
    }
    game_code->update_and_render = 0;
    game_code->get_sound_samples = 0;
}


#else

// NOTE: Game code translation unit is compiled along with platform one so should just be
// able to set function pointers to our linkage of them
extern "C" GAME_UPDATE_AND_RENDER(game_update_and_render);
extern "C" GAME_GET_SOUND_SAMPLES(game_get_sound_samples);

internal Game_Code load_game_code(void)
{
    Game_Code result = {};    
    result.update_and_render = game_update_and_render;
    result.get_sound_samples = game_get_sound_samples;
    
    return result;
}

#endif

internal LARGE_INTEGER win32_get_wall_clock(void)
{
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);
    return result;
}

internal r32 win32_get_wall_clock_seconds(void)
{
    LARGE_INTEGER wall_clock = win32_get_wall_clock();
    
    f32 result = wall_clock.QuadPart / (f32)global_perf_count_frequency;
    return result;
}

internal f32 win32_get_seconds_elapsed(LARGE_INTEGER start, LARGE_INTEGER end)
{
    f32 result = ((f32)(end.QuadPart - start.QuadPart) / (f32)global_perf_count_frequency);
    return result;
}

internal f32 win32_get_milliseconds_elapsed(LARGE_INTEGER start, LARGE_INTEGER end)
{
    f32 result = ((f32)((end.QuadPart - start.QuadPart) * 1000.0f) / (f32)global_perf_count_frequency);
    return result;
}

struct Platform_Work_Queue_Entry
{
    Platform_Work_Queue_Callback *callback;
    void *data;
};

struct Platform_Work_Queue
{
    u32 volatile completion_goal;
    u32 volatile completion_count;
    
    u32 volatile next_entry_to_write;
    u32 volatile next_entry_to_read;
    HANDLE semaphore_handle;
    
    Platform_Work_Queue_Entry entries[256];
};

internal PLATFORM_ADD_ENTRY(win32_add_entry)
{
    u32 new_next_entry_to_write = ((queue->next_entry_to_write + 1) % array_count(queue->entries));
    assert(new_next_entry_to_write != queue->next_entry_to_read);
    
    Platform_Work_Queue_Entry *entry = queue->entries + queue->next_entry_to_write;
    entry->callback = callback;
    entry->data = data;
    ++queue->completion_goal;
    _WriteBarrier();
    queue->next_entry_to_write = new_next_entry_to_write;
    ReleaseSemaphore(queue->semaphore_handle, 1, 0);
}

internal b32 win32_do_next_work_queue_entry(Platform_Work_Queue *queue)
{
    b32 we_should_sleep = false;
    
    u32 original_next_entry_to_read = queue->next_entry_to_read;
    u32 new_next_entry_to_read = (original_next_entry_to_read + 1) % array_count(queue->entries);
    if(original_next_entry_to_read != queue->next_entry_to_write)
    {
        u32 index = InterlockedCompareExchange((LONG volatile *)&queue->next_entry_to_read, new_next_entry_to_read, original_next_entry_to_read);
        
        if(index == original_next_entry_to_read)
        {
            Platform_Work_Queue_Entry entry = queue->entries[index];
            entry.callback(queue, entry.data);
            InterlockedIncrement((LONG volatile *)&queue->completion_count);
        }
    }
    else
    {
        we_should_sleep = true;
    }
    
    return we_should_sleep;
}

DWORD WINAPI thread_proc(LPVOID parameter)
{
    Platform_Work_Queue *queue = (Platform_Work_Queue *)parameter;
    
    while( 1 )
    {
        if(win32_do_next_work_queue_entry(queue))
        {
            WaitForSingleObjectEx(queue->semaphore_handle, INFINITE, FALSE);
        }
    }
}

internal void win32_make_queue(Platform_Work_Queue *queue, u32 thread_count)
{
    queue->completion_goal = 0;
    queue->completion_count = 0;
    
    queue->next_entry_to_write = 0;
    queue->next_entry_to_read = 0;
    
    u32 initial_count = 0;
    queue->semaphore_handle = CreateSemaphoreEx(0, initial_count, thread_count, 0, 0, SEMAPHORE_ALL_ACCESS);
    
    for(u32 thread_index = 0; thread_index < thread_count; ++thread_index)
    {
        DWORD thread_id;
        HANDLE thread_handle = CreateThread(0, 0, thread_proc, queue, 0, &thread_id);
        CloseHandle(thread_handle);
    }
}

internal PLATFORM_COMPLETE_ALL_WORK(win32_complete_all_work)
{
    while(queue->completion_goal != queue->completion_count)
    {
        win32_do_next_work_queue_entry(queue);
    }
    
    queue->completion_goal = 0;
    queue->completion_count = 0;
}

internal b32 win32_init_dsound(HWND window, int32 samples_per_second, int32 buffer_size)
{
    win32_startup_log("Initializing DirectSound");
    
    b32 result = false;
    
    HMODULE dsound_library = LoadLibraryW(L"dsound.dll");
    if(dsound_library)
    {
        Direct_Sound_Create *direct_sound_create = (Direct_Sound_Create *)GetProcAddress(dsound_library, "DirectSoundCreate");
        if(direct_sound_create)
        {
            WAVEFORMATEX wave_format = {};
            wave_format.wFormatTag = WAVE_FORMAT_PCM;
            wave_format.nChannels = 2;
            wave_format.nSamplesPerSec = samples_per_second;
            wave_format.wBitsPerSample = 16;
            wave_format.nBlockAlign = (wave_format.wBitsPerSample * wave_format.nChannels) / 8;
            wave_format.nAvgBytesPerSec = wave_format.nBlockAlign * wave_format.nSamplesPerSec;
            
            LPDIRECTSOUND direct_sound;
            if(SUCCEEDED(direct_sound_create(NULL, &direct_sound, NULL)))
            {
                LPDIRECTSOUNDBUFFER primary_buffer; 
                if(SUCCEEDED(direct_sound->SetCooperativeLevel(window, DSSCL_PRIORITY)))
                {
                    DSBUFFERDESC buffer_desc = {};
                    buffer_desc.dwSize = sizeof(buffer_desc);
                    buffer_desc.dwFlags = DSBCAPS_PRIMARYBUFFER;
                    
                    if(SUCCEEDED(direct_sound->CreateSoundBuffer(&buffer_desc, &primary_buffer, NULL)))
                    {
                        if(SUCCEEDED(primary_buffer->SetFormat(&wave_format)))
                        {
                            win32_startup_log("Primary DirectSound buffer was set");
                            
                            DSBUFFERDESC buffer_desc = {};
                            buffer_desc.dwSize = sizeof(buffer_desc);
                            buffer_desc.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS;
                            buffer_desc.dwBufferBytes = buffer_size;
                            buffer_desc.lpwfxFormat = &wave_format;
                            if(SUCCEEDED(direct_sound->CreateSoundBuffer(&buffer_desc, &global_secondary_buffer, 0)))
                            {
                                result = true;
                                win32_startup_log("Secondary DirectSound buffer created successfully.");
                            }                            
                        }
                    }
                }
                else
                {
                    win32_startup_log("SetCooperativeLevel failed");
                }
            }
            else
            {
                win32_startup_log("DirectSoundCreate failed");
            }
        }
        else
        {
            win32_startup_log("Failed to get DirectSoundCreate function");
        }
    }
    else
    {
        win32_startup_log("Failed to load dsound.dll");
    }
    
    return result;
}

internal b32 win32_fill_dsound_buffer(Game_Sound_Output_Buffer *sound_buffer, Win32_Sound_Output *sound_output, DWORD byte_to_lock, DWORD bytes_to_write)
{
    b32 result = false;
    
    if(global_win32_state.directsound_enabled)
    {
        assert(global_secondary_buffer);
        assert(bytes_to_write);
        
        void *region1;
        DWORD region1_size;
        void *region2;
        DWORD region2_size;
        if(SUCCEEDED(global_secondary_buffer->Lock(byte_to_lock, bytes_to_write, &region1, &region1_size, &region2, &region2_size, 0)))
        {
            DWORD region1_samples_count = (region1_size / sound_output->bytes_per_sample);
            DWORD region2_samples_count = (region2_size / sound_output->bytes_per_sample);
            assert( (region1_samples_count + region2_samples_count) == (DWORD)sound_buffer->sample_count );
            
            s16* src = (s16 *)sound_buffer->samples;        
            
            if(region1_samples_count > 0)
            {
                s16* dest = (s16 *)region1;
                for(DWORD region1_sample_index = 0; region1_sample_index < region1_samples_count; ++region1_sample_index)
                {
                    *dest++ = *src++;
                    *dest++ = *src++;
                }
            }
            
            if(region2_samples_count > 0)
            {
                s16 *dest = (s16 *)region2;
                for(DWORD region2_sample_index = 0; region2_sample_index < region2_samples_count; ++region2_sample_index)
                {
                    *dest++ = *src++;
                    *dest++ = *src++;
                }
            }
            
            sound_output->running_cursor += bytes_to_write;
            sound_output->running_cursor %= sound_output->secondary_buffer_size;
            
            if(global_secondary_buffer->Unlock(region1, region1_size, region2, region2_size) == DS_OK)
            {
                result = true;
            }
            else
            {
                log_error("Unlock failed");
            }
        }
        else
        {
            log_error("Lock failed");
        }
    }
    
    return result;
}

internal b32 win32_clear_dsound_buffer(Win32_Sound_Output *sound_output)
{
    b32 result = false;
    
    if(global_win32_state.directsound_enabled)
    {
        assert(global_secondary_buffer);
        if(global_secondary_buffer)
        {
            void *region1;
            DWORD region1_size;
            void *region2;
            DWORD region2_size;
            
            if(SUCCEEDED(global_secondary_buffer->Lock(0, sound_output->secondary_buffer_size, &region1, &region1_size, &region2, &region2_size, 0)))
            {
                uint8 *dest_sample = (uint8 *)region1;
                for(DWORD byte_index = 0; byte_index < region1_size; ++byte_index)
                {
                    *dest_sample++ = 0;
                }
                
                dest_sample = (uint8 *)region2;
                for(DWORD byte_index = 0; byte_index < region2_size; ++byte_index)
                {
                    *dest_sample++ = 0;
                }
                
                if(global_secondary_buffer->Unlock(region1, region1_size, region2, region2_size) == DS_OK)
                {
                    result = true;
                }
            }
        }
    }
    
    return result;
}

LRESULT CALLBACK win32_main_window_callback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;
    switch(message)
    {
        case WM_CLOSE:
        {
            global_running = false;
        } break;
        
        case WM_DESTROY:
        {
            global_running = false;
        } break;
        
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            log_error("Keyboard input came in through non-dispatch message! Message value: %d", message);
        } break;        
        
        case WM_SIZE:
        {
            s32 width = LOWORD(lParam);
            s32 height = HIWORD(lParam);
            
            global_screen_width = width;
            global_screen_height = height;
            
            global_win32_state.window_position_dirty = true;
            
            log_information("Resized window");
        } break;
        
        case WM_NCACTIVATE:
        {
            // NOTE: Don't know why but the title bar being activated is most reliable way of knowing
            // if we've gained focus (it doesn't grab focus when you click taskbar for instance)
            if(wParam == TRUE)
            {
                global_win32_state.window_made_active = true;
                log_information("Title bar activated");
            }
            else
            {
                global_win32_state.window_made_active = false;
                log_information("Title bar Deactivated");
            }
            
            result = DefWindowProcW(window, message, wParam, lParam);
        } break;
        
        case WM_KILLFOCUS:
        {            
            win32_relinquish_cursor_from_window(window);
            global_win32_state.window_made_active = false;
            global_win32_state.show_cursor = true;
            
            log_information("Lost focus");                        
        } break;
        
        case WM_SETCURSOR:
        {
            if(global_win32_state.show_cursor)
            {
                result = DefWindowProcW(window, message, wParam, lParam);
            }
            else
            {
                SetCursor(0);
            }
        } break;        
        
        case WM_MOVE:
        {
            global_win32_state.window_position_dirty = true;
            log_information("Moved Window");
        } break;
        
        case WM_GETMINMAXINFO:
        {
            // NOTE: Don't allow window smaller than minimum size.
            // NOTE: Need to offset the min values by the border and title bar
            RECT rc = {};
            rc.right = minimum_screen_width;
            rc.bottom = minimum_screen_height;
            DWORD style = GetWindowLong(window, GWL_STYLE);
            if(AdjustWindowRect(&rc, style, FALSE))
            {
                MINMAXINFO *min_max_info = (MINMAXINFO *)lParam;
                min_max_info->ptMinTrackSize.x = rc.right - rc.left;
                min_max_info->ptMinTrackSize.y = rc.bottom - rc.top;
            }                
        } break;
        
        default:
        {
            result = DefWindowProcW(window, message, wParam, lParam);
        } break;
    };
    
    return result;
}

internal void win32_set_pixel_format(HDC window_dc)
{
    int suggested_pixel_format_index = 0;
    GLuint extended_pick = 0;
    
    if(wgl_choose_pixel_format_arb)
    {
        int pixel_format_attrib_list[] =
        {
            WGL_DRAW_TO_WINDOW_ARB,  GL_TRUE,
            WGL_SUPPORT_OPENGL_ARB,  GL_TRUE,
            WGL_DOUBLE_BUFFER_ARB,   GL_TRUE,
            WGL_PIXEL_TYPE_ARB,      WGL_TYPE_RGBA_ARB,
            WGL_ACCELERATION_ARB,    WGL_FULL_ACCELERATION_ARB,
            //WGL_COLOR_BITS_ARB,      32,
            WGL_DEPTH_BITS_ARB,      24,
            0
        };
        
        if(!wgl_choose_pixel_format_arb(window_dc, pixel_format_attrib_list, 0, 1, &suggested_pixel_format_index, &extended_pick))
        {
            win32_startup_log("wglChoosePixelFormat failed.");
        }        
    }
    
    if(!extended_pick)
    {
        PIXELFORMATDESCRIPTOR desired_pixel_format = {};
        desired_pixel_format.nSize = sizeof(desired_pixel_format);
        desired_pixel_format.nVersion = 1;
        desired_pixel_format.iPixelType = PFD_TYPE_RGBA;
        desired_pixel_format.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
        desired_pixel_format.cColorBits = 32;
        desired_pixel_format.cDepthBits = 24;
        desired_pixel_format.iLayerType = PFD_MAIN_PLANE;
        
        suggested_pixel_format_index = ChoosePixelFormat(window_dc, &desired_pixel_format);
    }
    
    PIXELFORMATDESCRIPTOR suggested_pixel_format = {};
    if(DescribePixelFormat(window_dc, suggested_pixel_format_index, sizeof(suggested_pixel_format), &suggested_pixel_format))
    {
        if(!SetPixelFormat(window_dc, suggested_pixel_format_index, &suggested_pixel_format))
        {
            win32_log_last_win32_api_error("SetPixelFormat");
            win32_startup_log("SetPixelFormat failed.");
        }
    }
    else
    {
        win32_log_last_win32_api_error("DescribePixelFormat");
        win32_startup_log("DescribePixelFormat failed.");
    }
}

#if GL_DEBUGGING_ENABLED
internal GL_DEBUG_PROC(gl_debug_callback)
{
    // NOTE: This seems to log like crazy on some graphics cards so
    // just turn it on when actively debugging gl
#if 0
    
    char *source_msg = "Unknown";
    if(source == GL_DEBUG_SOURCE_API) source_msg = "API";
    else if(source == GL_DEBUG_SOURCE_WINDOW_SYSTEM) source_msg = "Window System";
    else if(source == GL_DEBUG_SOURCE_SHADER_COMPILER) source_msg = "Shader Compiler";
    else if(source == GL_DEBUG_SOURCE_THIRD_PARTY) source_msg = "Third Party";
    else if(source == GL_DEBUG_SOURCE_APPLICATION) source_msg = "Application User";
    else if(source == GL_DEBUG_SOURCE_OTHER_ARB) source_msg = "Other";
    
    char *type_msg = "Unknown";
    if(source == GL_DEBUG_TYPE_ERROR) source_msg = "Error";
    else if(source == GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR) source_msg = "Deprecated Behaviour";
    else if(source == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR) source_msg = "Undefined Behaviour";
    else if(source == GL_DEBUG_TYPE_PORTABILITY) source_msg = "Portability";
    else if(source == GL_DEBUG_TYPE_PERFORMANCE) source_msg = "Performance";
    else if(source == GL_DEBUG_TYPE_OTHER) source_msg = "Other";
    
    char *severity_msg = "Unknown";
    if(source == GL_DEBUG_SEVERITY_HIGH) severity_msg = "High";
    else if(source == GL_DEBUG_SEVERITY_MEDIUM) severity_msg = "Medium";
    else if(source == GL_DEBUG_SEVERITY_LOW) severity_msg = "Low";
    else if(source == GL_DEBUG_SEVERITY_NOTIFICATION) severity_msg = "Notification";
    
    // Type
    char message_fmt[] = "GL Output (%s): Severity: '%s'; Message: '%s'; Source: '%s'";
    
    size_t log_message_size = array_count(message_fmt) + strlen(type_msg) + strlen(severity_msg) + strlen(message) + strlen(source_msg) + 1;
    char *log_message = (char *)win32_allocate_memory(log_message_size);
    sprintf(log_message, message_fmt, type_msg, severity_msg, message, source_msg);
    
    win32_put_message_in_log(log_message);
    win32_free_memory(log_message);
    
#endif
}
#endif

internal void win32_load_wgl_extensions(void)
{
    win32_startup_log("Loading WGL functions");
    
    // NOTE: This crazy shit is because to get a gl extension you have to have a gl context, but we want the extension
    // that creates us a a gl context, so have to create a temporary window just to do this....
    WNDCLASSW window_class = {};
    window_class.lpfnWndProc = DefWindowProcW;
    window_class.hInstance = GetModuleHandle(0);
    window_class.lpszClassName = L"TwosomeWGLLoader";
    
    if(RegisterClassW(&window_class))
    {
        HWND window = CreateWindowW(window_class.lpszClassName, L"Twosome", 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, window_class.hInstance, 0);
        if(window)
        {
            HDC window_dc = GetDC(window);
            if(window_dc)
            {
                win32_set_pixel_format(window_dc);
                
                HGLRC open_gl_rc = wglCreateContext(window_dc);
                
                if(wglMakeCurrent(window_dc, open_gl_rc))
                {
                    wgl_choose_pixel_format_arb = (Wgl_Choose_Pixel_Format_Arb *)win32_get_gl_function("wglChoosePixelFormatARB");
                    wgl_create_context_attribs_arb = (Wgl_Create_Context_Attribs_Arb *)win32_get_gl_function("wglCreateContextAttribsARB");
                    wgl_swap_interval_ext = (Wgl_Swap_Interval_Ext *)win32_get_gl_function("wglSwapIntervalEXT");
                    
                    wglMakeCurrent(0, 0);
                }
                else
                {
                    win32_log_last_win32_api_error("wglMakeCurrent");
                }
                
                wglDeleteContext(open_gl_rc);
                ReleaseDC(window, window_dc);
            }
            else
            {
                win32_log_last_win32_api_error("GetDC");
            }
            
            DestroyWindow(window);
        }
        else
        {
            win32_log_last_win32_api_error("CreateWindowEx");
        }
    }
    else
    {
        win32_log_last_win32_api_error("RegisterClass");
    }
}

internal void win32_load_opengl_functions(void)
{
    win32_startup_log("Loading GL functions");
    
    glCreateShader = (Gl_Create_Shader*)win32_get_gl_function("glCreateShader");
    glUseProgram = (Gl_Use_Program *)win32_get_gl_function("glUseProgram");    
    glShaderSource = (Gl_Shader_Source *)win32_get_gl_function("glShaderSource");
    
    glCompileShader = (Gl_Compile_Shader *)win32_get_gl_function("glCompileShader");
    glGetShaderiv = (Gl_Get_Shader_Iv *)win32_get_gl_function("glGetShaderiv");
    glAttachShader = (Gl_Attach_Shader *)win32_get_gl_function("glAttachShader");
    glLinkProgram = (Gl_Link_Program *)win32_get_gl_function("glLinkProgram");
    glGetProgramiv = (Gl_Get_Program_Iv *)win32_get_gl_function("glGetProgramiv");
    glCreateProgram = (Gl_Create_Program *)win32_get_gl_function("glCreateProgram");
    glGetProgramInfoLog = (Gl_Get_Program_Info_Log *)win32_get_gl_function("glGetProgramInfoLog");
    glBindAttribLocation = (Gl_Bind_Attrib_Location *)win32_get_gl_function("glBindAttribLocation");
    glGetUniformLocation = (Gl_Get_Uniform_Location *)win32_get_gl_function("glGetUniformLocation");
    glVertexAttribPointer = (Gl_Vertex_Attrib_Pointer *)win32_get_gl_function("glVertexAttribPointer");
    glUniformMatrix4fv = (Gl_Uniform_Matrix_4fv *)win32_get_gl_function("glUniformMatrix4fv");
    glUniform4f = (Gl_Uniform_4f *)win32_get_gl_function("glUniform4f");
    glEnableVertexAttribArray = (Gl_Enable_Vertex_Attrib_Array *)win32_get_gl_function("glEnableVertexAttribArray");
    glGenVertexArrays = (Gl_Gen_Vertex_Arrays *)win32_get_gl_function("glGenVertexArrays");
    glGenBuffers = (Gl_Gen_Buffers *)win32_get_gl_function("glGenBuffers");
    glBindVertexArray = (Gl_Bind_Vertex_Array *)win32_get_gl_function("glBindVertexArray");
    glBindBuffer = (Gl_Bind_Buffer *)win32_get_gl_function("glBindBuffer");
    glBufferData = (Gl_Buffer_Data *)win32_get_gl_function("glBufferData");
    glBufferSubData = (Gl_Buffer_Sub_Data *)win32_get_gl_function("glBufferSubData");
    glActiveTexture = (Gl_Active_Texture*)win32_get_gl_function("glActiveTexture");
    glUniform1i = (Gl_Uniform_1i *)win32_get_gl_function("glUniform1i");
    glUniform4fv = (Gl_Uniform_4fv *)win32_get_gl_function("glUniform4fv");
    glGetShaderInfoLog = (Gl_Get_Shader_Info_Log *)win32_get_gl_function("glGetShaderInfoLog");
    glDeleteVertexArrays = (Gl_Delete_Vertex_Arrays *)win32_get_gl_function("glDeleteVertexArrays");
    glDeleteBuffers = (Gl_Delete_Buffers *)win32_get_gl_function("glDeleteBuffers");
    
#if GL_DEBUGGING_ENABLED
    Gl_Debug_Message_Callback *glDebugMessageCallback = (Gl_Debug_Message_Callback *)DEBUG_win32_get_optional_gl_function("glDebugMessageCallback");
    if(glDebugMessageCallback)
    {
        glDebugMessageCallback(&gl_debug_callback, 0);
        log_information("GL Debugging supported.");
    }
    else
    {
        log_warning("GL Debugging NOT supported.");
    }
#endif    
}

global_variable s32 win32_context_attrib_list[] =
{
    WGL_CONTEXT_MAJOR_VERSION_ARB, required_gl_major_version,
    WGL_CONTEXT_MINOR_VERSION_ARB, required_gl_minor_version,
    0
};

internal b32 win32_init_opengl(HDC window_dc, b32 vsync, Win32_State *state)
{
    win32_startup_log("Initializing OpenGL");
    
    b32 result = false;
    
    win32_load_wgl_extensions();
    win32_set_pixel_format(window_dc);
    
    HGLRC opengl_context = 0;
    if(wgl_create_context_attribs_arb)
    {
        opengl_context = wgl_create_context_attribs_arb(window_dc, 0, win32_context_attrib_list);
    }
    
    if(opengl_context)
    {
        win32_startup_log("Got GL context");
        
        assert(opengl_context);    
        
        if(wglMakeCurrent(window_dc, opengl_context))
        {
            if(check_gl_and_glsl_compatibility())
            {
                win32_load_opengl_functions();
                
                if(wgl_swap_interval_ext)
                {
                    if(vsync)
                    {                        
                        state->vsync_enabled = (wgl_swap_interval_ext(1));
                        if(state->vsync_enabled)
                        {
                            win32_startup_log("Enabled V-Sync");
                        }
                        else
                        {
                            win32_startup_log("Failed to enable V-Sync");
                        }
                    }
                    else
                    {
                        wgl_swap_interval_ext(0);
                        state->vsync_enabled = false;
                        
                        win32_startup_log("No V-Sync");
                    }
                    
                    if(global_win32_state.failed_to_get_an_opengl_function)
                    {
                        win32_startup_log("Failed to load required OpenGL function(/s)");
                    }
                    else
                    {
#if TWOSOME_SLOW
                        // NOTE: Since this is the point that we're checking that didn't fail
                        // to get a gl function, should stop futher calls to get gl functions
                        // as they will be bypassing this check
                        global_win32_state.DEBUG_stop_further_opengl_function_getting = true;
#endif                        
                        result = true;    
                    }
                }
            }
            else
            {
                win32_startup_log("Graphics card doesn't support required OpenGL version");
            }
        }
        else
        {
            win32_startup_log("Failed to make OpenGL context current");
        }
        
    }
    else
    {
        win32_startup_log("Failed to create OpenGL context");
    }    
    
    return result;
}

internal Game_Sound_Output_Buffer win32_get_game_sound_output_buffer(DWORD bytes_to_write, Win32_Sound_Output *sound_output, s16 *samples)
{
    Game_Sound_Output_Buffer buffer = {};
    buffer.samples_per_second = sound_output->samples_per_second;
    
    // NOTE: Always ask for multiple of 4 samples
    buffer.sample_count = align4(bytes_to_write / sound_output->bytes_per_sample);
    buffer.samples = samples;
    
    return buffer;
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmd_line, int cmd_show)
{
    //
    // NOTE: Startup Segment Begins!
    //
    Platform_Work_Queue work_queue = {};
    win32_make_queue(&work_queue, 2);
    
    LARGE_INTEGER perf_count_frequency_result;
    if(QueryPerformanceFrequency(&perf_count_frequency_result) == FALSE)
    {
        win32_fail_startup_and_show_notification("Hardware doesn't support a high-resolution performance counter.");
    }
    
    global_perf_count_frequency = perf_count_frequency_result.QuadPart;
    
    //
    // NOTE: Platform-layer services to game
    //
    Platform_API platform_api = {};
    {
#if TWOSOME_SLOW
        platform_api.DEBUG_log_message = win32_put_message_in_log;
        platform_api.DEBUG_show_assert_popup = DEBUG_platform_show_assert_popup;
#endif
        
#if TWOSOME_INTERNAL
        platform_api.DEBUG_free_file_memory = DEBUG_platform_free_file_memory;
        platform_api.DEBUG_read_entire_file = DEBUG_platform_read_entire_file;
        platform_api.DEBUG_write_entire_file = DEBUG_platform_write_entire_file;
        platform_api.DEBUG_delete_file = DEBUG_platform_delete_file;
        platform_api.DEBUG_count_files_in_directory = DEBUG_platform_count_files_in_directory;
        platform_api.DEBUG_get_latest_file_write_in_directory = DEBUG_platform_get_latest_file_write_time_in_directory;
        platform_api.DEBUG_get_last_file_write_time = DEBUG_get_last_write_time;
#endif
        platform_api.open_file = win32_open_file;
        platform_api.read_data_from_file = win32_read_data_from_file;
        platform_api.write_entire_file_to_app_output_directory = win32_write_entire_file_to_app_output_directory;
        platform_api.load_game_save_file = win32_load_game_save_file;
        platform_api.write_game_save_file = win32_write_game_save_file;
        platform_api.read_entire_file = win32_read_entire_file;
        platform_api.close_file = win32_close_file;
        platform_api.get_current_time = win32_get_current_time;
        
        platform_api.add_entry = win32_add_entry;
        platform_api.complete_all_work = win32_complete_all_work;
        platform_api.text_line_ending = win32_text_line_ending;
        platform_api.player_platform_option_flags = player_platform_option_fullscreen_flag | player_platform_option_quit_flag;
    }
    
    //
    // NOTE: Allocate game memory
    //
#if TWOSOME_INTERNAL
    LPVOID base_address = (LPVOID)terabytes(4);
#else
    LPVOID base_address = 0;
#endif
    
    Game_Memory game_memory = {};
    game_memory.permanent_storage_size = megabytes(32);
    game_memory.transient_storage_size = megabytes(192);
    
    global_win32_state.game_memory_block_size = game_memory.permanent_storage_size + game_memory.transient_storage_size;
#if TWOSOME_INTERNAL
    game_memory.debug_storage_size = megabytes(64);
    global_win32_state.game_memory_block_size += game_memory.debug_storage_size;
#endif
    
    global_win32_state.game_memory_block = VirtualAlloc(base_address, global_win32_state.game_memory_block_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    assert(global_win32_state.game_memory_block);
    
    if(!global_win32_state.game_memory_block)
    {
        win32_log_last_win32_api_error("VirtualAlloc");
        win32_fail_startup_and_show_notification(win32_get_memory_allocation_failed_log_message("game"));
    }
    
    game_memory.permanent_storage = global_win32_state.game_memory_block;
    game_memory.transient_storage = ((uint8 *)game_memory.permanent_storage + game_memory.permanent_storage_size);
    
#if TWOSOME_INTERNAL
    game_memory.debug_storage = ((u8*)game_memory.transient_storage + game_memory.transient_storage_size);
#endif
    
    game_memory.work_queue = &work_queue;
    game_memory.platform_api = platform_api;
    
    set_global_debug_state(&game_memory);
    
    // NOTE: Global Win32 State Temporary Arena
    {
        Memory_Index win32_temp_buffer_size = megabytes(16);
        void *win32_temp_buffer = win32_allocate_memory(win32_temp_buffer_size);
        initialize_arena(&global_win32_state.temp_arena, win32_temp_buffer_size, win32_temp_buffer);
        
        TRACK_MEMORY_ARENA(global_win32_state.temp_arena);
        
        if(!win32_temp_buffer)
        {
            win32_fail_startup_and_show_notification(win32_get_memory_allocation_failed_log_message("platform layer"));
        }
    }
    
    // NOTE: Get app output directory path
    if(!win32_get_app_output_directory_path(&global_win32_state))
    {
        win32_fail_startup_and_show_notification("Failed to get path to application data output directory.");
    }
    
    // NOTE: Startup log file
    global_win32_state.startup_log_file = win32_open_file("startup_log.txt", platform_directory_type_output, platform_file_access_mode_write);
    win32_startup_log("Platform startup begins");
    
    // NOTE: Get path to game directory
    if(!win32_get_exe_filepath(&global_win32_state))
    {
        win32_fail_startup_and_show_notification("Failed to get path to game's directory.");
    }
    
#if TWOSOME_SLOW
    {
        wchar_t log_filepath[MAX_PATH] = {};
        swprintf(log_filepath, array_count(log_filepath), L"%s%s", global_win32_state.exe_directory_path, L"log.txt");
        
        global_win32_state.log_fp = _wfopen(log_filepath, L"a");
    }
#endif    
    set_global_log_state(platform_api);
    
    // NOTE: We can't really continue with startup, if startup has already failed
    if(!global_win32_state.startup_failed)
    {
        
#if TWOSOME_INTERNAL
        Win32_DirectSound_Debug_Marker directsound_debug_markers[15];
        u32 directsound_debug_marker_index = 0;
#endif
        
        win32_startup_log("Loading game settings");
        Game_Settings game_settings = load_game_settings_from_file(&platform_api, &global_win32_state.temp_arena);
        
        UINT desired_schedular_ms = 1;
        bool32 sleep_is_granular = (timeBeginPeriod(desired_schedular_ms) == TIMERR_NOERROR);
        
        //
        // NOTE: Create Window
        //
        win32_startup_log("Creating window");
        HWND window = 0;
        {
            WNDCLASSW win_class = {};
            win_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
            win_class.lpfnWndProc = win32_main_window_callback;
            win_class.hInstance = instance;
            win_class.hCursor = LoadCursor(0, IDC_ARROW);
            win_class.hIcon = LoadIcon(instance, MAKEINTRESOURCE(IDI_ICON));
            
            win_class.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
            win_class.lpszClassName = L"twosome_window_class";
            if(RegisterClassW(&win_class))
            {
                DWORD window_style = WS_OVERLAPPEDWINDOW;
                
                window = CreateWindowW(win_class.lpszClassName, L"Twosome", window_style, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, 0, instance, NULL);            
                if(window)
                {
                    HDC opengl_dc = GetDC(window);
                    
                    if(opengl_dc && win32_init_opengl(opengl_dc, game_settings.vsync, &global_win32_state))
                    {
                        ReleaseDC(window, opengl_dc);
                    }
                    else
                    {
                        win32_log_last_win32_api_error("GetDC");
                        win32_fail_startup_and_show_notification("Failed to initialize OpenGL. Try updating your graphics drivers.");                    
                    }
                    
                    if(game_settings.fullscreen)
                    {
                        win32_startup_log("Going fullscreen");
                        game_settings.fullscreen = win32_set_fullscreen_mode(window, game_settings.fullscreen);
                    }
                }
                else
                {
                    win32_log_last_win32_api_error("CreateWindowEx");
                }
            }
            else
            {
                win32_log_last_win32_api_error("RegisterClass");
            }
        }
        
        if(!window)
        {
            win32_fail_startup_and_show_notification("Failed to create a window.");
        }
        
        int monitor_refresh_hz = 60;
        HDC refresh_dc = GetDC(window);
        int win32_refresh_rate = GetDeviceCaps(refresh_dc, VREFRESH);
        ReleaseDC(window, refresh_dc);
        if(win32_refresh_rate > 1)
        {
            monitor_refresh_hz = win32_refresh_rate;
        }
        else
        {
            win32_startup_log("Failed to get monitor refresh rate. Using default.");
        }
        
        u32 game_update_hz = monitor_refresh_hz;
        
#if 0
        {
#if TWOSOME_INTERNAL
            wgl_swap_interval_ext(0);
            global_win32_state.vsync_enabled = false;
            log_information("vsync disabled");
#endif
        }
#endif    
        
        // NOTE: Register for Raw Mouse Input
        win32_startup_log("Registering for Raw Input");
        {
            RAWINPUTDEVICE rid = { 0 };
            
            // NOTE Mouse
            rid.usUsagePage = 0x01;
            rid.usUsage = 0x02;
            rid.dwFlags = 0;
            rid.hwndTarget = 0;
            
            if(RegisterRawInputDevices(&rid, 1, sizeof(rid)) == FALSE)
            {
                win32_log_last_win32_api_error("RegisterRawInputDevices");
                win32_fail_startup_and_show_notification("Failed to register input device.");
            }
        }
        
        //
        // NOTE: DirectSound
        //
        u32 sound_update_hz = 30;
        u32 sound_tick_interval = (game_update_hz / sound_update_hz);
        
        Win32_Sound_Output sound_output = {};
        sound_output.samples_per_second = 44100;
        sound_output.bytes_per_sample = sizeof(s16)*2;
        // NOTE: Big enough for one second of sound data
        sound_output.secondary_buffer_size = (sound_output.samples_per_second*sound_output.bytes_per_sample);    
        sound_output.expected_sound_bytes_per_tick = (DWORD)((f32)(sound_output.samples_per_second * sound_output.bytes_per_sample) / (f32)sound_update_hz);
        sound_output.latency_ticks = min_sound_latency_ticks;
        
        // NOTE: Assert that direct sound buffer is big enough to hold 5 frames of audio data (including max latency)
        assert( ((sound_output.expected_sound_bytes_per_tick + (sound_output.expected_sound_bytes_per_tick*max_sound_latency_ticks)) * 5) <= sound_output.secondary_buffer_size );
        
        s16 *samples = (s16 *)win32_allocate_memory(sound_output.secondary_buffer_size);
        if(!samples)
        {
            win32_fail_startup_and_show_notification(win32_get_memory_allocation_failed_log_message("sound"));
        }
        
        if(win32_init_dsound(window, sound_output.samples_per_second, sound_output.secondary_buffer_size))
        {
            global_win32_state.directsound_enabled = true;
        }
        
        if(!win32_clear_dsound_buffer(&sound_output))
        {
            global_win32_state.directsound_enabled = false;
        }
        
        if(global_win32_state.directsound_enabled)
        {
            assert(global_secondary_buffer);            
            if(global_secondary_buffer->Play(0, 0, DSBPLAY_LOOPING) != DS_OK)
            {
                global_win32_state.directsound_enabled = false;
                win32_startup_log("Failed to play DirectSound buffer");                
            }
        }
        
        Game_Input input[2] = {};
        Game_Input *new_input = &input[0];
        Game_Input *old_input = &input[1];
        
        Memory_Index render_commands_push_buffer_size = megabytes(32);
        void *render_commands_push_buffer = win32_allocate_memory(render_commands_push_buffer_size);        
        TRACK_BUFFER_USAGE(render_commands_push_buffer, render_commands_push_buffer_size);
        if(!render_commands_push_buffer)
        {
            win32_fail_startup_and_show_notification(win32_get_memory_allocation_failed_log_message("rendering"));
        }
        
        if(!global_win32_state.failed_to_get_an_opengl_function)
        {
            win32_startup_log("Loading OpenGL resources");
            opengl_load_resources(&global_win32_state.temp_arena);
            // NOTE(HACK): We call SwapBuffers here so that shader creation from opengl_load_resources get executed,
            // apparently glFlush/glFinish supposed to do that but that didn't seem to do anything...
            // 
            {
                HDC device_context = GetDC(window);
                BOOL swap_buffers_result = SwapBuffers(device_context);
                assert(swap_buffers_result);
                ReleaseDC(window, device_context);
            }   
        }
        
#if TWOSOME_INTERNAL
        wchar_t temp_game_code_dll_full_path[MAX_PATH + 1];
        win32_build_exe_directory_filepath(&global_win32_state, L"twosome_temp.dll", array_count(temp_game_code_dll_full_path), temp_game_code_dll_full_path);
        
        wchar_t pdb_lock_full_path[DEBUG_win32_state_filename_count];
        win32_build_exe_directory_filepath(&global_win32_state, L"pdb.lock", array_count(pdb_lock_full_path), pdb_lock_full_path);
        
        wchar_t source_game_code_dll_full_path[MAX_PATH + 1];
        win32_build_exe_directory_filepath(&global_win32_state, L"twosome.dll", array_count(source_game_code_dll_full_path), source_game_code_dll_full_path);
        
        Game_Code game = DEBUG_load_game_code_from_dll(source_game_code_dll_full_path, temp_game_code_dll_full_path);
        
        
        // NOTE: Debug rendering init
        {
            Vec2 verts[] =
            {
                { 0, 0 },
                { 1, 0 },
                { 0, 1 },
                
                { 0, 1 },
                { 1, 0 },
                { 1, 1 },
            };
            
            create_mesh_and_set_data(&DEBUG_rendering.quad, (f32 *)verts, array_count(verts), vertex_flag_xy, GL_TRIANGLES, GL_STATIC_DRAW);
        }
        
#else
        Game_Code game = load_game_code();
#endif
        
        // NOTE: "If the window was previously hidden, the return value is zero." - MSDN
        if(ShowWindow(window, SW_SHOW) != 0)
        {
            win32_startup_log("Failed to show window");
        }
        
        win32_startup_log("Platform startup ends");
        
        win32_close_file(&global_win32_state.startup_log_file);
        
        if(global_win32_state.startup_failed)
        {
            global_running = false;
        }
        
        //
        // NOTE: Startup Segment Ends!
        //
        
        LARGE_INTEGER last_counter = win32_get_wall_clock();
        bool32 sound_is_valid = false;
        LARGE_INTEGER flip_wall_clock = win32_get_wall_clock();    
        while(global_running)
        {
#if TWOSOME_INTERNAL
            // NOTE: Check if need to reload dll for live-code editing
            {
                Platform_Date_Time new_dll_write_time = DEBUG_get_last_write_time_wchar(source_game_code_dll_full_path);
                
                if(compare_platform_date_times(new_dll_write_time, game.dll_last_write_time) != platform_date_time_compare_equal)
                {
                    // PDB file is created after dll so wait until it is finished.
                    DWORD attributes = GetFileAttributesW(pdb_lock_full_path);
                    if(attributes == INVALID_FILE_ATTRIBUTES)
                    {
                        DEBUG_unload_game_code(&game);
                        game = DEBUG_load_game_code_from_dll(source_game_code_dll_full_path, temp_game_code_dll_full_path);
                    }
                }
            }
#endif
            
            // NOTE: Do input swapping from last frame        
            for(s32 button_index = 0; button_index < array_count(new_input->buttons); ++button_index)
            {
                zero_object(Game_Button_State, new_input->buttons[button_index]);
                new_input->buttons[button_index].down = old_input->buttons[button_index].down;
            }
            
            f32 target_seconds_per_frame = 1.0f / (f32)game_update_hz;
            
            new_input->mouse_x = 0;
            new_input->mouse_y = 0;
            
            new_input->dt = target_seconds_per_frame;
            new_input->settings = game_settings;
            
            Game_Render_Commands render_commands = {};
            render_commands.push_buffer_base = (u8 *)render_commands_push_buffer;
            render_commands.push_buffer_size = render_commands_push_buffer_size;
            render_commands.push_buffer_at = render_commands.push_buffer_base + render_commands.push_buffer_size;
            render_commands.window_width = global_screen_width;
            render_commands.window_height = global_screen_height;        
            
            //
            // NOTE: Windows Message loop
            //
            win32_process_pending_messages(&global_win32_state, new_input, window, &game_settings);
            
            //
            // NOTE: Grabbing focus
            //
            {
                // NOTE(HACK): We ignore the first window position dirty flag because we always get this message
                // at startup from windows, but only care about it when game is running and you move the window using
                // the Windows+Arrow keys and want to constrain cursor to new window rect
                if(global_win32_state.window_position_dirty && !global_win32_state.recognise_window_position_dirty_hit)
                {
                    global_win32_state.recognise_window_position_dirty_hit = true;
                    global_win32_state.window_position_dirty = false;
                }
                
                if(global_win32_state.window_position_dirty || global_win32_state.window_made_active)
                {
                    win32_constrain_cursor_inside_window(window);
                    global_win32_state.window_made_active = false;
                    global_win32_state.window_position_dirty = false;
                    global_win32_state.show_cursor = false;
                }
                if(!global_win32_state.show_cursor && global_win32_state.manually_constraining_cursor_in_window)
                {
                    win32_center_cursor_inside_window(window);
                }
                
            }
            
#if TWOSOME_INTERNAL
            if(global_win32_state.replay.recording)
            {
                win32_record_input(&global_win32_state, new_input);
            }
            if(global_win32_state.replay.playing_back)
            {
                win32_playback_game_input(&global_win32_state, new_input);
            }
#endif
            
            //
            // NOTE: Update Game
            //
            {
                Game_Update_And_Render_Result update_and_render_result = game.update_and_render(&game_memory, new_input, &render_commands);
                
                if(update_and_render_result.quit_game)
                {
                    global_running = false;
                }
                if(update_and_render_result.game_restarted)
                {
                    // NOTE: If the game restarted, the game settings may have be updated, and we need to make
                    // sure we've got the up to date settings for when game inits
                    game_settings = load_game_settings_from_file(&platform_api, &global_win32_state.temp_arena);
                }
                if(update_and_render_result.toggle_fullscreen)
                {
                    game_settings.fullscreen = win32_set_fullscreen_mode(window, !game_settings.fullscreen);
                }
                if(update_and_render_result.game_assets_load_result == game_assets_load_result_failed)
                {
                    win32_fail_startup_and_show_notification("Failed to load asset file.");
                    global_running = false;
                }
            }
            
            DEBUG_FRAME_TIMESTAMP("Update Game Time (ms)", last_counter, win32_get_wall_clock());
            
            //
            // NOTE: Update Sound
            //
            {            
                if((global_win32_state.tick_count % sound_tick_interval) == 0)
                {
                    DWORD play_cursor;
                    DWORD write_cursor;
                    b32 update_sound_result = true;
                    
                    if(global_win32_state.directsound_enabled && SUCCEEDED(global_secondary_buffer->GetCurrentPosition(&play_cursor, &write_cursor)))
                    {
                        b32 was_sound_valid = sound_is_valid;
                        if(!sound_is_valid)
                        {
                            sound_output.running_cursor = write_cursor;
                            sound_is_valid = true;
                        }
                        
                        DWORD byte_to_lock = sound_output.running_cursor;
                        assert(byte_to_lock < sound_output.secondary_buffer_size);
                        
                        // NOTE: Detect if weren't able to keep up with write cursor, if so increase the latency
                        // i.e. we write more in per sound tick
                        if(byte_to_lock >= play_cursor && byte_to_lock < write_cursor)
                        {
                            sound_output.latency_ticks += 1;
                            sound_output.latency_ticks = clamp(sound_output.latency_ticks, min_sound_latency_ticks, max_sound_latency_ticks);
                            log_warning("Increased audio latency before get_sound_samples.");
                        }
                        
                        DWORD latency_bytes = (sound_output.expected_sound_bytes_per_tick * sound_output.latency_ticks);
                        DWORD target_cursor = (write_cursor + sound_output.expected_sound_bytes_per_tick + latency_bytes);
                        target_cursor = (target_cursor % sound_output.secondary_buffer_size);                    
                        
                        DWORD bytes_to_write;
                        // NOTE: Deal with the fact that it's a rolling buffer
                        if(byte_to_lock > target_cursor)
                        {
                            bytes_to_write = (sound_output.secondary_buffer_size - byte_to_lock);
                            bytes_to_write += target_cursor;
                        }
                        else
                        {
                            bytes_to_write = target_cursor - byte_to_lock;
                        }
                        
                        if(bytes_to_write >= sound_output.secondary_buffer_size/2)
                        {
                            // NOTE: Somethings gone a bit wrong here because bytes_to_write is,
                            // too massive so we just skip this frame for outputting audio,
                            // and hope everything will sort itself out...
                            bytes_to_write = 0;
                            log_warning("Bytes to write for audio is too massive: skipping this frame for audio.");
                        }
                        
                        assert( bytes_to_write < sound_output.secondary_buffer_size );
                        // NOTE: Assert that we don't write into the area being played
                        assert( target_cursor <= play_cursor || target_cursor >= write_cursor );                    
                        
                        Game_Sound_Output_Buffer sound_buffer = win32_get_game_sound_output_buffer(bytes_to_write, &sound_output, samples);
                        if(sound_buffer.sample_count > 0)
                        {
                            // NOTE: Bytes to write might increase a little in order to land on 4-byte boundary,
                            // so recalculate it
                            bytes_to_write = sound_buffer.sample_count * sound_output.bytes_per_sample;
                            
                            game.get_sound_samples(&game_memory, &sound_buffer);
                            
                            // NOTE: Increase audio latency if necessary
                            {
                                DWORD play_cursor_after_get_sound_samples;
                                DWORD write_cursor_after_get_sound_samples;
                                if(SUCCEEDED(global_secondary_buffer->GetCurrentPosition(&play_cursor_after_get_sound_samples, &write_cursor_after_get_sound_samples)))
                                {
                                    
                                    if(byte_to_lock >= play_cursor_after_get_sound_samples && byte_to_lock < write_cursor_after_get_sound_samples)
                                    {
                                        if(sound_output.latency_ticks == max_sound_latency_ticks)
                                        {
                                            log_warning("Attempted to increase latency after already hit max.");
                                        }
                                        
                                        sound_output.latency_ticks += 1;                            
                                        sound_output.latency_ticks = clamp(sound_output.latency_ticks, min_sound_latency_ticks, max_sound_latency_ticks);
                                        log_warning("Increased audio latency after get_sound_samples.");
                                    }   
                                }
                                else
                                {
                                    update_sound_result = false;
                                }
                            }
                            
                            if(!win32_fill_dsound_buffer(&sound_buffer, &sound_output, byte_to_lock, bytes_to_write))
                            {
                                update_sound_result = false;
                            }   
                        }
                        else
                        {
                            //TODO: invalid_code_path;
                        }
                        
#if TWOSOME_INTERNAL
                        {
                            Win32_DirectSound_Debug_Marker marker;
                            marker.output_play_cursor = play_cursor;
                            marker.output_write_cursor = write_cursor;
                            marker.output_location = byte_to_lock;
                            marker.output_byte_count = bytes_to_write;
                            
                            assert(directsound_debug_marker_index < array_count(directsound_debug_markers));
                            directsound_debug_markers[directsound_debug_marker_index++] = marker;
                            
                            directsound_debug_marker_index %= array_count(directsound_debug_markers);
                        }
#endif
                    }
                    else
                    {
                        // NOTE: If don't have directsound just ask game for the samples
                        // and do nothing with them (so the sounds progress in the game)
                        Game_Sound_Output_Buffer sound_buffer = win32_get_game_sound_output_buffer(sound_output.expected_sound_bytes_per_tick, &sound_output, samples);                        
                        game.get_sound_samples(&game_memory, &sound_buffer);
                        
                        update_sound_result = false;
                    }
                    
                    // NOTE: If directsound was enabled before, there me be some data in sound buffer
                    // which we should clear out to not get "repeating" sound
                    if(!update_sound_result && global_win32_state.directsound_enabled)
                    {
                        win32_clear_dsound_buffer(&sound_output);
                        global_win32_state.directsound_enabled = false;
                    }
                }
            }
            
            DEBUG_FRAME_TIMESTAMP("Update Sound Time (ms)", last_counter, win32_get_wall_clock());
            
            // NOTE: Execute render commands
            {
                SAMPLE_BUFFER_USAGE(render_commands.push_buffer_base, ((render_commands.push_buffer_base + render_commands.push_buffer_size) - render_commands.push_buffer_at) );
                
                Temporary_Memory temp_mem = begin_temporary_memory(&global_win32_state.temp_arena);
                Game_Render_Prep prep = sort_render_entries(&render_commands, &global_win32_state.temp_arena);
                
                opengl_execute_render_commands(&render_commands, &prep, &global_win32_state.temp_arena);
                
                end_temporary_memory(temp_mem);
            }
            
            DEBUG_FRAME_TIMESTAMP("Execute Rendering Time (ms)", last_counter, win32_get_wall_clock());
            
            // NOTE: DirectSound Debug Visual
#if 1
#if TWOSOME_INTERNAL
            {
                DEBUG_draw_directsound_visuals(&render_commands.view, directsound_debug_markers, array_count(directsound_debug_markers), &sound_output);
            }
#endif
#endif
            
            check_arena(&global_win32_state.temp_arena);
            
            ++global_win32_state.tick_count;
            
            // NOTE: Do buffer flip
            HDC device_context = GetDC(window);
            BOOL swap_buffers_result = SwapBuffers(device_context);
            assert(swap_buffers_result);
            ReleaseDC(window, device_context);
            
            // NOTE: If no vsync then we have to regulate the framerate ourselves
            if(!global_win32_state.vsync_enabled)
            {
                LARGE_INTEGER work_counter = win32_get_wall_clock();
                f32 work_seconds_elapsed = win32_get_seconds_elapsed(last_counter, work_counter);
                
                f32 seconds_elapsed_for_frame = work_seconds_elapsed;
                if(seconds_elapsed_for_frame < target_seconds_per_frame)
                {
                    if(sleep_is_granular)
                    {
                        DWORD sleep_ms = (DWORD)(1000.0f * (target_seconds_per_frame - seconds_elapsed_for_frame));
                        if(sleep_ms > 1)
                        {
                            //Sleep((sleep_ms - 1));
                            Sleep(1);
                        }
                    }
                    
                    seconds_elapsed_for_frame = win32_get_seconds_elapsed(last_counter, win32_get_wall_clock());
                    
                    while(seconds_elapsed_for_frame < target_seconds_per_frame)
                    {
                        seconds_elapsed_for_frame = win32_get_seconds_elapsed(last_counter, win32_get_wall_clock());
                    }
                }
                else
                {
                    // NOTE: Missed framerate
                }   
            }
            
            flip_wall_clock = win32_get_wall_clock();
            
            Game_Input *temp = new_input;
            new_input = old_input;
            old_input = temp;
            
            LARGE_INTEGER end_counter = win32_get_wall_clock();
            
            DEBUG_FRAME_TIMESTAMP("Frame Time (ms)", last_counter, end_counter);
            
            last_counter = end_counter;
            
#if TWOSOME_INTERNAL
            if(game.DEBUG_frame_end)
            {
                game.DEBUG_frame_end(&game_memory);
            }
#endif
        }
    }        
    
    return global_win32_state.startup_failed;
}
