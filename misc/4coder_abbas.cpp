#include "4coder_default_include.cpp"

CUSTOM_COMMAND_SIG(abbas_interactive_switch_buffer_in_other)
{
    exec_command(app, change_active_panel);
    exec_command(app, interactive_switch_buffer);
}


inline bool
IsH(String extension)
{
    bool Result = (match(extension, make_lit_string("h")) ||
                   match(extension, make_lit_string("hpp")) ||
                   match(extension, make_lit_string("hin")));
    
    return(Result);
}

inline bool
IsCPP(String extension)
{
    bool Result = (match(extension, make_lit_string("c")) ||
                   match(extension, make_lit_string("cpp")) ||
                   match(extension, make_lit_string("cin")));
    
    return(Result);
}


inline bool
IsINL(String extension)
{
    bool Result = (match(extension, make_lit_string("inl")) != 0);
    
    return(Result);
}

struct switch_to_result
{
    bool Switched;
    bool Loaded;
    View_Summary view;
    Buffer_Summary buffer;
};


inline void
SanitizeSlashes(String Value)
{
    for(int At = 0;
        At < Value.size;
        ++At)
    {
        if(Value.str[At] == '\\')
        {
            Value.str[At] = '/';
        }
    }
}


inline switch_to_result
SwitchToOrLoadFile(struct Application_Links *app, String FileName, bool CreateIfNotFound = false)
{
    switch_to_result Result = {};
    
    SanitizeSlashes(FileName);
    
    unsigned int access = AccessAll;
    View_Summary view = get_active_view(app, access);
    Buffer_Summary buffer = get_buffer_by_name(app, FileName.str, FileName.size, access);
    
    Result.view = view;
    Result.buffer = buffer;
    
    if(buffer.exists)
    {
        view_set_buffer(app, &view, buffer.buffer_id, 0);
        Result.Switched = true;
    }
    else
    {
        if(file_exists(app, FileName.str, FileName.size) || CreateIfNotFound)
        {
            // NOTE(allen): This opens the file and puts it in &view
            // This returns false if the open fails.
            view_open_file(app, &view, expand_str(FileName), false);
            
            Result.buffer = get_buffer_by_name(app, FileName.str, FileName.size, access);
            
            Result.Loaded = true;
            Result.Switched = true;
        }
    }
    
    return(Result);
}

CUSTOM_COMMAND_SIG(casey_find_corresponding_file)
{
    unsigned int access = AccessProtected;
    View_Summary view = get_active_view(app, access);
    Buffer_Summary buffer = get_buffer(app, view.buffer_id, access);
    
    String extension = file_extension(make_string(buffer.file_name, buffer.file_name_len));
    if (extension.str)
    {
        char *HExtensions[] =
        {
            "hpp",
            "hin",
            "h",
        };
        
        char *CExtensions[] =
        {
            "c",
            "cin",
            "cpp",
        };
        
        int ExtensionCount = 0;
        char **Extensions = 0;
        if(IsH(extension))
        {
            ExtensionCount = ArrayCount(CExtensions);
            Extensions = CExtensions;
        }
        else if(IsCPP(extension) || IsINL(extension))
        {
            ExtensionCount = ArrayCount(HExtensions);
            Extensions = HExtensions;
        }
        
        int MaxExtensionLength = 3;
        int Space = (int)(buffer.file_name_len + MaxExtensionLength);
        String FileNameStem = make_string(buffer.file_name, (int)(extension.str - buffer.file_name), 0);
        String TestFileName = make_string(app->memory, 0, Space);
        for(int ExtensionIndex = 0;
            ExtensionCount;
            ++ExtensionIndex)
        {
            TestFileName.size = 0;
            append(&TestFileName, FileNameStem);
            append(&TestFileName, Extensions[ExtensionIndex]);
            
            if(SwitchToOrLoadFile(app, TestFileName, ((ExtensionIndex + 1) == ExtensionCount)).Switched)
            {
                break;
            }
        }
    }
}

CUSTOM_COMMAND_SIG(casey_find_corresponding_file_other_window)
{
    unsigned int access = AccessProtected;
    View_Summary old_view = get_active_view(app, access);
    Buffer_Summary buffer = get_buffer(app, old_view.buffer_id, access);
    
    exec_command(app, change_active_panel);
    View_Summary new_view = get_active_view(app, AccessAll);
    view_set_buffer(app, &new_view, buffer.buffer_id, 0);
    
    exec_command(app, casey_find_corresponding_file);
}

inline void windows_bindings(Bind_Helper *context)
{
    begin_map(context, mapid_global);
    {
        bind(context, 'p', MDFR_CTRL, open_panel_vsplit);
        bind(context, '_', MDFR_CTRL, open_panel_hsplit);
        bind(context, 'P', MDFR_CTRL, close_panel);
        bind(context, 'w', MDFR_ALT, change_active_panel);
        //bind(context, '<', MDFR_CTRL, change_active_panel_backwards);
        
        // TODO: bind(context, 'n', MDFR_CTRL, interactive_new);
        //bind(context, 'o', MDFR_CTRL, interactive_open_or_new);
        
        bind(context, 'f', MDFR_ALT, interactive_open_or_new);
        bind(context, 'F', MDFR_ALT, open_in_other);
        
        //bind(context, 'o', MDFR_ALT, open_in_other);
        //bind(context, 'F', MDFR_ALT, open_in_other);
        
        //bind(context, 'k', MDFR_CTRL, interactive_kill_buffer);
        //bind(context, 'k', MDFR_CTRL, interactive_kill_buffer);
        bind(context, 'k', MDFR_ALT, interactive_kill_buffer);
        
        //bind(context, 'i', MDFR_CTRL, interactive_switch_buffer);
        bind(context, 'b', MDFR_ALT, interactive_switch_buffer);
        bind(context, 'B', MDFR_ALT, abbas_interactive_switch_buffer_in_other);
        //bind(context, 'h', MDFR_CTRL, project_go_to_root_directory);
        bind(context, 'h', MDFR_CTRL, project_go_to_root_directory);
        
        bind(context, 'c', MDFR_ALT, casey_find_corresponding_file);
        bind(context, 'C', MDFR_ALT, casey_find_corresponding_file_other_window);
        
        //bind(context, 'S', MDFR_CTRL, save_all_dirty_buffers);
        //bind(context, 'S', MDFR_ALT, save);
        
        bind(context, 'c', MDFR_ALT | MDFR_CTRL, open_color_tweaker);
        bind(context, '.', MDFR_ALT, change_to_build_panel);
        bind(context, ',', MDFR_ALT, close_build_panel);
        bind(context, 'n', MDFR_ALT, goto_next_jump_no_skips_sticky);
        bind(context, 'N', MDFR_ALT, goto_prev_jump_no_skips_sticky);
        bind(context, 'M', MDFR_ALT, goto_first_jump_sticky);
        bind(context, 'm', MDFR_ALT, build_in_build_panel);
        bind(context, 'z', MDFR_ALT, execute_any_cli);
        bind(context, 'Z', MDFR_ALT, execute_previous_cli);
        bind(context, 'x', MDFR_ALT, execute_arbitrary_command);
        
        bind(context, 'W', MDFR_ALT | MDFR_CTRL, show_scrollbar);
        bind(context, 'w', MDFR_ALT | MDFR_CTRL, hide_scrollbar);
        
        bind(context, 'b', MDFR_CTRL, toggle_filebar);
        bind(context, '@', MDFR_ALT, toggle_mouse);
        bind(context, 'E', MDFR_ALT, exit_4coder);
        bind(context, '+', MDFR_CTRL, increase_face_size);
        bind(context, '-', MDFR_CTRL, decrease_face_size);
        bind(context, key_f1, MDFR_NONE, project_fkey_command);
        bind(context, key_f2, MDFR_NONE, project_fkey_command);
        bind(context, key_f3, MDFR_NONE, project_fkey_command);
        bind(context, key_f4, MDFR_NONE, project_fkey_command);
        bind(context, key_f5, MDFR_NONE, project_fkey_command);
        bind(context, key_f6, MDFR_NONE, project_fkey_command);
        bind(context, key_f7, MDFR_NONE, project_fkey_command);
        bind(context, key_f8, MDFR_NONE, project_fkey_command);
        bind(context, key_f9, MDFR_NONE, project_fkey_command);
        bind(context, key_f10, MDFR_NONE, project_fkey_command);
        bind(context, key_f11, MDFR_NONE, project_fkey_command);
        bind(context, key_f12, MDFR_NONE, project_fkey_command);
        bind(context, key_f13, MDFR_NONE, project_fkey_command);
        bind(context, key_f14, MDFR_NONE, project_fkey_command);
        bind(context, key_f15, MDFR_NONE, project_fkey_command);
        bind(context, key_f16, MDFR_NONE, project_fkey_command);
    }
    end_map(context);
    
    begin_map(context, mapid_file);
    {
        bind_vanilla_keys(context, write_character);
        bind(context, key_mouse_left, MDFR_NONE, click_set_cursor);
        bind(context, key_mouse_left_release, MDFR_NONE, click_set_mark);
        bind(context, key_mouse_right, MDFR_NONE, click_set_mark);
        bind(context, key_left, MDFR_NONE, move_left);
        bind(context, key_right, MDFR_NONE, move_right);
        bind(context, key_del, MDFR_NONE, delete_char);
        bind(context, key_del, MDFR_SHIFT, delete_char);
        bind(context, key_back, MDFR_NONE, backspace_char);
        bind(context, key_back, MDFR_SHIFT, backspace_char);
        
        bind(context, key_up, MDFR_NONE, move_up);
        bind(context, key_down, MDFR_NONE, move_down);
        
        //
        bind(context, 'p', MDFR_CTRL, move_up);
        bind(context, 'n', MDFR_CTRL, move_down);
        
        bind(context, key_end, MDFR_NONE, seek_end_of_line);
        bind(context, key_home, MDFR_NONE, seek_beginning_of_line);
        //
        bind(context, 'e', MDFR_CTRL, seek_end_of_line);
        bind(context, 'a', MDFR_CTRL, seek_beginning_of_line);
        
        bind(context, key_page_up, MDFR_CTRL, goto_beginning_of_file);
        bind(context, key_page_down, MDFR_CTRL, goto_end_of_file);
        bind(context, key_page_up, MDFR_NONE, page_up);
        bind(context, key_page_down, MDFR_NONE, page_down);
        bind(context, key_right, MDFR_CTRL, seek_whitespace_right);
        bind(context, key_left, MDFR_CTRL, seek_whitespace_left);
        bind(context, key_up, MDFR_CTRL, seek_whitespace_up_end_line);
        bind(context, key_down, MDFR_CTRL, seek_whitespace_down_end_line);
        bind(context, key_up, MDFR_ALT, move_line_up);
        bind(context, key_down, MDFR_ALT, move_line_down);
        bind(context, key_back, MDFR_CTRL, backspace_word);
        bind(context, key_del, MDFR_CTRL, delete_word);
        bind(context, key_back, MDFR_ALT, snipe_token_or_word);
        bind(context, key_del, MDFR_ALT, snipe_token_or_word_right);
        
        bind(context, ' ', MDFR_CTRL, set_mark);
        
        //bind(context, 'a', MDFR_CTRL, replace_in_range);
        //bind(context, 'l', MDFR_ALT, replace_in_range);
        
        bind(context, 'o', MDFR_ALT, replace_in_range);
        
        bind(context, 'c', MDFR_CTRL, copy);
        bind(context, 'd', MDFR_CTRL, delete_range);
        bind(context, 'D', MDFR_CTRL, delete_line);
        
        //bind(context, 'e', MDFR_CTRL, center_view);
        bind(context, 'l', MDFR_CTRL, center_view);
        
        bind(context, 'E', MDFR_CTRL, left_adjust_view);
        
        bind(context, 's', MDFR_CTRL, search);
        //bind(context, 'f', MDFR_CTRL, search);
        //bind(context, 'F', MDFR_CTRL, list_all_locations);
        //bind(context, 'F', MDFR_ALT, list_all_substring_locations_case_insensitive);
        bind(context, 'S', MDFR_CTRL, list_all_substring_locations_case_insensitive);
        
        bind(context, 'g', MDFR_CTRL, goto_line);
        bind(context, 'G', MDFR_CTRL, list_all_locations_of_selection);
        
        //bind(context, 'K', MDFR_CTRL, kill_buffer);
        bind(context, 'k', MDFR_ALT, kill_buffer);
        
        //bind(context, 'L', MDFR_CTRL, toggle_line_wrap);
        bind(context, 'W', MDFR_CTRL, toggle_line_wrap);
        bind(context, 'L', MDFR_CTRL, duplicate_line);
        bind(context, 'm', MDFR_CTRL, cursor_mark_swap);
        bind(context, 'O', MDFR_CTRL, reopen);
        //bind(context, 'q', MDFR_CTRL, query_replace);
        bind(context, 'o', MDFR_ALT, query_replace);
        
        bind(context, 'Q', MDFR_CTRL, query_replace_identifier);
        bind(context, 'q', MDFR_ALT, query_replace_selection);
        bind(context, 'r', MDFR_CTRL, reverse_search);
        
        //bind(context, 's', MDFR_CTRL, save);
        bind(context, 's', MDFR_ALT, save);
        
        //bind(context, 's', MDFR_ALT, save_to_query);
        bind(context, 'S', MDFR_ALT, save_to_query);
        
        bind(context, 't', MDFR_CTRL, search_identifier);
        bind(context, 'T', MDFR_CTRL, list_all_locations_of_identifier);
        bind(context, 'v', MDFR_CTRL, paste_and_indent);
        bind(context, 'v', MDFR_ALT, toggle_virtual_whitespace);
        bind(context, 'V', MDFR_CTRL, paste_next_and_indent);
        bind(context, 'x', MDFR_CTRL, cut);
        bind(context, 'y', MDFR_CTRL, redo);
        bind(context, 'z', MDFR_CTRL, undo);
        bind(context, '1', MDFR_CTRL, view_buffer_other_panel);
        bind(context, '2', MDFR_CTRL, swap_buffers_between_panels);
        bind(context, '?', MDFR_CTRL, toggle_show_whitespace);
        bind(context, '~', MDFR_CTRL, clean_all_lines);
        bind(context, '\n', MDFR_NONE, newline_or_goto_position_sticky);
        bind(context, '\n', MDFR_SHIFT, newline_or_goto_position_same_panel_sticky);
        bind(context, ' ', MDFR_SHIFT, write_character);
    }
    end_map(context);
    
    begin_map(context, default_code_map);
    {
        bind(context, 'I', MDFR_CTRL, scope_absorb_down);
        bind(context, 'j', MDFR_ALT, list_all_functions_current_buffer);
        
        inherit_map(context, mapid_file);
        bind(context, key_right, MDFR_CTRL, seek_alphanumeric_or_camel_right);
        bind(context, key_left, MDFR_CTRL, seek_alphanumeric_or_camel_left);
        bind(context, '\n', MDFR_NONE, write_and_auto_tab);
        bind(context, '\n', MDFR_SHIFT, write_and_auto_tab);
        bind(context, '}', MDFR_NONE, write_and_auto_tab);
        bind(context, ')', MDFR_NONE, write_and_auto_tab);
        bind(context, ']', MDFR_NONE, write_and_auto_tab);
        bind(context, ';', MDFR_NONE, write_and_auto_tab);
        bind(context, '#', MDFR_NONE, write_and_auto_tab);
        bind(context, '\t', MDFR_NONE, word_complete);
        bind(context, '\t', MDFR_CTRL, auto_tab_range);
        bind(context, '\t', MDFR_SHIFT, auto_tab_line_at_cursor);
        bind(context, 'h', MDFR_ALT, write_hack);
        bind(context, 'r', MDFR_ALT, write_block);
        bind(context, 't', MDFR_ALT, write_todo);
        bind(context, 'y', MDFR_ALT, write_note);
        bind(context, 'D', MDFR_ALT, list_all_locations_of_type_definition);
        bind(context, 'T', MDFR_ALT, list_all_locations_of_type_definition_of_identifier);
        bind(context, '[', MDFR_CTRL, open_long_braces);
        bind(context, '{', MDFR_CTRL, open_long_braces_semicolon);
        bind(context, '}', MDFR_CTRL, open_long_braces_break);
        bind(context, '[', MDFR_ALT, highlight_surrounding_scope);
        bind(context, ']', MDFR_ALT, highlight_prev_scope_absolute);
        bind(context, '\'', MDFR_ALT, highlight_next_scope_absolute);
        bind(context, '/', MDFR_ALT, place_in_scope);
        bind(context, '-', MDFR_ALT, delete_current_scope);
        //bind(context, 'j', MDFR_ALT, scope_absorb_down);
        
        bind(context, 'i', MDFR_ALT, if0_off);
        bind(context, '1', MDFR_ALT, open_file_in_quotes);
        bind(context, '2', MDFR_ALT, open_matching_file_cpp);
        bind(context, '0', MDFR_CTRL, write_zero_struct);
        //bind(context, 'I', MDFR_CTRL, list_all_functions_current_buffer);
        bind(context, 'j', MDFR_ALT, list_all_functions_current_buffer);
    }
    
    end_map(context);
}

inline void mac_bindings(Bind_Helper *context)
{
    begin_map(context, mapid_global);
    {
        bind(context, 'p', MDFR_CMND, open_panel_vsplit);
        bind(context, '_', MDFR_CMND, open_panel_hsplit);
        bind(context, 'P', MDFR_CMND, close_panel);
        bind(context, 'w', MDFR_CMND, change_active_panel);
        //bind(context, '<', MDFR_CMND, change_active_panel_backwards);
        // TODO: bind(context, 'n', MDFR_CMND, interactive_new);
        bind(context, 'f', MDFR_CMND, interactive_open_or_new);        
        bind(context, 'F', MDFR_CMND, open_in_other);
        
        bind(context, 'k', MDFR_CMND, interactive_kill_buffer);
        bind(context, 'b', MDFR_CMND, interactive_switch_buffer);
        bind(context, 'B', MDFR_CMND, abbas_interactive_switch_buffer_in_other);
        
        bind(context, 'h', MDFR_CMND, project_go_to_root_directory);
        //TODO: bind(context, 'S', MDFR_CMND, save_all_dirty_buffers);
        
        bind(context, 'c', MDFR_CMND, casey_find_corresponding_file);
        bind(context, 'C', MDFR_CMND, casey_find_corresponding_file_other_window);
        
        bind(context, 'c', MDFR_CTRL, open_color_tweaker);
        bind(context, '.', MDFR_CTRL, change_to_build_panel);
        bind(context, ',', MDFR_CTRL, close_build_panel);
        bind(context, 'n', MDFR_CTRL, goto_next_jump_sticky);
        bind(context, 'N', MDFR_CTRL, goto_prev_jump_sticky);
        bind(context, 'M', MDFR_CTRL, goto_first_jump_sticky);
        bind(context, 'm', MDFR_CTRL, build_in_build_panel);
        bind(context, 'z', MDFR_CTRL, execute_any_cli);
        bind(context, 'Z', MDFR_CTRL, execute_previous_cli);
        bind(context, 'x', MDFR_CTRL, execute_arbitrary_command);
        
        bind(context, 'W', MDFR_CTRL, show_scrollbar);
        bind(context, 'w', MDFR_CTRL, hide_scrollbar);
        
        bind(context, 'b', MDFR_CTRL, toggle_filebar);
        bind(context, '@', MDFR_CTRL, toggle_mouse);
        bind(context, 'E', MDFR_CTRL, exit_4coder);
        bind(context, '+', MDFR_CTRL, increase_face_size);
        bind(context, '-', MDFR_CTRL, decrease_face_size);
        bind(context, key_f1, MDFR_NONE, project_fkey_command);
        bind(context, key_f2, MDFR_NONE, project_fkey_command);
        bind(context, key_f3, MDFR_NONE, project_fkey_command);
        bind(context, key_f4, MDFR_NONE, project_fkey_command);
        bind(context, key_f5, MDFR_NONE, project_fkey_command);
        bind(context, key_f6, MDFR_NONE, project_fkey_command);
        bind(context, key_f7, MDFR_NONE, project_fkey_command);
        bind(context, key_f8, MDFR_NONE, project_fkey_command);
        bind(context, key_f9, MDFR_NONE, project_fkey_command);
        bind(context, key_f10, MDFR_NONE, project_fkey_command);
        bind(context, key_f11, MDFR_NONE, project_fkey_command);
        bind(context, key_f12, MDFR_NONE, project_fkey_command);
        bind(context, key_f13, MDFR_NONE, project_fkey_command);
        bind(context, key_f14, MDFR_NONE, project_fkey_command);
        bind(context, key_f15, MDFR_NONE, project_fkey_command);
        bind(context, key_f16, MDFR_NONE, project_fkey_command);
    }
    end_map(context);
    
    begin_map(context, mapid_file);
    {
        bind_vanilla_keys(context, write_character);
        bind_vanilla_keys(context, MDFR_ALT, write_character);
        bind(context, key_mouse_left, MDFR_NONE, click_set_cursor);
        bind(context, key_mouse_left_release, MDFR_NONE, click_set_mark);
        bind(context, key_mouse_right, MDFR_NONE, click_set_mark);
        bind(context, key_left, MDFR_NONE, move_left);
        bind(context, key_right, MDFR_NONE, move_right);
        bind(context, key_del, MDFR_NONE, delete_char);
        bind(context, key_del, MDFR_SHIFT, delete_char);
        bind(context, key_back, MDFR_NONE, backspace_char);
        bind(context, key_back, MDFR_SHIFT, backspace_char);
        
        bind(context, key_up, MDFR_NONE, move_up);
        bind(context, key_down, MDFR_NONE, move_down);
        
        bind(context, 'p', MDFR_CTRL, move_up);
        bind(context, 'n', MDFR_CTRL, move_down);        
        
        bind(context, key_end, MDFR_NONE, seek_end_of_line);
        bind(context, key_home, MDFR_NONE, seek_beginning_of_line);
        bind(context, 'e', MDFR_CTRL, seek_end_of_line);
        bind(context, 'a', MDFR_CTRL, seek_beginning_of_line);
        
        bind(context, ' ', MDFR_CTRL, set_mark);
        
        bind(context, key_page_up, MDFR_CTRL, goto_beginning_of_file);
        bind(context, key_page_down, MDFR_CTRL, goto_end_of_file);
        bind(context, key_page_up, MDFR_NONE, page_up);
        bind(context, key_page_down, MDFR_NONE, page_down);
        bind(context, key_right, MDFR_CMND, seek_whitespace_right);
        bind(context, key_left, MDFR_CMND, seek_whitespace_left);
        bind(context, key_up, MDFR_CMND, seek_whitespace_up_end_line);
        bind(context, key_down, MDFR_CMND, seek_whitespace_down_end_line);
        bind(context, key_back, MDFR_CMND, backspace_word);
        bind(context, key_del, MDFR_CMND, delete_word);
        bind(context, key_back, MDFR_CTRL, snipe_token_or_word);
        bind(context, key_del, MDFR_CTRL, snipe_token_or_word_right);
        bind(context, '/', MDFR_CMND, set_mark);
		
        bind(context, 'o', MDFR_CMND, replace_in_range);
		
        bind(context, 'c', MDFR_CMND, copy);
        bind(context, 'd', MDFR_CMND, delete_range);
        bind(context, 'D', MDFR_CMND, delete_line);
        bind(context, 'l', MDFR_CMND, center_view);
        bind(context, 'E', MDFR_CMND, left_adjust_view);
        
        bind(context, 's', MDFR_CTRL, search);
        //bind(context, 'f', MDFR_CMND, search);
        //bind(context, 'F', MDFR_CMND, list_all_locations);
        //bind(context, 'F', MDFR_CTRL, list_all_substring_locations_case_insensitive);
        bind(context, 'S', MDFR_CTRL, list_all_substring_locations_case_insensitive);
        
        bind(context, 'g', MDFR_CMND, goto_line);
        bind(context, 'G', MDFR_CMND, list_all_locations_of_selection);
        bind(context, 'K', MDFR_CMND, kill_buffer);
        bind(context, 'l', MDFR_CMND, toggle_line_wrap);
        bind(context, 'L', MDFR_CMND, duplicate_line);
        bind(context, 'm', MDFR_CMND, cursor_mark_swap);
        bind(context, 'O', MDFR_CMND, reopen);
        bind(context, 'q', MDFR_CMND, query_replace);
        bind(context, 'Q', MDFR_CMND, query_replace_identifier);
        bind(context, 'r', MDFR_CMND, reverse_search);
        //bind(context, 's', MDFR_CMND, save);
        bind(context, 's', MDFR_CMND, save);
        //bind(context, 's', MDFR_CTRL, save_to_query);
        bind(context, 'S', MDFR_CMND, save_to_query);
        
        
        bind(context, 't', MDFR_CMND, search_identifier);
        bind(context, 'T', MDFR_CMND, list_all_locations_of_identifier);
        bind(context, 'v', MDFR_CMND, paste_and_indent);
        bind(context, 'v', MDFR_CTRL, toggle_virtual_whitespace);
        bind(context, 'V', MDFR_CMND, paste_next_and_indent);
        bind(context, 'x', MDFR_CMND, cut);
        bind(context, 'y', MDFR_CMND, redo);
        bind(context, 'z', MDFR_CMND, undo);
        bind(context, '1', MDFR_CMND, view_buffer_other_panel);
        bind(context, '2', MDFR_CMND, swap_buffers_between_panels);
        bind(context, '?', MDFR_CMND, toggle_show_whitespace);
        bind(context, '~', MDFR_CMND, clean_all_lines);
        bind(context, '\n', MDFR_NONE, newline_or_goto_position_sticky);
        bind(context, '\n', MDFR_SHIFT, newline_or_goto_position_same_panel_sticky);
        bind(context, ' ', MDFR_SHIFT, write_character);
    }
    end_map(context);
    
    begin_map(context, default_code_map);
    {
        inherit_map(context, mapid_file);
        bind(context, key_right, MDFR_CMND, seek_alphanumeric_or_camel_right);
        bind(context, key_left, MDFR_CMND, seek_alphanumeric_or_camel_left);
        bind(context, '\n', MDFR_NONE, write_and_auto_tab);
        bind(context, '\n', MDFR_SHIFT, write_and_auto_tab);
        bind(context, '}', MDFR_NONE, write_and_auto_tab);
        bind(context, ')', MDFR_NONE, write_and_auto_tab);
        bind(context, ']', MDFR_NONE, write_and_auto_tab);
        bind(context, ';', MDFR_NONE, write_and_auto_tab);
        bind(context, '#', MDFR_NONE, write_and_auto_tab);
        bind(context, '\t', MDFR_NONE, word_complete);
        bind(context, '\t', MDFR_CMND, auto_tab_range);
        bind(context, '\t', MDFR_SHIFT, auto_tab_line_at_cursor);
        bind(context, 'h', MDFR_CTRL, write_hack);
        bind(context, 'r', MDFR_CTRL, write_block);
        bind(context, 't', MDFR_CTRL, write_todo);
        bind(context, 'y', MDFR_CTRL, write_note);
        bind(context, 'D', MDFR_CTRL, list_all_locations_of_type_definition);
        bind(context, 'T', MDFR_CTRL, list_all_locations_of_type_definition_of_identifier);
        bind(context, '[', MDFR_CMND, open_long_braces);
        bind(context, '{', MDFR_CMND, open_long_braces_semicolon);
        bind(context, '}', MDFR_CMND, open_long_braces_break);
        bind(context, '[', MDFR_CTRL, highlight_surrounding_scope);
        bind(context, ']', MDFR_CTRL, highlight_prev_scope_absolute);
        bind(context, '\'', MDFR_CTRL, highlight_next_scope_absolute);
        bind(context, '/', MDFR_CTRL, place_in_scope);
        bind(context, '-', MDFR_CTRL, delete_current_scope);
        bind(context, 'j', MDFR_CTRL, scope_absorb_down);
        bind(context, 'i', MDFR_CTRL, if0_off);
        bind(context, '1', MDFR_CTRL, open_file_in_quotes);
        bind(context, '2', MDFR_CTRL, open_matching_file_cpp);
        bind(context, '0', MDFR_CMND, write_zero_struct);
        bind(context, 'j', MDFR_CMND, list_all_functions_current_buffer);
    }
    end_map(context);
}

extern "C" int32_t get_bindings(void *data, int32_t size)
{
    Bind_Helper _context = begin_bind_helper(data, size);
    Bind_Helper *context = &_context;
    
    set_all_default_hooks(context);
    
    set_command_caller(context, default_command_caller);
    
#if defined(__APPLE__) && defined(__MACH__)
    mac_bindings(context);
#else
    windows_bindings(context);
#endif
    
    int32_t result = end_bind_helper(context);
    
    return result;
}
