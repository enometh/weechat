/*
 * fset-buffer.c - buffer for Fast Set plugin
 *
 * Copyright (C) 2003-2017 Sébastien Helleu <flashcode@flashtux.org>
 *
 * This file is part of WeeChat, the extensible chat client.
 *
 * WeeChat is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * WeeChat is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "fset.h"
#include "fset-buffer.h"
#include "fset-config.h"
#include "fset-option.h"


struct t_gui_buffer *fset_buffer = NULL;
int fset_buffer_selected_line = 0;
struct t_hashtable *fset_buffer_hashtable_pointers = NULL;
struct t_hashtable *fset_buffer_hashtable_extra_vars = NULL;
char *fset_buffer_columns[] = { "name", "type", "default_value", "value",
                                NULL };
int fset_buffer_columns_default_size[] = { 64, 8, 16, 16 };


/*
 * Displays a line with an fset option.
 */

void
fset_buffer_display_line (int y, struct t_fset_option *option)
{
    char *line, str_format[32], str_value[1024];
    int i, selected_line, *ptr_length;

    selected_line = (y == fset_buffer_selected_line) ? 1 : 0;

    /* set pointers */
    weechat_hashtable_set (fset_buffer_hashtable_pointers, "fset_option", option);

    /* set column variables */
    for (i = 0; fset_buffer_columns[i]; i++)
    {
        ptr_length = (int *)weechat_hashtable_get (fset_option_max_length_field,
                                                   fset_buffer_columns[i]);
        snprintf (str_format, sizeof (str_format),
                  "%%-%ds",
                  (ptr_length) ? *ptr_length : fset_buffer_columns_default_size[i]);
        snprintf (str_value, sizeof (str_value),
                  str_format,
                  weechat_hdata_string (fset_hdata_fset_option,
                                        option,
                                        fset_buffer_columns[i]));
        weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                               fset_buffer_columns[i],
                               str_value);
    }

    /* set colors */
    snprintf (str_value, sizeof (str_value),
              "%s",
              weechat_color (weechat_config_string (fset_config_color_name[selected_line])));
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "color_name", str_value);
    snprintf (str_value, sizeof (str_value),
              "%s",
              weechat_color (weechat_config_string (fset_config_color_type[selected_line])));
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "color_type", str_value);
    snprintf (str_value, sizeof (str_value),
              "%s",
              weechat_color (weechat_config_string (fset_config_color_default_value[selected_line])));
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "color_default_value", str_value);
    snprintf (str_value, sizeof (str_value),
              "%s",
              weechat_color (weechat_config_string (fset_config_color_value[selected_line])));
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "color_value", str_value);

    /* build string for line */
    line = weechat_string_eval_expression (
        (selected_line) ? fset_config_eval_format_option_current : weechat_config_string (fset_config_format_option),
        fset_buffer_hashtable_pointers,
        fset_buffer_hashtable_extra_vars,
        NULL);

    if (line)
    {
        weechat_printf_y (fset_buffer, y, "%s", line);
        free (line);
    }
}

/*
 * Updates list of options in fset buffer.
 */

void
fset_buffer_refresh (int clear)
{
    char str_title[1024];
    int num_options, i;
    struct t_fset_option *ptr_option;

    if (!fset_buffer)
        return;

    num_options = weechat_arraylist_size (fset_options);

    if (clear)
    {
        weechat_buffer_clear (fset_buffer);
        fset_buffer_selected_line = (num_options > 0) ? 0 : -1;
    }

    snprintf (str_title, sizeof (str_title), _("Fast Set"));
    weechat_buffer_set (fset_buffer, "title", str_title);

    for (i = 0; i < num_options; i++)
    {
        ptr_option = weechat_arraylist_get (fset_options, i);
        fset_buffer_display_line (i, ptr_option);
    }
}

/*
 * Sets current selected line.
 */

void
fset_buffer_set_current_line (int line)
{
    int old_line;

    if ((line >= 0) && (line < weechat_arraylist_size (fset_options)))
    {
        old_line = fset_buffer_selected_line;
        fset_buffer_selected_line = line;

        fset_buffer_display_line (
            old_line,
            weechat_arraylist_get (fset_options, old_line));
        fset_buffer_display_line (
            fset_buffer_selected_line,
            weechat_arraylist_get (fset_options, fset_buffer_selected_line));
    }
}

/*
 * Gets info about a window.
 */

void
fset_buffer_get_window_info (struct t_gui_window *window,
                             int *start_line_y, int *chat_height)
{
    struct t_hdata *hdata_window, *hdata_window_scroll, *hdata_line;
    struct t_hdata *hdata_line_data;
    void *window_scroll, *start_line, *line_data;

    hdata_window = weechat_hdata_get ("window");
    hdata_window_scroll = weechat_hdata_get ("window_scroll");
    hdata_line = weechat_hdata_get ("line");
    hdata_line_data = weechat_hdata_get ("line_data");
    *start_line_y = 0;
    window_scroll = weechat_hdata_pointer (hdata_window, window, "scroll");
    if (window_scroll)
    {
        start_line = weechat_hdata_pointer (hdata_window_scroll, window_scroll,
                                            "start_line");
        if (start_line)
        {
            line_data = weechat_hdata_pointer (hdata_line, start_line, "data");
            if (line_data)
            {
                *start_line_y = weechat_hdata_integer (hdata_line_data,
                                                       line_data, "y");
            }
        }
    }
    *chat_height = weechat_hdata_integer (hdata_window, window,
                                          "win_chat_height");
}

/*
 * Checks if current line is outside window.
 *
 * Returns:
 *   1: line is outside window
 *   0: line is inside window
 */

void
fset_buffer_check_line_outside_window ()
{
    struct t_gui_window *window;
    int start_line_y, chat_height;
    char str_command[256];

    window = weechat_window_search_with_buffer (fset_buffer);
    if (!window)
        return;

    fset_buffer_get_window_info (window, &start_line_y, &chat_height);
    if ((start_line_y > fset_buffer_selected_line)
        || (start_line_y <= fset_buffer_selected_line - chat_height))
    {
        snprintf (str_command, sizeof (str_command),
                  "/window scroll -window %d %s%d",
                  weechat_window_get_integer (window, "number"),
                  (start_line_y > fset_buffer_selected_line) ? "-" : "+",
                  (start_line_y > fset_buffer_selected_line) ?
                  start_line_y - fset_buffer_selected_line :
                  fset_buffer_selected_line - start_line_y - chat_height + 1);
        weechat_command (fset_buffer, str_command);
    }
}

/*
 * Callback for signal "window_scrolled".
 */

int
fset_buffer_window_scrolled_cb (const void *pointer, void *data,
                                const char *signal, const char *type_data,
                                void *signal_data)
{
    int start_line_y, chat_height, line, num_options;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;

    /* scrolled another window/buffer? then just ignore */
    if (weechat_window_get_pointer (signal_data, "buffer") != fset_buffer)
        return WEECHAT_RC_OK;

    fset_buffer_get_window_info (signal_data, &start_line_y, &chat_height);

    line = fset_buffer_selected_line;
    while (line < start_line_y)
    {
        line += chat_height;
    }
    while (line >= start_line_y + chat_height)
    {
        line -= chat_height;
    }
    if (line < start_line_y)
        line = start_line_y;
    num_options = weechat_arraylist_size (fset_options);
    if (line >= num_options)
        line = num_options - 1;
    fset_buffer_set_current_line (line);

    return WEECHAT_RC_OK;
}

/*
 * Callback for user data in fset buffer.
 */

int
fset_buffer_input_cb (const void *pointer, void *data,
                      struct t_gui_buffer *buffer,
                      const char *input_data)
{
    char *actions[][2] = { { "t", "toggle"   },
                           { "+", "increase" },
                           { "-", "decrease" },
                           { "r", "reset"    },
                           { "u", "unset"    },
                           { "s", "set"      },
                           { "a", "append"   },
                           { NULL, NULL      } };
    char str_command[64];
    int i;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    /* close buffer */
    if (strcmp (input_data, "q") == 0)
    {
        weechat_buffer_close (buffer);
        return WEECHAT_RC_OK;
    }

    /* refresh buffer */
    if (strcmp (input_data, "$") == 0)
    {
        fset_option_get_options ();
        fset_buffer_refresh (1);
        return WEECHAT_RC_OK;
    }

    /* execute action on an option */
    for (i = 0; actions[i][0]; i++)
    {
        if (strcmp (input_data, actions[i][0]) == 0)
        {
            snprintf (str_command, sizeof (str_command),
                      "/fset %s", actions[i][1]);
            weechat_command (buffer, str_command);
            return WEECHAT_RC_OK;
        }
    }

    /* filter options with given text */
    fset_option_filter_options (input_data);

    return WEECHAT_RC_OK;
}

/*
 * Callback called when fset buffer is closed.
 */

int
fset_buffer_close_cb (const void *pointer, void *data,
                      struct t_gui_buffer *buffer)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) buffer;

    fset_buffer = NULL;
    fset_buffer_selected_line = 0;
    weechat_arraylist_clear (fset_options);

    return WEECHAT_RC_OK;
}

/*
 * Restore buffer callbacks (input and close) for buffer created by fset
 * plugin.
 */

void
fset_buffer_set_callbacks ()
{
    struct t_gui_buffer *ptr_buffer;

    ptr_buffer = weechat_buffer_search (FSET_PLUGIN_NAME, FSET_BUFFER_NAME);
    if (ptr_buffer)
    {
        fset_buffer = ptr_buffer;
        weechat_buffer_set_pointer (fset_buffer, "close_callback", &fset_buffer_close_cb);
        weechat_buffer_set_pointer (fset_buffer, "input_callback", &fset_buffer_input_cb);
    }
}

/*
 * Sets keys on fset buffer.
 */

void
fset_buffer_set_keys ()
{
    char *keys[][2] = { { "meta-t",  "toggle"   },
                        { "meta-+",  "increase" },
                        { "meta--",  "decrease" },
                        { "meta-r",  "reset"    },
                        { "meta-u",  "unset"    },
                        { "meta-s",  "set"      },
                        { "meta-a",  "append"   },
                        { NULL,     NULL        } };
    char str_key[64], str_command[64];
    int i;

    weechat_buffer_set (fset_buffer, "key_bind_meta2-A", "/fset -up");
    weechat_buffer_set (fset_buffer, "key_bind_meta2-B", "/fset -down");
    for (i = 0; keys[i][0]; i++)
    {
        if (weechat_config_boolean (fset_config_look_use_keys))
        {
            snprintf (str_key, sizeof (str_key), "key_bind_%s", keys[i][0]);
            snprintf (str_command, sizeof (str_command), "/fset -%s", keys[i][1]);
            weechat_buffer_set (fset_buffer, str_key, str_command);
        }
        else
        {
            snprintf (str_key, sizeof (str_key), "key_unbind_%s", keys[i][0]);
            weechat_buffer_set (fset_buffer, str_key, "");
        }
    }
}

/*
 * Opens fset buffer.
 */

void
fset_buffer_open ()
{
    if (!fset_buffer)
    {
        fset_buffer = weechat_buffer_new (
            FSET_BUFFER_NAME,
            &fset_buffer_input_cb, NULL, NULL,
            &fset_buffer_close_cb, NULL, NULL);

        /* failed to create buffer ? then exit */
        if (!fset_buffer)
            return;

        weechat_buffer_set (fset_buffer, "type", "free");
        weechat_buffer_set (fset_buffer, "title", _("Options"));
        fset_buffer_set_keys ();
        weechat_buffer_set (fset_buffer, "localvar_set_type", "option");

        fset_buffer_selected_line = 0;
    }
}

/*
 * Initializes fset buffer.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
fset_buffer_init ()
{
    fset_buffer_set_callbacks ();

    /* create hashtables used by the bar item callback */
    fset_buffer_hashtable_pointers = weechat_hashtable_new (
        8,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_POINTER,
        NULL,
        NULL);
    if (!fset_buffer_hashtable_pointers)
        return 0;

    fset_buffer_hashtable_extra_vars = weechat_hashtable_new (
        32,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING,
        NULL,
        NULL);
    if (!fset_buffer_hashtable_extra_vars)
    {
        weechat_hashtable_free (fset_buffer_hashtable_pointers);
        return 0;
    }

    return 1;
}

/*
 * Ends fset buffer.
 */

void
fset_buffer_end ()
{
    weechat_hashtable_free (fset_buffer_hashtable_pointers);
    fset_buffer_hashtable_pointers = NULL;

    weechat_hashtable_free (fset_buffer_hashtable_extra_vars);
    fset_buffer_hashtable_extra_vars = NULL;
}
