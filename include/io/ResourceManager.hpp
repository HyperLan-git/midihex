#pragma once
#include <map>
#include <stdexcept>
#include <string>

#include "Ints.hpp"

struct Texture {
    bool loaded = false;
    const char* path;
    u32 tex = 0;
    int w, h;

    Texture(const char* path) { this->path = path; }

    ~Texture() {}
};

class ResourceManager {
   public:
    ResourceManager();

    inline void addTexture(std::string id, const char* path) {
        textures.emplace(id, path);
    }

    inline const Texture& getTexture(std::string id,
                                     bool assertLoaded = true) const {
        if (textures.count(id) == 0)
            throw std::runtime_error("Could not find texture : " + id);
        const Texture& t = textures.at(id);
        if (assertLoaded && !t.loaded)
            throw std::runtime_error("Texture not loaded !");
        return t;
    }

    void load();

    ~ResourceManager();

   private:
    std::map<std::string, Texture> textures;
};