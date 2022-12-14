#pragma once
#include "base.h"
#include "oso.h"
#include "ged.h"
#include "term_util.h"

typedef struct {
    Ged *ged;
    oso *file_name;
    oso *osc_address;
    oso *osc_port;
    oso *osc_midi_bidule_path;
    int undo_history_limit;
    int softmargin_y;
    int softmargin_x;
    int hardmargin_y;
    int hardmargin_x;
    U32 prefs_touched;
    bool use_gui_cboard; // not bitfields due to taking address of
    bool strict_timing;
    bool osc_output_enabled;
    bool fancy_grid_dots;
    bool fancy_grid_rulers;
} Tui;

void tui_init(Tui *tui, Ged *ged);

void push_main_menu(void);

void push_controls_msg(void);

void push_opers_guide_msg(void);

void push_open_form(char const *initial);

void tui_load_conf(Tui *tui);

bool tui_suggest_nice_grid_size(Tui *tui, int win_h, int win_w, Usz *out_grid_h, Usz *out_grid_w);

void tui_restart_osc_udp_if_enabled(Tui *tui);

void tui_adjust_term_size(Tui *tui, WINDOW **win);

void tui_try_save(Tui *tui);


typedef enum
{
    Tui_menus_nothing = 0,
    Tui_menus_quit,
    Tui_menus_consumed_input,
} Tui_menus_result;

Tui_menus_result tui_drive_menus(Tui *tui, int key);
