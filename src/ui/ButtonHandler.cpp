#include "ButtonHandler.hpp"

#include <vector>

#include "Editor.hpp"
#include "portable-file-dialogs.h"

void ButtonHandler::registerAll(Editor* editor) {
    this->registerButton("New", [=]() {
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

        editor->setData(file);
    });
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
        editor->saveFile(selection);
    });
    this->registerButton("Track editor", [=]() { editor->openTrackEditor(); });
    this->registerButton("Add event", [=]() { editor->openAddEventEditor(); });
    this->registerButton("Remove event",
                         [=]() { editor->deleteSelectedEvent(); });
}