#include "ResourceManager.hpp"

#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h>

#include <fstream>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

ResourceManager::ResourceManager() {}

bool LoadTextureFromMemory(const void* data, size_t data_size,
                           GLuint& out_texture, int& out_width,
                           int& out_height) {
    // Load from file
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data =
        stbi_load_from_memory((const unsigned char*)data, (int)data_size,
                              &image_width, &image_height, NULL, 4);
    std::cout << data_size << ((image_data == NULL) ? "t":"f") << std::endl;
    if (image_data == NULL) return false;

    // Create a OpenGL texture identifier
    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload pixels into texture
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);

    out_texture = image_texture;
    out_width = image_width;
    out_height = image_height;

    return true;
}

bool LoadTextureFromFile(const char* file_name, GLuint& out_texture,
                         int& out_width, int& out_height) {
    int a;
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load(file_name, &image_width, &image_height, &a, 4);
    
    // Create a OpenGL texture identifier
    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload pixels into texture
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);

    out_texture = image_texture;
    out_width = image_width;
    out_height = image_height;
    return out_texture != 0;
}

void ResourceManager::load() {
    for (std::pair<const std::string, Texture>& p : this->textures) {
        Texture& t = p.second;
        t.loaded = LoadTextureFromFile(t.path, t.tex, t.w, t.h);
    }
}

ResourceManager::~ResourceManager() {
    for (std::pair<const std::string, Texture>& p : textures) {
        Texture& t = p.second;
        if (t.loaded) {
            glDeleteTextures(1, &t.tex);
        }
    }
}