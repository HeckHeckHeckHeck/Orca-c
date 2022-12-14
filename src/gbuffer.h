#pragma once
#include "base.h"

void gbuffer_copy_subrect(
    Glyph *src,
    Glyph *dest,
    Usz src_height,
    Usz src_width,
    Usz dest_height,
    Usz dest_width,
    Usz src_y,
    Usz src_x,
    Usz dest_y,
    Usz dest_x,
    Usz height,
    Usz width);

void gbuffer_fill_subrect(
    Glyph *gbuffer,
    Usz f_height,
    Usz f_width,
    Usz y,
    Usz x,
    Usz height,
    Usz width,
    Glyph fill_char);

Glyph gbuffer_peek_relative(Glyph *gbuf, Usz height, Usz width, Usz y, Usz x, Isz delta_y, Isz delta_x);

void gbuffer_poke(Glyph *gbuf, Usz height, Usz width, Usz y, Usz x, Glyph g);

void gbuffer_poke_relative(Glyph *gbuf, Usz height, Usz width, Usz y, Usz x, Isz delta_y, Isz delta_x, Glyph g);

typedef enum
{
    Mark_flag_none = 0,
    Mark_flag_input = 1 << 0,
    Mark_flag_output = 1 << 1,
    Mark_flag_haste_input = 1 << 2,
    Mark_flag_lock = 1 << 3,
    Mark_flag_sleep = 1 << 4,
} Mark_flags;

Mark_flags mbuffer_peek(Mark *mbuf, Usz height, Usz width, Usz y, Usz x);

Mark_flags mbuffer_peek_relative(Mark *mbuf, Usz height, Usz width, Usz y, Usz x, Isz offs_y, Isz offs_x);

void mbuffer_poke_flags_or(Mark *mbuf, Usz height, Usz width, Usz y, Usz x, Mark_flags flags);

void mbuffer_poke_relative_flags_or(
    Mark *mbuf,
    Usz height,
    Usz width,
    Usz y,
    Usz x,
    Isz offs_y,
    Isz offs_x,
    Mark_flags flags);


void mbuffer_clear(Mark *mbuf, Usz height, Usz width);

