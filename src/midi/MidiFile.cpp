#include "MidiFile.hpp"

#include <cstddef>
#include <cstring>
#include <iomanip>
#include <iostream>

#pragma region UTILS
const v_len V_LEN_ERROR = -1;

v_len readVarLen(u8*& data, u8* end) {
    u8* ptr = data;
    v_len result = (*ptr) & 0x7f;
    while (((*ptr) & 0x80) != 0) {
        ptr++;
        result <<= 7;
        result += (*ptr) & 0x7f;
        if (ptr > end) return V_LEN_ERROR;
    }
    ptr++;
    data = ptr;
    return result;
}

bool matchesHeader(u8* data, const char* content) {
    return data[0] == content[0] && data[1] == content[1] &&
           data[2] == content[2] && data[3] == content[3];
}
#pragma endregion

#pragma region READ
// Does not handle the allocation of the result
enum MidiError readMidiTrackHeader(u8* data, u8* end,
                                   struct MidiTrack& result) {
    if (data + SZ_TRACK_HEADER > end) {
        DEBG_PRINT("EOF !");
        return UNEXPECTED_EOF;
    }
    if (!matchesHeader(data, "MTrk")) {
        DEBG_PRINT("INVALID TRACK !");
        return INVALID_TRACK;
    }
    data += 4;

    result.length = READ_BIG_ENDIAN_U32(data);
    data += 4;
    if (data + result.length > end) return UNEXPECTED_EOF;

    result.data = data;

    return NONE;
}

#define CHECK_DATA_REMAINS(n) \
    if (data + n > end) return INVALID_EVENT;
#define CHECK_DATA_REMAINS_META(n) \
    if (data + n > end) {          \
        delete e.meta;             \
        return INVALID_EVENT;      \
    }

// TODO parallelize this
enum MidiError decodeMidiMessage(u8*& readData, u8* end, struct TrackEvent& e,
                                 u8 currentEventType, u8& prevEventType) {
    u8* data = readData;
    const u8 b = currentEventType;
    const u8 t1 = b >> 4;
    if (t1 >= NOTE_OFF && t1 <= PITCH_WHEEL) {
        e.type = MIDI;
        e.midi.channel = b & 0xF;
        e.midi.type = t1;
        switch (t1) {
            case NOTE_OFF:
            case NOTE_ON:
            case POLY_AFTERTOUCH:
            case CC:
            case PITCH_WHEEL:
                CHECK_DATA_REMAINS(2);
                e.midi.data0 = data[1];
                e.midi.data1 = data[2];
                data += 2;
                break;
            case PROGRAM_CHANGE:
            case AFTERTOUCH:
                CHECK_DATA_REMAINS(1);
                e.midi.data0 = data[1];
                data++;
                break;
        }
    } else if (b < TIMING_CLOCK && b > SYSEX) {
        e.type = SYSTEM_EVENT;
        e.sys.type = b;
        switch (b) {
            case UND0:
            case UND1:
            case UND2:
            case TUNE:
                break;
            case SONG_POSITION:
                CHECK_DATA_REMAINS(2);
                e.sys.data0 = data[1];
                e.sys.data1 = data[2];
                data += 2;
                break;
            case SONG_SELECT:
                CHECK_DATA_REMAINS(1);
                e.sys.data0 = data[1];
                data++;
                break;
            case SYSEX_END:
                CHECK_DATA_REMAINS(2);
                data++;
                e.meta = new MetaEvent(SYSEX_END);
                e.meta->length = readVarLen(data, end);
                e.meta->data = new u8[e.meta->length + 1];
                CHECK_DATA_REMAINS_META(e.meta->length);
                std::memcpy(e.meta->data, data, e.meta->length);
                e.meta->data[e.meta->length] = 0;
                data += 1 + data[1];
                break;
        }
    } else if (b == 0xFF && data + 1 < end && data[1] < 0x80) {
        e.type = META;
        data++;
        e.meta = new MetaEvent(data[0]);
        switch (data[0]) {
            case SEQUENCE_NUMBER:
                if (data[1] == 2) {
                    CHECK_DATA_REMAINS_META(4);
                    e.meta->seqNumber = (data[2] << 8) + data[3];
                    data += 4;
                } else {
                    e.meta->seqNumber = 0;
                    data++;
                }
                break;
            case END_OF_TRACK:
                data++;
                break;
            default:
            case TEXT:
            case COPYRIGHT:
            case NAME:
            case INSTRUMENT_NAME:
            case LYRIC:
            case MARKER:
            case CUE:
            case SPECIFIC:
            case DEVICE_NAME:
                CHECK_DATA_REMAINS_META(1);
                data++;
                e.meta->length = readVarLen(data, end);
                CHECK_DATA_REMAINS_META(e.meta->length);
                e.meta->data = new u8[e.meta->length + 1];
                std::memcpy(e.meta->data, data, e.meta->length);
                e.meta->data[e.meta->length] = 0;
                data--;
                data += e.meta->length;
                break;
            case MIDI_CHANNEL_PREFIX:
                CHECK_DATA_REMAINS_META(2);
                e.meta->channel = data[2];
                data += 2;
                break;
            case SET_TEMPO:
                CHECK_DATA_REMAINS_META(4);
                e.meta->MPQ = READ_BIG_ENDIAN_U24((data + 2));
                data += 4;
                break;
            case SMPTE_OFFSET:
                CHECK_DATA_REMAINS_META(6);
                e.meta->startTime.hours = data[2];
                e.meta->startTime.minutes = data[3];
                e.meta->startTime.seconds = data[4];
                e.meta->startTime.frames = data[5];
                e.meta->startTime.frameFractions = data[6];
                data += 6;
                break;
            case TIME_SIGNATURE:
                CHECK_DATA_REMAINS_META(5);
                e.meta->timeSignature.numerator = data[2];
                e.meta->timeSignature.denominator = data[3];
                e.meta->timeSignature.TPM = data[4];
                e.meta->timeSignature.noteDivision = data[5];
                data += 5;
                break;
            case KEY_SIGNATURE:
                CHECK_DATA_REMAINS_META(3);
                e.meta->key.sharps = data[2];
                e.meta->key.minor = data[3];
                data += 3;
                break;
        }
    } else if (b == SYSEX) {
        e.type = SYSEX_EVENT;
        CHECK_DATA_REMAINS(1);
        e.sysex = new SysExEvent();
        data++;
        {
            v_len len;
            for (len = 0; data + len < end; len++) {
                if ((data[len] >> 7) == 1) break;
            }
            if (data + len >= end) {
                delete e.sysex;
                return INVALID_EVENT;
            }
            e.sysex->length = len;
            e.sysex->data = new u8[len + 1];
            std::memcpy(e.sysex->data, data + 1, len);
            e.sysex->data[len] = 0;
        }
        data += 2 + data[1];
    } else {
        // Running status
        if (prevEventType == 0) return INVALID_EVENT;
        readData = data - 1;
        return decodeMidiMessage(readData, end, e, prevEventType,
                                 prevEventType);
    }
    prevEventType = b;
    readData = data;
    return NONE;
}

enum MidiError decodeMidiMessage(u8*& readData, u8* end, struct TrackEvent& e,
                                 u8& prevEventType) {
    return decodeMidiMessage(readData, end, e, *readData, prevEventType);
}
#include <cstring>

enum MidiError decodeMidiMessages(u8* data, u8* end,
                                  std::vector<TrackEvent>& res) {
    u32 listSz = (u32)((end - data) / 3.5f);
    res.resize(listSz);
    u32 event = 0;
    u8 prevB = 0;
    u32 time = 0;
    while (data < end) {
        struct TrackEvent& e = res[event];
        e.deltaTime = readVarLen(data, end);
        e.time = (time += e.deltaTime);
        if (e.deltaTime == V_LEN_ERROR) {
            return V_LEN_INVALID;
        }

        enum MidiError err = decodeMidiMessage(data, end, e, prevB);
        if (err != NONE) {
            return err;
        }

        data++;
        event++;
        if (event >= listSz) {
            listSz <<= 1;
            res.resize(listSz);
        }
    }
    res.resize(event);
    return NONE;
}

void computeTimes(std::vector<TrackEvent>& res) {
    v_len time = 0;
    for (TrackEvent& e : res) {
        time += e.deltaTime;
        e.time = time;
    }
}

// TODO reorder the result so we don't get owned by nonstandard files
void computeTimeMapsForTrack(struct MidiFile& file, struct MidiTrack& track) {
    for (const TrackEvent& e : track.list) {
        if (e.type == META) {
            if (e.meta->type == TIME_SIGNATURE) {
                TimeSignatureChange res{
                    .time = e.time,
                    .bar = getBar(file.timeSignatureInfo, e.time).bar,
                    .signature = e.meta->timeSignature};
                file.timeSignatureInfo.emplace_back(res);
            } else if (e.meta->type == SET_TEMPO) {
                TempoChange res{
                    .time = e.time,
                    .timeMicros = getTimeMicros(file.timingInfo, e.time),
                    .microsPerTick =
                        getMicrosPerTick(file.division, e.meta->MPQ)};
                file.timingInfo.emplace_back(res);
            }
        }
    }
    if (file.timingInfo.empty()) {
        // default 120 bpm
        file.timingInfo.emplace_back(TempoChange{
            .time = 0,
            .timeMicros = 0,
            .microsPerTick = getMicrosPerTick(file.division, 500000)});
    }
    if (file.timeSignatureInfo.empty()) {
        // default 4/4
        file.timeSignatureInfo.emplace_back(TimeSignatureChange{
            .time = 0,
            .bar = 0,
            .signature = TimeSignature{.numerator = 4,
                                       .denominator = 2,  // 2^2 = 4
                                       .TPM = 24,
                                       .noteDivision = 8}});
    }
}

void computeTimeSignatureMapForTrack(struct MidiFile& file,
                                     struct MidiTrack& track) {
    for (const TrackEvent& e : track.list) {
        if (e.type == META && e.meta->type == TIME_SIGNATURE) {
            TimeSignatureChange res{
                .time = e.time,
                .bar = getBar(file.timeSignatureInfo, e.time).bar,
                .signature = e.meta->timeSignature};
            file.timeSignatureInfo.emplace_back(res);
        }
    }

    if (file.timeSignatureInfo.empty()) {
        // default 4/4
        file.timeSignatureInfo.emplace_back(TimeSignatureChange{
            .time = 0,
            .bar = 0,
            .signature = TimeSignature{.numerator = 4,
                                       .denominator = 2,  // 2^2 = 4
                                       .TPM = 24,
                                       .noteDivision = 8}});
    }
}

void computeTimingMapForTrack(struct MidiFile& file, struct MidiTrack& track) {
    for (const TrackEvent& e : track.list) {
        if (e.type == META && e.meta->type == SET_TEMPO) {
            TempoChange res{
                .time = e.time,
                .timeMicros = getTimeMicros(file.timingInfo, e.time),
                .microsPerTick = getMicrosPerTick(file.division, e.meta->MPQ)};
            file.timingInfo.emplace_back(res);
        }
    }
    if (file.timingInfo.empty()) {
        // default 120 bpm
        file.timingInfo.emplace_back(TempoChange{
            .time = 0,
            .timeMicros = 0,
            .microsPerTick = getMicrosPerTick(file.division, 500000)});
    }
}

enum MidiError decodeTrack(struct MidiFile& file, struct MidiTrack& track) {
    enum MidiError err =
        decodeMidiMessages(track.data, track.data + track.length, track.list);
    if (err != NONE) return err;

    track.decoded = true;
    computeTimeMapsForTrack(file, track);
    return NONE;
}

// NB the data string must not be freed until the tracks have been decoded
enum MidiError readMidiFile(u8* data, std::size_t length,
                            struct MidiFile*& res) {
    if (length < SZ_FILE_HEADER) return UNEXPECTED_EOF;
    if (!matchesHeader(data, "MThd")) return INVALID_HEADER;

    u8* end = data + length;
    data += 4;

    struct MidiFile* result = new MidiFile();

    result->length = READ_BIG_ENDIAN_U32(data);
    data += 4;
    if (result->length != SZ_HEADER_CONTENT) {
        delete result;
        return INVALID_HEADER;
    }
    if (data + SZ_HEADER_CONTENT + SZ_TRACK_HEADER > end) {
        delete result;
        return UNEXPECTED_EOF;
    }

    result->format = READ_BIG_ENDIAN_U16(data);
    data += 2;

    result->tracks = READ_BIG_ENDIAN_U16(data);
    if (result->tracks == 0) {
        delete result;
        return INVALID_HEADER;
    }
    data += 2;

    result->division = READ_BIG_ENDIAN_U16(data);
    data += 2;

    result->data = new MidiTrack[result->tracks];

    for (u16 i = 0; i < result->tracks; i++) {
        result->data[i].length = 0;
        result->data[i].decoded = false;
    }

    for (u16 i = 0; i < result->tracks; i++) {
        enum MidiError err = readMidiTrackHeader(data, end, result->data[i]);
        data += result->data[i].length + SZ_TRACK_HEADER;
        if (err != NONE) {
            delete result;
            return err;
        }
    }
    res = result;
    return NONE;
}
#pragma endregion

#pragma region WRITE

#include <sstream>

#define WRITE_CHAR(stream, c)           \
    {                                   \
        dat = c;                        \
        stream.write(&dat, sizeof(u8)); \
    }

#define WRITE_BIG_ENDIAN_U16(stream, data) \
    WRITE_CHAR(stream, data >> 8);         \
    WRITE_CHAR(stream, data);

#define WRITE_BIG_ENDIAN_U24(stream, data) \
    WRITE_CHAR(stream, data >> 16);        \
    WRITE_BIG_ENDIAN_U16(stream, data)

#define WRITE_BIG_ENDIAN_U32(stream, data) \
    WRITE_CHAR(stream, data >> 24);        \
    WRITE_BIG_ENDIAN_U24(stream, data)

inline void feedDeltaTime(v_len deltaTime, std::stringstream& stream) {
    char dat;
    if (deltaTime >= 1 << (7 * 3))
        WRITE_CHAR(stream, ((deltaTime >> 21) & 0x7F) | 0x80);
    if (deltaTime >= 1 << (7 * 2))
        WRITE_CHAR(stream, ((deltaTime >> 14) & 0x7F) | 0x80);
    if (deltaTime >= 1 << 7)
        WRITE_CHAR(stream, ((deltaTime >> 7) & 0x7F) | 0x80);
    WRITE_CHAR(stream, deltaTime & 0x7F);
}

enum MidiError encodeTrackEvent(const struct TrackEvent& event,
                                std::stringstream& data) {
    char dat;
    feedDeltaTime(event.deltaTime, data);
    switch (event.type) {
        case MIDI:
            WRITE_CHAR(data, (event.midi.type << 4) | event.midi.channel);
            WRITE_CHAR(data, event.midi.data0);
            switch (event.midi.type) {
                default:
                    WRITE_CHAR(data, event.midi.data1);
                    break;
                case PROGRAM_CHANGE:
                case AFTERTOUCH:
                    break;
            }
            break;
        case SYSTEM_EVENT:
            WRITE_CHAR(data, event.sys.type);
            switch (event.sys.type) {
                case UND0:
                case UND1:
                case UND2:
                case TUNE:
                    break;
                case SONG_POSITION:
                    WRITE_CHAR(data, event.sys.data0);
                    WRITE_CHAR(data, event.sys.data1);
                    break;
                case SONG_SELECT:
                    WRITE_CHAR(data, event.sys.data0);
                    break;
            }
            break;
        case META:
            WRITE_CHAR(data, 0xFF);
            WRITE_CHAR(data, event.meta->type);
            switch (event.meta->type) {
                case SEQUENCE_NUMBER:
                    WRITE_CHAR(data, 0x02);
                    WRITE_BIG_ENDIAN_U16(data, event.meta->seqNumber);
                    break;
                case MIDI_CHANNEL_PREFIX:
                    WRITE_CHAR(data, 0x01);
                    WRITE_CHAR(data, event.meta->channel);
                    break;
                case END_OF_TRACK:
                    WRITE_CHAR(data, 0x00);
                    break;
                case SET_TEMPO:
                    WRITE_CHAR(data, 0x03);
                    WRITE_BIG_ENDIAN_U24(data, event.meta->MPQ);
                    break;
                case SMPTE_OFFSET:
                    WRITE_CHAR(data, 0x05);
                    WRITE_CHAR(data, event.meta->startTime.hours);
                    WRITE_CHAR(data, event.meta->startTime.minutes);
                    WRITE_CHAR(data, event.meta->startTime.seconds);
                    WRITE_CHAR(data, event.meta->startTime.frames);
                    WRITE_CHAR(data, event.meta->startTime.frameFractions);
                    break;
                case TIME_SIGNATURE:
                    WRITE_CHAR(data, 0x04);
                    WRITE_CHAR(data, event.meta->timeSignature.numerator);
                    WRITE_CHAR(data, event.meta->timeSignature.denominator);
                    WRITE_CHAR(data, event.meta->timeSignature.TPM);
                    WRITE_CHAR(data, event.meta->timeSignature.noteDivision);
                    break;
                case KEY_SIGNATURE:
                    WRITE_CHAR(data, 0x02);
                    WRITE_CHAR(data, event.meta->key.sharps);
                    WRITE_CHAR(data, event.meta->key.minor);
                    break;
                case TEXT:
                case COPYRIGHT:
                case NAME:
                case INSTRUMENT_NAME:
                case LYRIC:
                case MARKER:
                case CUE:
                case SPECIFIC:
                case SYSEX_END:
                default:
                    feedDeltaTime(event.meta->length, data);
                    data.write((char*)event.meta->data, event.meta->length);
                    break;
            }
            break;
        case SYSEX_EVENT:
            WRITE_CHAR(data, 0xF0);
            // data contains the final 0xF7
            data.write((char*)event.sysex->data, event.sysex->length);
            break;
        case UNKOWN:
            break;
        default:
            return INVALID_EVENT;
    }
    return NONE;
}

// FIXME implement running status (consecutive same type events compression)
// NOTE : after being done using it do delete[] track->data;
enum MidiError encodeMidiTrack(struct MidiTrack& track) {
    // XXX maybe optimize a bit by writing whole structures in one write?
    u32 len = (u32)(track.list.size() * 3.5f);
    std::string s(len, '\0');
    std::stringstream data(s);

    for (TrackEvent& event : track.list) {
        encodeTrackEvent(event, data);
    }

    data.seekg(0, std::ios::end);
    std::streampos sz = data.tellg();
    data.seekg(0, std::ios::beg);
    track.data = new u8[sz];
    track.length = sz;
    std::memcpy(track.data, data.str().c_str(), sz);
    return NONE;
}

enum MidiError writeMidiFile(struct MidiFile& file, std::ostream& stream) {
    constexpr char header[] = "MThd\0\0\0\6";
    char dat = 0;
    stream.write(header, 8);
    WRITE_BIG_ENDIAN_U16(stream, file.format);
    WRITE_BIG_ENDIAN_U16(stream, file.tracks);
    WRITE_BIG_ENDIAN_U16(stream, file.division);

    for (u16 t = 0; t < file.tracks; t++) {
        MidiTrack& track = file.data[t];
        if (!track.decoded) {
            return INVALID_TRACK;
        }
        enum MidiError err = encodeMidiTrack(track);
        if (err != NONE) {
            return err;
        }
        stream << "MTrk";
        WRITE_BIG_ENDIAN_U32(stream, track.length);
        stream.write((char*)track.data, track.length);
        delete[] track.data;
    }
    return NONE;
}

#pragma endregion

#pragma region PRINT
void printMidiTrackEvent(const struct TrackEvent& event) {
    std::cout << "Event dt = " << event.deltaTime << "\t";
    switch (event.type) {
        case MIDI:
            switch (event.midi.type) {
                case NOTE_OFF:
                    std::cout << "type = MIDI NOTE OFF channel="
                              << (u32)event.midi.channel
                              << " note=" << (u32)event.midi.data0
                              << " velocity=" << (u32)event.midi.data1 << "\n";
                    break;
                case NOTE_ON:
                    std::cout << "type = MIDI NOTE ON channel="
                              << (u32)event.midi.channel
                              << " note=" << (u32)event.midi.data0
                              << " velocity=" << (u32)event.midi.data1 << "\n";
                    break;
                case POLY_AFTERTOUCH:
                    std::cout << "type = MIDI POLYPHONIC AFTERTOUCH channel="
                              << (u32)event.midi.channel
                              << " note=" << (u32)event.midi.data0
                              << " pressure=" << (u32)event.midi.data1 << "\n";
                    break;
                case CC:
                    std::cout << "type = MIDI CONTROL CHANGE channel="
                              << (u32)event.midi.channel
                              << " controller=" << (u32)event.midi.data0
                              << " value=" << (u32)event.midi.data1 << "\n";
                    break;
                case PROGRAM_CHANGE:
                    std::cout << "type = MIDI PROGRAM CHANGE channel="
                              << (u32)event.midi.channel
                              << " patch=" << (u32)event.midi.data0 << "\n";
                    break;
                case AFTERTOUCH:
                    std::cout << "type = MIDI AFTERTOUCH channel="
                              << (u32)event.midi.channel
                              << " value=" << (u32)event.midi.data0 << "\n";
                    break;
                case PITCH_WHEEL:
                    std::cout << "type = MIDI PITCH WHEEL channel="
                              << (u32)event.midi.channel << " value="
                              << (u32)(event.midi.data0 +
                                       ((u16)(event.midi.data1) << 7))
                              << "\n";
                    break;
                default:
                    std::cout << "type = MIDI 0x" << std::hex << event.midi.type
                              << std::dec << "\n";
                    break;
            }
            break;
        case SYSTEM_EVENT:
            std::cout << "type = 0x" << std::hex << event.sys.type << std::dec
                      << " SYSTEM\n";
            break;
        case META:
            switch (event.meta->type) {
                case SEQUENCE_NUMBER:
                    std::cout << "type = META SEQUENCE NUMBER number="
                              << (u32)event.meta->seqNumber << "\n";
                    break;
                case TEXT:
                    std::cout << "type = META TEXT content=" << event.meta->data
                              << "\n";
                    break;
                case COPYRIGHT:
                    std::cout
                        << "type = META COPYRIGHT content=" << event.meta->data
                        << "\n";
                    break;
                case NAME:
                    std::cout << "type = META NAME content=" << event.meta->data
                              << "\n";
                    break;
                case INSTRUMENT_NAME:
                    std::cout << "type = META INSTRUMENT NAME content="
                              << event.meta->data << "\n";
                    break;
                case LYRIC:
                    std::cout
                        << "type = META LYRIC content=" << event.meta->data
                        << "\n";
                    break;
                case MARKER:
                    std::cout
                        << "type = META MARKER content=" << event.meta->data
                        << "\n";
                    break;
                case CUE:
                    std::cout << "type = META CUE content=" << event.meta->data
                              << "\n";
                    break;
                case MIDI_CHANNEL_PREFIX:
                    std::cout << "type = META CHANNEL PREFIX channel="
                              << (u32)event.meta->channel << "\n";
                    break;
                case END_OF_TRACK:
                    std::cout << "type = META END OF TRACK\n";
                    break;
                case SET_TEMPO:
                    std::cout << "type = META SET TEMPO mpq=" << event.meta->MPQ
                              << "\n";
                    break;
                case SMPTE_OFFSET:
                    std::cout << "type = META SET SMPTE OFFSET offset="
                              << (u32)event.meta->startTime.hours << ":"
                              << (u32)event.meta->startTime.minutes << ":"
                              << (u32)event.meta->startTime.seconds << " "
                              << (u32)event.meta->startTime.frames << ":"
                              << (u32)event.meta->startTime.frameFractions
                              << "\n";
                    break;
                case TIME_SIGNATURE:
                    std::cout << "type = META TIME SIGNATURE numerator="
                              << (u32)event.meta->timeSignature.numerator
                              << " denominator="
                              << (u32)event.meta->timeSignature.denominator
                              << " note division="
                              << (u32)event.meta->timeSignature.noteDivision
                              << " clocks per metronome="
                              << (u32)event.meta->timeSignature.TPM << "\n";
                    break;
                case KEY_SIGNATURE:
                    std::cout << "type = META KEY SIGNATURE sharps="
                              << (i32)event.meta->key.sharps << " minor="
                              << (event.meta->key.minor ? "true" : "false")
                              << "\n";
                    break;
                case SPECIFIC:
                default:
                    std::cout << "type = META 0x" << std::hex
                              << (u32)event.meta->type << std::dec << "\n";
                    break;
            }
            break;
        case SYSEX_EVENT: {
            std::cout << "type = SYSEX 0xF0 " << event.sysex->data << "\n";
        } break;
        case UNKOWN:
            std::cout << "type = UNKOWN 0x??\n";
            break;
    }
}

void printMidiTrack(const struct MidiTrack& track) {
    std::cout << "MTrk length: " << track.length
              << "\nDecoded: " << (track.decoded ? "true" : "false") << "\n";

    if (track.decoded) {
        const std::vector<TrackEvent>& list = track.list;

        for (const TrackEvent& e : list) {
            printMidiTrackEvent(e);
        }
    } else {
        std::cout << std::hex << std::setfill('0');
        for (u32 i = 0; i < track.length; i++) {
            std::cout << std::setw(2) << (u32)track.data[i]
                      << ((i % 16) != 15 ? ((i % 16) != 7 ? "\t" : "\t\t")
                                         : "\n");
        }
        std::cout << std::dec << "\n";
    }
}

void printMidiFile(const struct MidiFile& header) {
    std::cout << "MThd length: " << header.length
              << "\nFormat: " << header.format << "\nTracks: " << header.tracks
              << "\nTime divisions: " << header.division << "\n";

    if (!header.data) return;
    for (u16 i = 0; i < header.tracks; i++) {
        printMidiTrack(header.data[i]);
    }
}
#pragma endregion