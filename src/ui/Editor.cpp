#include "Editor.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>

#include "portable-file-dialogs.h"

Editor::Editor() {
    glfwSetErrorCallback([](int error, const char* description) {
        std::cerr << "GLFW Error " << error << ": " << description << std::endl;
    });
    if (!glfwInit()) throw std::runtime_error("Could not load GLFW !");

    // GL 3.0 + GLSL 130
    constexpr char glsl_version[] = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    // Create window with graphics context
    window = glfwCreateWindow(1280, 720, "Midihex", nullptr, nullptr);
    if (window == nullptr) throw std::runtime_error("Could not open window !");
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);  // Enable vsync

    // Setup context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Setup style
    ImGui::StyleColorsDark();
    ImGui::GetStyle().WindowRounding = 0.0f;

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    load();
}

void Editor::load() {
    resourceManager.addTexture("new_icon", "./resources/images/New.png");
    resourceManager.addTexture("load_icon", "./resources/images/Load.png");
    resourceManager.addTexture("save_icon", "./resources/images/Save.png");
    resourceManager.addTexture("add_event_icon",
                               "./resources/images/MidiAdd.png");
    resourceManager.addTexture("remove_event_icon",
                               "./resources/images/MidiSub.png");

    resourceManager.load();

    buttonHandler.registerAll(this);
}

void Editor::update() { this->buttonHandler.runAll(); }

constexpr ImGuiWindowFlags MAIN_WINDOW_FLAGS = ImGuiWindowFlags_NoCollapse |
                                               ImGuiWindowFlags_NoResize |
                                               ImGuiWindowFlags_NoMove;

void ImGuiPrint(std::shared_ptr<MidiFile> midi) {
    if (!midi) return;
    std::ostringstream str;
    std::streambuf* buf = std::cout.rdbuf(str.rdbuf());
    printMidiFile(*midi);
    std::cout.rdbuf(buf);
    ImGui::Text("%s", str.str().c_str());
}

// Sets default layout of windows on screen
void setupDockSpace() {
    ImGuiIO& io = ImGui::GetIO();

    ImGuiID dockspace = ImGui::GetID("MainDockSpace");

    ImGui::DockSpace(dockspace, {0, 0}, ImGuiDockNodeFlags_PassthruCentralNode);

    static bool first_time = true;
    if (!first_time) return;

    first_time = false;

    ImGui::DockBuilderRemoveNode(dockspace);
    ImGui::DockBuilderAddNode(
        dockspace, (ImGuiDockNodeFlags_)ImGuiDockNodeFlags_DockSpace |
                       ImGuiDockNodeFlags_NoSplit);
    ImGui::DockBuilderSetNodeSize(dockspace, io.DisplaySize);
    ImGuiID dock_id = ImGui::DockBuilderSplitNode(dockspace, ImGuiDir_Up, .7f,
                                                  nullptr, &dockspace);
    ImGuiID dock_id2 = ImGui::DockBuilderSplitNode(dock_id, ImGuiDir_Up, .2f,
                                                   nullptr, &dock_id);
    ImGuiID dock_id3 = ImGui::DockBuilderSplitNode(dockspace, ImGuiDir_Left,
                                                   .7f, nullptr, &dockspace);
    ImGui::DockBuilderDockWindow("Tools", dock_id2);
    ImGui::DockBuilderDockWindow("Table", dock_id);
    ImGui::DockBuilderDockWindow("File", dock_id3);
    ImGui::DockBuilderDockWindow("Parameters", dockspace);

    ImGui::DockBuilderFinish(dockspace);
}

void Editor::render() {
    ImGuiIO& io = ImGui::GetIO();
    std::shared_ptr<MidiFile> data = this->getData();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::GetFont()->Scale = 1.5f;

    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::SetNextWindowPos({0, 0}, ImGuiCond_Always);
    if (ImGui::Begin("Editor", NULL, MAIN_WINDOW_FLAGS)) {
        setupDockSpace();
    }
    ImGui::End();

    if (!error.empty())
        ImGui::OpenPopup("Error");
    else if (trackEditorOpen && data)
        ImGui::OpenPopup("Track editor");
    else if (addEventEditorOpen && data)
        ImGui::OpenPopup("Event add editor");

    renderTrackEditor(data);

    renderEventAddEditor(data);

    renderError();

    renderApplicationBar();

    renderFileParams(data);
    renderTable(data);
    renderParams(data);

    // ImGui::ShowDemoWindow

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
}

#include <fstream>
// TODO put that elsewhere
void Editor::loadFile(std::string path) {
    std::ifstream f(path, std::fstream::in);
    if (!f.is_open()) {
        this->showError("Could not open MIDI file !");
        return;
    }

    f.seekg(0, std::fstream::end);
    std::size_t sz = f.tellg();
    if (f.fail()) {
        f.close();
        this->showError("Could not read MIDI file !");
        return;
    }

    u8* buffer = new u8[sz];
    if (!buffer) {
        f.close();
        this->showError("Not enough memory !");
        return;
    }
    f.seekg(std::fstream::beg);
    f.read((char*)buffer, sz);
    f.close();

    struct MidiFile* midi = nullptr;

    enum MidiError err = readMidiFile(buffer, sz, midi);
    if (err) {
        this->showError(std::string("Error when parsing midi file headers ! ") +
                        std::to_string(err));
        delete[] buffer;
        return;
    }

    // printMidiFile(*midi);

    for (u32 i = 0; i < midi->tracks; i++) {
        struct MidiTrack& track = midi->data[i];
        err = decodeTrack(*midi, track);
        if (err) {
            delete midi;
            delete[] buffer;
            this->showError(std::string("Error when decoding midi tracks ! ") +
                            std::to_string(err));
            return;
        }
    }

    delete[] buffer;

    this->setData(std::shared_ptr<MidiFile>(midi));
}

void Editor::saveFile(std::string path) {
    std::ofstream stream(path, std::ios::out | std::ios::binary);
    std::shared_ptr<MidiFile> file = getData();
    if (!file) return;
    enum MidiError err = writeMidiFile(*file, stream);
    if (err != NONE) {
        std::cerr << "Could not save midi file : " << err << "\n";
    }
}

// XXX fix potential race condition
void Editor::addEvent(u16 track, u32 pos, const TrackEvent& e) {
    std::shared_ptr<MidiFile> data = getData();
    if (!data) return;
    MidiTrack& t = data->data[track];
    std::vector<TrackEvent>& eList = t.list;
    eList.insert(eList.cbegin() + pos, e);
    this->totalEvents++;
    if (e.type == META) {
        if (e.meta->type == SET_TEMPO) {
            for (u16 i = 0; i < data->tracks; i++)
                computeTimingMapForTrack(*data, data->data[i]);
        } else if (e.meta->type == TIME_SIGNATURE) {
            for (u16 i = 0; i < data->tracks; i++)
                computeTimeSignatureMapForTrack(*data, data->data[i]);
        }
    }
    computeTimes(eList);
}

void Editor::removeEvent(u16 track, u32 pos) {
    std::shared_ptr<MidiFile> data = getData();
    if (!data) return;
    MidiTrack& t = data->data[track];
    std::vector<TrackEvent>& eList = t.list;
    const TrackEvent e(eList[pos]);
    eList.erase(eList.cbegin() + pos);
    this->totalEvents--;
    if (e.type == META) {
        if (e.meta->type == SET_TEMPO) {
            for (u16 i = 0; i < data->tracks; i++)
                computeTimingMapForTrack(*data, t);
        } else if (e.meta->type == TIME_SIGNATURE) {
            for (u16 i = 0; i < data->tracks; i++)
                computeTimeSignatureMapForTrack(*data, t);
        }
    }
    computeTimes(eList);
}

Editor::~Editor() {
    while (glGetError() != GL_NO_ERROR);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
}