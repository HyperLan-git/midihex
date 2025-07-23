#pragma once

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imgui.h"

// XXX for dockbuilder
#include "imgui_internal.h"

#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h>

#include <atomic>
#include <memory>
#include <sstream>

#include "ButtonHandler.hpp"
#include "MidiFile.hpp"
#include "ResourceManager.hpp"
#include "ToolStrip.hpp"

// FIXME split this into multiple classes with references to main class' data
// TODO stop mixing logic and frontend as much
class Editor {
   public:
    Editor();

    void load();

    void update();
    // Must call periodically so the os doesn't show our window as unresponsive
    inline void updateWindow() const { glfwPollEvents(); }
    void render();

    void loadFile(std::string path);
    void saveFile(std::string path);

    void setData(std::shared_ptr<MidiFile> ptr) {
        std::atomic_store(&data, ptr);
        this->totalEvents = 0;
        if (!ptr) return;
        for (u32 track = 0; track < data->tracks; track++) {
            if (data->data[track].decoded)
                totalEvents += data->data[track].list.size();
        }
    }
    std::shared_ptr<MidiFile> getData() const {
        return std::atomic_load(&data);
    }

    bool shouldClose() { return glfwWindowShouldClose(window); }

    void openTrackEditor() {
        if (!data) return;
        trackEditorOpen = true;
    }
    void openAddEventEditor() {
        if (!data) return;
        addEventEditorOpen = true;
    }

    void showError(std::string error) { this->error = error; }

    void addEvent(u16 track, u32 pos, const TrackEvent& e);
    void removeEvent(u16 track, u32 pos);
    void addTrack(u16 idx);
    void removeTrack(u16 idx);

    void deleteSelectedEvent() {
        removeEvent(this->selectedTrack, this->selectedEvent);
    }

    ~Editor();

   private:
    GLFWwindow* window = nullptr;
    std::shared_ptr<MidiFile> data;

    ImFont* font;

    std::string errorString;

    u64 eventTableSize = 500, offset = 0;
    u64 totalEvents = 0;

    bool showAllTracks = true;
    u32 trackToShow = 0;

    ButtonHandler buttonHandler;

    ResourceManager resourceManager;

    ToolStrip toolStrip;

    bool trackEditorOpen = false;

    bool addEventEditorOpen = false;
    u32 selectedTrack = 0, selectedEvent = 0;

    bool tempoHasChanged = false, timeSignatureHasChanged = false;

    std::string error;

    bool printDataTextForTrackEvent(TrackEvent& ev);
    void printTextForTrackEventType(const TrackEvent& ev);

    void renderFileParams(std::shared_ptr<MidiFile>& data);
    void renderTable(std::shared_ptr<MidiFile>& data);
    void renderParams(std::shared_ptr<MidiFile>& data);
    void renderTrackEditor(std::shared_ptr<MidiFile>& data);
    void renderEventAddEditor(std::shared_ptr<MidiFile>& data);
    void renderError();
};