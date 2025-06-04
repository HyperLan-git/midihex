#include "Editor.hpp"

inline void createImageButton(const std::string& id, ButtonHandler& handler,
                              const Texture& texture) {
    if (ImGui::ImageButton(id.c_str(), texture.tex,
                           {(float)texture.w, (float)texture.h})) {
        handler.pressButton(id);
    }
}

void Editor::renderApplicationBar(std::shared_ptr<MidiFile>& data) {
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
                  &saveIcon = this->resourceManager.getTexture("save_icon");

    createImageButton("New", this->buttonHandler, newIcon);

    if (szTools.x >= newIcon.w * 3) ImGui::SameLine();
    createImageButton("Load", this->buttonHandler, loadIcon);

    if (szTools.x >= newIcon.w * 4) ImGui::SameLine();
    createImageButton("Save", this->buttonHandler, saveIcon);

    ImGui::End();
}

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
    if (!ImGui::BeginPopupModal("Track editor", &this->trackEditorOpen)) {
        return;
    }

    constexpr char COLS[][16] = {"Track number", "Open", "Move"};
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
            if (ImGui::Button("View")) {
                this->showAllTracks = false;
                this->trackToShow = i + 1;
            }
        }
        if (ImGui::TableNextColumn()) {
            if (ImGui::Button("^")) {
                std::cout << "move up\n";
                ImGui::CloseCurrentPopup();
                this->showError("Not implemented");
            }
            ImGui::SameLine();
            if (ImGui::Button("v")) {
                std::cout << "move down\n";
                ImGui::CloseCurrentPopup();
                this->showError("Not implemented");
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
        this->trackToShow = std::clamp(k, 0, (int)data->tracks);
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
    bool tempoChanged = false, timeSignatureChanged = false;
    for (u32 j = 0; j < data->tracks; j++) {
        if (this->trackToShow != 0 && this->trackToShow != j + 1) continue;
        MidiTrack& track = data->data[j];
        if (!track.decoded) continue;
        if (o >= track.list.size()) {
            o -= track.list.size();
            continue;
        }

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
            if (ImGui::TableNextColumn()) ImGui::Text("%u", message.deltaTime);
            if (ImGui::TableNextColumn()) ImGui::Text("%u", message.time);
            // TODO put those computations in update() if optimisation needed
            if (ImGui::TableNextColumn()) {
                BarTime bar = getBar(data->timeSignatureInfo, message.time);
                ImGui::Text("%u : %.4lf", bar.bar, bar.barTime);
            }
            if (ImGui::TableNextColumn()) {
                TempoChange tempo =
                    (t != data->timingInfo.cend())
                        ? *t
                        : TempoChange{.time = 0,
                                      .timeMicros = 0,
                                      .microsPerTick = getMicrosPerTick(
                                          data->division, 500000)};
                ImGui::Text("%llu", getTimeMicros(tempo, message.time));
            }
            if (ImGui::TableNextColumn()) printTextForTrackEventType(message);
            if (ImGui::TableNextColumn()) {
                bool changed = printDataTextForTrackEvent(message);
                if (changed && message.type == META &&
                    message.meta->type == SET_TEMPO) {
                    tempoChanged = true;
                } else if (changed && message.type == META &&
                           message.meta->type == TIME_SIGNATURE) {
                    timeSignatureChanged = true;
                }
            }
            ImGui::PopID();

            if (--remaining == 0) break;
        }
        if (remaining == 0) break;
        ImGui::PushID(j + this->totalEvents);
        ImGui::TableHeadersRow();
        ImGui::PopID();
    }

    ImGui::PushID(1);
    ImGui::TableHeadersRow();
    ImGui::PopID();
    // TODO move that to update()
    if (tempoChanged) {
        data->timingInfo.clear();

        for (u32 j = 0; j < data->tracks; j++) {
            computeTimeMapsForTrack(*data, data->data[j]);
        }
    }
    if (timeSignatureChanged) {
        data->timeSignatureInfo.clear();

        for (u32 j = 0; j < data->tracks; j++) {
            computeTimeSignatureMapForTrack(*data, data->data[j]);
        }
    }
    ImGui::EndTable();
    ImGui::End();
}