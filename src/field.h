#pragma once
#include "base.h"
#include <stdio.h> // FILE cannot be forward declared

// A reusable buffer for the per-grid-cell flags. Similar to how Field is a
// reusable buffer for Glyph, Mbuf_reusable is for Mark. The naming isn't so
// great. Also like Field, the VM doesn't have to care about the buffer being
// reusable -- it only cares about a 'Mark*' type. (With the same dimensions of
// the 'Field*' buffer, since it uses them together.) There are no procedures
// for saving/loading Mark* buffers to/from disk, since we currently don't need
// that functionality.

typedef struct Mbuf_reusable {
    Mark *buffer;
    Usz capacity;
} Mbuf_reusable;

// A reusable buffer for glyphs, stored with its dimensions. Also some helpers
// for loading/saving from files and doing common operations that a UI layer
// might want to do. Not used by the VM.

typedef struct {
    Glyph *buffer;
    U16 width, height;
} Field;

typedef enum
{
    Field_load_error_ok = 0,
    Field_load_error_cant_open_file = 1,
    Field_load_error_too_many_columns = 2,
    Field_load_error_too_many_rows = 3,
    Field_load_error_no_rows_read = 4,
    Field_load_error_not_a_rectangle = 5,
} Field_load_error;

void field_init(Field *field);
void field_init_fill(Field *field, Usz height, Usz width, Glyph fill_char);
void field_deinit(Field *field);
void field_resize_raw(Field *field, Usz height, Usz width);
void field_resize_raw_if_necessary(Field *field, Usz height, Usz width);
void field_copy(Field *src, Field *dest);
void field_fput(Field *field, FILE *stream);

Field_load_error field_load_file(char const *filepath, Field *field);

char const *field_load_error_string(Field_load_error fle);

void mbuf_reusable_init(Mbuf_reusable *mbr);
void mbuf_reusable_ensure_size(Mbuf_reusable *mbr, Usz height, Usz width);
void mbuf_reusable_deinit(Mbuf_reusable *mbr);

// ------------------------------------------------------------
// FIELD-UNDO
// ------------------------------------------------------------

typedef struct Undo_node {
    Field field;
    Usz tick_num;
    struct Undo_node *prev, *next;
} Undo_node;

typedef struct {
    Undo_node *first, *last;
    Usz count, limit;
} Undo_history;

static void undo_history_init(Undo_history *hist, Usz limit)
{
    *hist = (Undo_history){ 0 };
    hist->limit = limit;
}

static void undo_history_deinit(Undo_history *hist)
{
    Undo_node *a = hist->first;
    while (a) {
        Undo_node *b = a->next;
        field_deinit(&a->field);
        free(a);
        a = b;
    }
}

staticni bool undo_history_push(Undo_history *hist, Field *field, Usz tick_num)
{
    if (hist->limit == 0)
        return false;
    Undo_node *new_node;
    if (hist->count == hist->limit) {
        new_node = hist->first;
        if (new_node == hist->last) {
            hist->first = NULL;
            hist->last = NULL;
        } else {
            hist->first = new_node->next;
            hist->first->prev = NULL;
        }
    } else {
        new_node = malloc(sizeof(Undo_node));
        if (!new_node)
            return false;
        ++hist->count;
        field_init(&new_node->field);
    }
    field_copy(field, &new_node->field);
    new_node->tick_num = tick_num;
    if (hist->last) {
        hist->last->next = new_node;
        new_node->prev = hist->last;
    } else {
        hist->first = new_node;
        hist->last = new_node;
        new_node->prev = NULL;
    }
    new_node->next = NULL;
    hist->last = new_node;
    return true;
}


staticni void undo_history_apply(Undo_history *hist, Field *out_field, Usz *out_tick_num)
{
    Undo_node *last = hist->last;
    if (!last)
        return;
    field_copy(&last->field, out_field);
    *out_tick_num = last->tick_num;
}

static Usz undo_history_count(Undo_history *hist)
{
    return hist->count;
}

staticni void undo_history_pop(Undo_history *hist, Field *out_field, Usz *out_tick_num)
{
    Undo_node *last = hist->last;
    if (!last)
        return;
    field_copy(&last->field, out_field);
    *out_tick_num = last->tick_num;
    if (hist->first == last) {
        hist->first = NULL;
        hist->last = NULL;
    } else {
        Undo_node *new_last = last->prev;
        new_last->next = NULL;
        hist->last = new_last;
    }
    field_deinit(&last->field);
    free(last);
    --hist->count;
}


