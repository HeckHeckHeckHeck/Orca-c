#pragma once
#include "oso.h"
#include "osc_out.h"
#ifdef FEAT_PORTMIDI
    #include <portmidi.h>
#endif

typedef enum
{
    Midi_mode_type_null,
    Midi_mode_type_osc_bidule,
#ifdef FEAT_PORTMIDI
    Midi_mode_type_portmidi,
#endif
} Midi_mode_type;

typedef struct {
    Midi_mode_type type;
} Midi_mode_any;

typedef struct {
    Midi_mode_type type;
    char const *path;
} Midi_mode_osc_bidule;

#ifdef FEAT_PORTMIDI
typedef struct {
    Midi_mode_type type;
    PmDeviceID device_id;
    PortMidiStream *stream;
} Midi_mode_portmidi;
// Not sure whether it's OK to call Pm_Terminate() without having a successful
// call to Pm_Initialize() -- let's just treat it with tweezers.
//bool portmidi_is_initialized = false;
#endif

typedef union {
    Midi_mode_any any;
    Midi_mode_osc_bidule osc_bidule;
#ifdef FEAT_PORTMIDI
    Midi_mode_portmidi portmidi;
#endif
} Midi_mode;


void midi_mode_deinit(Midi_mode *mm);

void midi_mode_init_osc_bidule(Midi_mode *mm, char const *path);

void send_midi_note_offs(
    Oosc_dev *oosc_dev,
    Midi_mode *midi_mode,
    Susnote const *start,
    Susnote const *end);

void send_midi_chan_msg(
    Oosc_dev *oosc_dev,
    Midi_mode const *midi_mode,
    int type /*0..15*/,
    int chan /*0.. 15*/,
    int byte1 /*0..127*/,
    int byte2 /*0..127*/);

void midi_mode_init_null(Midi_mode *mm);

void send_num_message(Oosc_dev *oosc_dev, char const *osc_address, I32 num);

void send_midi_byte(Oosc_dev *oosc_dev, Midi_mode const *midi_mode, int x);

void send_control_message(Oosc_dev *oosc_dev, char const *osc_address);

PmError portmidi_init_if_necessary(void);

bool portmidi_find_device_id_by_name(char const *name, Usz namelen, PmError *out_pmerror, PmDeviceID *out_id);

PmError midi_mode_init_portmidi(Midi_mode *mm, PmDeviceID dev_id);

bool portmidi_find_name_of_device_id(PmDeviceID id, PmError *out_pmerror, oso **out_name);
