#include "tui.h"
#include "ged.h"
#include "base.h"
#include "field.h"
#include "gbuffer.h"
#include "oso.h"
#include "sysmisc.h"
#include "term_util.h"
#include "midi.h"
#include <getopt.h>
#include <locale.h>

#define SOKOL_IMPL
#include "sokol_time.h"
#undef SOKOL_IMPL


#define TIME_DEBUG 0
#if TIME_DEBUG
static int spin_track_timeout = 0;
#endif


staticni void usage(void)
{ // clang-format off
fprintf(stderr,
"Usage: orca [options] [file]\n\n"
"General options:\n"
"    --undo-limit <number>  Set the maximum number of undo steps.\n"
"                           If you plan to work with large files,\n"
"                           set this to a low number.\n"
"                           Default: 100\n"
"    --initial-size <nxn>   When creating a new grid file, use these\n"
"                           starting dimensions.\n"
"    --bpm <number>         Set the tempo (beats per minute).\n"
"                           Default: 120\n"
"    --seed <number>        Set the seed for the random function.\n"
"                           Default: 1\n"
"    -h or --help           Print this message and exit.\n"
"\n"
"OSC/MIDI options:\n"
"    --strict-timing\n"
"        Attempt to reduce timing jitter of outgoing MIDI and OSC\n"
"        messages. Uses more CPU time. May have no effect.\n"
"\n"
"    --osc-midi-bidule <path>\n"
"        Set MIDI to be sent via OSC formatted for Plogue Bidule.\n"
"        The path argument is the path of the Plogue OSC MIDI device.\n"
"        Example: /OSC_MIDI_0/MIDI\n"
);} // clang-format on


staticni bool read_int(char const *str, int *out)
{
    int a;
    int res = sscanf(str, "%d", &a);
    if (res != 1)
        return false;
    *out = a;
    return true;
}

typedef enum
{
    Brackpaste_seq_none = 0,
    Brackpaste_seq_begin,
    Brackpaste_seq_end,
} Brackpaste_seq;

// Try to getch to complete the sequence of a start or end of brackpet paste
// escape sequence. If it doesn't turn out to be one, unwind it back into the
// event queue with ungetch. Yeah this is golfed let me have fun.
staticni Brackpaste_seq brackpaste_seq_getungetch(WINDOW *win)
{
    int chs[5], n = 0, begorend; // clang-format off
  if ((chs[n++] = wgetch(win)) != '[') goto unwind;
  if ((chs[n++] = wgetch(win)) != '2') goto unwind;
  if ((chs[n++] = wgetch(win)) != '0') goto unwind;
  chs[n++] = begorend = wgetch(win);
  if (begorend != '0' && begorend != '1') goto unwind;
  if ((chs[n++] = wgetch(win)) != '~') goto unwind;
  return begorend == '0' ? Brackpaste_seq_begin : Brackpaste_seq_end;
unwind:
  while (n > 0) ungetch(chs[--n]);
  return Brackpaste_seq_none; // clang-format on
}

staticni void try_send_to_gui_clipboard(Ged const *a, bool *io_use_gui_clipboard)
{
    if (!*io_use_gui_clipboard)
        return;
#if 0 // If we want to use grid directly
  Usz curs_y, curs_x, curs_h, curs_w;
  if (!ged_try_selection_clipped_to_field(a, &curs_y, &curs_x, &curs_h,
                                          &curs_w))
    return;
  Cboard_error cberr =
      cboard_copy(a->clipboard_field.buffer, a->clipboard_field.height,
                  a->clipboard_field.width, curs_y, curs_x, curs_h, curs_w);
#endif
    Usz cb_h = a->clipboard_field.height, cb_w = a->clipboard_field.width;
    if (cb_h < 1 || cb_w < 1)
        return;
    Cboard_error cberr = cboard_copy(a->clipboard_field.buffer, cb_h, cb_w, 0, 0, cb_h, cb_w);
    if (cberr)
        *io_use_gui_clipboard = false;
}

enum
{
    Argopt_hardmargins = UCHAR_MAX + 1,
    Argopt_undo_limit,
    Argopt_init_grid_size,
    Argopt_osc_midi_bidule,
    Argopt_strict_timing,
    Argopt_bpm,
    Argopt_seed,
    Argopt_portmidi_deprecated,
    Argopt_osc_deprecated,
};

Ged ged;
WINDOW *window_main = NULL;
Tui tui;

void main_init(int argc, char **argv)
{
    static struct option tui_options[] = {
        { "hard-margins", required_argument, 0, Argopt_hardmargins },
        { "undo-limit", required_argument, 0, Argopt_undo_limit },
        { "initial-size", required_argument, 0, Argopt_init_grid_size },
        { "help", no_argument, 0, 'h' },
        { "osc-midi-bidule", required_argument, 0, Argopt_osc_midi_bidule },
        { "strict-timing", no_argument, 0, Argopt_strict_timing },
        { "bpm", required_argument, 0, Argopt_bpm },
        { "seed", required_argument, 0, Argopt_seed },
        { "portmidi-list-devices", no_argument, 0, Argopt_portmidi_deprecated },
        { "portmidi-output-device", required_argument, 0, Argopt_portmidi_deprecated },
        { "osc-server", required_argument, 0, Argopt_osc_deprecated },
        { "osc-port", required_argument, 0, Argopt_osc_deprecated },
        { NULL, 0, NULL, 0 }
    };
    int init_bpm = 120;
    int init_seed = 1;
    int init_grid_dim_y = 25, init_grid_dim_x = 57;
    bool explicit_initial_grid_size = false;

    tui_init(&tui, &ged);

    int longindex = 0;

#define OPTFAIL(...)                                                                               \
    {                                                                                              \
        fprintf(stderr, "Bad %s argument: %s\n", tui_options[longindex].name, optarg);             \
        fprintf(stderr, __VA_ARGS__);                                                              \
        fputc('\n', stderr);                                                                       \
        exit(1);                                                                                   \
    }

    for (;;) {
        int c = getopt_long(argc, argv, "h", tui_options, &longindex);
        if (c == -1)
            break;
        switch (c) {
            case 'h':
                usage();
                exit(0);
            case '?':
                usage();
                exit(1);
            case Argopt_hardmargins:
                if (read_nxn_or_n(optarg, &tui.hardmargin_x, &tui.hardmargin_y) &&
                    tui.hardmargin_x >= 0 && tui.hardmargin_y >= 0)
                    break;
                OPTFAIL("Must be 0 or positive integer.");
            case Argopt_undo_limit:
                if (read_int(optarg, &tui.undo_history_limit) && tui.undo_history_limit >= 0)
                    break;
                OPTFAIL("Must be 0 or positive integer.");
            case Argopt_bpm:
                if (read_int(optarg, &init_bpm) && init_bpm >= 1)
                    break;
                OPTFAIL("Must be positive integer.");
            case Argopt_seed:
                if (read_int(optarg, &init_seed) && init_seed >= 0)
                    break;
                OPTFAIL("Must be 0 or positive integer.");
            case Argopt_init_grid_size:
                if (sscanf(optarg, "%dx%d", &init_grid_dim_x, &init_grid_dim_y) != 2)
                    OPTFAIL("Bad format or count. Expected something like: 40x30");
                if (init_grid_dim_x <= 0 || init_grid_dim_x > ORCA_X_MAX)
                    OPTFAIL(
                        "X dimension for initial-size must be 1 <= n <= %d, was %d.",
                        ORCA_X_MAX,
                        init_grid_dim_x);
                if (init_grid_dim_y <= 0 || init_grid_dim_y > ORCA_Y_MAX)
                    OPTFAIL(
                        "Y dimension for initial-size must be 1 <= n <= %d, was %d.",
                        ORCA_Y_MAX,
                        init_grid_dim_y);
                explicit_initial_grid_size = true;
                break;
            case Argopt_osc_midi_bidule:
                osoput(&tui.osc_midi_bidule_path, optarg);
                break;
            case Argopt_strict_timing:
                tui.strict_timing = true;
                break;
            case Argopt_portmidi_deprecated:
                fprintf(
                    stderr,
                    "Option \"--%s\" has been removed.\nInstead, choose "
                    "your MIDI output device from within the ORCA menu.\n"
                    "This new menu allows you to pick your MIDI device "
                    "interactively\n",
                    tui_options[longindex].name);
                exit(1);
            case Argopt_osc_deprecated:
                fprintf(
                    stderr,
                    "Options --osc-server and --osc-port have been removed.\n"
                    "Instead, set the OSC server and port from within the ORCA menu.\n");
                exit(1);
        }
    }
#undef OPTFAIL

    if (optind == argc - 1) {
        osoput(&tui.file_name, argv[optind]);
    } else if (optind < argc - 1) {
        fprintf(stderr, "Expected only 1 file argument.\n");
        exit(1);
    }
    qnav_init(); // Initialize the menu/navigation global state
    // Initialize the 'Grid EDitor' stuff. This sits underneath the TUI.

    ged_init(&ged, (Usz)tui.undo_history_limit, (Usz)init_bpm, (Usz)init_seed);
    // This will need to be changed to work with conf/menu
    if (osolen(tui.osc_midi_bidule_path) > 0) {
        midi_mode_deinit(&ged.midi_mode);
        midi_mode_init_osc_bidule(&ged.midi_mode, osoc(tui.osc_midi_bidule_path));
    }
    stm_setup(); // Set up timer lib
    // Enable UTF-8 by explicitly initializing our locale before initializing
    // ncurses. Only needed (maybe?) if using libncursesw/wide-chars or UTF-8.
    // Using it unguarded will mess up box drawing chars in Linux virtual
    // consoles unless using libncursesw.
    setlocale(LC_ALL, "");
    initscr(); // Initialize ncurses
    // Allow ncurses to control newline translation. Fine to use with any modern
    // terminal, and will let ncurses run faster.
    nonl();
    // Set interrupt keys (interrupt, break, quit...) to not flush. Helps keep
    // ncurses state consistent, at the cost of less responsive terminal
    // interrupt. (This will rarely happen.)
    intrflush(stdscr, FALSE);
    // Receive keyboard input immediately without line buffering, and receive
    // ctrl+z, ctrl+c etc. as input instead of having a signal generated. We need
    // to do this even with wtimeout() if we don't want ctrl+z etc. to interrupt
    // the program.
    raw();
    noecho();                // Don't echo keyboard input
    keypad(stdscr, TRUE);    // Also receive arrow keys, etc.
    curs_set(0);             // Hide the terminal cursor
    set_escdelay(1);         // Short delay before triggering escape
    term_util_init_colors(); // Our color init routine
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
    if (has_mouse()) // no waiting for distinguishing click from press
        mouseinterval(0);
    printf("\033[?2004h\n"); // Ask terminal to use bracketed paste.

    tui_load_conf(&tui);                  // load orca.conf (if it exists)
    tui_restart_osc_udp_if_enabled(&tui); // start udp if conf enabled it

    wtimeout(stdscr, 0);

    tui_adjust_term_size(&tui, &window_main);

    bool grid_initialized = false;
    if (osolen(tui.file_name)) {
        Field_load_error fle = field_load_file(osoc(tui.file_name), &ged.field);
        switch (fle) {
            case Field_load_error_ok:
                if (ged.field.height < 1 || ged.field.width < 1) {
                    // Opening an empty file or attempting to open a directory can lead us
                    // here.
                    field_deinit(&ged.field);
                    qmsg_printf_push("Unusable File", "Not a usable file:\n%s", (osoc(tui.file_name)));
                    break;
                }
                grid_initialized = true;
                break;
            case Field_load_error_cant_open_file: {
                // Probably a new file, though TODO we should add an explicit
                // differentiation between "file exists and can't open it" and "file
                // doesn't seem to exist."
                Qmsg *qm = qmsg_printf_push(NULL, "New file: %s", osoc(tui.file_name));
                qmsg_set_dismiss_mode(qm, Qmsg_dismiss_mode_passthrough);
                break;
            }
            default:
                qmsg_printf_push(
                    "File Load Error",
                    "File load error:\n%s.",
                    field_load_error_string(fle));
                break;
        }
    }
    // If we haven't yet initialized the grid, because we were waiting for the
    // terminal size, do it now.
    if (!grid_initialized) {
        Usz new_field_h, new_field_w;
        if (explicit_initial_grid_size ||
            !tui_suggest_nice_grid_size(&tui, ged.win_h, ged.win_w, &new_field_h, &new_field_w)) {
            new_field_h = (Usz)init_grid_dim_y;
            new_field_w = (Usz)init_grid_dim_x;
        }
        field_init_fill(&ged.field, (Usz)new_field_h, (Usz)new_field_w, '.');
    }
    mbuf_reusable_ensure_size(&ged.mbuf_r, ged.field.height, ged.field.width);
    ged_make_cursor_visible(&ged);
    ged_send_osc_bpm(&ged, (I32)ged.bpm); // Send initial BPM
    ged_set_playing(&ged, true);          // Auto-play
}

int main(int argc, char **argv)
{
    main_init(argc, argv);

    int cur_timeout = 0;
    bool is_in_brackpaste = false;
    Usz brackpaste_starting_x = 0;
    Usz brackpaste_y = 0;
    Usz brackpaste_x = 0;
    Usz brackpaste_max_y = 0;
    Usz brackpaste_max_x = 0;

event_loop:;
    int key = wgetch(stdscr);
    if (cur_timeout != 0) {
        wtimeout(stdscr, 0); // Until we run out, don't wait between events.
        cur_timeout = 0;
    }
    switch (key) {
        case ERR: { // ERR indicates no more events.
            ged_do_stuff(&ged);
            bool drew_any = false;
            if (ged_is_draw_dirty(&ged) || qnav_stack.occlusion_dirty) {
                werase(window_main);
                ged_draw(
                    &ged,
                    window_main,
                    osoc(tui.file_name),
                    tui.fancy_grid_dots,
                    tui.fancy_grid_rulers);
                wnoutrefresh(window_main);
                drew_any = true;
            }
            drew_any |= qnav_draw(); // clears qnav_stack.occlusion_dirty
            if (drew_any)
                doupdate();
            double secs_to_d = ged_secs_to_deadline(&ged);

// clang-format off
#define DEADTIME(_millisecs, _new_timeout)                                                         \
    else if (secs_to_d < ms_to_sec(_millisecs)) new_timeout = _new_timeout;

            int new_timeout;
            // These values are tuned to work OK with the normal scheduling behavior
            // on Linux, Mac, and Windows. All of the usual caveats of trying to
            // guess what the scheduler will do apply.
            //
            // Of course, there's no guarantee about how the scheduler will work, so
            // if you are using a modified kernel or something, or the measurements
            // here are bad, or it's some OS that behaves differently than expected,
            // this won't be very good. But there's not really much we can do about
            // it, and it's better than doing nothing and burning up the CPU!

            if (tui.strict_timing) {
              if (false) {}
              // "If there's less than 1.5 milliseconds to the deadline, use a curses
              // timeout value of 0."
              DEADTIME(  1.5,  0)
              DEADTIME(  3.0,  1)
              DEADTIME(  5.0,  2)
              DEADTIME(  7.0,  3)
              DEADTIME(  9.0,  4)
              DEADTIME( 11.0,  5)
              DEADTIME( 13.0,  6)
              DEADTIME( 15.0,  7)
              DEADTIME( 25.0, 12)
              DEADTIME( 50.0, 20)
              DEADTIME(100.0, 40)
              else new_timeout = 50;
            } else {
              if (false) {}
              DEADTIME(  1.0,  0)
              DEADTIME(  2.0,  1)
              DEADTIME(  7.0,  2)
              DEADTIME( 15.0,  5)
              DEADTIME( 25.0, 10)
              DEADTIME( 50.0, 20)
              DEADTIME(100.0, 40)
              else new_timeout = 50;
            }
#undef DEADTIME

            if (new_timeout != cur_timeout) {
                wtimeout(stdscr, new_timeout);
                cur_timeout = new_timeout;
        #if TIME_DEBUG
                spin_track_timeout = cur_timeout;
        #endif
            }
            goto event_loop;
        }
        // END Case: ERR
        // clang-format on
        case KEY_RESIZE:
            tui_adjust_term_size(&tui, &window_main);
            qnav_adjust_term_size();
            goto event_loop;
#ifndef FEAT_NOMOUSE
        case KEY_MOUSE: {
            MEVENT mevent;
            if (window_main && getmouse(&mevent) == OK) {
                int win_y, win_x;
                int win_h, win_w;
                getbegyx(window_main, win_y, win_x);
                getmaxyx(window_main, win_h, win_w);
                int inwin_y = mevent.y - win_y;
                int inwin_x = mevent.x - win_x;
                if (inwin_y >= win_h)
                    inwin_y = win_h - 1;
                if (inwin_y < 0)
                    inwin_y = 0;
                if (inwin_x >= win_w)
                    inwin_x = win_w - 1;
                if (inwin_x < 0)
                    inwin_x = 0;
                ged_mouse_event(&ged, (Usz)inwin_y, (Usz)inwin_x, mevent.bstate);
            }
            goto event_loop;
        }
#endif
    }

    // If we have the menus open, we'll let the menus do what they want with
    // the input before the regular editor (which will be displayed
    // underneath.) The menus may tell us to quit, that they didn't do anything
    // with the input, or that they consumed the input and therefore we
    // shouldn't pass the input key to the rest of the editing system.
    switch (tui_drive_menus(&tui, key)) {
        case Tui_menus_nothing:
            break;
        case Tui_menus_quit:
            goto quit;
        case Tui_menus_consumed_input:
            goto event_loop;
    }

    // If this key input is intended to reach the grid, check to see if we're
    // in bracketed paste and use alternate 'filtered input for characters'
    // mode. We'll ignore most control sequences here.
    if (is_in_brackpaste) {
        if (key == 27 /* escape */) {
            if (brackpaste_seq_getungetch(stdscr) == Brackpaste_seq_end) {
                is_in_brackpaste = false;
                if (brackpaste_max_y > ged.ged_cursor.y)
                    ged.ged_cursor.h = brackpaste_max_y - ged.ged_cursor.y + 1;
                if (brackpaste_max_x > ged.ged_cursor.x)
                    ged.ged_cursor.w = brackpaste_max_x - ged.ged_cursor.x + 1;
                ged.needs_remarking = true;
                ged.is_draw_dirty = true;
            }
            goto event_loop;
        }
        if (key == KEY_ENTER)
            key = '\r';
        if (key >= CHAR_MIN && key <= CHAR_MAX) {
            if ((char)key == '\r' || (char)key == '\n') {
                brackpaste_x = brackpaste_starting_x;
                ++brackpaste_y;
                goto event_loop;
            }
            if (key != ' ') {
                char cleaned = (char)key;
                if (!orca_is_valid_glyph((Glyph)key))
                    cleaned = '.';
                if (brackpaste_y < ged.field.height && brackpaste_x < ged.field.width) {
                    gbuffer_poke(
                        ged.field.buffer,
                        ged.field.height,
                        ged.field.width,
                        brackpaste_y,
                        brackpaste_x,
                        cleaned);
                    // Could move this out one level if we wanted the final selection
                    // size to reflect even the pasted area which didn't fit on the
                    // grid.
                    if (brackpaste_y > brackpaste_max_y)
                        brackpaste_max_y = brackpaste_y;
                    if (brackpaste_x > brackpaste_max_x)
                        brackpaste_max_x = brackpaste_x;
                }
            }
            ++brackpaste_x;
        }
        goto event_loop;
    }

    // Regular inputs when we're not in a menu and not in bracketed paste.
    switch (key) {
        // Checking again for 'quit' here, because it's only listened for if we're
        // in the menus or *not* in bracketed paste mode.
        case CTRL_PLUS('q'):
            goto quit;
        case CTRL_PLUS('o'):
            push_open_form(osoc(tui.file_name));
            break;
        case 127: // backspace in terminal.app, apparently
        case KEY_BACKSPACE:
            if (ged.input_mode == Ged_input_mode_append) {
                ged_dir_input(&ged, Ged_dir_left, 1);
                ged_input_character(&ged, '.');
                ged_dir_input(&ged, Ged_dir_left, 1);
            } else {
                ged_input_character(&ged, '.');
            }
            break;
        case CTRL_PLUS('z'):
        case CTRL_PLUS('u'):
            ged_input_cmd(&ged, Ged_input_cmd_undo);
            break;
        case CTRL_PLUS('r'):
            ged.tick_num = 0;
            ged.needs_remarking = true;
            ged.is_draw_dirty = true;
            break;
        case '[':
            ged_adjust_rulers_relative(&ged, 0, -1);
            break;
        case ']':
            ged_adjust_rulers_relative(&ged, 0, 1);
            break;
        case '{':
            ged_adjust_rulers_relative(&ged, -1, 0);
            break;
        case '}':
            ged_adjust_rulers_relative(&ged, 1, 0);
            break;
        case '(':
            ged_resize_grid_relative(&ged, 0, -1);
            break;
        case ')':
            ged_resize_grid_relative(&ged, 0, 1);
            break;
        case '_':
            ged_resize_grid_relative(&ged, -1, 0);
            break;
        case '+':
            ged_resize_grid_relative(&ged, 1, 0);
            break;
        case '\r':
        case KEY_ENTER:
            break; // Currently unused.
        case CTRL_PLUS('i'):
        case KEY_IC:
            ged_input_cmd(&ged, Ged_input_cmd_toggle_append_mode);
            break;
        case '/':
            // Formerly 'piano'/trigger mode toggle. We're repurposing it here to
            // input a '?' instead of a '/' because '?' opens the help guide, and it
            // might be a bad idea to take that away, since orca will take over the
            // TTY and may leave users confused. I know of at least 1 person who was
            // saved by pressing '?' after they didn't know what to do. Hmm.
            ged_input_character(&ged, '?');
            break;
        case '<':
            ged_adjust_bpm(&ged, -1);
            break;
        case '>':
            ged_adjust_bpm(&ged, 1);
            break;
        case CTRL_PLUS('f'):
            ged_input_cmd(&ged, Ged_input_cmd_step_forward);
            break;
        case CTRL_PLUS('e'):
            ged_input_cmd(&ged, Ged_input_cmd_toggle_show_event_list);
            break;
        case CTRL_PLUS('x'):
            ged_input_cmd(&ged, Ged_input_cmd_cut);
            try_send_to_gui_clipboard(&ged, &tui.use_gui_cboard);
            break;
        case CTRL_PLUS('c'):
            ged_input_cmd(&ged, Ged_input_cmd_copy);
            try_send_to_gui_clipboard(&ged, &tui.use_gui_cboard);
            break;
        case CTRL_PLUS('v'):
            if (tui.use_gui_cboard) {
                bool added_hist = undo_history_push(&ged.undo_hist, &ged.field, ged.tick_num);
                Usz pasted_h, pasted_w;
                Cboard_error cberr = cboard_paste(
                    ged.field.buffer,
                    ged.field.height,
                    ged.field.width,
                    ged.ged_cursor.y,
                    ged.ged_cursor.x,
                    &pasted_h,
                    &pasted_w);
                if (cberr) {
                    if (added_hist)
                        undo_history_pop(&ged.undo_hist, &ged.field, &ged.tick_num);
                    tui.use_gui_cboard = false;
                    ged_input_cmd(&ged, Ged_input_cmd_paste);
                } else {
                    if (pasted_h > 0 && pasted_w > 0) {
                        ged.ged_cursor.h = pasted_h;
                        ged.ged_cursor.w = pasted_w;
                    }
                }
                ged.needs_remarking = true;
                ged.is_draw_dirty = true;
            } else {
                ged_input_cmd(&ged, Ged_input_cmd_paste);
            }
            break;
        case '\'':
            ged_input_cmd(&ged, Ged_input_cmd_toggle_selresize_mode);
            break;
        case '`':
        case '~':
            ged_input_cmd(&ged, Ged_input_cmd_toggle_slide_mode);
            break;
        case ' ':
            if (ged.input_mode == Ged_input_mode_append)
                ged_input_character(&ged, '.');
            else
                ged_input_cmd(&ged, Ged_input_cmd_toggle_play_pause);
            break;
        case 27: // Escape
            // Check for escape sequences we're interested in that ncurses didn't
            // handle. Such as bracketed paste.
            if (brackpaste_seq_getungetch(stdscr) == Brackpaste_seq_begin) {
                is_in_brackpaste = true;
                undo_history_push(&ged.undo_hist, &ged.field, ged.tick_num);
                brackpaste_y = ged.ged_cursor.y;
                brackpaste_x = ged.ged_cursor.x;
                brackpaste_starting_x = brackpaste_x;
                brackpaste_max_y = brackpaste_y;
                brackpaste_max_x = brackpaste_x;
                break;
            }
            ged_input_cmd(&ged, Ged_input_cmd_escape);
            break;

        case 330: // delete?
            ged_input_character(&ged, '.');
            break;

        // Cursor movement
        case KEY_UP:
        case CTRL_PLUS('k'):
            ged_dir_input(&ged, Ged_dir_up, 1);
            break;
        case CTRL_PLUS('j'):
        case KEY_DOWN:
            ged_dir_input(&ged, Ged_dir_down, 1);
            break;
        case CTRL_PLUS('h'):
        case KEY_LEFT:
            ged_dir_input(&ged, Ged_dir_left, 1);
            break;
        case CTRL_PLUS('l'):
        case KEY_RIGHT:
            ged_dir_input(&ged, Ged_dir_right, 1);
            break;

        // Selection size modification. These may not work in all terminals. (Only
        // tested in xterm so far.)
        case 337: // shift-up
            ged_modify_selection_size(&ged, -1, 0);
            break;
        case 336: // shift-down
            ged_modify_selection_size(&ged, 1, 0);
            break;
        case 393: // shift-left
            ged_modify_selection_size(&ged, 0, -1);
            break;
        case 402: // shift-right
            ged_modify_selection_size(&ged, 0, 1);
            break;
        case 567: // shift-control-up
            ged_modify_selection_size(&ged, -(int)ged.ruler_spacing_y, 0);
            break;
        case 526: // shift-control-down
            ged_modify_selection_size(&ged, (int)ged.ruler_spacing_y, 0);
            break;
        case 546: // shift-control-left
            ged_modify_selection_size(&ged, 0, -(int)ged.ruler_spacing_x);
            break;
        case 561: // shift-control-right
            ged_modify_selection_size(&ged, 0, (int)ged.ruler_spacing_x);
            break;

        // Move cursor further if control is held
        case 566: // control-up
            ged_dir_input(&ged, Ged_dir_up, (int)ged.ruler_spacing_y);
            break;
        case 525: // control-down
            ged_dir_input(&ged, Ged_dir_down, (int)ged.ruler_spacing_y);
            break;
        case 545: // control-left
            ged_dir_input(&ged, Ged_dir_left, (int)ged.ruler_spacing_x);
            break;
        case 560: // control-right
            ged_dir_input(&ged, Ged_dir_right, (int)ged.ruler_spacing_x);
            break;

        // Slide selection on alt-arrow
        case 564: // alt-up
            ged_slide_selection(&ged, -1, 0);
            break;
        case 523: // alt-down
            ged_slide_selection(&ged, 1, 0);
            break;
        case 543: // alt-left
            ged_slide_selection(&ged, 0, -1);
            break;
        case 558: // alt-right
            ged_slide_selection(&ged, 0, 1);
            break;

        case CTRL_PLUS('d'):
        case KEY_F(1):
            push_main_menu();
            break;
        case '?':
            push_controls_msg();
            break;
        case CTRL_PLUS('g'):
            push_opers_guide_msg();
            break;
        case CTRL_PLUS('s'):
            tui_try_save(&tui);
            break;

        default:
            if (key >= CHAR_MIN && key <= CHAR_MAX && orca_is_valid_glyph((Glyph)key))
                ged_input_character(&ged, (char)key);
#if 0
      else
        fprintf(stderr, "Unknown key number: %d\n", key);
#endif
            break;
    }
    goto event_loop;
quit:
    ged_stop_all_sustained_notes(&ged);
    qnav_deinit();
    if (window_main)
        delwin(window_main);
#ifndef FEAT_NOMOUSE
    printf("\033[?1003l\n"); // turn off console mouse events if they were active
#endif
    printf("\033[?2004h\n"); // Tell terminal to not use bracketed paste
    endwin();
    ged_deinit(&ged);
    osofree(tui.file_name);
    osofree(tui.osc_address);
    osofree(tui.osc_port);
    osofree(tui.osc_midi_bidule_path);
#ifdef FEAT_PORTMIDI
    //    if (portmidi_is_initialized)
    Pm_Terminate();
#endif
    return 0;
}

#undef TOUCHFLAG
#undef staticni
