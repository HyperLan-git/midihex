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

// FIXME split this into multiple classes with references to main class' data
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

    void openTrackEditor() { trackEditorOpen = true; }

    void showError(std::string error) { this->error = error; }

    void addEvent(MidiTrack& track, std::vector<TrackEvent>::const_iterator pos,
                  const TrackEvent& e);
    void removeEvent(MidiTrack& track, std::vector<TrackEvent>::iterator e);

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

    bool trackEditorOpen = false;

    std::string error;

    bool printDataTextForTrackEvent(TrackEvent& ev);
    void printTextForTrackEventType(const TrackEvent& ev);

    void renderFileParams(std::shared_ptr<MidiFile>& data);
    void renderTable(std::shared_ptr<MidiFile>& data);
    void renderParams(std::shared_ptr<MidiFile>& data);
    void renderApplicationBar(std::shared_ptr<MidiFile>& data);
    void renderTrackEditor(std::shared_ptr<MidiFile>& data);
    void renderError();
};