#include "ToolStrip.hpp"

#include "portable-file-dialogs.h"

ToolStrip::ToolStrip(Editor& editor, ResourceManager& resourceManager,
                     ButtonHandler& buttonHandler)
    : editor(editor),
      resourceManager(resourceManager),
      buttonHandler(buttonHandler) {
    buttonHandler.registerButton("New", [this]() {
        std::shared_ptr<MidiFile> file = std::make_shared<MidiFile>();
        file->data = new MidiTrack[1];
        file->division = 24;
        file->format = 0;
        file->length = 6;
        file->tracks = 1;
        file->data->decoded = true;
        file->data->data = NULL;
        file->data->length = 0;
        file->data->list = std::vector<TrackEvent>();

        TrackEvent endTrack;
        endTrack.deltaTime = 0;
        endTrack.time = 0;
        endTrack.type = META;
        endTrack.meta = new MetaEvent(END_OF_TRACK);

        file->data->list.push_back(endTrack);

        computeTimeMapsForTrack(*file, file->data[0]);

        this->editor.setData(file);
    });
    buttonHandler.registerButton("Load", [this]() {
        std::vector<std::string> selection =
            pfd::open_file("Select a file", ".",
                           {"Midi File", "*.mid *.MID", "All Files", "*"})
                .result();
        if (!selection.empty()) {
            std::cout << "Selected file: " << selection[0] << "\n";
            this->editor.loadFile(selection[0]);
        }
    });
    buttonHandler.registerButton("Save", [this]() {
        if (!this->editor.getData()) return;
        std::string selection =
            pfd::save_file("Save as", ".",
                           {"Midi File", "*.mid *.MID", "All Files", "*"})
                .result();
        this->editor.saveFile(selection);
    });
}

inline void createImageButton(const std::string& id, ButtonHandler& handler,
                              const Texture& texture) {
    if (ImGui::ImageButton(id.c_str(), texture.tex,
                           {(float)texture.w, (float)texture.h})) {
        handler.pressButton(id);
    }
}

void ToolStrip::render() {
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

    int cols = szTools.x / (newIcon.w + 20);

    if (cols > 1) ImGui::SameLine();
    createImageButton("Load", this->buttonHandler, loadIcon);

    if (cols > 2) ImGui::SameLine();
    createImageButton("Save", this->buttonHandler, saveIcon);

    if (cols > 3 || cols == 2) ImGui::SameLine();
    createImageButton("Add event", this->buttonHandler, addMidiIcon);

    if (cols > 4 || cols == 3) ImGui::SameLine();
    createImageButton("Remove event", this->buttonHandler, removeMidiIcon);

    ImGui::End();
}