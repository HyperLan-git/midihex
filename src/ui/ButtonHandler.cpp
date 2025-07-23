#include "ButtonHandler.hpp"

#include <vector>

#include "Editor.hpp"
#include "portable-file-dialogs.h"

void ButtonHandler::registerAll(Editor* editor) {
    this->registerButton("Track editor", [=]() { editor->openTrackEditor(); });
    this->registerButton("Add event", [=]() { editor->openAddEventEditor(); });
    this->registerButton("Remove event",
                         [=]() { editor->deleteSelectedEvent(); });
}