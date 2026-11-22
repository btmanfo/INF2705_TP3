#pragma once

#include <glbinding/gl/gl.h>

using namespace gl;

class Model
{
public:
    void load(const char* path);

    ~Model();

    void release();

    void draw();

private:
    GLuint vao_ = 0, vbo_ = 0, ebo_ = 0;
    GLsizei count_ = 0;
};

