#include "vis/shader.hpp"
#include <iostream>

namespace vis {

Shader::Shader() {
    shader = al_create_shader(ALLEGRO_SHADER_AUTO);
    const char* pixelSource = al_get_default_shader_source(ALLEGRO_SHADER_AUTO, ALLEGRO_PIXEL_SHADER);
    const char* vertexSource = al_get_default_shader_source(ALLEGRO_SHADER_AUTO, ALLEGRO_VERTEX_SHADER);

    std::cout << pixelSource << std::endl;
    std::cout << vertexSource << std::endl;
    if (!pixelSource || !vertexSource) {
        std::cerr << "Failed to get default shader source." << std::endl;
        al_destroy_shader(shader);
        shader = nullptr;
        return;
    }

    loadFromSource(vertexSource, pixelSource);
}

Shader::Shader(const std::string &vertexSource, const std::string &fragmentSource, bool fromFile) {
    load(vertexSource, fragmentSource, fromFile);
}

Shader::~Shader() {
    if (shader) {
        al_destroy_shader(shader);
    }
}

bool Shader::load(const std::string &vertexSource, const std::string &fragmentSource, bool fromFile) {
    if (shader) {
        al_destroy_shader(shader);
        shader = nullptr;
    }

    shader = al_create_shader(ALLEGRO_SHADER_AUTO);

    if (fromFile) {
        return loadFromFile(vertexSource, fragmentSource);
    } else {
        return loadFromSource(vertexSource, fragmentSource);
    }
}

void Shader::use() const {
    if (shader) {
        al_use_shader(shader);
    }
}

bool Shader::isCurrentShader() const {
    return shader && al_get_current_shader() == shader;
}

bool Shader::loadFromSource(const std::string &vertexSource, const std::string &fragmentSource)
{
    bool r1 = al_attach_shader_source(shader, ALLEGRO_VERTEX_SHADER, vertexSource.c_str());
    bool r2 = al_attach_shader_source(shader, ALLEGRO_PIXEL_SHADER, fragmentSource.c_str());
    if (r1 && r2) {
        if (!al_build_shader(shader)) {
            std::cerr << "Shader build failed: " << al_get_shader_log(shader) << std::endl;
            return false;
        }
    }
    return r1 && r2;
}

bool Shader::loadFromFile(const std::string &vertexPath, const std::string &fragmentPath)
{
    bool r1 = al_attach_shader_source_file(shader, ALLEGRO_VERTEX_SHADER, vertexPath.c_str());
    bool r2 = al_attach_shader_source_file(shader, ALLEGRO_PIXEL_SHADER, fragmentPath.c_str());
    if (r1 && r2) {
        if (!al_build_shader(shader)) {
            std::cerr << "Shader build failed: " << al_get_shader_log(shader) << std::endl;
            return false;
        } else {
            std::cout << "Shader loaded successfully from files: " << vertexPath << " and " << fragmentPath << std::endl;
        }
    }
    return r1 && r2;
};

bool Shader::setInt(const char *name, int value) {
    if (!isCurrentShader()) return false;
    return al_set_shader_int(name, value);
};

bool Shader::setFloat(const char *name, float value) {
    if (!isCurrentShader()) return false;
    return al_set_shader_float(name, value);
};

bool Shader::setBool(const char *name, bool value) {
    if (!isCurrentShader()) return false;
    return al_set_shader_bool(name, value);
};

bool Shader::setIntVector(const char *name, int num_components, const int *values, int count) {
    if (!isCurrentShader()) return false;
    return al_set_shader_int_vector(name, num_components, values, count);
};

bool Shader::setFloatVector(const char *name, int num_components, const float *values, int count) {
    if (!isCurrentShader()) return false;
    return al_set_shader_float_vector(name, num_components, values, count);
};

bool Shader::setTexture(const char *name, ALLEGRO_BITMAP *texture, int unit) {
    if (!isCurrentShader()) return false;
    return al_set_shader_sampler(name, texture, unit);
};

bool Shader::setMatrix(const char *name, const ALLEGRO_TRANSFORM *matrix) {
    if (!isCurrentShader()) return false;
    return al_set_shader_matrix(name, matrix);
};

}