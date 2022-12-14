#pragma once
#include "base.h"

enum
{
    Oevent_osc_int_count = 35,
    Oevent_udp_string_count = 16
};

typedef enum
{
    Oevent_type_midi_note,
    Oevent_type_midi_cc,
    Oevent_type_midi_pb,
    Oevent_type_osc_ints,
    Oevent_type_udp_string,
} Oevent_types;

typedef struct {
    U8 oevent_type;
} Oevent_any;

typedef struct {
    U8 oevent_type;
    U8 channel;
    U8 octave;
    U8 note;
    U8 velocity;
    U8 duration : 7;
    U8 mono : 1;
} Oevent_midi_note;

typedef struct {
    U8 oevent_type;
    U8 channel;
    U8 control;
    U8 value;
} Oevent_midi_cc;

typedef struct {
    U8 oevent_type;
    U8 channel;
    U8 lsb;
    U8 msb;
} Oevent_midi_pb;

typedef struct {
    U8 oevent_type;
    Glyph glyph;
    U8 count;
    U8 numbers[Oevent_osc_int_count];
} Oevent_osc_ints;

typedef struct {
    U8 oevent_type;
    U8 count;
    char chars[Oevent_udp_string_count];
} Oevent_udp_string;

typedef union {
    Oevent_any any;
    Oevent_midi_note midi_note;
    Oevent_midi_cc midi_cc;
    Oevent_midi_pb midi_pb;
    Oevent_osc_ints osc_ints;
    Oevent_udp_string udp_string;
} Oevent;

typedef struct {
    Oevent *buffer;
    Usz count, capacity;
} Oevent_list;

void oevent_list_init(Oevent_list *olist);
void oevent_list_deinit(Oevent_list *olist);
void oevent_list_clear(Oevent_list *olist);
ORCA_NOINLINE
void oevent_list_copy(Oevent_list const *src, Oevent_list *dest);
ORCA_NOINLINE
Oevent *oevent_list_alloc_item(Oevent_list *olist);
