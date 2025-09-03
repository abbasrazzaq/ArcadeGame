#ifndef TWOSOME_SHARED_H
#define TWOSOME_SHARED_H


#include "twosome_math.h"
#include "twosome_memory.h"
#include "twosome_random.h"


#define virtual_screen_width 800
#define virtual_screen_height 600

#define virtual_screen_center_x (virtual_screen_width / 2)
#define virtual_screen_center_y (virtual_screen_height / 2)

#define minimum_screen_width 640
#define minimum_screen_height 480

#define working_screen_height 768


internal f32 normalize_value(f32 value, f32 min, f32 max)
{
    f32 result = ( (value - min) / (max - min) );
    assert(result >= 0.0f && result <= 1.0f);
    result = clamp01(result);
    
    return result;
}

// NOTE: Turn -1 to 1 to 0 to 1
#define normalize_range_neg11(value) ( normalize_value((value), -1.0f, 1.0f) )

#define in_range(value, min, max) ( (value) >= (min) && (value) <= (max) )
#define in_range01(value) ( in_range((value), 0, 1) )


internal Vec3 move_from_initial_to_target(Vec3 initial, Vec3 target, Vec3 current, f32 dt, f32 time_to_target)
{
    Vec3 current_to_target = (target - current);
    Vec3 initial_to_target = (target - initial);
    Vec3 move = (initial_to_target) * (dt * (1.0f / time_to_target));

    if(magnitude(move) > magnitude(current_to_target))
    {
        move = current_to_target;
    }

    Vec3 result = current + move;
    return result;
}

internal int32 sign_int32(int32 v)
{
    int32 result = v > 0 ? 1 : -1;
    return result;
}

internal real32 sign_real32(real32 v)
{
    real32 result = v > 0 ? 1.0f : -1.0f;
    return result;
}

internal b32 string_contains_char(char c, char *str)
{
    b32 result = false;
    
    while(*str)
    {
        if(*str++ == c)
        {
            result = true;
            break;
        }
    }

    return result;
}

internal b32 strings_match(char *a, char *b)
{
    b32 result = true;
    
    while( 1 )
    {
        if(*a == *b)
        {
            if(!*a)
            {
                break;
            }
        }
        else
        {
            result = false;
            break;
        }

        ++a;
        ++b;
    }

    return result;
}

internal void cat_strings(size_t source_a_count, char *source_a, size_t source_b_count, char *source_b, size_t dest_count, char *dest)
{
    for(size_t index = 0; index < source_a_count; ++index)
    {
        *dest++ = *source_a++;
    }

    for(size_t index = 0; index < source_b_count; ++index)
    {
        *dest++ = *source_b++;
    }

    *dest++ = 0;
}

internal char number_to_char(u32 number)
{
    assert( in_range(number, 0, 9) );
    number = clamp(number, 0, 9);

    char result = '0' + (char )number;
    return result;
}

internal Memory_Index string_length(char *str)
{
    Memory_Index result = 0;
    while(*(str++))
    {
        ++result;
    }

    return result;
}

internal char *string_copy(char *src, char *dest)
{
    while(*src)
    {
        *dest = *src;
        src++;
        dest++;
    }

    *dest = '\0';

    return dest;
}

internal void copy_memory(void *dest, void *src, Memory_Index size)
{
    u8 *d = (u8 *)dest;
    u8 *s = (u8 *)src;
    while(size--)
    {
        *d++ = *s++;
    }
}

internal b32 is_newline(char c)
{
    b32 result = (c == '\n' || c == '\r');
    return result;
}

internal b32 is_whitespace(char c)
{
    b32 result = (c == ' ' || c == '\t');
    return result;
}

internal b32 is_char_number(char c)
{
    b32 result = (c >= '0' && c <= '9');
    return result;
}

internal s32 char_to_s32(char c)
{
    assert(is_char_number(c));
    s32 result = (c - '0');
    return result;
}

internal f32 round(f32 value)
{
    f32 result = (f32)((s32)(value + 0.5f));

    return result;
}

internal f32 round_up(f32 value)
{
    f32 decimal_value = (value - (f32)((s32)value));
    f32 result = decimal_value > 0.0f ? ((f32)(((s32)value + 1))) : value;

    return result;
}

internal char *s32_to_string(s32 dval, char *str)
{
    u32 digits[40];
    u32 digits_count = 0;

    s32 dval_absolute = abs(dval);
    
    while(dval_absolute > 0 && digits_count < array_count(digits))
    {
        digits[digits_count++] = (dval_absolute % 10);
        dval_absolute /= 10;
    }
    assert(digits_count < array_count(digits));

    if(digits_count > 0)
    {
        // NOTE: Negative sign
        if(dval < 0)
        {
            *str++ = '-';
        }
        
        for(s32 digit_index = digits_count - 1; digit_index > -1; --digit_index)
        {
            *str++ = (char)('0' + digits[digit_index]);
        }   
    }
    else
    {
        *str++ = '0';
    }

    *str = '\0';
    
    return str;
}

internal char *b32_to_string(b32 val, char *str)
{
    s32 one_or_zero = (val == false) ? 0 : 1;
    char *end_of_str = s32_to_string(one_or_zero, str);
    return end_of_str;
}

internal char *f32_to_string(f32 fval, char *str)
{
    f32 fval_absolute = fabs(fval);
    // NOTE: This function converts up to 2 decimals places (so we're gonna round to 2 d.p)
    fval_absolute += 0.005f;
        
    u32 digits[40];
    u32 digits_count = 0;

    // NOTE: Decimal Part
    {
        s32 dval = (s32)fval_absolute;
        while(dval > 0 && digits_count < array_count(digits))
        {
            digits[digits_count++] = (dval % 10);
            dval /= 10;
        }

        assert(digits_count < array_count(digits));

        // NOTE: Negative sign
        if(fval < 0.0f)
        {
            *str++ = '-';
        }

        if(digits_count > 0)
        {
            for(s32 digit_index = digits_count - 1; digit_index > -1; --digit_index)
            {
                *str++ = (char)('0' + digits[digit_index]);
            }   
        }
        else
        {
            *str++ = '0';
        }
    }

    // NOTE: Point!
    {
        *str++ = '.';
    }

    // NOTE: Fractional Part
    {
        s32 dec = (s32)(fval_absolute * 100) % 100;
        digits_count = 0;
        assert(array_count(digits) > 2);
        for(s32 decimal_places = 0; decimal_places < 2; ++decimal_places)
        {
            if(dec > 0)
            {
                digits[decimal_places] = (dec % 10);
                dec /= 10;
            }
            else
            {
                digits[decimal_places] = 0;
            }
        }

        for(s32 decimal_places = 1; decimal_places > -1; --decimal_places)
        {
            *str++ = (char)('0' + digits[decimal_places]);
                
        }   
    }

    *str = '\0';

    return str;
}

internal b32 parse_string_for_s32(char *str, s32 *out)
{
    b32 result = false;
    
    s32 parsed_s32 = 0;
    u32 digit_count = 0;
    s32 sign_coefficient = 1;
    
    while(*str)
    {
        char c = *str++;

        if(is_char_number(c))
        {
            s32 digit = char_to_s32(c);

            parsed_s32 *= 10;
            parsed_s32 += digit;

            ++digit_count;
        }
        else if(c == '-' && digit_count == 0)
        {
            sign_coefficient = -1;
        }
        else
        {
            break;
        }
    }

    // NOTE: If were able to get numbers from string then sucessfully parsed
    if(digit_count > 0)
    {
        *out = (parsed_s32 * sign_coefficient);
        result = true;
    }
    
    return result;
}

internal b32 parse_string_for_b32(char *str, b32 *out)
{
    b32 result = false;
    
    s32 value_s32;
    if(parse_string_for_s32(str, &value_s32))
    {
        result = true;
        
        if(value_s32 == 0)
        {
            *out = false;
        }
        else if(value_s32 == 1)
        {
            *out = true;
        }
        else
        {
            result = false;
        }
    }
    
    return result;
}

internal b32 parse_string_for_f32(char *str, f32 *out)
{
    b32 result = false;
    
    s32 large = 0;
    u32 large_length = 0;
    s32 small = 0;
    u32 small_length = 0;
    s32 sign_coefficient = 1;
    b32 float_terminated = false;
    
    // NOTE: Values before decimal point
    while(*str)
    {
        char c = *str++;

        if(c == '.')
        {
            break;
        }
        else
        {
            if(is_char_number(c))
            {
                s32 digit = char_to_s32(c);

                large *= 10;
                large += digit;

                ++large_length;
            }
            else if(c == '-' && large_length == 0)
            {
                sign_coefficient = -1;
            }
            else
            {
                float_terminated = true;
                break;
            }
        }
    }

    // NOTE: Values after decimal point
    if(!float_terminated)
    {
        while(*str)
        {
            char c = *str++;

            if(is_char_number(c))
            {
                s32 digit = char_to_s32(c);

                small *= 10;
                small += digit;

                ++small_length;
            }
            else
            {
                break;
            } 
        }
    }

    // NOTE: If were able to get numbers from string then successfuly parsed
    if(large_length > 0 || small_length > 0)
    {
        f32 large_f32 = (f32)large;
        f32 small_f32 = 0.0f;
        if(small_length > 0)
        {
            small_f32 = ((f32)small) / (pow(10.0f, (f32)small_length));
        }

        *out = ((large_f32 + small_f32) * sign_coefficient);

        result = true;
    }

    return result;
}

internal uint32 safe_truncate_uint64(uint64 value)
{
    assert(value <= 0xFFFFFFFF);
    uint32 result = (uint32)value;
    return result;
}

#if TWOSOME_SLOW

// NOTE: Function lets you format string without having a buffer of certain
// size upfront. Just uses up necessary amount from tran_arena, up to caller
// to wrap function in temporary memory
internal char *DEBUG_format_string(Memory_Arena *tran_arena, char *format, ...)
{
    va_list v1;
    va_start(v1, format);
    
    char *result = push_array(tran_arena, char, 0);

    char *out = result;
    *out = '\0';
    for(; *format; ++format)
    {
        if(*format == '%')
        {
            ++format;

            switch(*format)
            {
                case 's':
                {
                    char *string = va_arg(v1, char *);

                    // NOTE: Copy string in
                    while(*string)
                    {
                        *out++ = *string++;
                    }
                    
                } break;

                case 'd':
                {
                    int value = va_arg(v1, int);
                    sprintf(out, "%d", value);

                    // Move past what we just added
                    while(*out)
                    {
                        ++out;
                    }                    
                } break;

                case 'f':
                {
                    double value = va_arg(v1, double);
                    sprintf(out, "%f", value);

                    // Move past what we just added
                    while(*out)
                    {
                        ++out;
                    }
                } break;
                
                invalid_default_case;
            };
        }
        else
        {
            *out++ = *format;
        }
    }

    (*out++) = '\0';
    result = push_array(tran_arena, char, (out - result));
    
    return result;
}

internal void log_message(DEBUG_Log_State *log_state, char *type, char *src_file, char *function, s32 line, char *message, ...)
{
    if(log_state->initialized)
    {
        char buffer[1024];
        buffer[0] = '\0';
    
        char *src_filename = src_file;
        while(*src_file)
        {
            if(*src_file == '\\' || *src_file == '/')
            {
                src_filename = src_file + 1;
            }

            ++src_file;
        }
    
        Platform_Date_Time time = log_state->get_current_time();
        /*
          == Severity [hh:mm:ss dd/mm/yyyy]
          "message"
          function: src_file (line)      
          ==
        */
        sprintf(buffer, "== %s [%d:%d:%d %d/%d/%d]\n", type, time.hour, time.minute, time.second, time.day, time.month, time.year);

        // NOTE: Message
        {
            va_list args;
            va_start(args, message);
            vsprintf(buffer + string_length(buffer), message, args);
            va_end(args);
            sprintf(buffer + string_length(buffer), "\n");
        }
    
        sprintf(buffer + string_length(buffer), "%s: %s (%d)\n", function, src_filename, line);
        sprintf(buffer + string_length(buffer), "==\n");

        log_state->log_message(buffer);   
    }       
}
#endif

#include "twosome_game_settings.h"

#endif
