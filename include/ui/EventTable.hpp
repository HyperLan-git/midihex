#pragma once

#include <memory>

#include "MidiFile.hpp"

class EventTable {
   public:
    EventTable(std::shared_ptr<MidiFile>&);

   private:
    std::shared_ptr<MidiFile>& data;
};