#pragma once

#include <glbinding/gl/gl.h>
#include <glm/glm.hpp>

#include "model.hpp"

class Windmill
{
public:
    Windmill();

    void initBaseMatrices();

    void loadModels();

    void release();

    void update(float deltaTime);

    void draw(glm::mat4& projView);

    glm::vec3 flourEmitterPosition() const;
    glm::vec3 flourEmitterDirection() const;

private:
    void drawModel(Model& model, glm::mat4& projView, glm::mat4& modelMatrix);
    glm::mat4 millstoneMatrix() const;
    void drawMillstone(glm::mat4& projView);
    glm::mat4 drawBladebeam(glm::mat4& modelMatrix, glm::mat4& projView);
    void drawBladeframe(glm::mat4& modelMatrix, glm::mat4& projView);
    void drawBlades(glm::mat4& modelMatrix, glm::mat4& projView);
private:
    Model blade_;
    Model bladeframe_;
    Model bladebeam_;
    Model mainbeam_;
    Model millstone_;
    Model roof_;
    Model walls_;
    glm::mat4 baseModel_;
    glm::mat4 walls;

public:
    float windSpeed;
    float windAngle;

    float angularSpeed;
    float rotorAngle;

    float roofAngle;

    GLuint mvpUniformLocation;
};


