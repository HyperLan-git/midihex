#pragma once

#include <memory>

#include "ButtonHandler.hpp"
#include "MidiFile.hpp"
#include "ResourceManager.hpp"

class Editor;

class ToolStrip {
   public:
    ToolStrip(Editor& editor, ResourceManager& resourceManager,
              ButtonHandler& buttonHandler);

    void render();

   private:
    Editor& editor;
    ResourceManager& resourceManager;
    ButtonHandler& buttonHandler;
};

#include "Editor.hpp"