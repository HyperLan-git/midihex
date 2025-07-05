#ifndef MIDIFILE_H
#define MIDIFILE_H

#include <cstdbool>
#include <cstring>
#include <vector>

#include "Ints.hpp"

// Variable length quantity (encoded values between 8 and 28 bits)
typedef u32 v_len;

// XXX I didnt even need to implement system messages wtf....
#pragma region ENUMS
enum MidiTrackType {
    MULTI_CHANNEL = 0,  // single track file format
    TRACKS = 1,         // multiple tracks file format
    // TODO add actual support to those
    SONGS = 2  // multiple songs file format
};

enum MidiEventType {
    NOTE_OFF = 0x8,
    NOTE_ON = 0x9,
    POLY_AFTERTOUCH = 0xA,  // Polyphonic aftertouch
    CC = 0xB,
    PROGRAM_CHANGE = 0xC,
    AFTERTOUCH = 0xD,
    PITCH_WHEEL = 0xE
};

// USELESS for midi files
enum SystemEventType {
    SYSEX = 0xF0,
    UND0 = 0xF1,
    SONG_POSITION = 0xF2,
    SONG_SELECT = 0xF3,
    UND1 = 0xF4,
    UND2 = 0xF5,
    TUNE = 0xF6,
    SYSEX_END = 0xF7,

    // Real time events
    TIMING_CLOCK = 0xF8,
    UND3 = 0xF9,
    START = 0xFA,
    CONTINUE = 0xFB,
    STOP = 0xFC,
    UND4 = 0xFD,
    ACTIVE_SENSING = 0xFE,
    RESET = 0xFF
};

enum MetaEventType {
    SEQUENCE_NUMBER = 0x0,
    TEXT = 0x1,
    COPYRIGHT = 0x2,
    NAME = 0x3,
    INSTRUMENT_NAME = 0x4,
    LYRIC = 0x5,
    MARKER = 0x6,  // Sections of song like "First verse", "Second chorus"...
    CUE = 0x7,     // Description of things happening on stage
    DEVICE_NAME = 0x9,
    MIDI_CHANNEL_PREFIX = 0x20,
    END_OF_TRACK = 0x2F,
    SET_TEMPO = 0x51,
    SMPTE_OFFSET = 0x54,
    TIME_SIGNATURE = 0x58,
    KEY_SIGNATURE = 0x59,
    SPECIFIC = 0x7F  // Vendor specific
};

enum TrackEventType { UNKOWN, MIDI, META, SYSEX_EVENT, SYSTEM_EVENT };

enum MidiError {
    NONE,
    V_LEN_INVALID,
    UNEXPECTED_EOF,
    INVALID_HEADER,
    INVALID_TRACK,
    NOT_ENOUGH_MEMORY,
    INVALID_EVENT
};
#pragma endregion

#define DEBUG
#ifdef DEBUG
#include <iostream>
#define DEBG_PRINT(message) std::cout << message << std::endl;
#else
#define DEBG_PRINT(message) ;
#endif

#pragma region STRUCTS
struct MidiEvent {
    u8 type : 4;
    u8 channel : 4;

    u8 data0;
    u8 data1;
};

struct SystemEvent {
    u8 type;

    u8 data0;
    u8 data1;
};

struct SMPTETime {
    u8 hours;
    u8 minutes;
    u8 seconds;
    u8 frames;
    u8 frameFractions;
};

struct TimeSignature {
    u8 numerator;
    u8 denominator;
    u8 TPM;           // clock cycles per metronome tick
    u8 noteDivision;  // 32nd notes per quarter note
};

struct KeySignature {
    i8 sharps;  // negative = flats
    u8 minor;
};

struct MetaEvent {
    u8 type;

    union {
        struct {
            u8 *data;  // Owned
            v_len length;
        };
        u8 channel;  // midi channel prefix
        u32 MPQ;     // set tempo
        u16 seqNumber;
        struct SMPTETime startTime;
        struct TimeSignature timeSignature;
        struct KeySignature key;
    };

    MetaEvent(u8 type) : type(type) {}

    // For events with text data
    MetaEvent(u8 type, u8 *data, v_len len)
        : type(type), data(data), length(len) {}

    // For set tempo
    MetaEvent(u8 type, u32 MPQ) : type(type), MPQ(MPQ) {}

    // For SMPTE offset
    MetaEvent(struct SMPTETime time) : type(SMPTE_OFFSET), startTime(time) {}

    // For time signature
    MetaEvent(struct TimeSignature timeSig)
        : type(TIME_SIGNATURE), timeSignature(timeSig) {}

    // For key signature
    MetaEvent(struct KeySignature keySig) : type(KEY_SIGNATURE), key(keySig) {}

    MetaEvent(const MetaEvent &cpy) {
        std::memcpy(this, &cpy, sizeof(MetaEvent));
        switch (type) {
            case TEXT:
            case COPYRIGHT:
            case NAME:
            case INSTRUMENT_NAME:
            case DEVICE_NAME:
            case LYRIC:
            case MARKER:
            case CUE:
            case SPECIFIC:
            default:
                // + 1 for the null char
                this->data = new u8[cpy.length + 1];
                std::memcpy(this->data, cpy.data, cpy.length + 1);
                break;
            case END_OF_TRACK:
            case SET_TEMPO:
            case MIDI_CHANNEL_PREFIX:
            case SMPTE_OFFSET:
            case SEQUENCE_NUMBER:
            case TIME_SIGNATURE:
            case KEY_SIGNATURE:
                break;
        }
    }

    MetaEvent(MetaEvent &&mov) {
        std::memcpy(this, &mov, sizeof(MetaEvent));
        switch (type) {
            case TEXT:
            case COPYRIGHT:
            case NAME:
            case INSTRUMENT_NAME:
            case DEVICE_NAME:
            case LYRIC:
            case MARKER:
            case CUE:
            case SPECIFIC:
            default:
                mov.data = nullptr;
                break;
            case SEQUENCE_NUMBER:
            case END_OF_TRACK:
            case SET_TEMPO:
            case MIDI_CHANNEL_PREFIX:
            case SMPTE_OFFSET:
            case TIME_SIGNATURE:
            case KEY_SIGNATURE:
                break;
        }
    }

    ~MetaEvent() {
        switch (type) {
            case TEXT:
            case COPYRIGHT:
            case NAME:
            case INSTRUMENT_NAME:
            case DEVICE_NAME:
            case LYRIC:
            case MARKER:
            case CUE:
            case SPECIFIC:
            default:
                delete[] data;
                break;
            case SEQUENCE_NUMBER:
            case END_OF_TRACK:
            case SET_TEMPO:
            case SMPTE_OFFSET:
            case MIDI_CHANNEL_PREFIX:
            case TIME_SIGNATURE:
            case KEY_SIGNATURE:
                break;
        }
    }
};

struct SysExEvent {
    v_len length;

    u8 *data = nullptr;
    // + 1 length for the null char

    SysExEvent() {}
    SysExEvent(u8 *data, v_len len) : length(len), data(data) {}
    SysExEvent(v_len len) : length(len), data(new u8[len + 1]) {}
    SysExEvent(const SysExEvent &cpy)
        : length(cpy.length), data(new u8[cpy.length + 1]) {
        if (length > 0) std::memcpy(data, cpy.data, cpy.length + 1);
    }
    SysExEvent(SysExEvent &&mov) : length(mov.length) {
        std::swap(mov.data, this->data);
    }

    ~SysExEvent() { delete[] data; }
};

struct TrackEvent {
    v_len deltaTime;
    v_len time;

    enum TrackEventType type;
    union {
        struct MidiEvent midi;
        struct SystemEvent sys;
        struct MetaEvent *meta;
        struct SysExEvent *sysex;
    };

    TrackEvent() { type = UNKOWN; }

    TrackEvent(struct MidiEvent event, v_len deltaTime = 0, v_len time = 0) {
        this->type = MIDI;
        this->deltaTime = deltaTime;
        this->time = time;
        this->midi = event;
    }

    TrackEvent(const TrackEvent &toCopy) : type(UNKOWN) { *this = toCopy; }

    TrackEvent(TrackEvent &&toMove) { *this = std::move(toMove); }

    ~TrackEvent() {
        switch (type) {
            case MIDI:
            case SYSTEM_EVENT:
            case UNKOWN:
                break;
            case META:
                delete meta;
                break;
            case SYSEX_EVENT:
                delete sysex;
                break;
        }
    }

    constexpr TrackEvent &operator=(TrackEvent &&e) {
        this->deltaTime = e.deltaTime;
        this->type = e.type;

        switch (type) {
            case MIDI:
                this->midi = std::move(e.midi);
                break;
            case SYSTEM_EVENT:
                this->sys = std::move(e.sys);
                break;
            case UNKOWN:
                break;
            case META:
                this->meta = e.meta;
                e.meta = nullptr;
                break;
            case SYSEX_EVENT:
                this->sysex = e.sysex;
                e.sysex = nullptr;
                break;
        }
        return *this;
    }

    constexpr TrackEvent &operator=(const TrackEvent &e) {
        switch (type) {
            case MIDI:
            case SYSTEM_EVENT:
            case UNKOWN:
                break;
            case META:
                delete this->meta;
                break;
            case SYSEX_EVENT:
                delete this->sysex;
                break;
        }
        deltaTime = e.deltaTime;
        time = e.time;
        type = e.type;
        switch (type) {
            case MIDI:
                this->midi = e.midi;
                break;
            case SYSTEM_EVENT:
                this->sys = e.sys;
                break;
            case UNKOWN:
                break;
            case META:
                this->meta = new MetaEvent(*e.meta);
                break;
            case SYSEX_EVENT:
                this->sysex = new SysExEvent(*e.sysex);
                break;
        }
        return *this;
    }
};

struct MidiTrack {
    u32 length;

    bool decoded;
    // TODO fix that thing
    u8 *data;  // Not owned
    std::vector<TrackEvent> list;

    MidiTrack() : length(0), decoded(false), data(NULL) {}

    MidiTrack(MidiTrack &&t)
        : length(t.length),
          decoded(t.decoded),
          data(t.data),
          list(std::move(t.list)) {}

    MidiTrack &operator=(MidiTrack &&t) {
        this->length = t.length;
        this->decoded = t.decoded;
        this->data = t.data;
        this->list = std::move(t.list);
        return *this;
    }
};

struct TempoChange {
    v_len time;
    u64 timeMicros;
    double microsPerTick;
};

struct TimeSignatureChange {
    v_len time;
    u16 bar;
    struct TimeSignature signature;
};

struct MidiFile {
    u32 length;
    u16 format;
    u16 tracks;
    u16 division;

    // Should be an array of tracks for multi-track files
    struct MidiTrack *data = nullptr;

    // TODO put that in track data for type 2 files
    std::vector<TempoChange> timingInfo;
    std::vector<TimeSignatureChange> timeSignatureInfo;
    ~MidiFile() { delete[] data; }
};

struct BarTime {
    u16 bar;
    double barTime;
};

#pragma endregion

#define SZ_FILE_HEADER 14
#define SZ_HEADER_CONTENT 6
#define SZ_TRACK_HEADER 8

#define READ_BIG_ENDIAN_U32(data) \
    ((data[0] << 24) + (data[1] << 16) + (data[2] << 8) + data[3])
#define READ_BIG_ENDIAN_U24(data) ((data[0] << 16) + (data[1] << 8) + data[2])
#define READ_BIG_ENDIAN_U16(data) ((data[0] << 8) + data[1])

enum MidiError readMidiFile(u8 *data, std::size_t length,
                            struct MidiFile *&res);

enum MidiError decodeTrack(struct MidiFile &file, struct MidiTrack &track);

void computeTimes(std::vector<TrackEvent> &track);

void computeTimeMapsForTrack(struct MidiFile &file, struct MidiTrack &track);
void computeTimeSignatureMapForTrack(struct MidiFile &file,
                                     struct MidiTrack &track);
void computeTimingMapForTrack(struct MidiFile &file, struct MidiTrack &track);

// XXX make an operator?
enum MidiError encodeTrackEvent(const struct TrackEvent &event,
                                std::stringstream &stream);
enum MidiError writeMidiFile(struct MidiFile &file, std::ostream &stream);

void printMidiFile(const struct MidiFile &header);

inline double getMicrosPerTick(u16 division, u32 MPB) {
    // See https://www.recordingblogs.com/wiki/time-division-of-a-midi-file
    if (division >> 15 == 0) {
        // division = tick/beat
        // mpb = us/beat
        // us = mpb * beat = mpb * tick / division
        return (1.0 / division * MPB);
    } else {
        // Careful with the division it is stored in negative
        i8 div = -(i8)((division & 0xFF00) >> 8);
        return 1000000.0 / (div * (division & 0xFF));
    }
}

// Perform BSearch to find last event just before a time
template <typename T>
const T &getLastBefore(const std::vector<T> &events, v_len time) {
    if (events.empty()) throw std::runtime_error("Empty list to BSearch in !");
    std::size_t idx = 0, idx2 = events.size() - 1;
    std::size_t mid = (idx + idx2) / 2;
    while (idx + 1 < idx2) {
        if (events[mid].time > time) {
            idx2 = mid;
        } else {
            idx = mid;
        }
        mid = (idx + idx2) / 2;
    }
    if (events[idx2].time <= time) {
        return events[idx2];
    }
    if (idx2 > 0) return events[idx2 - 1];
    return events[0];
}

inline struct BarTime getBar(
    const std::vector<TimeSignatureChange> &timeSignatureInfo, v_len time) {
    if (time == 0) return BarTime{.bar = 0, .barTime = 0};
    const struct TimeSignatureChange &sig =
        getLastBefore<TimeSignatureChange>(timeSignatureInfo, time);
    double bar =
        sig.bar + (time - sig.time) * (1 << sig.signature.denominator) /
                      (double)sig.signature.TPM / sig.signature.numerator / 16;
    return BarTime{.bar = (u16)bar,
                   .barTime = (bar - (u16)bar) * sig.signature.numerator /
                              (1 << sig.signature.denominator) * 4};
}

inline u64 getTimeMicros(const std::vector<TempoChange> &timingInfo,
                         v_len time) {
    if (time == 0) return 0;
    const struct TempoChange &timing = getLastBefore(timingInfo, time);
    return (time - timing.time) * timing.microsPerTick + timing.timeMicros;
}

inline u64 getTimeMicros(const TempoChange &lastTempoChange, v_len time) {
    return (time - lastTempoChange.time) * lastTempoChange.microsPerTick +
           lastTempoChange.timeMicros;
}

#endif /* MIDIFILE_H */
