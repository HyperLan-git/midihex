#include "ButtonHandler.hpp"

#include <vector>

#include "Editor.hpp"
#include "portable-file-dialogs.h"

void ButtonHandler::registerAll(Editor* editor) {
    this->registerButton(
        "New", [=]() { editor->setData(std::shared_ptr<MidiFile>()); });
    this->registerButton("Load", [=]() {
        std::vector<std::string> selection =
            pfd::open_file("Select a file", ".",
                           {"Midi File", "*.mid *.MID", "All Files", "*"})
                .result();
        if (!selection.empty()) {
            std::cout << "Selected file: " << selection[0] << "\n";
            editor->loadFile(selection[0]);
        }
    });
    this->registerButton("Save", [=]() {
        if (!editor->getData()) return;
        std::string selection =
            pfd::save_file("Save as", ".",
                           {"Midi File", "*.mid *.MID", "All Files", "*"})
                .result();
        std::cout << "Selected file: " << selection << "\n";
        editor->saveFile(selection);
    });
    this->registerButton("Track editor", [=]() { editor->openTrackEditor(); });
    this->registerButton("Add event", [=]() { editor->openAddEventEditor(); });
    this->registerButton("Remove event",
                         [=]() { editor->deleteSelectedEvent(); });
}