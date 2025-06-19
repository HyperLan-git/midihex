#include <algorithm>

#include "Editor.hpp"

void Editor::printTextForTrackEventType(const TrackEvent& ev) {
    switch (ev.type) {
        case MIDI:
            switch (ev.midi.type) {
                case NOTE_OFF:
                    ImGui::Text("MIDI NOTE OFF");
                    break;
                case NOTE_ON:
                    ImGui::Text("MIDI NOTE ON");
                    break;
                case POLY_AFTERTOUCH:
                    ImGui::Text("MIDI POLYPHONIC AFTERTOUCH");
                    break;
                case CC:
                    ImGui::Text("MIDI CONTROL CHANGE");
                    break;
                case PROGRAM_CHANGE:
                    ImGui::Text("MIDI PROGRAM CHANGE");
                    break;
                case AFTERTOUCH:
                    ImGui::Text("MIDI AFTERTOUCH");
                    break;
                case PITCH_WHEEL:
                    ImGui::Text("MIDI PITCH WHEEL");
                    break;
                default:
                    ImGui::Text("MIDI 0x%02X", ev.midi.type);
                    break;
            }
            break;
        case SYSTEM_EVENT:
            ImGui::Text("SYSTEM 0x%02X", ev.sys.type);
            break;
        case META:
            switch (ev.meta->type) {
                case SEQUENCE_NUMBER:
                    ImGui::Text("META SEQUENCE NUMBER");
                    break;
                case TEXT:
                    ImGui::Text("META TEXT");
                    break;
                case COPYRIGHT:
                    ImGui::Text("META COPYRIGHT");
                    break;
                case NAME:
                    ImGui::Text("META NAME");
                    break;
                case INSTRUMENT_NAME:
                    ImGui::Text("META INSTRUMENT NAME");
                    break;
                case LYRIC:
                    ImGui::Text("META LYRIC");
                    break;
                case MARKER:
                    ImGui::Text("META MARKER");
                    break;
                case CUE:
                    ImGui::Text("META CUE");
                    break;
                case DEVICE_NAME:
                    ImGui::Text("META DEVICE NAME");
                    break;
                case MIDI_CHANNEL_PREFIX:
                    ImGui::Text("META CHANNEL PREFIX");
                    break;
                case END_OF_TRACK:
                    ImGui::Text("META END OF TRACK");
                    break;
                case SET_TEMPO:
                    ImGui::Text("META SET TEMPO");
                    break;
                case SMPTE_OFFSET:
                    ImGui::Text("META SET SMPTE OFFSET");
                    break;
                case TIME_SIGNATURE:
                    ImGui::Text("META TIME SIGNATURE");
                    break;
                case KEY_SIGNATURE:
                    ImGui::Text("META KEY SIGNATURE");
                    break;
                case SPECIFIC:
                default:
                    ImGui::Text("META 0x%02X", ev.meta->type);
                    break;
            }
            break;
        case SYSEX_EVENT:
            ImGui::Text("SYSEX 0xF0");
            break;
        case UNKOWN:
            ImGui::Text("UNKOWN 0x??");
            break;
    }
}
// TODO #define TABLE_LABEL(label, row)
constexpr u32 WIDTH = 100;
bool Editor::printDataTextForTrackEvent(TrackEvent& ev) {
    int i, j, k;
    bool changed = false;
    switch (ev.type) {
        case MIDI:
            switch (ev.midi.type) {
                case NOTE_OFF:
                case NOTE_ON:
                    i = ev.midi.channel;
                    j = ev.midi.data0;
                    k = ev.midi.data1;
                    ImGui::PushItemWidth(WIDTH);
                    changed |= ImGui::InputInt("channel", &i, 1, 8);
                    ImGui::SameLine();
                    ImGui::PushItemWidth(WIDTH);
                    changed |= ImGui::InputInt("note", &j, 1, 12);
                    ImGui::SameLine();
                    ImGui::PushItemWidth(WIDTH);
                    changed |= ImGui::InputInt("velocity", &k, 1, 32);
                    ev.midi.channel = std::clamp(i, 0, 15);
                    ev.midi.data0 = std::clamp(j, 0, 127);
                    ev.midi.data1 = std::clamp(k, 0, 127);
                    break;
                case POLY_AFTERTOUCH:
                    i = ev.midi.channel;
                    j = ev.midi.data0;
                    k = ev.midi.data1;
                    ImGui::PushItemWidth(WIDTH);
                    changed |= ImGui::InputInt("channel", &i, 1, 8);
                    ImGui::SameLine();
                    ImGui::PushItemWidth(WIDTH);
                    changed |= ImGui::InputInt("note", &j, 1, 12);
                    ImGui::SameLine();
                    ImGui::PushItemWidth(WIDTH);
                    changed |= ImGui::InputInt("pressure", &k, 1, 32);
                    ev.midi.channel = std::clamp(i, 0, 15);
                    ev.midi.data0 = std::clamp(j, 0, 127);
                    ev.midi.data1 = std::clamp(k, 0, 127);
                    break;
                case CC:
                    i = ev.midi.channel;
                    j = ev.midi.data0;
                    k = ev.midi.data1;
                    ImGui::PushItemWidth(WIDTH);
                    changed |= ImGui::InputInt("channel", &i, 1, 8);
                    ImGui::SameLine();
                    ImGui::PushItemWidth(WIDTH);
                    changed |= ImGui::InputInt("controller", &j, 1, 10);
                    ImGui::SameLine();
                    ImGui::PushItemWidth(WIDTH);
                    changed |= ImGui::InputInt("value", &k, 1, 32);
                    ev.midi.channel = std::clamp(i, 0, 15);
                    ev.midi.data0 = std::clamp(j, 0, 127);
                    ev.midi.data1 = std::clamp(k, 0, 127);
                    break;
                case PROGRAM_CHANGE:
                    i = ev.midi.channel;
                    j = ev.midi.data0;
                    ImGui::PushItemWidth(WIDTH);
                    changed |= ImGui::InputInt("channel", &i, 1, 8);
                    ImGui::SameLine();
                    ImGui::PushItemWidth(WIDTH);
                    changed |= ImGui::InputInt("patch", &j, 1, 10);
                    ev.midi.channel = std::clamp(i, 0, 15);
                    ev.midi.data0 = std::clamp(j, 0, 127);
                    break;
                case AFTERTOUCH:
                    i = ev.midi.channel;
                    j = ev.midi.data0;
                    ImGui::PushItemWidth(WIDTH);
                    changed |= ImGui::InputInt("channel", &i, 1, 8);
                    ImGui::SameLine();
                    ImGui::PushItemWidth(WIDTH);
                    changed |= ImGui::InputInt("value", &j, 1, 32);
                    ev.midi.channel = std::clamp(i, 0, 15);
                    ev.midi.data0 = std::clamp(j, 0, 127);
                    break;
                case PITCH_WHEEL:
                    i = ev.midi.channel;
                    j = ev.midi.data0 + ((u16)(ev.midi.data1) << 7) - 0x2000;
                    ImGui::PushItemWidth(WIDTH);
                    changed |= ImGui::InputInt("channel", &i, 1, 8);
                    ImGui::SameLine();
                    ImGui::PushItemWidth(WIDTH);
                    changed |= ImGui::InputInt("value", &j, 1, 32);
                    ev.midi.channel = std::clamp(i, 0, 15);
                    j += 0x2000;
                    ev.midi.data0 = j & 0x7F;
                    ev.midi.data1 = (j >> 7) & 0x7F;
                    break;
                default:
                    break;
            }
            break;
        case SYSTEM_EVENT:
            break;
        case META:
            switch (ev.meta->type) {
                case SEQUENCE_NUMBER:
                    i = ev.meta->seqNumber;
                    changed |= ImGui::InputInt("sequence number", &i, 0, 10);
                    ev.meta->seqNumber = std::clamp(i, 0, 255);
                    break;
                case TEXT:
                case COPYRIGHT:
                case NAME:
                case INSTRUMENT_NAME:
                case LYRIC:
                case MARKER:
                case CUE:
                case DEVICE_NAME:
                    ImGui::PushItemWidth(WIDTH * 3);
                    changed |= ImGui::InputText("content", (char*)ev.meta->data,
                                                ev.meta->length + 1);
                    break;
                case MIDI_CHANNEL_PREFIX:
                    i = ev.meta->channel;
                    changed |= ImGui::InputInt("channel", &i, 0, 8);
                    ev.meta->channel = std::clamp(i, 0, 15);
                    break;
                case END_OF_TRACK:
                    break;
                case SET_TEMPO:
                    i = (int)ev.meta->MPQ;
                    changed |= ImGui::InputInt("MPQ", &i, 0, 0x10000);
                    ev.meta->MPQ = std::clamp(i, 0, 0xFFFFFF);
                    break;
                case SMPTE_OFFSET: {
                    int h = ev.meta->startTime.hours,
                        m = ev.meta->startTime.minutes,
                        s = ev.meta->startTime.seconds,
                        f = ev.meta->startTime.frames,
                        ff = ev.meta->startTime.frameFractions;
                    ImGui::PushItemWidth(WIDTH);
                    changed |= ImGui::InputInt("hours", &h, 0, 5);
                    ImGui::SameLine();
                    ImGui::PushItemWidth(WIDTH);
                    changed |= ImGui::InputInt("minutes", &m, 0, 5);
                    ImGui::SameLine();
                    ImGui::PushItemWidth(WIDTH);
                    changed |= ImGui::InputInt("seconds", &s, 0, 5);

                    ImGui::PushItemWidth(WIDTH);
                    changed |= ImGui::InputInt("frames", &f, 0, 5);
                    ImGui::SameLine();
                    ImGui::PushItemWidth(WIDTH);
                    changed |= ImGui::InputInt("frame fractions", &ff, 0, 10);
                    ev.meta->startTime.hours = std::clamp(h, 0, 0xFF);
                    ev.meta->startTime.minutes = std::clamp(m, 0, 59);
                    ev.meta->startTime.seconds = std::clamp(s, 0, 59);
                    ev.meta->startTime.frames = std::clamp(f, 0, 29);
                    ev.meta->startTime.frameFractions = std::clamp(ff, 0, 99);
                } break;
                case TIME_SIGNATURE: {
                    int n = ev.meta->timeSignature.numerator,
                        d = ev.meta->timeSignature.denominator,
                        nd = ev.meta->timeSignature.noteDivision,
                        tpm = ev.meta->timeSignature.TPM;
                    ImGui::PushItemWidth(WIDTH);
                    changed |= ImGui::InputInt("numerator", &n, 0, 5);
                    ImGui::SameLine();
                    ImGui::PushItemWidth(WIDTH);
                    changed |= ImGui::InputInt("denominator", &d, 0, 5);

                    ImGui::PushItemWidth(WIDTH);
                    changed |= ImGui::InputInt("note division", &nd, 0, 5);
                    ImGui::SameLine();
                    ImGui::PushItemWidth(WIDTH);
                    changed |= ImGui::InputInt("tpm", &tpm, 0, 5);
                    ev.meta->timeSignature.numerator = std::clamp(n, 0, 0xFF);
                    ev.meta->timeSignature.denominator = std::clamp(d, 0, 0xFF);
                    ev.meta->timeSignature.noteDivision =
                        std::clamp(nd, 0, 0xFF);
                    ev.meta->timeSignature.TPM = std::clamp(tpm, 0, 0xFF);
                } break;
                case KEY_SIGNATURE:
                    i = ev.meta->key.sharps;
                    ImGui::PushItemWidth(WIDTH);
                    changed |= ImGui::InputInt("sharps", &i, 0, 4);
                    ImGui::SameLine();
                    ImGui::PushItemWidth(WIDTH);
                    changed |=
                        ImGui::Checkbox("minor", (bool*)&ev.meta->key.minor);
                    ev.meta->key.sharps = std::clamp(i, -7, 7);
                    break;
                case SPECIFIC:
                default:
                    break;
            }
            break;
        case SYSEX_EVENT:
            break;
        default:
        case UNKOWN:
            break;
    }
    return changed;
}