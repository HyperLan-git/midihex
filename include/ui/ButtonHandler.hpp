#pragma once

#include <functional>
#include <map>
#include <string>

typedef std::function<void()> ButtonCallback;

struct ButtonHandle {
    bool pressed = false;
    ButtonCallback handler = NULL;
};

class Editor;

class ButtonHandler {
   public:
    ButtonHandler() {}

    void registerAll(Editor *editor);

    void registerButton(const std::string &buttonID, ButtonCallback callback) {
        this->statuses.emplace(buttonID, ButtonHandle(false, callback));
    }

    void pressButton(const std::string &buttonID) {
        // TODO debug warn
        if (this->statuses.count(buttonID) == 0) return;
        this->statuses[buttonID].pressed = true;
    }

    void runAll() {
        for (std::pair<const std::string, ButtonHandle> &pair : statuses) {
            if (pair.second.pressed) {
                pair.second.handler();
                pair.second.pressed = false;
            }
        }
    }

   private:
    std::map<std::string, ButtonHandle> statuses;
};