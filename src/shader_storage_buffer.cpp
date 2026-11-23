#include "shader_storage_buffer.hpp"

#include <utility>

ShaderStorageBuffer::ShaderStorageBuffer()
{
}

ShaderStorageBuffer::ShaderStorageBuffer(ShaderStorageBuffer&& other)
{
    *this = std::move(other);
}


ShaderStorageBuffer::~ShaderStorageBuffer()
{
    release();
}

void ShaderStorageBuffer::release()
{
    if (id_)
    {
        glDeleteBuffers(1, &id_);
        id_ = 0;
    }
}

void ShaderStorageBuffer::allocate(const void* data, GLsizeiptr byteSize, GLenum usage)
{
    if (!id_)
        glGenBuffers(1, &id_);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, id_);
    glBufferData(GL_SHADER_STORAGE_BUFFER, byteSize, data, usage);
}

void ShaderStorageBuffer::setBindingIndex(GLuint index)
{
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, id_);
}

void ShaderStorageBuffer::updateData(const void* data, GLintptr offset, GLsizeiptr byteSize)
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, id_);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, byteSize, data);
}

void ShaderStorageBuffer::bindAsArray()
{
    glBindBuffer(GL_ARRAY_BUFFER, id_);
}

ShaderStorageBuffer& ShaderStorageBuffer::operator=(ShaderStorageBuffer&& other)
{
    if (this != &other)
    {
        release();
        id_ = std::exchange(other.id_, 0);
    }
    return *this;
}
