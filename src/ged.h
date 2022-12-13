#pragma once
#include "field.h"
#include "vmio.h"
#include "term_util.h"
#include "midi.h"
#include "osc_out.h"

typedef enum
{
    Ged_input_mode_normal = 0,
    Ged_input_mode_append,
    Ged_input_mode_selresize,
    Ged_input_mode_slide,
} Ged_input_mode;

typedef struct {
    Usz y;
    Usz x;
    Usz h;
    Usz w;
} Ged_cursor;

typedef struct {
    Field field;
    Field scratch_field;
    Field clipboard_field;
    Mbuf_reusable mbuf_r;
    Undo_history undo_hist;
    Oevent_list oevent_list;
    Oevent_list scratch_oevent_list;
    Susnote_list susnote_list;
    Ged_cursor ged_cursor;
    Usz tick_num;
    Usz ruler_spacing_y;
    Usz ruler_spacing_x;
    Ged_input_mode input_mode;
    Usz bpm;
    U64 clock;
    double accum_secs;
    double time_to_next_note_off;
    Oosc_dev *oosc_dev;
    Midi_mode midi_mode;
    Usz activity_counter;
    Usz random_seed;
    Usz drag_start_y;
    Usz drag_start_x;
    int win_h;
    int win_w;
    int softmargin_y;
    int softmargin_x;
    int grid_h;
    int grid_scroll_y;
    int grid_scroll_x;     // not sure if i like this being int
    U8 midi_bclock_sixths; // 0..5, holds 6th of the quarter note step
    bool needs_remarking : 1;
    bool is_draw_dirty : 1;
    bool is_playing : 1;
    bool midi_bclock : 1;
    bool draw_event_list : 1;
    bool is_mouse_down : 1;
    bool is_mouse_dragging : 1;
    bool is_hud_visible : 1;
} Ged;

void ged_init(Ged *a, Usz undo_limit, Usz init_bpm, Usz init_seed);

void ged_make_cursor_visible(Ged *a);

void ged_send_osc_bpm(Ged *a, I32 bpm);

void ged_set_playing(Ged *a, bool playing);

void ged_do_stuff(Ged *a);

bool ged_is_draw_dirty(Ged *a);

void ged_draw(Ged *a, WINDOW *win, char const *filename, bool use_fancy_dots, bool use_fancy_rulers);

double ged_secs_to_deadline(Ged const *a);

ORCA_OK_IF_UNUSED void ged_mouse_event(Ged *a, Usz vis_y, Usz vis_x, mmask_t mouse_bstate);

typedef enum
{
    Ged_dir_up,
    Ged_dir_down,
    Ged_dir_left,
    Ged_dir_right,
} Ged_dir;

void ged_dir_input(Ged *a, Ged_dir dir, int step_length);

void ged_input_character(Ged *a, char c);

typedef enum
{
    Ged_input_cmd_undo,
    Ged_input_cmd_toggle_append_mode,
    Ged_input_cmd_toggle_selresize_mode,
    Ged_input_cmd_toggle_slide_mode,
    Ged_input_cmd_step_forward,
    Ged_input_cmd_toggle_show_event_list,
    Ged_input_cmd_toggle_play_pause,
    Ged_input_cmd_cut,
    Ged_input_cmd_copy,
    Ged_input_cmd_paste,
    Ged_input_cmd_escape,
} Ged_input_cmd;


void ged_set_window_size(Ged *a, int win_h, int win_w, int softmargin_y, int softmargin_x);

void ged_resize_grid(
    Field *field,
    Mbuf_reusable *mbr,
    Usz new_height,
    Usz new_width,
    Usz tick_num,
    Field *scratch_field,
    Undo_history *undo_hist,
    Ged_cursor *ged_cursor);

void ged_cursor_confine(Ged_cursor *tc, Usz height, Usz width);

void ged_update_internal_geometry(Ged *a);

void ged_input_cmd(Ged *a, Ged_input_cmd ev);

void ged_adjust_rulers_relative(Ged *a, Isz delta_y, Isz delta_x);

void ged_resize_grid_relative(Ged *a, Isz delta_y, Isz delta_x);

void ged_clear_osc_udp(Ged *a);

bool ged_set_osc_udp(Ged *a, char const *dest_addr, char const *dest_port);

bool ged_is_using_osc_udp(Ged *a);

void ged_adjust_bpm(Ged *a, Isz delta_bpm);

void ged_modify_selection_size(Ged *a, int delta_y, int delta_x);

bool ged_slide_selection(Ged *a, int delta_y, int delta_x);

void ged_stop_all_sustained_notes(Ged *a);

void ged_deinit(Ged *a);

