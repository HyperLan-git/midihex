#include <cstring>
#include <fstream>
#include <iostream>
#include <thread>

#include "Editor.hpp"
#include "MidiFile.hpp"

int processMain(int argc, char** argv, Editor* e) {
    if (argc >= 2) {
        e->loadFile(argv[1]);
    }
    while (!e->shouldClose()) {
        e->update();
    }
    return 0;
}

int main(int argc, char** argv) {
    Editor e;
    std::thread process(processMain, argc, argv, &e);
    while (!e.shouldClose()) {
        e.updateWindow();
        e.render();
    }
    process.join();
    return 0;
}