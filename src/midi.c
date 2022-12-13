#include "base.h"
#include "osc_out.h"
#include "oso.h"
#include "midi.h"
#include "sokol_time.h"

PmError portmidi_init_if_necessary(void)
{
//    if (portmidi_is_initialized)
//        return 0;
    PmError e = Pm_Initialize();
    if (e)
        return e;
//    portmidi_is_initialized = true;
    return 0;
}

void midi_mode_init_null(Midi_mode *mm)
{
    mm->any.type = Midi_mode_type_null;
}

void midi_mode_init_osc_bidule(Midi_mode *mm, char const *path)
{
    mm->osc_bidule.type = Midi_mode_type_osc_bidule;
    mm->osc_bidule.path = path;
}

#ifdef FEAT_PORTMIDI
enum
{
    Portmidi_artificial_latency = 1,
};

struct {
    U64 clock_base;
    bool did_init;
} portmidi_global_data;

PmTimestamp portmidi_timestamp_now(void)
{
    if (!portmidi_global_data.did_init) {
        portmidi_global_data.did_init = true;
        portmidi_global_data.clock_base = stm_now();
    }
    return (PmTimestamp)(stm_ms(stm_since(portmidi_global_data.clock_base)));
}

PmTimestamp portmidi_timeproc(void *time_info)
{
    (void)time_info;
    return portmidi_timestamp_now();
}


PmError midi_mode_init_portmidi(Midi_mode *mm, PmDeviceID dev_id)
{
    PmError e = portmidi_init_if_necessary();
    if (e)
        goto fail;
    e = Pm_OpenOutput(
        &mm->portmidi.stream,
        dev_id,
        NULL,
        128,
        portmidi_timeproc,
        NULL,
        Portmidi_artificial_latency);
    if (e)
        goto fail;
    mm->portmidi.type = Midi_mode_type_portmidi;
    mm->portmidi.device_id = dev_id;
    return pmNoError;
fail:
    midi_mode_init_null(mm);
    return e;
}
// Returns true on success. todo currently output only
bool portmidi_find_device_id_by_name(
    char const *name,
    Usz namelen,
    PmError *out_pmerror,
    PmDeviceID *out_id)
{
    *out_pmerror = portmidi_init_if_necessary();
    if (*out_pmerror)
        return false;
    int num = Pm_CountDevices();
    for (int i = 0; i < num; ++i) {
        PmDeviceInfo const *info = Pm_GetDeviceInfo(i);
        if (!info || !info->output)
            continue;
        Usz len = strlen(info->name);
        if (len != namelen)
            continue;
        if (strncmp(name, info->name, namelen) == 0) {
            *out_id = i;
            return true;
        }
    }
    return false;
}

bool portmidi_find_name_of_device_id(PmDeviceID id, PmError *out_pmerror, oso **out_name)
{
    *out_pmerror = portmidi_init_if_necessary();
    if (*out_pmerror)
        return false;
    int num = Pm_CountDevices();
    if (id < 0 || id >= num)
        return false;
    PmDeviceInfo const *info = Pm_GetDeviceInfo(id);
    if (!info || !info->output)
        return false;
    osoput(out_name, info->name);
    return true;
}
#endif

void midi_mode_deinit(Midi_mode *mm)
{
    switch (mm->any.type) {
        case Midi_mode_type_null:
        case Midi_mode_type_osc_bidule:
            break;
#ifdef FEAT_PORTMIDI
        case Midi_mode_type_portmidi:
            // Because PortMidi seems to work correctly ony more platforms when using
            // its timing stuff, we are using it. And because we are using it, and
            // because it may be buffering events for sending 'later', we might have
            // pending outgoing MIDI events. We'll need to wait until they finish being
            // before calling Pm_Close, otherwise users could have problems like MIDI
            // notes being stuck on. This is slow and blocking, but not much we can do
            // about it right now.
            //
            // TODO use nansleep on platforms that support it.
            for (U64 start = stm_now();
                 stm_ms(stm_since(start)) <= (double)Portmidi_artificial_latency;)
                sleep(0);
            Pm_Close(mm->portmidi.stream);
            break;
#endif
    }
}


void send_midi_3bytes(Oosc_dev *oosc_dev, Midi_mode const *midi_mode, int status, int byte1, int byte2)
{
    switch (midi_mode->any.type) {
        case Midi_mode_type_null:
            break;
        case Midi_mode_type_osc_bidule: {
            if (!oosc_dev)
                break;
            oosc_send_int32s(oosc_dev, midi_mode->osc_bidule.path, (int[]){ status, byte1, byte2 }, 3);
            break;
        }
#ifdef FEAT_PORTMIDI
        case Midi_mode_type_portmidi: {
            // timestamp is totally fake, to prevent problems with some MIDI systems
            // getting angry if there's no timestamping info.
            //
            // Eventually, we will want to create real timestamps based on a real orca
            // clock, instead of ad-hoc at the last moment like this. When we do that,
            // we'll need to thread the timestamping/timing info through the function
            // calls, instead of creating it at the last moment here. (This timestamp
            // is actually 'useless', because it doesn't convey any additional
            // information. But if we don't provide it, at least to PortMidi, some
            // people's MIDI setups may malfunction and have terrible timing problems.)
            PmTimestamp pm_timestamp = portmidi_timestamp_now();
            PmError pme = Pm_WriteShort(
                midi_mode->portmidi.stream,
                pm_timestamp,
                Pm_Message(status, byte1, byte2));
            (void)pme;
            break;
        }
#endif
    }
}

void send_midi_chan_msg(
    Oosc_dev *oosc_dev,
    Midi_mode const *midi_mode,
    int type /*0..15*/,
    int chan /*0.. 15*/,
    int byte1 /*0..127*/,
    int byte2 /*0..127*/)
{
    send_midi_3bytes(oosc_dev, midi_mode, type << 4 | chan, byte1, byte2);
}

void send_midi_note_offs(
    Oosc_dev *oosc_dev,
    Midi_mode *midi_mode,
    Susnote const *start,
    Susnote const *end)
{
    for (; start != end; ++start) {
#if 0
    float under = start->remaining;
    if (under < 0.0) {
      fprintf(stderr, "cutoff slop: %f\n", under);
    }
#endif
        U16 chan_note = start->chan_note;
        send_midi_chan_msg(oosc_dev, midi_mode, 0x8, chan_note >> 8, chan_note & 0xFF, 0);
    }
}

void send_control_message(Oosc_dev *oosc_dev, char const *osc_address)
{
    if (!oosc_dev)
        return;
    oosc_send_int32s(oosc_dev, osc_address, NULL, 0);
}

void send_num_message(Oosc_dev *oosc_dev, char const *osc_address, I32 num)
{
    if (!oosc_dev)
        return;
    I32 nums[1];
    nums[0] = num;
    oosc_send_int32s(oosc_dev, osc_address, nums, ORCA_ARRAY_COUNTOF(nums));
}

void send_midi_byte(Oosc_dev *oosc_dev, Midi_mode const *midi_mode, int x)
{
    // PortMidi wants 0 and 0 for the unused bytes. Likewise, Bidule's
    // MIDI-via-OSC won't accept the message unless there are at least all 3
    // bytes, with the second 2 set to zero.
    send_midi_3bytes(oosc_dev, midi_mode, x, 0, 0);
}
