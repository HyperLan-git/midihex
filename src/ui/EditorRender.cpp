#include "Editor.hpp"

inline void createImageButton(const std::string& id, ButtonHandler& handler,
                              const Texture& texture) {
    if (ImGui::ImageButton(id.c_str(), texture.tex,
                           {(float)texture.w, (float)texture.h})) {
        handler.pressButton(id);
    }
}

void Editor::renderApplicationBar() {
    constexpr ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_HorizontalScrollbar;
    constexpr ImVec2 sz = {500, 100};
    ImGui::SetNextWindowSize(sz, ImGuiCond_Once);
    if (!ImGui::Begin("Tools", NULL, flags)) {
        ImGui::End();
        return;
    }
    if (ImGui::GetWindowDockNode() == NULL) ImGui::SetWindowSize(sz);
    ImVec2 szTools = ImGui::GetWindowSize();

    const Texture &newIcon = this->resourceManager.getTexture("new_icon"),
                  &loadIcon = this->resourceManager.getTexture("load_icon"),
                  &saveIcon = this->resourceManager.getTexture("save_icon"),
                  &addMidiIcon =
                      this->resourceManager.getTexture("add_event_icon"),
                  &removeMidiIcon =
                      this->resourceManager.getTexture("remove_event_icon");

    createImageButton("New", this->buttonHandler, newIcon);

    if (szTools.x >= newIcon.w * 3) ImGui::SameLine();
    createImageButton("Load", this->buttonHandler, loadIcon);

    if (szTools.x >= newIcon.w * 4) ImGui::SameLine();
    createImageButton("Save", this->buttonHandler, saveIcon);

    if (szTools.x >= newIcon.w * 5) ImGui::SameLine();
    createImageButton("Add event", this->buttonHandler, addMidiIcon);

    if (szTools.x >= newIcon.w * 6) ImGui::SameLine();
    createImageButton("Remove event", this->buttonHandler, removeMidiIcon);

    ImGui::End();
}

#pragma region ADD_EVENT_EDITOR

// TODO maybe put all these in the caller's responsibility or in a new class
TrackEvent buffer = TrackEvent(
    MidiEvent{.type = NOTE_OFF, .channel = 0, .data0 = 0, .data1 = 0});

constexpr v_len MAX_DATA_LEN = 512;
u8* strBuf = new u8[MAX_DATA_LEN];
u8* strBuf2 = new u8[MAX_DATA_LEN];
std::string hexString, metaString;
MetaEvent* metaBuffer = new MetaEvent(TEXT, strBuf, 0);
SysExEvent* sysExBuffer = new SysExEvent(strBuf2, 0);

enum TrackEventType getTrackEventType(int num) {
    switch (num) {
        case MIDI:
            return MIDI;
        case SYSTEM_EVENT:
            return SYSTEM_EVENT;
        case META:
            return META;
        case SYSEX_EVENT:
            return SYSEX_EVENT;
        default:
            return UNKOWN;
    }
}
constexpr enum MetaEventType metaTypes[] = {
    SEQUENCE_NUMBER, TEXT,
    COPYRIGHT,       NAME,
    INSTRUMENT_NAME, LYRIC,
    MARKER,          CUE,
    DEVICE_NAME,     MIDI_CHANNEL_PREFIX,
    SET_TEMPO,       SMPTE_OFFSET,
    TIME_SIGNATURE,  KEY_SIGNATURE,
    SPECIFIC};

constexpr long metaTypesNum = sizeof(metaTypes) / sizeof(metaTypes[0]);

enum MetaEventType getMetaEventType(int num) {
    if (num < metaTypesNum) return metaTypes[num];
    return SPECIFIC;
}

int getMetaEventID(enum MetaEventType type) {
    for (int i = 0; i < metaTypesNum; i++) {
        if (metaTypes[i] == type) return i;
    }
    return 0;
}

std::stringstream resultString;
std::string result;
#include <iomanip>

int callbackResizeHex(ImGuiInputTextCallbackData* data) {
    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
        // Resize string callback
        // If for some reason we refuse the new length (BufTextLen) and/or
        // capacity (BufSize) we need to set them back to what we want.
        std::string* str = (std::string*)data->UserData;
        IM_ASSERT(data->Buf == str->data());
        str->resize(data->BufTextLen);
        data->Buf = str->data();
    } else if (data->EventFlag == ImGuiInputTextFlags_CallbackCharFilter) {
        if ((data->EventChar < '0' || data->EventChar > '9') &&
            (data->EventChar < 'a' || data->EventChar > 'f') &&
            (data->EventChar < 'A' || data->EventChar > 'F') &&
            (data->EventChar != ' ' && data->EventChar != '\n'))
            return 1;
    }
    return 0;
}

// Returns used array length
std::size_t hexStringToArray(const std::string& hex, u8* array,
                             std::size_t sz) {
    std::size_t a = 0;
    bool char2 = false;
    for (std::size_t i = 0; i < hex.size(); i++) {
        const char c = hex[i];
        if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
            (c >= 'A' && c <= 'F')) {
            u8 val = 0;
            if (c >= '0' && c <= '9') {
                val = c - '0';
            } else if (c >= 'a' && c <= 'f') {
                val = c - 'a' + 10;
            } else {
                val = c - 'A' + 10;
            }

            if (char2) {
                array[a] += val;
                if (++a >= sz) return a;
            } else {
                array[a] = val << 4;
            }
            char2 = !char2;
        }
    }
    return a + char2;
}

bool renderHexEditor(const char* label, std::string& result,
                     std::size_t max_sz = 65535 * 2) {
    bool changed = ImGui::InputTextMultiline(
        label, result.data(), result.size() + 1, {0, 0},
        ImGuiInputTextFlags_CallbackCharFilter |
            ImGuiInputTextFlags_CallbackResize,
        callbackResizeHex, &result);
    if (result.size() > max_sz) result.resize(max_sz);

    return changed;
}

bool renderStringEditor(const char* label, std::string& result,
                        std::size_t max_sz = 65535) {
    bool changed = ImGui::InputTextMultiline(
        label, result.data(), result.size() + 1, {0, 0},
        ImGuiInputTextFlags_CallbackResize, callbackResizeHex, &result);
    if (result.size() > max_sz) result.resize(max_sz);

    return changed;
}

void Editor::renderEventAddEditor(std::shared_ptr<MidiFile>& data) {
    if (!data) return;
    ImGui::SetNextWindowSizeConstraints({500, 450}, {900, 600});
    if (!ImGui::BeginPopupModal("Event add editor",
                                &this->addEventEditorOpen)) {
        return;
    }
    int v;
    int buf = this->selectedTrack;
    bool changed = result.empty();

    ImGui::SetNextItemWidth(150);
    changed |= ImGui::InputInt("New event's track", &buf);
    this->selectedTrack = std::clamp(buf, 0, data->tracks - 1);

    buf = this->selectedEvent;
    ImGui::SetNextItemWidth(150);
    changed |= ImGui::InputInt("New event's index", &buf);
    this->selectedEvent = std::clamp(
        buf, 0, (int)(data->data[this->selectedTrack].list.size() - 1));

    ImGui::SetNextItemWidth(150);
    v = buffer.deltaTime;
    changed |= ImGui::InputInt("Delta time", &v);
    buffer.deltaTime = std::clamp(v, 0, 0xFFFFFFF);

    constexpr char EVENT_TYPES[] = "Track\0Meta\0Sysex\0";

    buf = (int)buffer.type - 1;
    ImGui::SetNextItemWidth(150);
    changed |= ImGui::Combo("Event type", &buf, EVENT_TYPES);
    buffer.type = getTrackEventType(buf + 1);

    constexpr char MIDI_TYPES[] =
        "NOTE OFF\0NOTE ON\0POLYPHONIC AFTERTOUCH\0"
        "MIDI CC\0PROGRAM CHANGE\0AFTERTOUCH\0PITCH WHEEL\0";
    constexpr char META_TYPES[] =
        "SEQUENCE NUMBER\0TEXT\0COPYRIGHT\0NAME\0INSTRUMENT "
        "NAME\0LYRIC\0MARKER\0CUE\0DEVICE NAME\0MIDI CHANNEL PREFIX\0SET "
        "TEMPO\0SMPTE OFFSET\0TIME SIGNATURE\0KEY SIGNATURE\0VENDOR SPECIFIC\0";

    constexpr char MIDI_EVENTS_PARAMETERS[][2][11] = {
        {"note", "velocity"},    {"note", "velocity"}, {"note", "pressure"},
        {"controller", "value"}, {"program", "empty"}, {"value", "empty"},
        {"LSB", "MSB"}};

    int w;
    bool b;
    switch (buffer.type) {
        case MIDI:
            w = buffer.midi.type - NOTE_OFF;
            ImGui::SetNextItemWidth(200);
            changed |= ImGui::Combo("Channel event type", &w, MIDI_TYPES);
            buffer.midi.type = w + NOTE_OFF;

            v = buffer.midi.channel;
            ImGui::SetNextItemWidth(150);
            changed |= ImGui::InputInt("Channel", &v, 1, 5);
            buffer.midi.channel = std::clamp(v, 0, 0xF);

            ImGui::SetNextItemWidth(100);
            v = buffer.midi.data0;
            changed |= ImGui::InputInt(MIDI_EVENTS_PARAMETERS[w][0], &v);
            buffer.midi.data0 = std::clamp(v, 0, 0x7F);

            ImGui::SameLine();
            ImGui::SetNextItemWidth(100);
            v = buffer.midi.data1;

            if (buffer.midi.type == PROGRAM_CHANGE ||
                buffer.midi.type == AFTERTOUCH) {
                ImGui::BeginDisabled();
                changed |= ImGui::InputInt(MIDI_EVENTS_PARAMETERS[w][1], &v);
                ImGui::EndDisabled();
            } else {
                changed |= ImGui::InputInt(MIDI_EVENTS_PARAMETERS[w][1], &v);
            }
            buffer.midi.data1 = std::clamp(v, 0, 0x7F);

            break;
        case META:
            buffer.meta = metaBuffer;
            w = getMetaEventID((enum MetaEventType)buffer.meta->type);
            ImGui::SetNextItemWidth(200);
            changed |= ImGui::Combo("Meta event type", &w, META_TYPES);
            buffer.meta->type = getMetaEventType(w);
            switch (buffer.meta->type) {
                case MIDI_CHANNEL_PREFIX:
                    v = buffer.meta->channel;
                    ImGui::SetNextItemWidth(150);
                    changed |= ImGui::InputInt("Channel", &v, 1, 5);
                    buffer.meta->channel = std::clamp(v, 0, 15);
                    break;
                case SET_TEMPO:
                    v = buffer.meta->MPQ;
                    ImGui::SetNextItemWidth(150);
                    changed |=
                        ImGui::InputInt("Microseconds per quarter note", &v);
                    buffer.meta->MPQ = std::clamp(v, 0, 0xFFFFFF);
                    break;
                case SEQUENCE_NUMBER:
                    v = buffer.meta->seqNumber;
                    ImGui::SetNextItemWidth(150);
                    changed |= ImGui::InputInt("Sequence number", &v);
                    buffer.meta->seqNumber = std::clamp(v, 0, 0xFFFF);
                    break;
                case SMPTE_OFFSET:
                    v = buffer.meta->startTime.hours;
                    ImGui::SetNextItemWidth(100);
                    ImGui::PushID(1);
                    changed |= ImGui::InputInt(":", &v);
                    ImGui::PopID();
                    buffer.meta->startTime.hours = std::clamp(v, 0, 0xFF);

                    ImGui::SameLine();
                    v = buffer.meta->startTime.minutes;
                    ImGui::SetNextItemWidth(100);
                    changed |= ImGui::InputInt(":", &v);
                    buffer.meta->startTime.minutes = std::clamp(v, 0, 59);

                    ImGui::SameLine();
                    v = buffer.meta->startTime.seconds;
                    ImGui::SetNextItemWidth(150);
                    changed |= ImGui::InputInt("Time (hh:mm:ss)", &v);
                    buffer.meta->startTime.seconds = std::clamp(v, 0, 59);

                    v = buffer.meta->startTime.frames;
                    ImGui::SetNextItemWidth(100);
                    changed |= ImGui::InputInt("Frames", &v);
                    buffer.meta->startTime.frames = std::clamp(v, 0, 99);

                    ImGui::SameLine();
                    v = buffer.meta->startTime.frameFractions;
                    ImGui::SetNextItemWidth(100);
                    changed |= ImGui::InputInt("Frame fractions", &v);
                    buffer.meta->startTime.frameFractions =
                        std::clamp(v, 0, 99);
                    break;
                case TIME_SIGNATURE:
                    v = buffer.meta->timeSignature.numerator;
                    ImGui::SetNextItemWidth(100);
                    changed |= ImGui::InputInt("Numerator", &v);
                    buffer.meta->timeSignature.numerator =
                        std::clamp(v, 0, 255);

                    ImGui::SameLine();
                    v = buffer.meta->timeSignature.denominator;
                    ImGui::SetNextItemWidth(100);
                    changed |= ImGui::InputInt("Denominator (2^)", &v);
                    buffer.meta->timeSignature.denominator =
                        std::clamp(v, 0, 255);

                    v = buffer.meta->timeSignature.TPM;
                    ImGui::SetNextItemWidth(100);
                    changed |= ImGui::InputInt("Midi ticks per beat", &v);
                    buffer.meta->timeSignature.TPM = std::clamp(v, 0, 255);

                    v = buffer.meta->timeSignature.noteDivision;
                    ImGui::SetNextItemWidth(100);
                    changed |= ImGui::InputInt("32nd notes per beat", &v);
                    buffer.meta->timeSignature.noteDivision =
                        std::clamp(v, 0, 255);
                    break;
                case KEY_SIGNATURE:
                    ImGui::SetNextItemWidth(100);
                    b = buffer.meta->key.minor;
                    changed |= ImGui::Checkbox("Minor key", &b);
                    buffer.meta->key.minor = b ? 1 : 0;

                    v = buffer.meta->key.sharps;
                    ImGui::SetNextItemWidth(100);
                    changed |=
                        ImGui::InputInt("Sharps (negative for flats)", &v);
                    buffer.meta->key.sharps = std::clamp(v, -7, 7);
                    break;
                case SPECIFIC:
                    buffer.meta->data = strBuf;
                    b = renderHexEditor("Content", hexString);
                    changed |= b;
                    if (b)
                        buffer.meta->length =
                            hexStringToArray(hexString, strBuf, MAX_DATA_LEN);
                    break;
                default:
                    buffer.meta->data = strBuf;
                    changed |= renderStringEditor("Text", metaString,
                                                  MAX_DATA_LEN - 1);
                    buffer.meta->length = metaString.size();
                    std::memcpy(strBuf, metaString.c_str(),
                                metaString.size() + 1);
            }
            break;
        case SYSEX_EVENT:
            buffer.sysex = sysExBuffer;
            b = renderHexEditor("Content", hexString);
            changed |= b;
            if (b) {
                buffer.sysex->length =
                    hexStringToArray(hexString, strBuf2, MAX_DATA_LEN);
                if (buffer.sysex->length < MAX_DATA_LEN - 1) {
                    buffer.sysex->data[buffer.sysex->length++] = (u8)0xF7;
                } else {
                    buffer.sysex->data[MAX_DATA_LEN - 1] = (u8)0xF7;
                }
            }
            break;
        default:
            ImGui::Text("Wrong event type %d", buffer.type);
            break;
    }

    if (changed) {
        resultString = std::stringstream();
        enum MidiError err = encodeTrackEvent(buffer, resultString);
        if (err != NONE) std::cout << "Err: " << err << std::endl;
        resultString.seekg(0, std::ios::end);
        std::streampos sz = resultString.tellg();
        resultString.seekg(0, std::ios::beg);
        std::string resStr = resultString.str();
        std::stringstream tmp;
        tmp << std::hex << std::uppercase << std::setfill('0');
        for (u32 i = 0; i < sz; i++) {
            tmp << std::setw(2) << (u16)(resStr[i] & 0xff)
                << (i % 8 == 7 ? "\n" : " ");
        }
        result = tmp.str();
        if (result.size() > 0) result.resize(result.size() - 1);
    }

    ImGui::InputTextMultiline("Result", result.data(), result.size() + 1,
                              {0, 0}, ImGuiInputTextFlags_ReadOnly);

    if (ImGui::Button("Add")) {
        this->addEvent(this->selectedTrack, this->selectedEvent, buffer);
        this->addEventEditorOpen = false;
    }
    ImGui::EndPopup();
}

#pragma endregion

void Editor::renderError() {
    bool open = !error.empty();
    ImGui::SetNextWindowSizeConstraints({250, 100}, {500, 200});
    if (!ImGui::BeginPopupModal("Error", &open)) {
        if (!open) error.clear();
        return;
    }
    if (!open) error.clear();
    ImGui::Text("%s", error.c_str());
    ImGui::EndPopup();
}

void Editor::renderTrackEditor(std::shared_ptr<MidiFile>& data) {
    if (!data) return;
    if (!ImGui::BeginPopupModal("Track editor", &this->trackEditorOpen)) {
        return;
    }
    if (ImGui::Button("Add")) {
        this->addTrack(data->tracks);
    }

    constexpr char COLS[][13] = {"Track number", "Events", "Open", "Move"};
    constexpr int N_COLS = sizeof(COLS) / sizeof(COLS[0]);
    constexpr ImGuiTableFlags tableFlags =
        ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Borders |
        ImGuiTableFlags_Hideable | ImGuiTableFlags_ScrollY;
    if (!ImGui::BeginTable("Tracks", N_COLS, tableFlags)) {
        ImGui::End();
        return;
    }

    for (int i = 0; i < N_COLS; i++) {
        if (i == N_COLS - 1)
            ImGui::TableSetupColumn(COLS[i],
                                    ImGuiTableColumnFlags_WidthStretch);
        else
            ImGui::TableSetupColumn(COLS[i]);
    }
    ImGui::TableSetupScrollFreeze(N_COLS, 1);
    ImGui::TableHeadersRow();

    for (u32 i = 0; i < data->tracks; i++) {
        ImGui::PushID(i);
        if (ImGui::TableNextColumn()) ImGui::Text("%d", i + 1);
        if (ImGui::TableNextColumn()) {
            ImGui::Text("%lu", data->data[i].list.size());
        }
        if (ImGui::TableNextColumn()) {
            if (ImGui::Button("View")) {
                this->showAllTracks = false;
                this->trackToShow = i + 1;
            }
        }
        if (ImGui::TableNextColumn()) {
            if (i == 0) ImGui::BeginDisabled();
            if (ImGui::Button("^") && i > 0) {
                std::swap(data->data[i - 1], data->data[i]);
            }
            if (i == 0) ImGui::EndDisabled();
            ImGui::SameLine();
            if (i + 1 == data->tracks) ImGui::BeginDisabled();
            if (ImGui::Button("v") && i + 1 < data->tracks) {
                std::swap(data->data[i + 1], data->data[i]);
            }
            if (i + 1 == data->tracks) ImGui::EndDisabled();
            ImGui::SameLine();
            if (ImGui::Button("del")) {
                if (data->tracks == 1) {
                    this->trackEditorOpen = false;
                    this->showError("Cannot delete the last track in a file !");
                } else
                    this->removeTrack(i);
            }
        }
        ImGui::PopID();
    }

    ImGui::EndTable();
    ImGui::EndPopup();
}

void Editor::renderFileParams(std::shared_ptr<MidiFile>& data) {
    if (!ImGui::Begin("File", NULL, 0) || !data) {
        ImGui::End();
        return;
    }
    int item = data->format;
    ImGui::SetNextItemWidth(200);
    ImGui::Combo("File type", &item,
                 "Single track\0Multi track\0Independent multi track\0");
    if (item == 0 && data->tracks > 1) {
        // TODO warning to delete all tracks but one
        // ImGui::Popup
    }

    data->format = std::clamp(item, 0, 2);

    bool changed = false;
    bool type = data->division >> 15;
    ImGui::SetNextItemWidth(200);
    changed |= ImGui::Checkbox(
        "Type of time division (Ticks per quarter note/Frames per sec)", &type);

    if (type) {
        int i = (-(i8)((data->division >> 8) & 0xFF)) & 0x7F,
            j = data->division & 0xFF;
        ImGui::SetNextItemWidth(100);
        changed |= ImGui::InputInt("Frames per second", &i, 1, 2);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100);
        changed |= ImGui::InputInt("Ticks per frame", &j, 1, 10);
        data->division =
            ((-(i8)std::clamp(i, 24, 30)) << 8) | (std::clamp(j, 1, 0xFF));
    } else {
        int i = data->division & 0x7FFF;
        ImGui::SetNextItemWidth(200);
        changed |= ImGui::InputInt("Time division (TPQ)", &i, 1, 0x7FFF);
        data->division = std::clamp(i, 1, 0x7FFF);
    }
    if (changed) {
        data->timingInfo.clear();

        for (u32 j = 0; j < data->tracks; j++) {
            computeTimeMapsForTrack(*data, data->data[j]);
        }
    }

    ImGui::SetNextItemWidth(200);
    if (ImGui::Button("Track editor")) {
        buttonHandler.pressButton("Track editor");
    }
    ImGui::End();
}

constexpr int WIDTH = 125;
void Editor::renderParams(std::shared_ptr<MidiFile>& data) {
    if (!ImGui::Begin("Parameters", NULL, 0)) {
        ImGui::End();
        return;
    }
    int i = this->eventTableSize, j = this->offset, k = this->trackToShow;
    ImGui::PushItemWidth(WIDTH);
    ImGui::InputInt("Table size", &i, 1, 100);
    this->eventTableSize = std::clamp((u64)i, (u64)0, (u64)2000);
    ImGui::PushItemWidth(WIDTH);
    ImGui::InputInt("Offset", &j, 1, 100);
    if (data && k != 0 && k <= data->tracks)
        this->offset =
            std::clamp((u64)j, (u64)0, data->data[k - 1].list.size());
    else
        this->offset = std::clamp((u64)j, (u64)0u, this->totalEvents);
    ImGui::PushItemWidth(WIDTH);
    ImGui::Checkbox("All tracks", &this->showAllTracks);
    ImGui::SameLine();
    ImGui::PushItemWidth(WIDTH);
    if (!data || this->showAllTracks) {
        this->trackToShow = 0;
        ImGui::BeginDisabled();
        ImGui::InputInt("Track", &k, 1, 5);
        ImGui::EndDisabled();
    } else {
        ImGui::InputInt("Track", &k, 1, 5);
        this->trackToShow = std::clamp(k, 1, (int)data->tracks);
    }
    ImGui::End();
}

void Editor::renderTable(std::shared_ptr<MidiFile>& data) {
    if (!ImGui::Begin("Table", NULL, 0)) {
        ImGui::End();
        return;
    }
    if (!data || data->tracks == 0 || !data->data[0].decoded ||
        this->eventTableSize == 0) {
        ImGui::Text("No data");
        ImGui::End();
        return;
    }
    constexpr ImGuiTableFlags tableFlags =
        ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable |
        ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable |
        ImGuiTableFlags_ScrollY;
    constexpr char COLS[][16] = {"Delta-time", "Tick",         "Bar : Beat",
                                 "Time us",    "Message type", "Data"};
    constexpr int N_COLS = sizeof(COLS) / sizeof(COLS[0]);
    if (!ImGui::BeginTable("DataTable", N_COLS, tableFlags)) {
        ImGui::End();
        return;
    }
    for (int i = 0; i < N_COLS; i++) {
        if (i == N_COLS - 1)
            ImGui::TableSetupColumn(COLS[i],
                                    ImGuiTableColumnFlags_WidthStretch);
        else
            ImGui::TableSetupColumn(COLS[i]);
    }
    ImGui::TableSetupScrollFreeze(N_COLS, 1);
    ImGui::TableHeadersRow();
    u64 remaining = this->eventTableSize;
    u32 o = offset;
    const TempoChange defaultTempo =
        TempoChange{.time = 0,
                    .timeMicros = 0,
                    .microsPerTick = getMicrosPerTick(data->division, 500000)};
    for (u32 j = 0; j < data->tracks; j++) {
        if (this->trackToShow != 0 && this->trackToShow != j + 1) continue;
        MidiTrack& track = data->data[j];
        if (!track.decoded) continue;
        if (o >= track.list.size()) {
            o -= track.list.size();
            continue;
        }

        bool changeDT = false;
        std::vector<TempoChange>::const_iterator t = data->timingInfo.cbegin();
        for (u32 i = o; i < track.list.size(); i++) {
            o = 0;
            ImGui::PushID(remaining);
            TrackEvent& message = track.list[i];
            ImGui::TableNextRow();
            while (t + 1 < data->timingInfo.cend() &&
                   (t + 1)->time <= message.time) {
                t++;
            }
            if (message.type == META && message.meta->type == END_OF_TRACK) {
                if (((this->eventTableSize - remaining) % 2) == 1) {
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0,
                                           0x805555FF);
                } else {
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0,
                                           0x802222FF);
                }
            }
            if (ImGui::TableNextColumn()) {
                int v = message.deltaTime;
                changeDT |= ImGui::InputInt("##deltatime", &v);
                message.deltaTime = std::clamp(v, 0, 0xFFFFFF);
            }
            if (ImGui::TableNextColumn()) ImGui::Text("%u", message.time);
            // TODO put those computations in update() if optimisation
            // needed
            if (ImGui::TableNextColumn()) {
                BarTime bar = getBar(data->timeSignatureInfo, message.time);
                ImGui::Text("%u : %.4lf", bar.bar, bar.barTime);
            }
            if (ImGui::TableNextColumn()) {
                const TempoChange& tempo =
                    (t != data->timingInfo.cend()) ? *t : defaultTempo;
                // Stupid warning needs an explicit cast
                if (sizeof(unsigned long long) == sizeof(u64))
                    ImGui::Text("%llu", (unsigned long long)getTimeMicros(
                                            tempo, message.time));
                else
                    ImGui::Text("%lu", (unsigned long)getTimeMicros(
                                           tempo, message.time));
            }
            if (ImGui::TableNextColumn()) printTextForTrackEventType(message);
            if (ImGui::TableNextColumn()) {
                bool changed = printDataTextForTrackEvent(message);
                if (changed && message.type == META &&
                    message.meta->type == SET_TEMPO) {
                    tempoHasChanged = true;
                } else if (changed && message.type == META &&
                           message.meta->type == TIME_SIGNATURE) {
                    timeSignatureHasChanged = true;
                }
            }

            constexpr ImGuiSelectableFlags selectable_flags =
                ImGuiSelectableFlags_SpanAllColumns |
                ImGuiSelectableFlags_AllowItemOverlap;
            ImGui::SameLine();
            if (ImGui::Selectable(
                    "", this->selectedEvent == i && selectedTrack == j,
                    selectable_flags, ImVec2(0, 0))) {
                this->selectedEvent = i;
                this->selectedTrack = j;
            }

            ImGui::PopID();

            if (--remaining == 0) break;
        }
        if (remaining == 0) break;
        ImGui::PushID(j + this->totalEvents + 1);
        ImGui::TableHeadersRow();
        ImGui::PopID();

        if (changeDT) {
            computeTimes(track.list);
        }
    }

    ImGui::PushID(1);
    ImGui::TableHeadersRow();
    ImGui::PopID();

    ImGui::EndTable();
    ImGui::End();
}