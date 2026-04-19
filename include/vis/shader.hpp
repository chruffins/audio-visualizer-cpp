#pragma once

#include <string>
#include <allegro5/allegro.h>

namespace vis {

class Shader {
public:
    Shader();
    Shader(const std::string& vertexSource, const std::string& fragmentSource, bool fromFile = false);
    ~Shader();

    bool load(const std::string& vertexSource, const std::string& fragmentSource, bool fromFile = false);
    void use() const;

    bool isLoaded() const { return shader != nullptr; }
    bool setInt(const char* name, int value);
    bool setFloat(const char* name, float value);
    bool setBool(const char* name, bool value);
    bool setIntVector(const char* name, int num_components, const int* values, int count);
    bool setFloatVector(const char* name, int num_components, const float* values, int count);
    bool setTexture(const char* name, ALLEGRO_BITMAP* texture, int unit);
    bool setMatrix(const char* name, const ALLEGRO_TRANSFORM* matrix);
private:
    bool isCurrentShader() const;
    bool loadFromSource(const std::string& vertexSource, const std::string& fragmentSource);
    bool loadFromFile(const std::string& vertexPath, const std::string& fragmentPath);

    ALLEGRO_SHADER* shader = nullptr;
};

};