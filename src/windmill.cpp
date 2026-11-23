#include "windmill.hpp"

#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


using namespace gl;
using namespace glm;


Windmill::Windmill()
: windSpeed(0.f)
, windAngle(0.f)
, angularSpeed(0.0f)
, rotorAngle(0.f)
, roofAngle(0.0f)
{}

void Windmill::initBaseMatrices() {

    baseModel_ = glm::mat4(1.0f);
    baseModel_ = glm::translate(baseModel_, glm::vec3(0.0f, 0.0f, -10.0f));
    baseModel_ = glm::scale(baseModel_, glm::vec3(5.0f, 5.0f, 5.0f));
    baseModel_ = glm::translate(baseModel_, glm::vec3(0.0f,0.06f,0.0f));
    walls = baseModel_;

}
void Windmill::loadModels()
{
    blade_.load("../models/windmill-blade.ply");
    bladeframe_.load("../models/windmill-bladeframe.ply");
    bladebeam_.load("../models/windmill-bladebeam.ply");
    mainbeam_.load("../models/windmill-mainbeam.ply");
    millstone_.load("../models/windmill-millstone.ply");
    roof_.load("../models/windmill-roof.ply");
    walls_.load("../models/windmill-walls.ply");
}

void Windmill::update(float deltaTime)
{
    if (deltaTime < 0.001)
        return;

    const float RADIUS = 12.5f;
    const float TSR = 6.0f;
    float angularAccel = TSR / RADIUS * windSpeed * deltaTime;

    const float FRICTION_FACTOR = 0.2f;
    float friction = angularSpeed * FRICTION_FACTOR;
    angularAccel -= friction;
    angularSpeed += angularAccel * deltaTime;
    rotorAngle += angularSpeed * deltaTime;

    roofAngle += TSR / RADIUS * (windAngle - roofAngle) * angularSpeed * deltaTime;
}

void Windmill::drawModel(Model& model, glm::mat4& projView, glm::mat4& modelMatrix) {

    glm::mat4 mvp = projView * modelMatrix;
    glUniformMatrix4fv(mvpUniformLocation,1,GL_FALSE, glm::value_ptr(mvp));
    model.draw();
}

glm::mat4 Windmill::millstoneMatrix() const
{
    glm::mat4 millstone = glm::rotate(walls, 5.0f * rotorAngle, glm::vec3(0.0f, 1.0f, 0.0f));
    millstone = glm::translate(millstone, glm::vec3(-0.48f, 0.21f, 0.0f));
    return glm::rotate(millstone, 5.0f * 2.27f * rotorAngle, glm::vec3(1.0f, 0.0f, 0.0f));
}

void Windmill::drawMillstone(glm::mat4& projView)
{
    glm::mat4 millstone = millstoneMatrix();
    drawModel(millstone_, projView, millstone);
}

glm::vec3 Windmill::flourEmitterPosition() const
{
    return glm::vec3(millstoneMatrix() * glm::vec4(-0.1f, 0.0f, 0.0f, 1.0f));
}

glm::vec3 Windmill::flourEmitterDirection() const
{
    return glm::normalize(glm::vec3(millstoneMatrix() * glm::vec4(-1.0f, 0.0f, 0.0f, 0.0f)));
}

glm::mat4 Windmill::drawBladebeam(glm::mat4& modelMatrix, glm::mat4& projView) {

    glm::mat4 bladebeam = modelMatrix;
    bladebeam = glm::translate(bladebeam, glm::vec3(0.0f, 0.25f, 0.7f));
    bladebeam = glm::rotate(bladebeam, rotorAngle, glm::vec3(0.0f, 0.0f, 1.0f));

    drawModel(bladebeam_, projView, bladebeam);

    return bladebeam;

}
void Windmill::drawBladeframe(glm::mat4& modelMatrix, glm::mat4& projView) {

    for (int i = 0; i < 4; i++) {

        glm::mat4 bladeframe = modelMatrix;

        bladeframe = glm::rotate(
            bladeframe,
            glm::quarter_pi<float>() + i * glm::half_pi<float>(),
            glm::vec3(0.0f, 0.0f, 1.0f)
        );

        bladeframe = glm::translate(
            bladeframe,
            glm::vec3(0.0f, 2.38f, 0.0f)
        );

        bladeframe = glm::rotate(
            bladeframe,
            -glm::half_pi<float>(),
            glm::vec3(1.0f, 0.0f, 0.0f)
        );

        drawModel(bladeframe_, projView, bladeframe);
    }

}

void Windmill::drawBlades(glm::mat4& modelMatrix, glm::mat4& projView) {

    for (int i = 0; i < 4; i++) {

        glm::mat4 blade = modelMatrix;

        blade = glm::rotate(
            blade,
            glm::quarter_pi<float>() + i * glm::half_pi<float>(),
            glm::vec3(0.0f, 0.0f, 1.0f)
        );

        blade = glm::translate(
            blade,
            glm::vec3(-1.23f, 2.75f, 0.0f)
        );

        blade = glm::rotate(
            blade,
            -glm::half_pi<float>(),
            glm::vec3(1.0f, 0.0f, 0.0f)
        );

        drawModel(blade_, projView, blade);
    }
}
void Windmill::draw(glm::mat4& projView)
{

    drawModel(walls_, projView, walls);

    glm::mat4 roof = baseModel_;
    roof = glm::translate(roof, glm::vec3(0.0f, 3.03f, 0.0f));
    roof = glm::rotate(roof, roofAngle, glm::vec3(0.0f, 1.0f, 0.0f));
    drawModel(roof_, projView, roof);

    glm::mat4 mainbeam = walls;
    mainbeam = glm::rotate(mainbeam, 5 * rotorAngle, glm::vec3(0.0f, 1.0f, 0.0f));
    drawModel(mainbeam_, projView, mainbeam);

    drawMillstone(projView);

    glm::mat4 blades = drawBladebeam(roof,projView);
    blades = glm::translate(blades, glm::vec3(0.0f, 0.0f, 0.5f));
    blades = glm::scale(blades, glm::vec3(0.5f, 0.5f, 0.5f));

    drawBladeframe(blades, projView);
    drawBlades(blades, projView);
}


void Windmill::release()
{
    blade_.release();
    bladeframe_.release();
    bladebeam_.release();
    mainbeam_.release();
    millstone_.release();
    roof_.release();
    walls_.release();
}
