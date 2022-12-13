//#include "base.h"
//#include "oso.h"
#include "sysmisc.h"
#include "field.h"
//#include "term_util.h"
#include "tui.h"
#include "midi.h"

char const *const conf_file_name = "orca.conf";

#define CONFOPT_STRING(x) #x,
#define CONFOPT_ENUM(x) Confopt_##x,
#define CONFOPTS(_)                                                                                \
    _(portmidi_output_device)                                                                      \
    _(osc_output_address)                                                                          \
    _(osc_output_port)                                                                             \
    _(osc_output_enabled)                                                                          \
    _(midi_beat_clock)                                                                             \
    _(margins)                                                                                     \
    _(grid_dot_type)                                                                               \
    _(grid_ruler_type)

char const *const confopts[] = { CONFOPTS(CONFOPT_STRING) };

enum
{
    Confoptslen = ORCA_ARRAY_COUNTOF(confopts)
};

enum
{
    CONFOPTS(CONFOPT_ENUM)
};

#undef CONFOPTS
#undef CONFOPT_STRING
#undef CONFOPT_ENUM


enum
{
    Main_menu_id = 1,
    Open_form_id,
    Save_as_form_id,
    Set_tempo_form_id,
    Set_grid_dims_form_id,
    Autofit_menu_id,
    Confirm_new_file_menu_id,
    Cosmetics_menu_id,
    Osc_menu_id,
    Osc_output_address_form_id,
    Osc_output_port_form_id,
    Playback_menu_id,
    Set_soft_margins_form_id,
    Set_fancy_grid_dots_menu_id,
    Set_fancy_grid_rulers_menu_id,
#ifdef FEAT_PORTMIDI
    Portmidi_output_device_menu_id,
#endif
};

enum
{
    Autofit_nicely_id = 1,
    Autofit_tightly_id,
};

enum
{
    Confirm_new_file_reject_id = 1,
    Confirm_new_file_accept_id,
};

enum
{
    Main_menu_quit = 1,
    Main_menu_controls,
    Main_menu_opers_guide,
    Main_menu_new,
    Main_menu_open,
    Main_menu_save,
    Main_menu_save_as,
    Main_menu_set_tempo,
    Main_menu_set_grid_dims,
    Main_menu_autofit_grid,
    Main_menu_about,
    Main_menu_cosmetics,
    Main_menu_playback,
    Main_menu_osc,
#ifdef FEAT_PORTMIDI
    Main_menu_choose_portmidi_output,
#endif
};


// Use this to create a bitflag out of a Confopt_. These flags are used to
// indicate that a setting has been touched by the user. In other words, these
// flags are used to check whether or not a particular setting should be
// written back to the conf file during a conf file save.
#define TOUCHFLAG(_confopt) (1u << (Usz)_confopt)

bool conf_read_boolish(char const *val, bool *out)
{
    static char const *const trues[] = { "1", "true", "yes" };
    static char const *const falses[] = { "0", "false", "no" };
    for (Usz i = 0; i < ORCA_ARRAY_COUNTOF(trues); i++) {
        if (strcmp(val, trues[i]) != 0)
            continue;
        *out = true;
        return true;
    }
    for (Usz i = 0; i < ORCA_ARRAY_COUNTOF(falses); i++) {
        if (strcmp(val, falses[i]) != 0)
            continue;
        *out = false;
        return true;
    }
    return false;
}


char const *const prefval_plain = "plain";
char const *const prefval_fancy = "fancy";

bool plainorfancy(char const *val, bool *out)
{
    if (strcmp(val, prefval_plain) == 0) {
        *out = false;
        return true;
    }
    if (strcmp(val, prefval_fancy) == 0) {
        *out = true;
        return true;
    }
    return false;
}

ORCA_OK_IF_UNUSED void print_loading_message(char const *s)
{
    Usz len = strlen(s);
    if (len > INT_MAX)
        return;
    int h, w;
    getmaxyx(stdscr, h, w);
    int y = h / 2;
    int x = (int)len < w ? (w - (int)len) / 2 : 0;
    werase(stdscr);
    wmove(stdscr, y, x);
    waddstr(stdscr, s);
    refresh();
}

void push_main_menu(void)
{
    Qmenu *qm = qmenu_create(Main_menu_id);
    qmenu_set_title(qm, "ORCA");
    qmenu_add_choice(qm, Main_menu_new, "New");
    qmenu_add_choice(qm, Main_menu_open, "Open...");
    qmenu_add_choice(qm, Main_menu_save, "Save");
    qmenu_add_choice(qm, Main_menu_save_as, "Save As...");
    qmenu_add_spacer(qm);
    qmenu_add_choice(qm, Main_menu_set_tempo, "Set BPM...");
    qmenu_add_choice(qm, Main_menu_set_grid_dims, "Set Grid Size...");
    qmenu_add_choice(qm, Main_menu_autofit_grid, "Auto-fit Grid");
    qmenu_add_spacer(qm);
    qmenu_add_choice(qm, Main_menu_osc, "OSC Output...");
#ifdef FEAT_PORTMIDI
    qmenu_add_choice(qm, Main_menu_choose_portmidi_output, "MIDI Output...");
#endif
    qmenu_add_spacer(qm);
    qmenu_add_choice(qm, Main_menu_playback, "Clock & Timing...");
    qmenu_add_choice(qm, Main_menu_cosmetics, "Appearance...");
    qmenu_add_spacer(qm);
    qmenu_add_choice(qm, Main_menu_controls, "Controls...");
    qmenu_add_choice(qm, Main_menu_opers_guide, "Operators...");
    qmenu_add_choice(qm, Main_menu_about, "About ORCA...");
    qmenu_add_spacer(qm);
    qmenu_add_choice(qm, Main_menu_quit, "Quit");
    qmenu_push_to_nav(qm);
}


void push_confirm_new_file_menu(void)
{
    Qmenu *qm = qmenu_create(Confirm_new_file_menu_id);
    qmenu_set_title(qm, "Are you sure?");
    qmenu_add_choice(qm, Confirm_new_file_reject_id, "Cancel");
    qmenu_add_choice(qm, Confirm_new_file_accept_id, "Create New File");
    qmenu_push_to_nav(qm);
}

void push_autofit_menu(void)
{
    Qmenu *qm = qmenu_create(Autofit_menu_id);
    qmenu_set_title(qm, "Auto-fit Grid");
    qmenu_add_choice(qm, Autofit_nicely_id, "Nicely");
    qmenu_add_choice(qm, Autofit_tightly_id, "Tightly");
    qmenu_push_to_nav(qm);
}

enum
{
    Cosmetics_soft_margins_id = 1,
    Cosmetics_grid_dots_id,
    Cosmetics_grid_rulers_id,
};

void push_cosmetics_menu(void)
{
    Qmenu *qm = qmenu_create(Cosmetics_menu_id);
    qmenu_set_title(qm, "Appearance");
    qmenu_add_choice(qm, Cosmetics_soft_margins_id, "Margins...");
    qmenu_add_choice(qm, Cosmetics_grid_dots_id, "Grid dots...");
    qmenu_add_choice(qm, Cosmetics_grid_rulers_id, "Grid rulers...");
    qmenu_push_to_nav(qm);
}

void push_soft_margins_form(int init_y, int init_x)
{
    char buff[128];
    int snres = snprintf(buff, sizeof buff, "%dx%d", init_x, init_y);
    char const *inistr = snres > 0 && (Usz)snres < sizeof buff ? buff : "2x1";
    qform_single_line_input(Set_soft_margins_form_id, "Set Margins", inistr);
}

void push_plainorfancy_menu(int menu_id, char const *title, bool initial_fancy)
{
    Qmenu *qm = qmenu_create(menu_id);
    qmenu_set_title(qm, title);
    qmenu_add_printf(qm, 1, "(%c) Fancy", initial_fancy ? '*' : ' ');
    qmenu_add_printf(qm, 2, "(%c) Plain", !initial_fancy ? '*' : ' ');
    if (!initial_fancy)
        qmenu_set_current_item(qm, 2);
    qmenu_push_to_nav(qm);
}
enum
{
    Osc_menu_output_enabledisable = 1,
    Osc_menu_output_address,
    Osc_menu_output_port,
};

void push_osc_menu(bool output_enabled)
{
    Qmenu *qm = qmenu_create(Osc_menu_id);
    qmenu_set_title(qm, "OSC Output");
    qmenu_add_printf(
        qm,
        Osc_menu_output_enabledisable,
        "[%c] OSC Output Enabled",
        output_enabled ? '*' : ' ');
    qmenu_add_choice(qm, Osc_menu_output_address, "OSC Output Address...");
    qmenu_add_choice(qm, Osc_menu_output_port, "OSC Output Port...");
    qmenu_push_to_nav(qm);
}

void push_osc_output_address_form(char const *initial)
{
    qform_single_line_input(Osc_output_address_form_id, "Set OSC Output Address", initial);
}

void push_osc_output_port_form(char const *initial)
{
    qform_single_line_input(Osc_output_port_form_id, "Set OSC Output Port", initial);
}

enum
{
    Playback_menu_midi_bclock = 1,
};

void push_playback_menu(bool midi_bclock_enabled)
{
    Qmenu *qm = qmenu_create(Playback_menu_id);
    qmenu_set_title(qm, "Clock & Timing");
    qmenu_add_printf(
        qm,
        Playback_menu_midi_bclock,
        "[%c] Send MIDI Beat Clock",
        midi_bclock_enabled ? '*' : ' ');
    qmenu_push_to_nav(qm);
}

void push_about_msg(void)
{
    // clang-format off
    static char const* logo[] = {
    "lqqqk|lqqqk|lqqqk|lqqqk",
    "x   x|x   j|x    |lqqqu",
    "mqqqj|m    |mqqqj|m   j",
    };
    static char const* footer =
    "Live Programming Environment";
    // clang-format on
    int rows = (int)ORCA_ARRAY_COUNTOF(logo);
    int cols = (int)strlen(logo[0]);
    int hpad = 5, tpad = 2, bpad = 2;
    int sep = 1;
    int footer_len = (int)strlen(footer);
    int width = footer_len;
    if (cols > width)
        width = cols;
    width += hpad * 2;
    int logo_left_pad = (width - cols) / 2;
    int footer_left_pad = (width - footer_len) / 2;
    Qmsg *qm = qmsg_push(tpad + rows + sep + 1 + bpad, width);
    WINDOW *w = qmsg_window(qm);
    for (int row = 0; row < rows; ++row) {
        wmove(w, row + tpad, logo_left_pad);
        wattrset(w, A_BOLD);
        for (int col = 0; col < cols; ++col) {
            char c = logo[row][col];
            chtype ch;
            if (c == ' ')
                ch = (chtype)' ';
            else if (c == '|')
                ch = ACS_VLINE | (chtype)fg_bg(C_black, C_natural) | A_BOLD;
            else
                ch = NCURSES_ACS(c) | A_BOLD;
            waddch(w, ch);
        }
    }
    wattrset(w, A_DIM);
    wmove(w, tpad + rows + sep, footer_left_pad);
    waddstr(w, footer);
}
void push_controls_msg(void)
{
    struct Ctrl_item {
        char const *input;
        char const *desc;
    };
    static struct Ctrl_item items[] = {
        { "Ctrl+Q", "Quit" },
        { "Arrow Keys", "Move Cursor" },
        { "Ctrl+D or F1", "Open Main Menu" },
        { "0-9, A-Z, a-z,", "Insert Character" },
        { "! : % / = # *", NULL },
        { "Spacebar", "Play/Pause" },
        { "Ctrl+Z or Ctrl+U", "Undo" },
        { "Ctrl+X", "Cut" },
        { "Ctrl+C", "Copy" },
        { "Ctrl+V", "Paste" },
        { "Ctrl+S", "Save" },
        { "Ctrl+F", "Frame Step Forward" },
        { "Ctrl+R", "Reset Frame Number" },
        { "Ctrl+I or Insert", "Append/Overwrite Mode" },
        // {"/", "Key Trigger Mode"},
        { "' (quote)", "Rectangle Selection Mode" },
        { "Shift+Arrow Keys", "Adjust Rectangle Selection" },
        { "Alt+Arrow Keys", "Slide Selection" },
        { "` (grave) or ~", "Slide Selection Mode" },
        { "Escape", "Return to Normal Mode or Deselect" },
        { "( ) _ + [ ] { }", "Adjust Grid Size and Rulers" },
        { "< and >", "Adjust BPM" },
        { "?", "Controls (this message)" },
    };
    int w_input = 0;
    int w_desc = 0;
    for (Usz i = 0; i < ORCA_ARRAY_COUNTOF(items); ++i) {
        // use wcswidth instead of strlen if you need wide char support. but note
        // that won't be useful for UTF-8 or unicode chars in higher plane (emoji,
        // complex zwj, etc.)
        if (items[i].input) {
            int wl = (int)strlen(items[i].input);
            if (wl > w_input)
                w_input = wl;
        }
        if (items[i].desc) {
            int wr = (int)strlen(items[i].desc);
            if (wr > w_desc)
                w_desc = wr;
        }
    }
    int mid_pad = 2;
    int total_width = 1 + w_input + mid_pad + w_desc + 1;
    Qmsg *qm = qmsg_push(ORCA_ARRAY_COUNTOF(items), total_width);
    qmsg_set_title(qm, "Controls");
    WINDOW *w = qmsg_window(qm);
    for (int i = 0; i < (int)ORCA_ARRAY_COUNTOF(items); ++i) {
        if (items[i].input) {
            wmove(w, i, 1 + w_input - (int)strlen(items[i].input));
            waddstr(w, items[i].input);
        }
        if (items[i].desc) {
            wmove(w, i, 1 + w_input + mid_pad);
            waddstr(w, items[i].desc);
        }
    }
}
void push_opers_guide_msg(void)
{
    struct Guide_item {
        char glyph;
        char const *name;
        char const *desc;
    };
    static struct Guide_item items[] = {
        { 'A', "add", "Outputs sum of inputs." },
        { 'B', "subtract", "Outputs difference of inputs." },
        { 'C', "clock", "Outputs modulo of frame." },
        { 'D', "delay", "Bangs on modulo of frame." },
        { 'E', "east", "Moves eastward, or bangs." },
        { 'F', "if", "Bangs if inputs are equal." },
        { 'G', "generator", "Writes operands with offset." },
        { 'H', "halt", "Halts southward operand." },
        { 'I', "increment", "Increments southward operand." },
        { 'J', "jumper", "Outputs northward operand." },
        { 'K', "konkat", "Reads multiple variables." },
        { 'L', "lesser", "Outputs smallest input." },
        { 'M', "multiply", "Outputs product of inputs." },
        { 'N', "north", "Moves Northward, or bangs." },
        { 'O', "read", "Reads operand with offset." },
        { 'P', "push", "Writes eastward operand." },
        { 'Q', "query", "Reads operands with offset." },
        { 'R', "random", "Outputs random value." },
        { 'S', "south", "Moves southward, or bangs." },
        { 'T', "track", "Reads eastward operand." },
        { 'U', "uclid", "Bangs on Euclidean rhythm." },
        { 'V', "variable", "Reads and writes variable." },
        { 'W', "west", "Moves westward, or bangs." },
        { 'X', "write", "Writes operand with offset." },
        { 'Y', "jymper", "Outputs westward operand." },
        { 'Z', "lerp", "Transitions operand to target." },
        { '^', "heck-roof", "FREE to be IMPLEMENTED :)" },
        { '*', "bang", "Bangs neighboring operands." },
        { '#', "comment", "Halts line." },
        // {'*', "self", "Sends ORCA command."},
        { ':', "midi", "Sends MIDI note." },
        { '!', "cc", "Sends MIDI control change." },
        { '?', "pb", "Sends MIDI pitch bend." },
        // {'%', "mono", "Sends MIDI monophonic note."},
        { '=', "osc", "Sends OSC message." },
        { ';', "udp", "Sends UDP message." },
    };
    int w_desc = 0;
    for (Usz i = 0; i < ORCA_ARRAY_COUNTOF(items); ++i) {
        if (items[i].desc) {
            int wr = (int)strlen(items[i].desc);
            if (wr > w_desc)
                w_desc = wr;
        }
    }
    int left_pad = 1, mid_pad = 1, right_pad = 1;
    int total_width = left_pad + 1 + mid_pad + w_desc + right_pad;
    Qmsg *qm = qmsg_push(ORCA_ARRAY_COUNTOF(items), total_width);
    qmsg_set_title(qm, "Operators");
    WINDOW *w = qmsg_window(qm);
    for (int i = 0; i < (int)ORCA_ARRAY_COUNTOF(items); ++i) {
        wmove(w, i, left_pad);
        waddch(w, (chtype)items[i].glyph | A_bold);
        wmove(w, i, left_pad + 1 + mid_pad);
        wattrset(w, A_normal);
        waddstr(w, items[i].desc);
    }
}

void push_open_form(char const *initial)
{
    qform_single_line_input(Open_form_id, "Open", initial);
}

bool hacky_try_save(Field *field, char const *filename)
{
    if (!filename)
        return false;
    if (field->height == 0 || field->width == 0)
        return false;
    FILE *f = fopen(filename, "w");
    if (!f)
        return false;
    field_fput(field, f);
    fclose(f);
    return true;
}

bool try_save_with_msg(Field *field, oso const *str)
{
    if (!osolen(str))
        return false;
    bool ok = hacky_try_save(field, osoc(str));
    if (ok) {
        Qmsg *qm = qmsg_printf_push(NULL, "Saved to:\n%s", osoc(str));
        qmsg_set_dismiss_mode(qm, Qmsg_dismiss_mode_passthrough);
    } else {
        qmsg_printf_push("Error Saving File", "Unable to save file to:\n%s", osoc(str));
    }
    return ok;
}

void push_save_as_form(char const *initial)
{
    qform_single_line_input(Save_as_form_id, "Save As", initial);
}

void push_set_tempo_form(Usz initial)
{
    char buff[64];
    int snres = snprintf(buff, sizeof buff, "%zu", initial);
    char const *inistr = snres > 0 && (Usz)snres < sizeof buff ? buff : "120";
    qform_single_line_input(Set_tempo_form_id, "Set BPM", inistr);
}

void push_set_grid_dims_form(Usz init_height, Usz init_width)
{
    char buff[128];
    int snres = snprintf(buff, sizeof buff, "%zux%zu", init_width, init_height);
    char const *inistr = snres > 0 && (Usz)snres < sizeof buff ? buff : "57x25";
    qform_single_line_input(Set_grid_dims_form_id, "Set Grid Size", inistr);
}


#ifdef FEAT_PORTMIDI
void push_portmidi_output_device_menu(Midi_mode const *midi_mode)
{
    Qmenu *qm = qmenu_create(Portmidi_output_device_menu_id);
    qmenu_set_title(qm, "PortMidi Device Selection");
    PmError e = portmidi_init_if_necessary();
    if (e) {
        qmenu_destroy(qm);
        qmsg_printf_push(
            "PortMidi Error",
            "PortMidi error during initialization:\n%s",
            Pm_GetErrorText(e));
        return;
    }
    int num = Pm_CountDevices();
    int output_devices = 0;
    int cur_dev_id = 0;
    bool has_cur_dev_id = false;
    if (midi_mode->any.type == Midi_mode_type_portmidi) {
        cur_dev_id = midi_mode->portmidi.device_id;
        has_cur_dev_id = true;
    }
    for (int i = 0; i < num; ++i) {
        PmDeviceInfo const *info = Pm_GetDeviceInfo(i);
        if (!info || !info->output)
            continue;
        bool is_cur_dev_id = has_cur_dev_id && cur_dev_id == i;
        qmenu_add_printf(qm, i, "(%c) #%d - %s", is_cur_dev_id ? '*' : ' ', i, info->name);
        ++output_devices;
    }
    if (output_devices == 0) {
        qmenu_destroy(qm);
        qmsg_printf_push("No PortMidi Devices", "No PortMidi output devices found.");
        return;
    }
    if (has_cur_dev_id) {
        qmenu_set_current_item(qm, cur_dev_id);
    }
    qmenu_push_to_nav(qm);
}
#endif

void tui_init(Tui *tui, Ged *ged)
{
    tui->ged = ged;
    tui->file_name = NULL;
    tui->file_name = NULL;
    tui->osc_address = NULL;
    tui->osc_port = NULL;
    tui->osc_midi_bidule_path = NULL;
    tui->undo_history_limit = 100;
    tui->softmargin_y = 4;
    tui->softmargin_x = 4;
    tui->hardmargin_y = 8;
    tui->hardmargin_x = 8;
    tui->prefs_touched = 0;
    tui->use_gui_cboard = true;
    tui->strict_timing = false;
    tui->osc_output_enabled = false;
    tui->fancy_grid_dots = true;
    tui->fancy_grid_rulers = true;
}

void tui_load_conf(Tui *tui)
{
    oso *portmidi_output_device = NULL, *osc_output_address = NULL, *osc_output_port = NULL;
    U32 touched = 0;
    Ezconf_r ez;
    for (ezconf_r_start(&ez, conf_file_name); ezconf_r_step(&ez, confopts, Confoptslen);) {
        switch (ez.index) {
            case Confopt_portmidi_output_device:
                osoput(&portmidi_output_device, ez.value);
                break;
            case Confopt_osc_output_address: {
                // Don't actually allocate heap string if string is empty
                Usz len = strlen(ez.value);
                if (len > 0)
                    osoputlen(&osc_output_address, ez.value, len);
                touched |= TOUCHFLAG(Confopt_osc_output_address);
                break;
            }
            case Confopt_osc_output_port: {
                osoput(&osc_output_port, ez.value);
                touched |= TOUCHFLAG(Confopt_osc_output_port);
                break;
            }
            case Confopt_osc_output_enabled: {
                bool enabled;
                if (conf_read_boolish(ez.value, &enabled)) {
                    tui->osc_output_enabled = enabled;
                    touched |= TOUCHFLAG(Confopt_osc_output_enabled);
                }
                break;
            }
            case Confopt_midi_beat_clock: {
                bool enabled;
                if (conf_read_boolish(ez.value, &enabled)) {
                    tui->ged->midi_bclock = enabled;
                    touched |= TOUCHFLAG(Confopt_midi_beat_clock);
                }
                break;
            }
            case Confopt_margins: {
                int softmargin_y, softmargin_x;
                if (read_nxn_or_n(ez.value, &softmargin_x, &softmargin_y) && softmargin_y >= 0 &&
                    softmargin_x >= 0) {
                    tui->softmargin_y = softmargin_y;
                    tui->softmargin_x = softmargin_x;
                    touched |= TOUCHFLAG(Confopt_margins);
                }
                break;
            }
            case Confopt_grid_dot_type: {
                bool fancy;
                if (plainorfancy(ez.value, &fancy)) {
                    tui->fancy_grid_dots = fancy;
                    touched |= TOUCHFLAG(Confopt_grid_dot_type);
                }
                break;
            }
            case Confopt_grid_ruler_type: {
                bool fancy;
                if (plainorfancy(ez.value, &fancy)) {
                    tui->fancy_grid_rulers = fancy;
                    touched |= TOUCHFLAG(Confopt_grid_ruler_type);
                }
                break;
            }
        }
    }

    if (touched & TOUCHFLAG(Confopt_osc_output_address)) {
        ososwap(&tui->osc_address, &osc_output_address);
    } else {
        // leave null
    }
    if (touched & TOUCHFLAG(Confopt_osc_output_port)) {
        ososwap(&tui->osc_port, &osc_output_port);
    } else {
        osoput(&tui->osc_port, "49162");
    }

#ifdef FEAT_PORTMIDI
    if (tui->ged->midi_mode.any.type == Midi_mode_type_null && osolen(portmidi_output_device)) {
        // PortMidi can be hilariously slow to initialize. Since it will be
        // initialized automatically if the user has a prefs entry for PortMidi
        // devices, we should show a message to the user letting them know why
        // orca is locked up/frozen. (When it's done via menu action, that's
        // fine, since they can figure out why.)
        print_loading_message("Waiting on PortMidi...");
        PmError pmerr;
        PmDeviceID devid;
        if (portmidi_find_device_id_by_name(
                osoc(portmidi_output_device),
                osolen(portmidi_output_device),
                &pmerr,
                &devid)) {
            midi_mode_deinit(&tui->ged->midi_mode);
            pmerr = midi_mode_init_portmidi(&tui->ged->midi_mode, devid);
            if (pmerr) {
                // todo stuff
            }
        }
    }
#endif
    tui->prefs_touched |= touched;
    osofree(portmidi_output_device);
    osofree(osc_output_address);
    osofree(osc_output_port);
}

void tui_save_prefs(Tui *tui)
{
    Ezconf_opt optsbuff[Confoptslen];
    Ezconf_w ez;
    ezconf_w_start(&ez, optsbuff, ORCA_ARRAY_COUNTOF(optsbuff), conf_file_name);
    oso *midi_output_device_name = NULL;
    switch (tui->ged->midi_mode.any.type) {
        case Midi_mode_type_null:
            break;
        case Midi_mode_type_osc_bidule:
            // TODO
            break;
#ifdef FEAT_PORTMIDI
        case Midi_mode_type_portmidi: {
            PmError pmerror;
            if (!portmidi_find_name_of_device_id(
                    tui->ged->midi_mode.portmidi.device_id,
                    &pmerror,
                    &midi_output_device_name) ||
                osolen(midi_output_device_name) < 1) {
                osowipe(&midi_output_device_name);
                break;
            }
            ezconf_w_addopt(
                &ez,
                confopts[Confopt_portmidi_output_device],
                Confopt_portmidi_output_device);
            break;
        }
#endif
    }
    // Add all conf items
    // NOPE ---touched by user that we want---
    // to write to config file.
    // "Touched" items include conf items that were present on disk when we first
    // loaded the config file, plus the items that the user has modified by
    // interacting with the menus.
    for (int i = 0; i < Confoptslen; i++) {
        if (i == Confopt_portmidi_output_device) {
            // This has its own special logic
            continue;
        }
        //        if (tui->prefs_touched & TOUCHFLAG(i))
        ezconf_w_addopt(&ez, confopts[i], i);
    }
    while (ezconf_w_step(&ez)) {
        switch (ez.optid) {
            case Confopt_osc_output_address:
                // Fine to not write anything here
                if (osolen(tui->osc_address))
                    fputs(osoc(tui->osc_address), ez.file);
                break;
            case Confopt_osc_output_port:
                if (osolen(tui->osc_port))
                    fputs(osoc(tui->osc_port), ez.file);
                break;
            case Confopt_osc_output_enabled:
                fputc(tui->osc_output_enabled ? '1' : '0', ez.file);
                break;
#ifdef FEAT_PORTMIDI
            case Confopt_portmidi_output_device:
                fputs(osoc(midi_output_device_name), ez.file);
                break;
#endif
            case Confopt_midi_beat_clock:
                fputc(tui->ged->midi_bclock ? '1' : '0', ez.file);
                break;
            case Confopt_margins:
                fprintf(ez.file, "%dx%d", tui->softmargin_x, tui->softmargin_y);
                break;
            case Confopt_grid_dot_type:
                fputs(tui->fancy_grid_dots ? prefval_fancy : prefval_plain, ez.file);
                break;
            case Confopt_grid_ruler_type:
                fputs(tui->fancy_grid_rulers ? prefval_fancy : prefval_plain, ez.file);
                break;
        }
    }
    osofree(midi_output_device_name);
    if (ez.error) {
        char const *msg = ezconf_w_errorstring(ez.error);
        qmsg_printf_push("Config Error", "Error when writing configuration file:\n%s", msg);
    }
}

bool tui_suggest_nice_grid_size(Tui *tui, int win_h, int win_w, Usz *out_grid_h, Usz *out_grid_w)
{
    int softmargin_y = tui->softmargin_y, softmargin_x = tui->softmargin_x;
    int ruler_spacing_y = (int)tui->ged->ruler_spacing_y, ruler_spacing_x = (int)tui->ged->ruler_spacing_x;
    if (win_h < 1 || win_w < 1 || softmargin_y < 0 || softmargin_x < 0 || ruler_spacing_y < 1 ||
        ruler_spacing_x < 1)
        return false;
    // TODO overflow checks
    int h = (win_h - softmargin_y - Hud_height - 1) / ruler_spacing_y;
    h *= ruler_spacing_y;
    int w = (win_w - softmargin_x * 2 - 1) / ruler_spacing_x;
    w *= ruler_spacing_x;
    if (h < ruler_spacing_y)
        h = ruler_spacing_y;
    if (w < ruler_spacing_x)
        w = ruler_spacing_x;
    h++;
    w++;
    if (h >= ORCA_Y_MAX || w >= ORCA_X_MAX)
        return false;
    *out_grid_h = (Usz)h;
    *out_grid_w = (Usz)w;
    return true;
}

bool tui_suggest_tight_grid_size(Tui *tui, int win_h, int win_w, Usz *out_grid_h, Usz *out_grid_w)
{
    int softmargin_y = tui->softmargin_y, softmargin_x = tui->softmargin_x;
    if (win_h < 1 || win_w < 1 || softmargin_y < 0 || softmargin_x < 0)
        return false;
    // TODO overflow checks
    int h = win_h - softmargin_y - Hud_height;
    int w = win_w - softmargin_x * 2;
    if (h < 1 || w < 1 || h >= ORCA_Y_MAX || w >= ORCA_X_MAX)
        return false;
    *out_grid_h = (Usz)h;
    *out_grid_w = (Usz)w;
    return true;
}


bool tui_restart_osc_udp_if_enabled_diderror(Tui *tui)
{
    bool error = false;
    if (tui->osc_output_enabled && tui->osc_port) {
        error = !ged_set_osc_udp(tui->ged, osoc(tui->osc_address) /* null ok here */, osoc(tui->osc_port));
    } else {
        ged_clear_osc_udp(tui->ged);
    }
    return error;
}
void tui_restart_osc_udp_showerror(void)
{
    qmsg_printf_push("OSC Networking Error", "Failed to set up OSC networking");
}
void tui_restart_osc_udp_if_enabled(Tui *tui)
{
    bool old_inuse = ged_is_using_osc_udp(tui->ged);
    bool did_error = tui_restart_osc_udp_if_enabled_diderror(tui);
    bool new_inuse = ged_is_using_osc_udp(tui->ged);
    if (old_inuse != new_inuse) {
        Qblock *qb = qnav_top_block();
        if (qb && qb->tag == Qblock_type_qmenu && qmenu_id(qmenu_of(qb)) == Osc_menu_id) {
            int itemid = qmenu_current_item(qmenu_of(qb));
            qnav_stack_pop();
            push_osc_menu(new_inuse);
            qmenu_set_current_item(qmenu_of(qnav_top_block()), itemid);
        }
    }
    if (did_error)
        tui_restart_osc_udp_showerror();
}
void tui_adjust_term_size(Tui *tui, WINDOW **cont_window)
{
    int term_h, term_w;
    getmaxyx(stdscr, term_h, term_w);
    assert(term_h >= 0 && term_w >= 0);
    int content_y = 0, content_x = 0;
    int content_h = term_h, content_w = term_w;
    if (tui->hardmargin_y > 0 && term_h > tui->hardmargin_y * 2 + 2) {
        content_y += tui->hardmargin_y;
        content_h -= tui->hardmargin_y * 2;
    }
    if (tui->hardmargin_x > 0 && term_w > tui->hardmargin_x * 2 + 2) {
        content_x += tui->hardmargin_x;
        content_w -= tui->hardmargin_x * 2;
    }
    bool remake_window = true;
    if (*cont_window) {
        int cwin_y, cwin_x, cwin_h, cwin_w;
        getbegyx(*cont_window, cwin_y, cwin_x);
        getmaxyx(*cont_window, cwin_h, cwin_w);
        remake_window = cwin_y != content_y || cwin_x != content_x || cwin_h != content_h ||
                        cwin_w != content_w;
    }
    if (remake_window) {
        if (*cont_window)
            delwin(*cont_window);
        wclear(stdscr);
        *cont_window = derwin(stdscr, content_h, content_w, content_y, content_x);
        tui->ged->is_draw_dirty = true;
    }
    // OK to call this unconditionally -- deriving the sub-window areas is
    // more than a single comparison, and we don't want to split up or
    // duplicate the math and checks for it, so this routine will calculate
    // the stuff it needs to and then early-out if there's no further work.
    ged_set_window_size(tui->ged, content_h, content_w, tui->softmargin_y, tui->softmargin_x);
}

void tui_try_save(Tui *tui)
{
    if (osolen(tui->file_name) > 0)
        try_save_with_msg(&tui->ged->field, tui->file_name);
    else
        push_save_as_form("");
}


void pop_qnav_if_main_menu(void)
{
    Qblock *qb = qnav_top_block();
    if (qb && qb->tag == Qblock_type_qmenu && qmenu_id(qmenu_of(qb)) == Main_menu_id)
        qnav_stack_pop();
}

void tui_plainorfancy_menu_was_picked(Tui *tui, int picked_id, bool *p_is_fancy, U32 pref_touch_flag)
{
    bool is_fancy = picked_id == 1; // 1 -> fancy, 2 -> plain
    qnav_stack_pop();
    // ^- doesn't actually matter when we do this, with our current code
    if (is_fancy == *p_is_fancy)
        return;
    *p_is_fancy = is_fancy;
    tui->prefs_touched |= pref_touch_flag;
    tui_save_prefs(tui);
    tui->ged->is_draw_dirty = true;
}

Tui_menus_result tui_drive_menus(Tui *tui, int key)
{
    Qblock *qb = qnav_top_block();
    if (!qb)
        return Tui_menus_nothing;
    if (key == CTRL_PLUS('q'))
        return Tui_menus_quit;
    switch (qb->tag) {
        case Qblock_type_qmsg: {
            Qmsg *qm = qmsg_of(qb);
            Qmsg_action act;
            if (qmsg_drive(qm, key, &act)) {
                if (act.dismiss)
                    qnav_stack_pop();
                if (act.passthrough)
                    return Tui_menus_nothing;
            }
            break;
        }
        case Qblock_type_qmenu: {
            Qmenu *qm = qmenu_of(qb);
            Qmenu_action act;
            // special case for main menu: pressing the key to open it will close
            // it again.
            if (qmenu_id(qm) == Main_menu_id && (key == CTRL_PLUS('d') || key == KEY_F(1))) {
                qnav_stack_pop();
                break;
            }
            if (!qmenu_drive(qm, key, &act))
                break;
            switch (act.any.type) {
                case Qmenu_action_type_canceled:
                    qnav_stack_pop();
                    break;
                case Qmenu_action_type_picked:
                    switch (qmenu_id(qm)) {
                        case Main_menu_id:
                            switch (act.picked.id) {
                                case Main_menu_quit:
                                    return Tui_menus_quit;
                                case Main_menu_playback:
                                    push_playback_menu(tui->ged->midi_bclock);
                                    break;
                                case Main_menu_cosmetics:
                                    push_cosmetics_menu();
                                    break;
                                case Main_menu_osc:
                                    push_osc_menu(ged_is_using_osc_udp(tui->ged));
                                    break;
                                case Main_menu_controls:
                                    push_controls_msg();
                                    break;
                                case Main_menu_opers_guide:
                                    push_opers_guide_msg();
                                    break;
                                case Main_menu_about:
                                    push_about_msg();
                                    break;
                                case Main_menu_new:
                                    push_confirm_new_file_menu();
                                    break;
                                case Main_menu_open:
                                    push_open_form(osoc(tui->file_name));
                                    break;
                                case Main_menu_save:
                                    tui_try_save(tui);
                                    break;
                                case Main_menu_save_as:
                                    push_save_as_form(osoc(tui->file_name));
                                    break;
                                case Main_menu_set_tempo:
                                    push_set_tempo_form(tui->ged->bpm);
                                    break;
                                case Main_menu_set_grid_dims:
                                    push_set_grid_dims_form(tui->ged->field.height, tui->ged->field.width);
                                    break;
                                case Main_menu_autofit_grid:
                                    push_autofit_menu();
                                    break;
#ifdef FEAT_PORTMIDI
                                case Main_menu_choose_portmidi_output:
                                    push_portmidi_output_device_menu(&tui->ged->midi_mode);
                                    break;
#endif
                            }
                            break;
                        case Autofit_menu_id: {
                            Usz new_field_h, new_field_w;
                            bool did_get_ok_size = false;
                            switch (act.picked.id) {
                                case Autofit_nicely_id:
                                    did_get_ok_size = tui_suggest_nice_grid_size(
                                        tui,
                                        tui->ged->win_h,
                                        tui->ged->win_w,
                                        &new_field_h,
                                        &new_field_w);
                                    break;
                                case Autofit_tightly_id:
                                    did_get_ok_size = tui_suggest_tight_grid_size(
                                        tui,
                                        tui->ged->win_h,
                                        tui->ged->win_w,
                                        &new_field_h,
                                        &new_field_w);
                                    break;
                            }
                            if (did_get_ok_size) {
                                ged_resize_grid(
                                    &tui->ged->field,
                                    &tui->ged->mbuf_r,
                                    new_field_h,
                                    new_field_w,
                                    tui->ged->tick_num,
                                    &tui->ged->scratch_field,
                                    &tui->ged->undo_hist,
                                    &tui->ged->ged_cursor);
                                ged_update_internal_geometry(tui->ged);
                                tui->ged->needs_remarking = true;
                                tui->ged->is_draw_dirty = true;
                                ged_make_cursor_visible(tui->ged);
                            }
                            qnav_stack_pop();
                            pop_qnav_if_main_menu();
                            break;
                        }
                        case Confirm_new_file_menu_id:
                            switch (act.picked.id) {
                                case Confirm_new_file_reject_id:
                                    qnav_stack_pop();
                                    break;
                                case Confirm_new_file_accept_id: {
                                    Usz new_field_h, new_field_w;
                                    if (tui_suggest_nice_grid_size(
                                            tui,
                                            tui->ged->win_h,
                                            tui->ged->win_w,
                                            &new_field_h,
                                            &new_field_w)) {
                                        undo_history_push(&tui->ged->undo_hist, &tui->ged->field, tui->ged->tick_num);
                                        field_resize_raw(&tui->ged->field, new_field_h, new_field_w);
                                        memset(
                                            tui->ged->field.buffer,
                                            '.',
                                            new_field_h * new_field_w * sizeof(Glyph));
                                        ged_cursor_confine(&tui->ged->ged_cursor, new_field_h, new_field_w);
                                        mbuf_reusable_ensure_size(&tui->ged->mbuf_r, new_field_h, new_field_w);
                                        ged_update_internal_geometry(tui->ged);
                                        ged_make_cursor_visible(tui->ged);
                                        tui->ged->needs_remarking = true;
                                        tui->ged->is_draw_dirty = true;
                                        osoclear(&tui->file_name);
                                        qnav_stack_pop();
                                        pop_qnav_if_main_menu();
                                    }
                                    break;
                                }
                            }
                            break;
                        case Cosmetics_menu_id:
                            switch (act.picked.id) {
                                case Cosmetics_soft_margins_id:
                                    push_soft_margins_form(tui->softmargin_y, tui->softmargin_x);
                                    break;
                                case Cosmetics_grid_dots_id:
                                    push_plainorfancy_menu(
                                        Set_fancy_grid_dots_menu_id,
                                        "Grid Dots",
                                        tui->fancy_grid_dots);
                                    break;
                                case Cosmetics_grid_rulers_id:
                                    push_plainorfancy_menu(
                                        Set_fancy_grid_rulers_menu_id,
                                        "Grid Rulers",
                                        tui->fancy_grid_rulers);
                                    break;
                            }
                            break;
                        case Playback_menu_id:
                            switch (act.picked.id) {
                                case Playback_menu_midi_bclock: {
                                    bool new_enabled = !tui->ged->midi_bclock;
                                    tui->ged->midi_bclock = new_enabled;
                                    if (tui->ged->is_playing) {
                                        int msgbyte = new_enabled ? 0xFA /* start */ : 0xFC /* stop */;
                                        send_midi_byte(tui->ged->oosc_dev, &tui->ged->midi_mode, msgbyte);
                                        // TODO timing judder will be experienced here, because the
                                        // deadline calculation conditions will have been changed by
                                        // toggling the midi_bclock flag. We would have to transfer the
                                        // current remaining time from the reference clock point into the
                                        // accum time, and mutiply or divide it.
                                    }
                                    tui->prefs_touched |= TOUCHFLAG(Confopt_midi_beat_clock);
                                    qnav_stack_pop();
                                    push_playback_menu(new_enabled);
                                    tui_save_prefs(tui);
                                    break;
                                }
                            }
                            break;
                        case Set_fancy_grid_dots_menu_id:
                            tui_plainorfancy_menu_was_picked(
                                tui,
                                act.picked.id,
                                &tui->fancy_grid_dots,
                                TOUCHFLAG(Confopt_grid_dot_type));
                            break;
                        case Set_fancy_grid_rulers_menu_id:
                            tui_plainorfancy_menu_was_picked(
                                tui,
                                act.picked.id,
                                &tui->fancy_grid_rulers,
                                TOUCHFLAG(Confopt_grid_ruler_type));
                            break;
                        case Osc_menu_id:
                            switch (act.picked.id) {
                                case Osc_menu_output_enabledisable: {
                                    qnav_stack_pop();
                                    tui->osc_output_enabled = !ged_is_using_osc_udp(tui->ged);
                                    // Funny dance to keep the qnav stack in good order
                                    bool diderror = tui_restart_osc_udp_if_enabled_diderror(tui);
                                    push_osc_menu(ged_is_using_osc_udp(tui->ged));
                                    if (diderror) {
                                        tui->osc_output_enabled = false;
                                        tui_restart_osc_udp_showerror();
                                    }
                                    tui->prefs_touched |= TOUCHFLAG(Confopt_osc_output_enabled);
                                    tui_save_prefs(tui);
                                    break;
                                }
                                case Osc_menu_output_address:
                                    push_osc_output_address_form(osoc(tui->osc_address) /* null ok */);
                                    break;
                                case Osc_menu_output_port:
                                    push_osc_output_port_form(osoc(tui->osc_port) /* null ok */);
                                    break;
                            }
                            break;
#ifdef FEAT_PORTMIDI
                        case Portmidi_output_device_menu_id: {
                            ged_stop_all_sustained_notes(tui->ged);
                            midi_mode_deinit(&tui->ged->midi_mode);
                            PmError pme = midi_mode_init_portmidi(&tui->ged->midi_mode, act.picked.id);
                            qnav_stack_pop();
                            if (pme) {
                                qmsg_printf_push(
                                    "PortMidi Error",
                                    "Error setting PortMidi output device:\n%s",
                                    Pm_GetErrorText(pme));
                            } else {
                                tui_save_prefs(tui);
                            }
                            break;
                        }
#endif
                    }
                    break;
            }
            break;
        }
        case Qblock_type_qform: {
            Qform *qf = qform_of(qb);
            Qform_action act;
            if (qform_drive(qf, key, &act)) {
                switch (act.any.type) {
                    case Qform_action_type_canceled:
                        qnav_stack_pop();
                        break;
                    case Qform_action_type_submitted: {
                        switch (qform_id(qf)) {
                            case Open_form_id: {
                                oso *temp_name = qform_get_nonempty_single_line_input(qf);
                                if (!temp_name)
                                    break;
                                expand_home_tilde(&temp_name);
                                if (!temp_name)
                                    break;
                                bool added_hist = undo_history_push(
                                    &tui->ged->undo_hist,
                                    &tui->ged->field,
                                    tui->ged->tick_num);
                                Field_load_error fle = field_load_file(osoc(temp_name), &tui->ged->field);
                                if (fle == Field_load_error_ok) {
                                    qnav_stack_pop();
                                    osoputoso(&tui->file_name, temp_name);
                                    mbuf_reusable_ensure_size(
                                        &tui->ged->mbuf_r,
                                        tui->ged->field.height,
                                        tui->ged->field.width);
                                    ged_cursor_confine(
                                        &tui->ged->ged_cursor,
                                        tui->ged->field.height,
                                        tui->ged->field.width);
                                    ged_update_internal_geometry(tui->ged);
                                    ged_make_cursor_visible(tui->ged);
                                    tui->ged->needs_remarking = true;
                                    tui->ged->is_draw_dirty = true;
                                    pop_qnav_if_main_menu();
                                } else {
                                    if (added_hist)
                                        undo_history_pop(&tui->ged->undo_hist, &tui->ged->field, &tui->ged->tick_num);
                                    qmsg_printf_push(
                                        "Error Loading File",
                                        "%s:\n%s",
                                        osoc(temp_name),
                                        field_load_error_string(fle));
                                }
                                osofree(temp_name);
                                break;
                            }
                            case Save_as_form_id: {
                                oso *temp_name = qform_get_nonempty_single_line_input(qf);
                                if (!temp_name)
                                    break;
                                qnav_stack_pop();
                                bool saved_ok = try_save_with_msg(&tui->ged->field, temp_name);
                                if (saved_ok)
                                    osoputoso(&tui->file_name, temp_name);
                                osofree(temp_name);
                                break;
                            }
                            case Set_tempo_form_id: {
                                oso *tmpstr = qform_get_nonempty_single_line_input(qf);
                                if (!tmpstr)
                                    break;
                                int newbpm = atoi(osoc(tmpstr));
                                if (newbpm > 0) {
                                    tui->ged->bpm = (Usz)newbpm;
                                    qnav_stack_pop();
                                }
                                osofree(tmpstr);
                                break;
                            }
                            case Osc_output_address_form_id: {
                                oso *addr = NULL;
                                // Empty string is OK here
                                if (qform_get_single_text_line(qf, &addr)) {
                                    if (osolen(addr))
                                        ososwap(&tui->osc_address, &addr);
                                    else
                                        osowipe(&tui->osc_address);
                                    qnav_stack_pop();
                                    tui_restart_osc_udp_if_enabled(tui);
                                    tui->prefs_touched |= TOUCHFLAG(Confopt_osc_output_address);
                                    tui_save_prefs(tui);
                                }
                                osofree(addr);
                                break;
                            }
                            case Osc_output_port_form_id: {
                                oso *portstr = qform_get_nonempty_single_line_input(qf);
                                if (!portstr)
                                    break;
                                qnav_stack_pop();
                                ososwap(&tui->osc_port, &portstr);
                                tui_restart_osc_udp_if_enabled(tui);
                                osofree(portstr);
                                tui->prefs_touched |= TOUCHFLAG(Confopt_osc_output_port);
                                tui_save_prefs(tui);
                                break;
                            }
                            case Set_grid_dims_form_id: {
                                oso *tmpstr = qform_get_nonempty_single_line_input(qf);
                                if (!tmpstr)
                                    break;
                                int newheight, newwidth;
                                if (sscanf(osoc(tmpstr), "%dx%d", &newwidth, &newheight) == 2 &&
                                    newheight > 0 && newwidth > 0 && newheight < ORCA_Y_MAX &&
                                    newwidth < ORCA_X_MAX) {
                                    if (tui->ged->field.height != (Usz)newheight ||
                                        tui->ged->field.width != (Usz)newwidth) {
                                        ged_resize_grid(
                                            &tui->ged->field,
                                            &tui->ged->mbuf_r,
                                            (Usz)newheight,
                                            (Usz)newwidth,
                                            tui->ged->tick_num,
                                            &tui->ged->scratch_field,
                                            &tui->ged->undo_hist,
                                            &tui->ged->ged_cursor);
                                        ged_update_internal_geometry(tui->ged);
                                        tui->ged->needs_remarking = true;
                                        tui->ged->is_draw_dirty = true;
                                        ged_make_cursor_visible(tui->ged);
                                    }
                                    qnav_stack_pop();
                                }
                                osofree(tmpstr);
                                break;
                            }
                            case Set_soft_margins_form_id: {
                                oso *tmpstr = qform_get_nonempty_single_line_input(qf);
                                if (!tmpstr)
                                    break;
                                bool do_save = false;
                                int newy, newx;
                                if (read_nxn_or_n(osoc(tmpstr), &newx, &newy) && newy >= 0 &&
                                    newx >= 0 &&
                                    (newy != tui->softmargin_y || newx != tui->softmargin_x)) {
                                    tui->prefs_touched |= TOUCHFLAG(Confopt_margins);
                                    tui->softmargin_y = newy;
                                    tui->softmargin_x = newx;
                                    ungetch(KEY_RESIZE); // kinda lame but whatever
                                    do_save = true;
                                }
                                qnav_stack_pop();
                                // Might push message, so gotta pop old guy first
                                if (do_save)
                                    tui_save_prefs(tui);
                                osofree(tmpstr);
                                break;
                            }
                        }
                        break;
                    }
                }
            }
            break;
        }
    }
    return Tui_menus_consumed_input;
}


