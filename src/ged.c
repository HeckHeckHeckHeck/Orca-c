#include "ged.h"
#include "gbuffer.h"
#include "sim.h"
#include "sokol_time.h"

typedef enum
{
    Glyph_class_unknown,
    Glyph_class_grid,
    Glyph_class_comment,
    Glyph_class_uppercase,
    Glyph_class_lowercase,
    Glyph_class_movement,
    Glyph_class_numeric,
    Glyph_class_bang,
} Glyph_class;

static Glyph_class glyph_class_of(Glyph glyph)
{
    if (glyph == '.')
        return Glyph_class_grid;
    if (glyph >= '0' && glyph <= '9')
        return Glyph_class_numeric;
    switch (glyph) {
        case 'N':
        case 'n':
        case 'E':
        case 'e':
        case 'S':
        case 's':
        case 'W':
        case 'w':
            return Glyph_class_movement;
        case '!':
        case ':':
        case ';':
        case '=':
        case '%':
        case '?':
            return Glyph_class_lowercase;
        case '*':
            return Glyph_class_bang;
        case '#':
            return Glyph_class_comment;
    }
    if (glyph >= 'A' && glyph <= 'Z')
        return Glyph_class_uppercase;
    if (glyph >= 'a' && glyph <= 'z')
        return Glyph_class_lowercase;
    return Glyph_class_unknown;
}

static attr_t term_attrs_of_cell(Glyph g, Mark m)
{
    Glyph_class gclass = glyph_class_of(g);
    attr_t attr = A_normal;
    switch (gclass) {
        case Glyph_class_unknown:
            attr = A_bold | fg_bg(C_red, C_natural);
            break;
        case Glyph_class_grid:
            attr = A_bold | fg_bg(C_black, C_natural);
            break;
        case Glyph_class_comment:
            attr = A_dim | Cdef_normal;
            break;
        case Glyph_class_uppercase:
            attr = A_normal | fg_bg(C_black, C_cyan);
            break;
        case Glyph_class_lowercase:
        case Glyph_class_movement:
        case Glyph_class_numeric:
            attr = A_bold | Cdef_normal;
            break;
        case Glyph_class_bang:
            attr = A_bold | Cdef_normal;
            break;
    }
    if (gclass != Glyph_class_comment) {
        if ((m & (Mark_flag_lock | Mark_flag_input)) == (Mark_flag_lock | Mark_flag_input)) {
            // Standard locking input
            attr = A_normal | Cdef_normal;
        } else if ((m & Mark_flag_input) == Mark_flag_input) {
            // Non-locking input
            attr = A_normal | Cdef_normal;
        } else if (m & Mark_flag_lock) {
            // Locked only
            attr = A_dim | Cdef_normal;
        }
    }
    if (m & Mark_flag_output) {
        attr = A_reverse;
    }
    if (m & Mark_flag_haste_input) {
        attr = A_bold | fg_bg(C_cyan, C_natural);
    }
    return attr;
}

void print_activity_indicator(WINDOW *win, Usz activity_counter)
{
    // 7 segments that can each light up as Colors different colors.
    // This gives us Colors^Segments total configurations.
    enum
    {
        Segments = 7,
        Colors = 4
    };
    Usz states = 1; // calculate Colors^Segments
    for (Usz i = 0; i < Segments; ++i)
        states *= Colors;
    // Wrap the counter to the range of displayable configurations.
    Usz val = activity_counter % states;
    chtype lamps[Colors];
#if 1 // Appearance where segments are always lit
    lamps[0] = ACS_HLINE | fg_bg(C_black, C_natural) | A_bold;
    lamps[1] = ACS_HLINE | fg_bg(C_white, C_natural) | A_normal;
    lamps[2] = ACS_HLINE | A_bold;
    lamps[3] = lamps[1];
#elif 0 // Brighter appearance where segments are always lit
    lamps[0] = ACS_HLINE | fg_bg(C_black, C_natural) | A_bold;
    lamps[1] = ACS_HLINE | A_normal;
    lamps[2] = ACS_HLINE | A_bold;
    lamps[3] = lamps[1];
#else   // Appearance where segments can turn off completely
    lamps[0] = ' ';
    lamps[1] = ACS_HLINE | fg_bg(C_black, C_natural) | A_bold;
    lamps[2] = ACS_HLINE | A_normal;
    lamps[3] = lamps[1];
#endif
    chtype buffer[Segments];
    for (Usz i = 0; i < Segments; ++i) {
        // Instead of a left-to-right, straightforward ascending least-to-most
        // significant digits display, we'll display it as a spiral.
        Usz j = i % 2 ? (6 - i / 2) : (i / 2);
        buffer[j] = lamps[val % Colors];
        val = val / Colors;
    }
    waddchnstr(win, buffer, Segments);
    // If you want to see what various combinations of colors and attributes look
    // like in different terminals.
#if 0
  waddch(win, 'a' | fg_bg(C_black, C_natural) | A_dim);
  waddch(win, 'b' | fg_bg(C_black, C_natural) | A_normal);
  waddch(win, 'c' | fg_bg(C_black, C_natural) | A_bold);
  waddch(win, 'd' | A_dim);
  waddch(win, 'e' | A_normal);
  waddch(win, 'f' | A_bold);
  waddch(win, 'g' | fg_bg(C_white, C_natural) | A_dim);
  waddch(win, 'h' | fg_bg(C_white, C_natural) | A_normal);
  waddch(win, 'i' | fg_bg(C_white, C_natural) | A_bold);
#endif
}

void advance_faketab(WINDOW *win, int offset_x, int tabstop)
{
    if (tabstop < 1)
        return;
    int y, x, h, w;
    getyx(win, y, x);
    getmaxyx(win, h, w);
    (void)h;
    x = ((x + tabstop - 1) / tabstop) * tabstop + offset_x % tabstop;
    if (w < 1)
        w = 1;
    if (x >= w)
        x = w - 1;
    wmove(win, y, x);
}

void apply_time_to_sustained_notes(
    Oosc_dev *oosc_dev,
    Midi_mode *midi_mode,
    double time_elapsed,
    Susnote_list *susnote_list,
    double *next_note_off_deadline)
{
    Usz start_removed, end_removed;
    susnote_list_advance_time(
        susnote_list,
        time_elapsed,
        &start_removed,
        &end_removed,
        next_note_off_deadline);
    if (ORCA_UNLIKELY(start_removed != end_removed)) {
        Susnote const *restrict susnotes_off = susnote_list->buffer;
        send_midi_note_offs(oosc_dev, midi_mode, susnotes_off + start_removed, susnotes_off + end_removed);
    }
}

// The way orca handles MIDI sustains, timing, and overlapping note-ons (plus
// the 'mono' thing being added) has changed multiple times over time. Now we
// are in a situation where this function is a complete mess and needs an
// overhaul. If you see something in the function below and think, "wait, that
// seems redundant/weird", that's because it is, not because there's a good
// reason.
void send_output_events(
    Oosc_dev *oosc_dev,
    Midi_mode *midi_mode,
    Usz bpm,
    Susnote_list *susnote_list,
    Oevent const *events,
    Usz count)
{
    enum
    {
        Midi_on_capacity = 512
    };
    typedef struct {
        U8 channel;
        U8 note_number;
        U8 velocity;
    } Midi_note_on;
    typedef struct {
        U8 note_number;
        U8 velocity;
        U8 duration;
    } Midi_mono_on;
    Midi_note_on midi_note_ons[Midi_on_capacity];
    Midi_mono_on midi_mono_ons[16]; // Keep only a single one per channel
    Susnote new_susnotes[Midi_on_capacity];
    Usz midi_note_count = 0;
    Usz monofied_chans = 0; // bitset of channels with new mono notes
    double frame_secs = 60.0 / (double)bpm / 4.0;

    for (Usz i = 0; i < count; ++i) {
        Oevent const *e = events + i;
        switch ((Oevent_types)e->any.oevent_type) {
            case Oevent_type_midi_note: {
                if (midi_note_count == Midi_on_capacity)
                    break;
                Oevent_midi_note const *em = &e->midi_note;
                Usz note_number = (Usz)(12u * em->octave + em->note);
                if (note_number > 127)
                    note_number = 127;
                Usz channel = em->channel;
                if (channel > 15)
                    break;
                if (em->mono) {
                    // 'mono' note-ons are strange. The more typical branch you'd expect to
                    // see, where you can play multiple notes per channel, is below.
                    monofied_chans |= 1u << (channel & 0xFu);
                    midi_mono_ons[channel] = (Midi_mono_on){ .note_number = (U8)note_number,
                                                             .velocity = em->velocity,
                                                             .duration = em->duration };
                } else {
                    midi_note_ons[midi_note_count] = (Midi_note_on){ .channel = (U8)channel,
                                                                     .note_number = (U8)note_number,
                                                                     .velocity = em->velocity };
                    new_susnotes[midi_note_count] = (Susnote){
                        .remaining = (float)(frame_secs * (double)em->duration),
                        .chan_note = (U16)((channel << 8u) | note_number)
                    };
                    ++midi_note_count;
                }
                break;
            }
            case Oevent_type_midi_cc: {
                Oevent_midi_cc const *ec = &e->midi_cc;
                // Note that we're not preserving the exact order of MIDI events as
                // emitted by the orca VM. Notes and CCs that are emitted in the same
                // step will always have the CCs sent first. Not sure if this is OK or
                // not. If it's not OK, we can either loop again a second time to always
                // send CCs after notes, or if that's not also OK, we can make the stack
                // buffer more complicated and interleave the CCs in it.
                send_midi_chan_msg(oosc_dev, midi_mode, 0xb, ec->channel, ec->control, ec->value);
                break;
            }
            case Oevent_type_midi_pb: {
                Oevent_midi_pb const *ep = &e->midi_pb;
                // Same caveat regarding ordering with MIDI CC also applies here.
                send_midi_chan_msg(oosc_dev, midi_mode, 0xe, ep->channel, ep->lsb, ep->msb);
                break;
            }
            case Oevent_type_osc_ints: {
                // kinda lame
                if (!oosc_dev)
                    continue;
                Oevent_osc_ints const *eo = &e->osc_ints;
                char path[] = { '/', eo->glyph, '\0' };
                I32 ints[ORCA_ARRAY_COUNTOF(eo->numbers)];
                Usz nnum = eo->count;
                for (Usz inum = 0; inum < nnum; ++inum) {
                    ints[inum] = eo->numbers[inum];
                }
                oosc_send_int32s(oosc_dev, path, ints, nnum);
                break;
            }
            case Oevent_type_udp_string: {
                if (!oosc_dev)
                    continue;
                Oevent_udp_string const *eo = &e->udp_string;
                oosc_send_datagram(oosc_dev, eo->chars, eo->count);
                break;
            }
        }
    }

do_note_ons:
    if (midi_note_count > 0) {
        Usz start_note_offs, end_note_offs;
        susnote_list_add_notes(
            susnote_list,
            new_susnotes,
            midi_note_count,
            &start_note_offs,
            &end_note_offs);
        if (start_note_offs != end_note_offs) {
            Susnote const *restrict susnotes_off = susnote_list->buffer;
            send_midi_note_offs(
                oosc_dev,
                midi_mode,
                susnotes_off + start_note_offs,
                susnotes_off + end_note_offs);
        }
        for (Usz i = 0; i < midi_note_count; ++i) {
            Midi_note_on mno = midi_note_ons[i];
            send_midi_chan_msg(oosc_dev, midi_mode, 0x9, mno.channel, mno.note_number, mno.velocity);
        }
    }
    if (monofied_chans) {
        // The behavior we end up with is that if regular note-ons are played in
        // the same frame/step as a mono, the regular note-ons will have the actual
        // MIDI note on sent, followed immediately by a MIDI note off. I don't know
        // if this is good or not.
        Usz start_note_offs, end_note_offs;
        susnote_list_remove_by_chan_mask(susnote_list, monofied_chans, &start_note_offs, &end_note_offs);
        if (start_note_offs != end_note_offs) {
            Susnote const *restrict susnotes_off = susnote_list->buffer;
            send_midi_note_offs(
                oosc_dev,
                midi_mode,
                susnotes_off + start_note_offs,
                susnotes_off + end_note_offs);
        }
        midi_note_count = 0;           // We're going to use this list again. Reset it.
        for (Usz i = 0; i < 16; i++) { // Add these notes to list of note-ons
            if (!(monofied_chans & 1u << i))
                continue;
            midi_note_ons[midi_note_count] = (Midi_note_on){ .channel = (U8)i,
                                                             .note_number = midi_mono_ons[i].note_number,
                                                             .velocity = midi_mono_ons[i].velocity };
            new_susnotes[midi_note_count] = (Susnote){
                .remaining = (float)(frame_secs * (double)midi_mono_ons[i].duration),
                .chan_note = (U16)((i << 8u) | midi_mono_ons[i].note_number)
            };
            midi_note_count++;
        }
        monofied_chans = false;
        goto do_note_ons; // lol super wasteful for doing susnotes again
    }
}

void ged_cursor_init(Ged_cursor *tc)
{
    tc->x = tc->y = 0;
    tc->w = tc->h = 1;
}

void ged_init(Ged *a, Usz undo_limit, Usz init_bpm, Usz init_seed)
{
    field_init(&a->field);
    field_init(&a->scratch_field);
    field_init(&a->clipboard_field);
    mbuf_reusable_init(&a->mbuf_r);
    undo_history_init(&a->undo_hist, undo_limit);
    oevent_list_init(&a->oevent_list);
    oevent_list_init(&a->scratch_oevent_list);
    susnote_list_init(&a->susnote_list);
    ged_cursor_init(&a->ged_cursor);
    a->tick_num = 0;
    a->ruler_spacing_y = a->ruler_spacing_x = 8;
    a->input_mode = Ged_input_mode_normal;
    a->bpm = init_bpm;
    a->clock = 0;
    a->accum_secs = 0.0;
    a->time_to_next_note_off = 1.0;
    a->oosc_dev = NULL;
    midi_mode_init_null(&a->midi_mode);
    a->activity_counter = 0;
    a->random_seed = init_seed;
    a->drag_start_y = a->drag_start_x = 0;
    a->win_h = a->win_w = 0;
    a->softmargin_y = a->softmargin_x = 0;
    a->grid_h = 0;
    a->grid_scroll_y = a->grid_scroll_x = 0;
    a->midi_bclock_sixths = 0;
    a->needs_remarking = true;
    a->is_draw_dirty = false;
    a->is_playing = false;
    a->midi_bclock = false;
    a->draw_event_list = false;
    a->is_mouse_down = false;
    a->is_mouse_dragging = false;
    a->is_hud_visible = false;
}

void ged_deinit(Ged *a)
{
    field_deinit(&a->field);
    field_deinit(&a->scratch_field);
    field_deinit(&a->clipboard_field);
    mbuf_reusable_deinit(&a->mbuf_r);
    undo_history_deinit(&a->undo_hist);
    oevent_list_deinit(&a->oevent_list);
    oevent_list_deinit(&a->scratch_oevent_list);
    susnote_list_deinit(&a->susnote_list);
    if (a->oosc_dev)
        oosc_dev_destroy(a->oosc_dev);
    midi_mode_deinit(&a->midi_mode);
}

void clear_and_run_vm(
    Glyph *restrict gbuf,
    Mark *restrict mbuf,
    Usz height,
    Usz width,
    Usz tick_number,
    Oevent_list *oevent_list,
    Usz random_seed)
{
    mbuffer_clear(mbuf, height, width);
    oevent_list_clear(oevent_list);
    orca_run(gbuf, mbuf, height, width, tick_number, oevent_list, random_seed);
}

void ged_cursor_move_relative(Ged_cursor *tc, Usz field_h, Usz field_w, Isz delta_y, Isz delta_x)
{
    Isz y0 = (Isz)tc->y + delta_y;
    Isz x0 = (Isz)tc->x + delta_x;
    if (y0 >= (Isz)field_h)
        y0 = (Isz)field_h - 1;
    if (y0 < 0)
        y0 = 0;
    if (x0 >= (Isz)field_w)
        x0 = (Isz)field_w - 1;
    if (x0 < 0)
        x0 = 0;
    tc->y = (Usz)y0;
    tc->x = (Usz)x0;
}

bool ged_is_draw_dirty(Ged *a)
{
    return a->is_draw_dirty || a->needs_remarking;
}

void ged_stop_all_sustained_notes(Ged *a)
{
    Susnote_list *sl = &a->susnote_list;
    send_midi_note_offs(a->oosc_dev, &a->midi_mode, sl->buffer, sl->buffer + sl->count);
    susnote_list_clear(sl);
    a->time_to_next_note_off = 1.0;
}

void ged_clear_osc_udp(Ged *a)
{
    if (a->oosc_dev) {
        if (a->midi_mode.any.type == Midi_mode_type_osc_bidule) {
            ged_stop_all_sustained_notes(a);
        }
        oosc_dev_destroy(a->oosc_dev);
        a->oosc_dev = NULL;
    }
}

bool ged_is_using_osc_udp(Ged *a)
{
    return (bool)a->oosc_dev;
}

bool ged_set_osc_udp(Ged *a, char const *dest_addr, char const *dest_port)
{
    ged_clear_osc_udp(a);
    if (dest_port) {
        Oosc_udp_create_error err = oosc_dev_create_udp(&a->oosc_dev, dest_addr, dest_port);
        if (err) {
            return false;
        }
    }
    return true;
}

double ged_secs_to_deadline(Ged const *a)
{
    if (!a->is_playing)
        return 1.0;
    double secs_span = 60.0 / (double)a->bpm / 4.0;
    // If MIDI beat clock output is enabled, we need to send an event every 24
    // parts per quarter note. Since we've already divided quarter notes into 4
    // for ORCA's timing semantics, divide it by a further 6. This same logic is
    // mirrored in ged_do_stuff().
    if (a->midi_bclock)
        secs_span /= 6.0;
    double rem = secs_span - (stm_sec(stm_since(a->clock)) + a->accum_secs);
    double next_note_off = a->time_to_next_note_off;
    if (next_note_off < rem)
        rem = next_note_off;
    if (rem < 0.0)
        rem = 0.0;
    return rem;
}

void ged_do_stuff(Ged *a)
{
    if (!a->is_playing)
        return;
    double secs_span = 60.0 / (double)a->bpm / 4.0;
    if (a->midi_bclock) // see also ged_secs_to_deadline()
        secs_span /= 6.0;
    Oosc_dev *oosc_dev = a->oosc_dev;
    Midi_mode *midi_mode = &a->midi_mode;
    bool crossed_deadline = false;
#if TIME_DEBUG
    Usz spins = 0;
    U64 spin_start = stm_now();
    (void)spin_start;
#endif
    for (;;) {
        U64 now = stm_now();
        U64 diff = stm_diff(now, a->clock);
        double sdiff = stm_sec(diff) + a->accum_secs;
        if (sdiff >= secs_span) {
            a->clock = now;
            a->accum_secs = sdiff - secs_span;
#if TIME_DEBUG
            if (a->accum_secs > 0.000001) {
                fprintf(stderr, "late: %.2f u-secs\n", a->accum_secs * 1000 * 1000);
                if (a->accum_secs > 0.00005) {
                    fprintf(stderr, "guilty timeout: %d\n", spin_track_timeout);
                }
            }
#endif
            crossed_deadline = true;
            break;
        }
        if (secs_span - sdiff > ms_to_sec(0.1))
            break;
#if TIME_DEBUG
        ++spins;
#endif
    }
#if TIME_DEBUG
    if (spins > 0) {
        fprintf(
            stderr,
            "%d spins in %f us with timeout %d\n",
            (int)spins,
            stm_us(stm_since(spin_start)),
            spin_track_timeout);
    }
#endif
    if (!crossed_deadline)
        return;
    if (a->midi_bclock) {
        send_midi_byte(oosc_dev, midi_mode, 0xF8); // MIDI beat clock
        Usz sixths = a->midi_bclock_sixths;
        a->midi_bclock_sixths = (U8)((sixths + 1) % 6);
        if (sixths != 0)
            return;
    }
    apply_time_to_sustained_notes(
        oosc_dev,
        midi_mode,
        secs_span,
        &a->susnote_list,
        &a->time_to_next_note_off);
    clear_and_run_vm(
        a->field.buffer,
        a->mbuf_r.buffer,
        a->field.height,
        a->field.width,
        a->tick_num,
        &a->oevent_list,
        a->random_seed);
    ++a->tick_num;
    a->needs_remarking = true;
    a->is_draw_dirty = true;

    Usz count = a->oevent_list.count;
    if (count > 0) {
        send_output_events(oosc_dev, midi_mode, a->bpm, &a->susnote_list, a->oevent_list.buffer, count);
        a->activity_counter += count;
    }
}

Isz isz_clamp(Isz x, Isz low, Isz high)
{
    return x < low ? low : x > high ? high : x;
}


// todo cleanup to use proper unsigned/signed w/ overflow check
Isz scroll_offset_on_axis_for_cursor_pos(Isz win_len, Isz cont_len, Isz cursor_pos, Isz pad, Isz cur_scroll)
{
    if (win_len <= 0 || cont_len <= 0)
        return 0;
    if (cont_len <= win_len)
        return -((win_len - cont_len) / 2);
    if (pad * 2 >= win_len) {
        pad = (win_len - 1) / 2;
    }
    Isz min_vis_scroll = cursor_pos - win_len + 1 + pad;
    Isz max_vis_scroll = cursor_pos - pad;
    Isz new_scroll;
    if (cur_scroll < min_vis_scroll)
        new_scroll = min_vis_scroll;
    else if (cur_scroll > max_vis_scroll)
        new_scroll = max_vis_scroll;
    else
        new_scroll = cur_scroll;
    return isz_clamp(new_scroll, 0, cont_len - win_len);
}

void ged_make_cursor_visible(Ged *a)
{
    int grid_h = a->grid_h;
    int cur_scr_y = a->grid_scroll_y;
    int cur_scr_x = a->grid_scroll_x;
    int new_scr_y = (int)scroll_offset_on_axis_for_cursor_pos(
        grid_h,
        (Isz)a->field.height,
        (Isz)a->ged_cursor.y,
        5,
        cur_scr_y);
    int new_scr_x = (int)scroll_offset_on_axis_for_cursor_pos(
        a->win_w,
        (Isz)a->field.width,
        (Isz)a->ged_cursor.x,
        5,
        cur_scr_x);
    if (new_scr_y == cur_scr_y && new_scr_x == cur_scr_x)
        return;
    a->grid_scroll_y = new_scr_y;
    a->grid_scroll_x = new_scr_x;
    a->is_draw_dirty = true;
}

void ged_update_internal_geometry(Ged *a)
{
    int win_h = a->win_h;
    int softmargin_y = a->softmargin_y;
    bool show_hud = win_h > Hud_height + 1;
    int grid_h = show_hud ? win_h - Hud_height : win_h;
    if (grid_h > a->field.height) {
        int halfy = (grid_h - a->field.height + 1) / 2;
        grid_h -= halfy < softmargin_y ? halfy : softmargin_y;
    }
    a->grid_h = grid_h;
    a->is_draw_dirty = true;
    a->is_hud_visible = show_hud;
}

void ged_set_window_size(Ged *a, int win_h, int win_w, int softmargin_y, int softmargin_x)
{
    if (a->win_h == win_h && a->win_w == win_w && a->softmargin_y == softmargin_y &&
        a->softmargin_x == softmargin_x) {
        return;
    }
    a->win_h = win_h;
    a->win_w = win_w;
    a->softmargin_y = softmargin_y;
    a->softmargin_x = softmargin_x;
    ged_update_internal_geometry(a);
    ged_make_cursor_visible(a);
}

void ged_cursor_confine(Ged_cursor *tc, Usz height, Usz width)
{
    if (height == 0 || width == 0)
        return;
    if (tc->y >= height)
        tc->y = height - 1;
    if (tc->x >= width)
        tc->x = width - 1;
}

void draw_oevent_list(WINDOW *win, Oevent_list const *oevent_list)
{
    wmove(win, 0, 0);
    int win_h = getmaxy(win);
    wprintw(win, "Count: %d", (int)oevent_list->count);
    for (Usz i = 0, num_events = oevent_list->count; i < num_events; ++i) {
        int cury = getcury(win);
        if (cury + 1 >= win_h)
            return;
        wmove(win, cury + 1, 0);
        Oevent const *ev = oevent_list->buffer + i;
        Oevent_types evt = ev->any.oevent_type;
        switch (evt) {
            case Oevent_type_midi_note: {
                Oevent_midi_note const *em = &ev->midi_note;
                wprintw(
                    win,
                    "MIDI Note\tchannel %d\toctave %d\tnote %d\tvelocity %d\tlength %d",
                    (int)em->channel,
                    (int)em->octave,
                    (int)em->note,
                    (int)em->velocity,
                    (int)em->duration);
                break;
            }
            case Oevent_type_midi_cc: {
                Oevent_midi_cc const *ec = &ev->midi_cc;
                wprintw(
                    win,
                    "MIDI CC\tchannel %d\tcontrol %d\tvalue %d",
                    (int)ec->channel,
                    (int)ec->control,
                    (int)ec->value);
                break;
            }
            case Oevent_type_midi_pb: {
                Oevent_midi_pb const *ep = &ev->midi_pb;
                wprintw(
                    win,
                    "MIDI PB\tchannel %d\tmsb %d\tlsb %d",
                    (int)ep->channel,
                    (int)ep->msb,
                    (int)ep->lsb);
                break;
            }
            case Oevent_type_osc_ints: {
                Oevent_osc_ints const *eo = &ev->osc_ints;
                wprintw(win, "OSC\t%c\tcount: %d ", eo->glyph, eo->count);
                waddch(win, ACS_VLINE);
                for (Usz j = 0; j < eo->count; ++j) {
                    wprintw(win, " %d", eo->numbers[j]);
                }
                break;
            }
            case Oevent_type_udp_string: {
                Oevent_udp_string const *eo = &ev->udp_string;
                wprintw(win, "UDP\tcount %d\t", (int)eo->count);
                for (Usz j = 0; j < (Usz)eo->count; ++j) {
                    waddch(win, (chtype)eo->chars[j]);
                }
                break;
            }
        }
    }
}

void ged_resize_grid(
    Field *field,
    Mbuf_reusable *mbr,
    Usz new_height,
    Usz new_width,
    Usz tick_num,
    Field *scratch_field,
    Undo_history *undo_hist,
    Ged_cursor *ged_cursor)
{
    assert(new_height > 0 && new_width > 0);
    undo_history_push(undo_hist, field, tick_num);
    field_copy(field, scratch_field);
    field_resize_raw(field, new_height, new_width);
    // junky copies until i write a smarter thing
    memset(field->buffer, '.', new_height * new_width * sizeof(Glyph));
    gbuffer_copy_subrect(
        scratch_field->buffer,
        field->buffer,
        scratch_field->height,
        scratch_field->width,
        field->height,
        field->width,
        0,
        0,
        0,
        0,
        scratch_field->height,
        scratch_field->width);
    ged_cursor_confine(ged_cursor, new_height, new_width);
    mbuf_reusable_ensure_size(mbr, new_height, new_width);
}

void draw_grid_cursor(
    WINDOW *win,
    int draw_y,
    int draw_x,
    int draw_h,
    int draw_w,
    Glyph const *gbuffer,
    Usz field_h,
    Usz field_w,
    int scroll_y,
    int scroll_x,
    Usz cursor_y,
    Usz cursor_x,
    Usz cursor_h,
    Usz cursor_w,
    Ged_input_mode input_mode,
    bool is_playing)
{
    (void)input_mode;
    if (cursor_y >= field_h || cursor_x >= field_w)
        return;
    if (scroll_y < 0) {
        draw_y += -scroll_y;
        scroll_y = 0;
    }
    if (scroll_x < 0) {
        draw_x += -scroll_x;
        scroll_x = 0;
    }
    Usz offset_y = (Usz)scroll_y;
    Usz offset_x = (Usz)scroll_x;
    if (offset_y >= field_h || offset_x >= field_w)
        return;
    if (draw_y >= draw_h || draw_x >= draw_w)
        return;
    attr_t const curs_attr = A_reverse | A_bold | fg_bg(C_yellow, C_natural);
    if (offset_y <= cursor_y && offset_x <= cursor_x) {
        Usz cdraw_y = cursor_y - offset_y + (Usz)draw_y;
        Usz cdraw_x = cursor_x - offset_x + (Usz)draw_x;
        if (cdraw_y < (Usz)draw_h && cdraw_x < (Usz)draw_w) {
            Glyph beneath = gbuffer[cursor_y * field_w + cursor_x];
            char displayed;
            if (beneath == '.') {
                displayed = is_playing ? '@' : '~';
            } else {
                displayed = beneath;
            }
            chtype ch = (chtype)displayed | curs_attr;
            wmove(win, (int)cdraw_y, (int)cdraw_x);
            waddchnstr(win, &ch, 1);
        }
    }

    // Early out for selection area that won't have any visual effect
    if (cursor_h <= 1 && cursor_w <= 1)
        return;

    // Now mutate visually selected area under grid to have the selection color
    // attributes. (This will rewrite the attributes on the cursor character we
    // wrote above, but if it was the only character that would have been
    // changed, we already early-outed.)
    //
    // We'll do this by reading back the characters on the grid from the curses
    // window buffer, changing the attributes, then writing it back. This is
    // easier than pulling the glyphs from the gbuffer, since we already did the
    // ruler calculations to turn . into +, and we don't need special behavior
    // for any other attributes (e.g. we don't show a special state for selected
    // uppercase characters.)
    //
    // First, confine cursor selection to the grid field/gbuffer that actually
    // exists, in case the cursor selection exceeds the area of the field.
    Usz sel_rows = field_h - cursor_y;
    if (cursor_h < sel_rows)
        sel_rows = cursor_h;
    Usz sel_cols = field_w - cursor_x;
    if (cursor_w < sel_cols)
        sel_cols = cursor_w;
    // Now, confine the selection area to what's visible on screen. Kind of
    // tricky since we have to handle it being partially visible from any edge on
    // any axis, and we have to be mindful overflow.
    Usz vis_sel_y;
    Usz vis_sel_x;
    if (offset_y > cursor_y) {
        vis_sel_y = 0;
        Usz sub_y = offset_y - cursor_y;
        if (sub_y > sel_rows)
            sel_rows = 0;
        else
            sel_rows -= sub_y;
    } else {
        vis_sel_y = cursor_y - offset_y;
    }
    if (offset_x > cursor_x) {
        vis_sel_x = 0;
        Usz sub_x = offset_x - cursor_x;
        if (sub_x > sel_cols)
            sel_cols = 0;
        else
            sel_cols -= sub_x;
    } else {
        vis_sel_x = cursor_x - offset_x;
    }
    vis_sel_y += (Usz)draw_y;
    vis_sel_x += (Usz)draw_x;
    if (vis_sel_y >= (Usz)draw_h || vis_sel_x >= (Usz)draw_w)
        return;
    Usz vis_sel_h = (Usz)draw_h - vis_sel_y;
    Usz vis_sel_w = (Usz)draw_w - vis_sel_x;
    if (sel_rows < vis_sel_h)
        vis_sel_h = sel_rows;
    if (sel_cols < vis_sel_w)
        vis_sel_w = sel_cols;
    if (vis_sel_w == 0 || vis_sel_h == 0)
        return;
    enum
    {
        Bufcount = 4096
    };
    chtype chbuffer[Bufcount];
    if (Bufcount < vis_sel_w)
        vis_sel_w = Bufcount;
    for (Usz iy = 0; iy < vis_sel_h; ++iy) {
        int at_y = (int)(vis_sel_y + iy);
        int num = mvwinchnstr(win, at_y, (int)vis_sel_x, chbuffer, (int)vis_sel_w);
        for (int ix = 0; ix < num; ++ix) {
            chbuffer[ix] = (chtype)((chbuffer[ix] & (A_CHARTEXT | A_ALTCHARSET)) | (chtype)curs_attr);
        }
        waddchnstr(win, chbuffer, (int)num);
    }
}

void draw_hud(
    WINDOW *win,
    int win_y,
    int win_x,
    int height,
    int width,
    char const *filename,
    Usz field_h,
    Usz field_w,
    Usz ruler_spacing_y,
    Usz ruler_spacing_x,
    Usz tick_num,
    Usz bpm,
    Ged_cursor const *ged_cursor,
    Ged_input_mode input_mode,
    Usz activity_counter)
{
    (void)height;
    (void)width;
    enum
    {
        Tabstop = 8
    };
    wmove(win, win_y, win_x);
    wprintw(win, "%zux%zu", field_w, field_h);
    advance_faketab(win, win_x, Tabstop);
    wprintw(win, "%zu/%zu", ruler_spacing_x, ruler_spacing_y);
    advance_faketab(win, win_x, Tabstop);
    wprintw(win, "%zuf", tick_num);
    advance_faketab(win, win_x, Tabstop);
    wprintw(win, "%zu", bpm);
    advance_faketab(win, win_x, Tabstop);
    print_activity_indicator(win, activity_counter);
    wmove(win, win_y + 1, win_x);
    wprintw(win, "%zu,%zu", ged_cursor->x, ged_cursor->y);
    advance_faketab(win, win_x, Tabstop);
    wprintw(win, "%zu:%zu", ged_cursor->w, ged_cursor->h);
    advance_faketab(win, win_x, Tabstop);
    switch (input_mode) {
        case Ged_input_mode_normal:
            wattrset(win, A_normal);
            waddstr(win, "insert");
            break;
        case Ged_input_mode_append:
            wattrset(win, A_bold);
            waddstr(win, "append");
            break;
        case Ged_input_mode_selresize:
            wattrset(win, A_bold);
            waddstr(win, "select");
            break;
        case Ged_input_mode_slide:
            wattrset(win, A_reverse);
            waddstr(win, "slide");
            break;
    }
    advance_faketab(win, win_x, Tabstop);
    wattrset(win, A_normal);
    waddstr(win, filename);
}

void draw_glyphs_grid(
    WINDOW *win,
    int draw_y,
    int draw_x,
    int draw_h,
    int draw_w,
    Glyph const *restrict gbuffer,
    Mark const *restrict mbuffer,
    Usz field_h,
    Usz field_w,
    Usz offset_y,
    Usz offset_x,
    Usz ruler_spacing_y,
    Usz ruler_spacing_x,
    bool use_fancy_dots,
    bool use_fancy_rulers)
{
    assert(draw_y >= 0 && draw_x >= 0);
    assert(draw_h >= 0 && draw_w >= 0);
    enum
    {
        Bufcount = 4096
    };
    chtype chbuffer[Bufcount];
    // todo buffer limit
    if (offset_y >= field_h || offset_x >= field_w)
        return;
    if (draw_y >= draw_h || draw_x >= draw_w)
        return;
    Usz rows = (Usz)(draw_h - draw_y);
    if (field_h - offset_y < rows)
        rows = field_h - offset_y;
    Usz cols = (Usz)(draw_w - draw_x);
    if (field_w - offset_x < cols)
        cols = field_w - offset_x;
    if (Bufcount < cols)
        cols = Bufcount;
    if (rows == 0 || cols == 0)
        return;
    bool use_rulers = ruler_spacing_y != 0 && ruler_spacing_x != 0;
    chtype bullet = use_fancy_dots ? ACS_BULLET : '.';
    enum
    {
        T = 1 << 0,
        B = 1 << 1,
        L = 1 << 2,
        R = 1 << 3
    };
    chtype rs[(T | B | L | R) + 1];
    if (use_rulers) {
        for (Usz i = 0; i < sizeof rs / sizeof(chtype); ++i)
            rs[i] = '+';
        if (use_fancy_rulers) {
            rs[T | L] = ACS_ULCORNER;
            rs[T | R] = ACS_URCORNER;
            rs[B | L] = ACS_LLCORNER;
            rs[B | R] = ACS_LRCORNER;
            rs[T] = ACS_TTEE;
            rs[B] = ACS_BTEE;
            rs[L] = ACS_LTEE;
            rs[R] = ACS_RTEE;
        }
    }
    for (Usz iy = 0; iy < rows; ++iy) {
        Usz line_offset = (offset_y + iy) * field_w + offset_x;
        Glyph const *g_row = gbuffer + line_offset;
        Mark const *m_row = mbuffer + line_offset;
        bool use_y_ruler = use_rulers && (iy + offset_y) % ruler_spacing_y == 0;
        for (Usz ix = 0; ix < cols; ++ix) {
            Glyph g = g_row[ix];
            Mark m = m_row[ix];
            chtype ch;
            if (g == '.') {
                if (use_y_ruler && (ix + offset_x) % ruler_spacing_x == 0) {
                    int p = 0; // clang-format off
          if (iy + offset_y     == 0      ) p |= T;
          if (iy + offset_y + 1 == field_h) p |= B;
          if (ix + offset_x     == 0      ) p |= L;
          if (ix + offset_x + 1 == field_w) p |= R;
          ch = rs[p]; // clang-format on
                } else {
                    ch = bullet;
                }
            } else {
                ch = (chtype)g;
            }
            attr_t attrs = term_attrs_of_cell(g, m);
            chbuffer[ix] = ch | attrs;
        }
        wmove(win, draw_y + (int)iy, draw_x);
        waddchnstr(win, chbuffer, (int)cols);
    }
}

void draw_glyphs_grid_scrolled(
    WINDOW *win,
    int draw_y,
    int draw_x,
    int draw_h,
    int draw_w,
    Glyph const *restrict gbuffer,
    Mark const *restrict mbuffer,
    Usz field_h,
    Usz field_w,
    int scroll_y,
    int scroll_x,
    Usz ruler_spacing_y,
    Usz ruler_spacing_x,
    bool use_fancy_dots,
    bool use_fancy_rulers)
{
    if (scroll_y < 0) {
        draw_y += -scroll_y;
        scroll_y = 0;
    }
    if (scroll_x < 0) {
        draw_x += -scroll_x;
        scroll_x = 0;
    }
    draw_glyphs_grid(
        win,
        draw_y,
        draw_x,
        draw_h,
        draw_w,
        gbuffer,
        mbuffer,
        field_h,
        field_w,
        (Usz)scroll_y,
        (Usz)scroll_x,
        ruler_spacing_y,
        ruler_spacing_x,
        use_fancy_dots,
        use_fancy_rulers);
}

void ged_draw(Ged *a, WINDOW *win, char const *filename, bool use_fancy_dots, bool use_fancy_rulers)
{
    // We can predictavely step the next simulation tick and then use the
    // resulting mark buffer for better UI visualization. If we don't do this,
    // after loading a fresh file or after the user performs some edit (or even
    // after a regular simulation step), the new glyph buffer won't have had
    // phase 0 of the simulation run, which means the ports and other flags won't
    // be set on the mark buffer, so the colors for disabled cells, ports, etc.
    // won't be set.
    //
    // We can just perform a simulation step using the current state, keep the
    // mark buffer that it produces, then roll back the glyph buffer to where it
    // was before. This should produce results similar to having specialized UI
    // code that looks at each glyph and figures out the ports, etc.
    if (a->needs_remarking && !a->is_playing) {
        field_resize_raw_if_necessary(&a->scratch_field, a->field.height, a->field.width);
        field_copy(&a->field, &a->scratch_field);
        mbuf_reusable_ensure_size(&a->mbuf_r, a->field.height, a->field.width);
        clear_and_run_vm(
            a->scratch_field.buffer,
            a->mbuf_r.buffer,
            a->field.height,
            a->field.width,
            a->tick_num,
            &a->scratch_oevent_list,
            a->random_seed);
        a->needs_remarking = false;
    }
    int win_w = a->win_w;
    draw_glyphs_grid_scrolled(
        win,
        0,
        0,
        a->grid_h,
        win_w,
        a->field.buffer,
        a->mbuf_r.buffer,
        a->field.height,
        a->field.width,
        a->grid_scroll_y,
        a->grid_scroll_x,
        a->ruler_spacing_y,
        a->ruler_spacing_x,
        use_fancy_dots,
        use_fancy_rulers);
    draw_grid_cursor(
        win,
        0,
        0,
        a->grid_h,
        win_w,
        a->field.buffer,
        a->field.height,
        a->field.width,
        a->grid_scroll_y,
        a->grid_scroll_x,
        a->ged_cursor.y,
        a->ged_cursor.x,
        a->ged_cursor.h,
        a->ged_cursor.w,
        a->input_mode,
        a->is_playing);
    if (a->is_hud_visible) {
        filename = filename ? filename : "unnamed";
        int hud_x = win_w > 50 + a->softmargin_x * 2 ? a->softmargin_x : 0;
        draw_hud(
            win,
            a->grid_h,
            hud_x,
            Hud_height,
            win_w,
            filename,
            a->field.height,
            a->field.width,
            a->ruler_spacing_y,
            a->ruler_spacing_x,
            a->tick_num,
            a->bpm,
            &a->ged_cursor,
            a->input_mode,
            a->activity_counter);
    }
    if (a->draw_event_list)
        draw_oevent_list(win, &a->oevent_list);
    a->is_draw_dirty = false;
}

void ged_send_osc_bpm(Ged *a, I32 bpm)
{
    send_num_message(a->oosc_dev, "/orca/bpm", bpm);
}

void ged_adjust_bpm(Ged *a, Isz delta_bpm)
{
    Isz new_bpm = (Isz)a->bpm;
    if (delta_bpm < 0 || new_bpm < INT_MAX - delta_bpm)
        new_bpm += delta_bpm;
    else
        new_bpm = INT_MAX;
    if (new_bpm < 1)
        new_bpm = 1;
    if ((Usz)new_bpm != a->bpm) {
        a->bpm = (Usz)new_bpm;
        a->is_draw_dirty = true;
        ged_send_osc_bpm(a, (I32)new_bpm);
    }
}

void ged_move_cursor_relative(Ged *a, Isz delta_y, Isz delta_x)
{
    ged_cursor_move_relative(&a->ged_cursor, a->field.height, a->field.width, delta_y, delta_x);
    ged_make_cursor_visible(a);
    a->is_draw_dirty = true;
}

Usz guarded_selection_axis_resize(Usz x, int delta)
{
    if (delta < 0) {
        if (delta > INT_MIN && (Usz)(-delta) < x) {
            x -= (Usz)(-delta);
        }
    } else if (x < SIZE_MAX - (Usz)delta) {
        x += (Usz)delta;
    }
    return x;
}

void ged_modify_selection_size(Ged *a, int delta_y, int delta_x)
{
    Usz cur_h = a->ged_cursor.h, cur_w = a->ged_cursor.w;
    Usz new_h = guarded_selection_axis_resize(cur_h, delta_y);
    Usz new_w = guarded_selection_axis_resize(cur_w, delta_x);
    if (cur_h != new_h || cur_w != new_w) {
        a->ged_cursor.h = new_h;
        a->ged_cursor.w = new_w;
        a->is_draw_dirty = true;
    }
}

bool ged_try_selection_clipped_to_field(Ged const *a, Usz *out_y, Usz *out_x, Usz *out_h, Usz *out_w)
{
    Usz curs_y = a->ged_cursor.y, curs_x = a->ged_cursor.x;
    Usz curs_h = a->ged_cursor.h, curs_w = a->ged_cursor.w;
    Usz field_h = a->field.height, field_w = a->field.width;
    if (curs_y >= field_h || curs_x >= field_w)
        return false;
    if (field_h - curs_y < curs_h)
        curs_h = field_h - curs_y;
    if (field_w - curs_x < curs_w)
        curs_w = field_w - curs_x;
    *out_y = curs_y;
    *out_x = curs_x;
    *out_h = curs_h;
    *out_w = curs_w;
    return true;
}

bool ged_slide_selection(Ged *a, int delta_y, int delta_x)
{
    Usz curs_y_0, curs_x_0, curs_h_0, curs_w_0;
    Usz curs_y_1, curs_x_1, curs_h_1, curs_w_1;
    if (!ged_try_selection_clipped_to_field(a, &curs_y_0, &curs_x_0, &curs_h_0, &curs_w_0))
        return false;
    ged_move_cursor_relative(a, delta_y, delta_x);
    if (!ged_try_selection_clipped_to_field(a, &curs_y_1, &curs_x_1, &curs_h_1, &curs_w_1))
        return false;
    // Don't create a history entry if nothing is going to happen.
    if (curs_y_0 == curs_y_1 && curs_x_0 == curs_x_1 && curs_h_0 == curs_h_1 && curs_w_0 == curs_w_1)
        return false;
    undo_history_push(&a->undo_hist, &a->field, a->tick_num);
    Usz field_h = a->field.height;
    Usz field_w = a->field.width;
    gbuffer_copy_subrect(
        a->field.buffer,
        a->field.buffer,
        field_h,
        field_w,
        field_h,
        field_w,
        curs_y_0,
        curs_x_0,
        curs_y_1,
        curs_x_1,
        curs_h_0,
        curs_w_0);
    // Erase/clear the area that was within the selection rectangle in the
    // starting position, but wasn't written to during the copy. (In other words,
    // this is the area that was 'left behind' when we moved the selection
    // rectangle, plus any area that was along the bottom and right edge of the
    // field that didn't have anything to copy to it when the selection rectangle
    // extended outside of the field.)
    Usz ey, eh, ex, ew;
    if (curs_y_1 > curs_y_0) {
        ey = curs_y_0;
        eh = curs_y_1 - curs_y_0;
    } else {
        ey = curs_y_1 + curs_h_0;
        eh = (curs_y_0 + curs_h_0) - ey;
    }
    if (curs_x_1 > curs_x_0) {
        ex = curs_x_0;
        ew = curs_x_1 - curs_x_0;
    } else {
        ex = curs_x_1 + curs_w_0;
        ew = (curs_x_0 + curs_w_0) - ex;
    }
    gbuffer_fill_subrect(a->field.buffer, field_h, field_w, ey, curs_x_0, eh, curs_w_0, '.');
    gbuffer_fill_subrect(a->field.buffer, field_h, field_w, curs_y_0, ex, curs_h_0, ew, '.');
    a->needs_remarking = true;
    return true;
}

void ged_dir_input(Ged *a, Ged_dir dir, int step_length)
{
    switch (a->input_mode) {
        case Ged_input_mode_normal:
        case Ged_input_mode_append:
            switch (dir) {
                case Ged_dir_up:
                    ged_move_cursor_relative(a, -step_length, 0);
                    break;
                case Ged_dir_down:
                    ged_move_cursor_relative(a, step_length, 0);
                    break;
                case Ged_dir_left:
                    ged_move_cursor_relative(a, 0, -step_length);
                    break;
                case Ged_dir_right:
                    ged_move_cursor_relative(a, 0, step_length);
                    break;
            }
            break;
        case Ged_input_mode_selresize:
            switch (dir) {
                case Ged_dir_up:
                    ged_modify_selection_size(a, -step_length, 0);
                    break;
                case Ged_dir_down:
                    ged_modify_selection_size(a, step_length, 0);
                    break;
                case Ged_dir_left:
                    ged_modify_selection_size(a, 0, -step_length);
                    break;
                case Ged_dir_right:
                    ged_modify_selection_size(a, 0, step_length);
                    break;
            }
            break;
        case Ged_input_mode_slide:
            switch (dir) {
                case Ged_dir_up:
                    ged_slide_selection(a, -step_length, 0);
                    break;
                case Ged_dir_down:
                    ged_slide_selection(a, step_length, 0);
                    break;
                case Ged_dir_left:
                    ged_slide_selection(a, 0, -step_length);
                    break;
                case Ged_dir_right:
                    ged_slide_selection(a, 0, step_length);
                    break;
            }
            break;
    }
}

Usz view_to_scrolled_grid(Usz field_len, Usz visual_coord, int scroll_offset)
{
    if (field_len == 0)
        return 0;
    if (scroll_offset < 0) {
        if ((Usz)(-scroll_offset) <= visual_coord) {
            visual_coord -= (Usz)(-scroll_offset);
        } else {
            visual_coord = 0;
        }
    } else {
        visual_coord += (Usz)scroll_offset;
    }
    if (visual_coord >= field_len)
        visual_coord = field_len - 1;
    return visual_coord;
}

ORCA_OK_IF_UNUSED void ged_mouse_event(Ged *a, Usz vis_y, Usz vis_x, mmask_t mouse_bstate)
{
    if (mouse_bstate & BUTTON1_RELEASED) {
        // hard-disables tracking, but also disables further mouse stuff.
        // mousemask() with our original parameters seems to work to get into the
        // state we want, though.
        //
        // printf("\033[?1003l\n");
        mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
        a->is_mouse_down = false;
        a->is_mouse_dragging = false;
        a->drag_start_y = 0;
        a->drag_start_x = 0;
    } else if ((mouse_bstate & BUTTON1_PRESSED) || a->is_mouse_down) {
        Usz y = view_to_scrolled_grid(a->field.height, vis_y, a->grid_scroll_y);
        Usz x = view_to_scrolled_grid(a->field.width, vis_x, a->grid_scroll_x);
        if (!a->is_mouse_down) {
            // some sequence to hopefully make terminal start reporting all further
            // mouse movement events. 'REPORT_MOUSE_POSITION' alone in the mousemask
            // doesn't seem to work, at least not for xterm. we need to set it only
            // only when needed, otherwise some terminals will send movement updates
            // when we don't want them.
            printf("\033[?1003h\n");
            // need to do this or double clicking can cause terminal state to get
            // corrupted, since we're bypassing curses here. might cause flicker.
            // wish i could figure out why report mouse position isn't working on its
            // own.
            fflush(stdout);
            wclear(stdscr);
            a->is_mouse_down = true;
            a->ged_cursor.y = y;
            a->ged_cursor.x = x;
            a->ged_cursor.h = 1;
            a->ged_cursor.w = 1;
            a->is_draw_dirty = true;
        } else {
            if (!a->is_mouse_dragging && (y != a->ged_cursor.y || x != a->ged_cursor.x)) {
                a->is_mouse_dragging = true;
                a->drag_start_y = a->ged_cursor.y;
                a->drag_start_x = a->ged_cursor.x;
            }
            if (a->is_mouse_dragging) {
                Usz tcy = a->drag_start_y;
                Usz tcx = a->drag_start_x;
                Usz loy = y < tcy ? y : tcy;
                Usz lox = x < tcx ? x : tcx;
                Usz hiy = y > tcy ? y : tcy;
                Usz hix = x > tcx ? x : tcx;
                a->ged_cursor.y = loy;
                a->ged_cursor.x = lox;
                a->ged_cursor.h = hiy - loy + 1;
                a->ged_cursor.w = hix - lox + 1;
                a->is_draw_dirty = true;
            }
        }
    }
#if defined(NCURSES_MOUSE_VERSION) && NCURSES_MOUSE_VERSION >= 2
    else {
        if (mouse_bstate & BUTTON4_PRESSED) {
            a->grid_scroll_y -= 1;
            a->is_draw_dirty = true;
        } else if (mouse_bstate & BUTTON5_PRESSED) {
            a->grid_scroll_y += 1;
            a->is_draw_dirty = true;
        }
    }
#endif
}

void ged_adjust_rulers_relative(Ged *a, Isz delta_y, Isz delta_x)
{
    Isz new_y = (Isz)a->ruler_spacing_y + delta_y;
    Isz new_x = (Isz)a->ruler_spacing_x + delta_x;
    if (new_y < 4)
        new_y = 4;
    else if (new_y > 16)
        new_y = 16;
    if (new_x < 4)
        new_x = 4;
    else if (new_x > 16)
        new_x = 16;
    if ((Usz)new_y == a->ruler_spacing_y && (Usz)new_x == a->ruler_spacing_x)
        return;
    a->ruler_spacing_y = (Usz)new_y;
    a->ruler_spacing_x = (Usz)new_x;
    a->is_draw_dirty = true;
}

Usz adjust_rulers_humanized(Usz ruler, Usz in, Isz delta_rulers)
{
    // slightly more confusing because desired grid sizes are +1 (e.g. ruler of
    // length 8 wants to snap to 25 and 33, not 24 and 32). also this math is
    // sloppy.
    assert(ruler > 0);
    if (in == 0)
        return delta_rulers > 0 ? ruler * (Usz)delta_rulers : 1;
    // could overflow if inputs are big
    if (delta_rulers < 0)
        in += ruler - 1;
    Isz n = ((Isz)in - 1) / (Isz)ruler + delta_rulers;
    if (n < 0)
        n = 0;
    return ruler * (Usz)n + 1;
}

// Resizes by number of ruler divisions, and snaps size to closest division in
// a way a human would expect. Adds +1 to the output, so grid resulting size is
// 1 unit longer than the actual ruler length.
bool ged_resize_grid_snap_ruler(
    Field *field,
    Mbuf_reusable *mbr,
    Usz ruler_y,
    Usz ruler_x,
    Isz delta_h,
    Isz delta_w,
    Usz tick_num,
    Field *scratch_field,
    Undo_history *undo_hist,
    Ged_cursor *ged_cursor)
{
    assert(ruler_y > 0);
    assert(ruler_x > 0);
    Usz field_h = field->height;
    Usz field_w = field->width;
    assert(field_h > 0);
    assert(field_w > 0);
    if (ruler_y == 0 || ruler_x == 0 || field_h == 0 || field_w == 0)
        return false;
    Usz new_field_h = field_h;
    Usz new_field_w = field_w;
    if (delta_h != 0)
        new_field_h = adjust_rulers_humanized(ruler_y, field_h, delta_h);
    if (delta_w != 0)
        new_field_w = adjust_rulers_humanized(ruler_x, field_w, delta_w);
    if (new_field_h > ORCA_Y_MAX)
        new_field_h = ORCA_Y_MAX;
    if (new_field_w > ORCA_X_MAX)
        new_field_w = ORCA_X_MAX;
    if (new_field_h == field_h && new_field_w == field_w)
        return false;
    ged_resize_grid(field, mbr, new_field_h, new_field_w, tick_num, scratch_field, undo_hist, ged_cursor);
    return true;
}

void ged_resize_grid_relative(Ged *a, Isz delta_y, Isz delta_x)
{
    ged_resize_grid_snap_ruler(
        &a->field,
        &a->mbuf_r,
        a->ruler_spacing_y,
        a->ruler_spacing_x,
        delta_y,
        delta_x,
        a->tick_num,
        &a->scratch_field,
        &a->undo_hist,
        &a->ged_cursor);
    a->needs_remarking = true; // could check if we actually resized
    a->is_draw_dirty = true;
    ged_update_internal_geometry(a);
    ged_make_cursor_visible(a);
}

void ged_write_character(Ged *a, char c)
{
    undo_history_push(&a->undo_hist, &a->field, a->tick_num);
    gbuffer_poke(a->field.buffer, a->field.height, a->field.width, a->ged_cursor.y, a->ged_cursor.x, c);
    // Indicate we want the next simulation step to be run predictavely,
    // so that we can use the reulsting mark buffer for UI visualization.
    // This is "expensive", so it could be skipped for non-interactive
    // input in situations where max throughput is necessary.
    a->needs_remarking = true;
    if (a->input_mode == Ged_input_mode_append) {
        ged_cursor_move_relative(&a->ged_cursor, a->field.height, a->field.width, 0, 1);
    }
    a->is_draw_dirty = true;
}

bool ged_fill_selection_with_char(Ged *a, Glyph c)
{
    Usz curs_y, curs_x, curs_h, curs_w;
    if (!ged_try_selection_clipped_to_field(a, &curs_y, &curs_x, &curs_h, &curs_w))
        return false;
    gbuffer_fill_subrect(
        a->field.buffer,
        a->field.height,
        a->field.width,
        curs_y,
        curs_x,
        curs_h,
        curs_w,
        c);
    return true;
}

bool ged_copy_selection_to_clipbard(Ged *a)
{
    Usz curs_y, curs_x, curs_h, curs_w;
    if (!ged_try_selection_clipped_to_field(a, &curs_y, &curs_x, &curs_h, &curs_w))
        return false;
    Usz field_h = a->field.height;
    Usz field_w = a->field.width;
    Field *cb_field = &a->clipboard_field;
    field_resize_raw_if_necessary(cb_field, curs_h, curs_w);
    gbuffer_copy_subrect(
        a->field.buffer,
        cb_field->buffer,
        field_h,
        field_w,
        curs_h,
        curs_w,
        curs_y,
        curs_x,
        0,
        0,
        curs_h,
        curs_w);
    return true;
}

void ged_input_character(Ged *a, char c)
{
    switch (a->input_mode) {
        case Ged_input_mode_append:
            ged_write_character(a, c);
            break;
        case Ged_input_mode_normal:
        case Ged_input_mode_selresize:
        case Ged_input_mode_slide:
            if (a->ged_cursor.h <= 1 && a->ged_cursor.w <= 1) {
                ged_write_character(a, c);
            } else {
                undo_history_push(&a->undo_hist, &a->field, a->tick_num);
                ged_fill_selection_with_char(a, c);
                a->needs_remarking = true;
                a->is_draw_dirty = true;
            }
            break;
    }
}

void ged_set_playing(Ged *a, bool playing)
{
    if (playing == a->is_playing)
        return;
    if (playing) {
        undo_history_push(&a->undo_hist, &a->field, a->tick_num);
        a->is_playing = true;
        a->clock = stm_now();
        a->midi_bclock_sixths = 0;
        // dumb'n'dirty, get us close to the next step time, but not quite
        a->accum_secs = 60.0 / (double)a->bpm / 4.0;
        if (a->midi_bclock) {
            send_midi_byte(a->oosc_dev, &a->midi_mode, 0xFA); // "start"
            a->accum_secs /= 6.0;
        }
        a->accum_secs -= 0.0001;
        send_control_message(a->oosc_dev, "/orca/started");
    } else {
        ged_stop_all_sustained_notes(a);
        a->is_playing = false;
        send_control_message(a->oosc_dev, "/orca/stopped");
        if (a->midi_bclock)
            send_midi_byte(a->oosc_dev, &a->midi_mode, 0xFC); // "stop"
    }
    a->is_draw_dirty = true;
}

void ged_input_cmd(Ged *a, Ged_input_cmd ev)
{
    switch (ev) {
        case Ged_input_cmd_undo:
            if (undo_history_count(&a->undo_hist) == 0)
                break;
            if (a->is_playing)
                undo_history_apply(&a->undo_hist, &a->field, &a->tick_num);
            else
                undo_history_pop(&a->undo_hist, &a->field, &a->tick_num);
            ged_cursor_confine(&a->ged_cursor, a->field.height, a->field.width);
            ged_update_internal_geometry(a);
            ged_make_cursor_visible(a);
            a->needs_remarking = true;
            a->is_draw_dirty = true;
            break;
        case Ged_input_cmd_toggle_append_mode:
            a->input_mode = a->input_mode == Ged_input_mode_append ? Ged_input_mode_normal
                                                                   : Ged_input_mode_append;
            a->is_draw_dirty = true;
            break;
        case Ged_input_cmd_toggle_selresize_mode:
            a->input_mode = a->input_mode == Ged_input_mode_selresize ? Ged_input_mode_normal
                                                                      : Ged_input_mode_selresize;
            a->is_draw_dirty = true;
            break;
        case Ged_input_cmd_toggle_slide_mode:
            a->input_mode = a->input_mode == Ged_input_mode_slide ? Ged_input_mode_normal
                                                                  : Ged_input_mode_slide;
            a->is_draw_dirty = true;
            break;
        case Ged_input_cmd_step_forward:
            undo_history_push(&a->undo_hist, &a->field, a->tick_num);
            clear_and_run_vm(
                a->field.buffer,
                a->mbuf_r.buffer,
                a->field.height,
                a->field.width,
                a->tick_num,
                &a->oevent_list,
                a->random_seed);
            ++a->tick_num;
            a->activity_counter += a->oevent_list.count;
            a->needs_remarking = true;
            a->is_draw_dirty = true;
            break;
        case Ged_input_cmd_toggle_play_pause:
            ged_set_playing(a, !a->is_playing);
            break;
        case Ged_input_cmd_toggle_show_event_list:
            a->draw_event_list = !a->draw_event_list;
            a->is_draw_dirty = true;
            break;
        case Ged_input_cmd_cut:
            if (ged_copy_selection_to_clipbard(a)) {
                undo_history_push(&a->undo_hist, &a->field, a->tick_num);
                ged_fill_selection_with_char(a, '.');
                a->needs_remarking = true;
                a->is_draw_dirty = true;
            }
            break;
        case Ged_input_cmd_copy:
            ged_copy_selection_to_clipbard(a);
            break;
        case Ged_input_cmd_paste: {
            Usz field_h = a->field.height;
            Usz field_w = a->field.width;
            Usz curs_y = a->ged_cursor.y;
            Usz curs_x = a->ged_cursor.x;
            if (curs_y >= field_h || curs_x >= field_w)
                break;
            Field *cb_field = &a->clipboard_field;
            Usz cbfield_h = cb_field->height;
            Usz cbfield_w = cb_field->width;
            Usz cpy_h = cbfield_h;
            Usz cpy_w = cbfield_w;
            if (field_h - curs_y < cpy_h)
                cpy_h = field_h - curs_y;
            if (field_w - curs_x < cpy_w)
                cpy_w = field_w - curs_x;
            if (cpy_h == 0 || cpy_w == 0)
                break;
            undo_history_push(&a->undo_hist, &a->field, a->tick_num);
            gbuffer_copy_subrect(
                cb_field->buffer,
                a->field.buffer,
                cbfield_h,
                cbfield_w,
                field_h,
                field_w,
                0,
                0,
                curs_y,
                curs_x,
                cpy_h,
                cpy_w);
            a->ged_cursor.h = cpy_h;
            a->ged_cursor.w = cpy_w;
            a->needs_remarking = true;
            a->is_draw_dirty = true;
            break;
        }
        case Ged_input_cmd_escape:
            if (a->input_mode != Ged_input_mode_normal) {
                a->input_mode = Ged_input_mode_normal;
                a->is_draw_dirty = true;
            } else if (a->ged_cursor.h != 1 || a->ged_cursor.w != 1) {
                a->ged_cursor.h = 1;
                a->ged_cursor.w = 1;
                a->is_draw_dirty = true;
            }
            break;
    }
}
