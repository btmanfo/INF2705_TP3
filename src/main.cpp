#include <cstddef>
#include <cstdint>

#include <array>
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "bezier.hpp"
#include "shader_storage_buffer.hpp"
#include <imgui/imgui.h>

#include <inf2705/OpenGLApplication.hpp>

#include "model.hpp"
#include "windmill.hpp"


using namespace gl;
using namespace glm;

struct Vertex
{
    vec2 position;
    vec4 color;
};


struct Particle
{
    glm::vec3 position;
    GLfloat zOrientation;
    glm::vec4 velocity; // vec3 avec remplissage
    glm::vec4 color;
    glm::vec2 size;
    GLfloat timeToLive;
    GLfloat maxTimeToLive;
};

static_assert(sizeof(Particle) == 64);
static_assert(offsetof(Particle, zOrientation) == 12);
static_assert(offsetof(Particle, velocity) == 16);
static_assert(offsetof(Particle, color) == 32);
static_assert(offsetof(Particle, size) == 48);
static_assert(offsetof(Particle, timeToLive) == 56);
static_assert(offsetof(Particle, maxTimeToLive) == 60);

struct App : public OpenGLApplication
{
    App()
    : nSide_(5)
    , oldNSide_(0)
    , cameraPosition_(35.f, 28.f, 45.f)
    , cameraOrientation_(0.f, 0.f)
    , currentScene_(1)
    , isMouseMotionEnabled_(false)
    {
    }

    void init() override
    {
        // Le message expliquant les touches de clavier.
        setKeybindMessage(
            "ESC : quitter l'application." "\n"
            "T : changer de scène." "\n"
            "W : déplacer la caméra vers l'avant." "\n"
            "S : déplacer la caméra vers l'arrière." "\n"
            "A : déplacer la caméra vers la gauche." "\n"
            "D : déplacer la caméra vers la droite." "\n"
            "Q : déplacer la caméra vers le bas." "\n"
            "E : déplacer la caméra vers le haut." "\n"
            "Flèches : tourner la caméra." "\n"
            "Souris : tourner la caméra" "\n"
            "Espace : activer/désactiver la souris." "\n"
        );

        // Config de base.

       glClearColor(0.0f, 0.5f, 0.5f, 1.0f);

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);

        loadShaderPrograms();

        initShapeData();

        loadModels();
        initGround();
        windmill_.initBaseMatrices();
        orientCameraTowardMill();
        initBezier();
        initGrass();
        initParticles();
    }


    void checkShaderCompilingError(const char* name, GLuint id)
    {
        GLint success;
        GLchar infoLog[1024];

        glGetShaderiv(id, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(id, 1024, NULL, infoLog);
            glDeleteShader(id);
            std::cout << "Shader \"" << name << "\" compile error: " << infoLog << std::endl;
        }
    }


    void checkProgramLinkingError(const char* name, GLuint id)
    {
        GLint success;
        GLchar infoLog[1024];

        glGetProgramiv(id, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(id, 1024, NULL, infoLog);
            glDeleteProgram(id);
            std::cout << "Program \"" << name << "\" linking error: " << infoLog << std::endl;
        }
    }


    // Appelée à chaque trame. Le buffer swap est fait juste après.
    void drawFrame() override
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui::Begin("Scene Parameters");
        ImGui::Combo("Scene", &currentScene_, SCENE_NAMES, N_SCENE_NAMES);
        ImGui::End();

        switch (currentScene_)
        {
            case 0: sceneShape();  break;
            case 1: sceneModels(); break;
        }
    }

    // Appelée lorsque la fenêtre se ferme.
    void onClose() override
    {
        glUseProgram(0);
        glBindVertexArray(0);
        glDeleteVertexArrays(1, &bezierVao_);
        glDeleteBuffers(1, &bezierVbo_);
        glDeleteProgram(bezierSP_);
        glDeleteVertexArrays(1, &grassVao_);
        glDeleteBuffers(1, &grassVbo_);
        glDeleteTextures(1, &grassTexture_);
        glDeleteProgram(grassSP_);
        glDeleteVertexArrays(1, &particlesVao_);
        glDeleteTextures(1, &particleTexture_);
        glDeleteProgram(particlesUpdateSP_);
        glDeleteProgram(particlesDrawSP_);
        particles_[0].release();
        particles_[1].release();
        grass_.release();
        windmill_.release();
        glDeleteVertexArrays(1, &vao_);
        glDeleteBuffers(1, &vbo_);
        glDeleteBuffers(1, &ebo_);
        glDeleteProgram(basicSP_);
        glDeleteProgram(transformSP_);
    }

    // Appelée lors d'une touche de clavier.
    void onKeyPress(const sf::Event::KeyPressed& key) override
    {
        using enum sf::Keyboard::Key;
        switch (key.code)
        {
            case Escape:
                requestClose();
            break;
            case Space:
                isMouseMotionEnabled_ = !isMouseMotionEnabled_;
                if (isMouseMotionEnabled_)
                {
                    window_.setMouseCursorGrabbed(true);
                    window_.setMouseCursorVisible(false);
                }
                else
                {
                    window_.setMouseCursorGrabbed(false);
                    window_.setMouseCursorVisible(true);
                }
            break;
            case T:
                currentScene_ = ++currentScene_ < N_SCENE_NAMES ? currentScene_ : 0;
            break;
            default: break;
        }
    }

    void onResize(const sf::Event::Resized& event) override
    {
    }

    void onMouseMove(const sf::Event::MouseMoved& mouseDelta) override
    {
        if (!isMouseMotionEnabled_ || isAnimatingCamera_)
            return;

        const float MOUSE_SENSITIVITY = 0.1;
        float cameraMouvementX = mouseDelta.position.y * MOUSE_SENSITIVITY;
        float cameraMouvementY = mouseDelta.position.x * MOUSE_SENSITIVITY;
        cameraOrientation_.y -= cameraMouvementY * deltaTime_;
        cameraOrientation_.x -= cameraMouvementX * deltaTime_;
    }

    void updateCameraInput()
    {
        if (!window_.hasFocus() || ImGui::GetIO().WantCaptureKeyboard)
            return;

        if (isMouseMotionEnabled_)
        {
            sf::Vector2u windowSize = window_.getSize();
            sf::Vector2i windowHalfSize(windowSize.x / 2.0f, windowSize.y / 2.0f);
            sf::Mouse::setPosition(windowHalfSize, window_);
        }

        float cameraMouvementX = 0;
        float cameraMouvementY = 0;

        const float KEYBOARD_MOUSE_SENSITIVITY = 1.5f;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
            cameraMouvementX -= KEYBOARD_MOUSE_SENSITIVITY;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
            cameraMouvementX += KEYBOARD_MOUSE_SENSITIVITY;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
            cameraMouvementY -= KEYBOARD_MOUSE_SENSITIVITY;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
            cameraMouvementY += KEYBOARD_MOUSE_SENSITIVITY;

        cameraOrientation_.y -= cameraMouvementY * deltaTime_;
        cameraOrientation_.x -= cameraMouvementX * deltaTime_;

        // Keyboard input
        glm::vec3 positionOffset = glm::vec3(0.0);
        const float SPEED = 10.f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W))
            positionOffset.z -= SPEED;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S))
            positionOffset.z += SPEED;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
            positionOffset.x -= SPEED;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
            positionOffset.x += SPEED;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Q))
            positionOffset.y -= SPEED;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::E))
            positionOffset.y += SPEED;

        positionOffset = glm::rotate(glm::mat4(1.0f), cameraOrientation_.y, glm::vec3(0.0, 1.0, 0.0)) * glm::vec4(positionOffset, 1);
        cameraPosition_ += positionOffset * glm::vec3(deltaTime_);
    }

    void loadModels()
    {
        windmill_.loadModels();
        grass_.load("../models/grass.ply");
    }

    GLuint loadShaderObject(GLenum type, const char* path)
    {
        GLuint shader = glCreateShader(type);
        std::string shaderSrc = readFile(path);
        auto src = shaderSrc.c_str();
        glShaderSource(shader,1,&src,nullptr);
        glCompileShader(shader);
        checkShaderCompilingError("shaderObject", shader);
        return shader;
    }

    void loadShaderPrograms()
    {
        const char* BASIC_VERTEX_SRC_PATH = "./shaders/basic.vs.glsl";
        const char* BASIC_FRAGMENT_SRC_PATH = "./shaders/basic.fs.glsl";
        basicSP_ = glCreateProgram();

        GLuint basicFragmentShader = loadShaderObject(GL_FRAGMENT_SHADER, BASIC_FRAGMENT_SRC_PATH);
        GLuint basicVertexShader = loadShaderObject(GL_VERTEX_SHADER, BASIC_VERTEX_SRC_PATH);

        glAttachShader(basicSP_, basicFragmentShader);
        glAttachShader(basicSP_, basicVertexShader);
        glLinkProgram(basicSP_);
        checkProgramLinkingError("shaderProgram", basicSP_);

        glDetachShader(basicSP_, basicFragmentShader);
        glDetachShader(basicSP_, basicVertexShader);
        glDeleteShader(basicFragmentShader);
        glDeleteShader(basicVertexShader);

        const char* TRANSFORM_VERTEX_SRC_PATH = "./shaders/transform.vs.glsl";
        const char* TRANSFORM_FRAGMENT_SRC_PATH = "./shaders/transform.fs.glsl";

        GLuint transformFragmentShader = loadShaderObject(GL_FRAGMENT_SHADER,TRANSFORM_FRAGMENT_SRC_PATH);
        GLuint transformVertexShader = loadShaderObject(GL_VERTEX_SHADER,TRANSFORM_VERTEX_SRC_PATH);

        transformSP_ = glCreateProgram();
        glAttachShader(transformSP_, transformFragmentShader);
        glAttachShader(transformSP_, transformVertexShader);
        glLinkProgram(transformSP_);
        checkProgramLinkingError("shaderProgram", transformSP_);

        glDetachShader(transformSP_, transformFragmentShader);
        glDetachShader(transformSP_, transformVertexShader);
        glDeleteShader(transformFragmentShader);
        glDeleteShader(transformVertexShader);

        GLuint bezierVertex = loadShaderObject(GL_VERTEX_SHADER, "./shaders/bezier.vs.glsl");
        GLuint bezierFragment = loadShaderObject(GL_FRAGMENT_SHADER, "./shaders/bezier.fs.glsl");
        bezierSP_ = glCreateProgram();
        glAttachShader(bezierSP_, bezierVertex);
        glAttachShader(bezierSP_, bezierFragment);
        glLinkProgram(bezierSP_);
        checkProgramLinkingError("bezier", bezierSP_);
        glDetachShader(bezierSP_, bezierVertex);
        glDetachShader(bezierSP_, bezierFragment);
        glDeleteShader(bezierVertex);
        glDeleteShader(bezierFragment);
        bezierMvpLocation_ = glGetUniformLocation(bezierSP_, "mvp");

        loadGrassShader();
        loadParticleShaders();

        mvpUniformLocation_ = glGetUniformLocation(transformSP_,"mvp");
        windmill_.mvpUniformLocation = glGetUniformLocation(transformSP_, "mvp");

    }

    void loadGrassShader()
    {
        const std::array<GLuint, 5> shaders = {
            loadShaderObject(GL_VERTEX_SHADER, "./shaders/grass.vs.glsl"),
            loadShaderObject(GL_TESS_CONTROL_SHADER, "./shaders/grass.tcs.glsl"),
            loadShaderObject(GL_TESS_EVALUATION_SHADER, "./shaders/grass.tes.glsl"),
            loadShaderObject(GL_GEOMETRY_SHADER, "./shaders/grass.gs.glsl"),
            loadShaderObject(GL_FRAGMENT_SHADER, "./shaders/grass.fs.glsl")
        };
        grassSP_ = glCreateProgram();
        for (GLuint shader : shaders)
            glAttachShader(grassSP_, shader);
        glLinkProgram(grassSP_);
        checkProgramLinkingError("grass", grassSP_);
        for (GLuint shader : shaders)
        {
            glDetachShader(grassSP_, shader);
            glDeleteShader(shader);
        }
        grassMvpLocation_ = glGetUniformLocation(grassSP_, "mvp");
        grassModelViewLocation_ = glGetUniformLocation(grassSP_, "modelView");
        grassTextureLocation_ = glGetUniformLocation(grassSP_, "wheatTexture");
    }

    void initGrass()
    {
        std::vector<glm::vec3> vertices;
        const float cellSize = 5.0f;
        for (int z = -25; z < 25; z += 5)
        {
            for (int x = -25; x < 25; x += 5)
            {
                const bool millClearing = x >= -5 && x < 5 && z >= -15 && z < -5;
                const bool accessPath = x == 0 && z >= -5;
                if (millClearing || accessPath)
                    continue;

                const glm::vec3 a(x, -0.1f, z);
                const glm::vec3 b(x + cellSize, -0.1f, z);
                const glm::vec3 c(x, -0.1f, z + cellSize);
                const glm::vec3 d(x + cellSize, -0.1f, z + cellSize);
                vertices.insert(vertices.end(), {a, c, b, b, c, d});
            }
        }
        grassVertexCount_ = static_cast<GLsizei>(vertices.size());
        glGenVertexArrays(1, &grassVao_);
        glBindVertexArray(grassVao_);
        glGenBuffers(1, &grassVbo_);
        glBindBuffer(GL_ARRAY_BUFFER, grassVbo_);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);

        sf::Image image;
        if (!image.loadFromFile("../textures/wheat_crop.png"))
            throw std::runtime_error("Impossible de charger la texture de ble");
        image.flipVertically();
        glGenTextures(1, &grassTexture_);
        glBindTexture(GL_TEXTURE_2D, grassTexture_);
        const auto size = image.getSize();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.getPixelsPtr());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void drawGrass(const glm::mat4& projView)
    {
        const GLboolean wasCulling = glIsEnabled(GL_CULL_FACE);
        GLint previousPatchSize, previousActiveTexture, previousTexture;
        glGetIntegerv(GL_PATCH_VERTICES, &previousPatchSize);
        glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActiveTexture);
        glActiveTexture(GL_TEXTURE0);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);

        glDisable(GL_CULL_FACE);
        glPatchParameteri(GL_PATCH_VERTICES, 3);
        glUseProgram(grassSP_);
        const glm::mat4 view = getViewMatrix();
        glUniformMatrix4fv(grassMvpLocation_, 1, GL_FALSE, glm::value_ptr(projView));
        glUniformMatrix4fv(grassModelViewLocation_, 1, GL_FALSE, glm::value_ptr(view));
        glUniform1i(grassTextureLocation_, 0);
        glBindTexture(GL_TEXTURE_2D, grassTexture_);
        glBindVertexArray(grassVao_);
        glDrawArrays(GL_PATCHES, 0, grassVertexCount_);
        glBindVertexArray(0);

        glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTexture));
        glActiveTexture(static_cast<GLenum>(previousActiveTexture));
        glPatchParameteri(GL_PATCH_VERTICES, previousPatchSize);
        if (wasCulling)
            glEnable(GL_CULL_FACE);
    }

    GLuint createParticleProgram(const std::vector<GLuint>& shaders, const char* name)
    {
        const GLuint program = glCreateProgram();
        for (GLuint shader : shaders)
            glAttachShader(program, shader);
        glLinkProgram(program);
        checkProgramLinkingError(name, program);
        for (GLuint shader : shaders)
        {
            glDetachShader(program, shader);
            glDeleteShader(shader);
        }
        return program;
    }

    void loadParticleShaders()
    {
        particlesUpdateSP_ = createParticleProgram({
            loadShaderObject(GL_COMPUTE_SHADER, "./shaders/particlesUpdate.cs.glsl")
        }, "particlesUpdate");
        particlesDrawSP_ = createParticleProgram({
            loadShaderObject(GL_VERTEX_SHADER, "./shaders/particlesDraw.vs.glsl"),
            loadShaderObject(GL_GEOMETRY_SHADER, "./shaders/particlesDraw.gs.glsl"),
            loadShaderObject(GL_FRAGMENT_SHADER, "./shaders/particlesDraw.fs.glsl")
        }, "particlesDraw");
        particlesTimeLocation_ = glGetUniformLocation(particlesUpdateSP_, "time");
        particlesDeltaTimeLocation_ = glGetUniformLocation(particlesUpdateSP_, "deltaTime");
        emitterPositionLocation_ = glGetUniformLocation(particlesUpdateSP_, "emitterPosition");
        emitterDirectionLocation_ = glGetUniformLocation(particlesUpdateSP_, "emitterDirection");
        particleCountLocation_ = glGetUniformLocation(particlesUpdateSP_, "particleCount");
        particlesViewLocation_ = glGetUniformLocation(particlesDrawSP_, "modelView");
        particlesProjectionLocation_ = glGetUniformLocation(particlesDrawSP_, "projection");
        particlesTextureLocation_ = glGetUniformLocation(particlesDrawSP_, "textureSampler");
    }

    void initParticles()
    {
        const std::array<Particle, MAX_PARTICLES> initial{};
        // Les deux buffers sont lus et écrits sur le GPU, puis lus comme sommets.
        particles_[0].allocate(initial.data(), sizeof(initial), GL_DYNAMIC_COPY);
        particles_[1].allocate(nullptr, sizeof(initial), GL_DYNAMIC_COPY);
        glGenVertexArrays(1, &particlesVao_);
        glBindVertexArray(particlesVao_);
        for (GLuint attribute = 0; attribute < 4; ++attribute)
            glEnableVertexAttribArray(attribute);
        glBindVertexArray(0);

        sf::Image image;
        if (!image.loadFromFile("../textures/smoke.png"))
            throw std::runtime_error("Impossible de charger la texture de farine");
        image.flipVertically();
        glGenTextures(1, &particleTexture_);
        glBindTexture(GL_TEXTURE_2D, particleTexture_);
        const auto size = image.getSize();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size.x, size.y, 0,
            GL_RGBA, GL_UNSIGNED_BYTE, image.getPixelsPtr());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void updateParticles()
    {
        totalTime_ += deltaTime_;
        timerParticles_ += deltaTime_;
        const float spawnInterval = 0.2f;
        const unsigned int spawned = static_cast<unsigned int>(timerParticles_ / spawnInterval);
        timerParticles_ -= static_cast<float>(spawned) * spawnInterval;
        nParticles_ = std::min(MAX_PARTICLES, nParticles_ + spawned);

        glUseProgram(particlesUpdateSP_);
        glUniform1f(particlesTimeLocation_, totalTime_);
        glUniform1f(particlesDeltaTimeLocation_, deltaTime_);
        const glm::vec3 position = windmill_.flourEmitterPosition();
        const glm::vec3 direction = windmill_.flourEmitterDirection();
        glUniform3fv(emitterPositionLocation_, 1, glm::value_ptr(position));
        glUniform3fv(emitterDirectionLocation_, 1, glm::value_ptr(direction));
        glUniform1ui(particleCountLocation_, nParticles_);
        particles_[particleInput_].setBindingIndex(0);
        particles_[1 - particleInput_].setBindingIndex(1);
        // Traiter toute la capacité pour garder les emplacements inactifs à zéro.
        glDispatchCompute((MAX_PARTICLES + 63) / 64, 1, 1);
    }

    void drawParticles()
    {
        if (nParticles_ == 0)
            return;

        const GLboolean wasBlending = glIsEnabled(GL_BLEND);
        GLboolean previousDepthMask;
        GLint srcRgb, dstRgb, srcAlpha, dstAlpha, equationRgb, equationAlpha;
        GLint previousActiveTexture, previousTexture, previousArrayBuffer;
        glGetBooleanv(GL_DEPTH_WRITEMASK, &previousDepthMask);
        glGetIntegerv(GL_BLEND_SRC_RGB, &srcRgb);
        glGetIntegerv(GL_BLEND_DST_RGB, &dstRgb);
        glGetIntegerv(GL_BLEND_SRC_ALPHA, &srcAlpha);
        glGetIntegerv(GL_BLEND_DST_ALPHA, &dstAlpha);
        glGetIntegerv(GL_BLEND_EQUATION_RGB, &equationRgb);
        glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &equationAlpha);
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previousArrayBuffer);
        glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActiveTexture);
        glActiveTexture(GL_TEXTURE0);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);

        glEnable(GL_BLEND);
        glBlendEquation(GL_FUNC_ADD);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        glUseProgram(particlesDrawSP_);
        const glm::mat4 view = getViewMatrix();
        const glm::mat4 projection = getPerspectiveProjectionMatrix();
        glUniformMatrix4fv(particlesViewLocation_, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(particlesProjectionLocation_, 1, GL_FALSE, glm::value_ptr(projection));
        glUniform1i(particlesTextureLocation_, 0);
        glBindTexture(GL_TEXTURE_2D, particleTexture_);
        glBindVertexArray(particlesVao_);
        // Dessiner l'entrée, pendant que le compute shader écrit l'autre buffer.
        particles_[particleInput_].bindAsArray();
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Particle),
            reinterpret_cast<void*>(offsetof(Particle, position)));
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(Particle),
            reinterpret_cast<void*>(offsetof(Particle, zOrientation)));
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Particle),
            reinterpret_cast<void*>(offsetof(Particle, color)));
        glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Particle),
            reinterpret_cast<void*>(offsetof(Particle, size)));
        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(nParticles_));
        glBindVertexArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(previousArrayBuffer));
        glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTexture));
        glActiveTexture(static_cast<GLenum>(previousActiveTexture));
        glDepthMask(previousDepthMask);
        glBlendFuncSeparate(static_cast<GLenum>(srcRgb), static_cast<GLenum>(dstRgb),
            static_cast<GLenum>(srcAlpha), static_cast<GLenum>(dstAlpha));
        glBlendEquationSeparate(static_cast<GLenum>(equationRgb), static_cast<GLenum>(equationAlpha));
        if (!wasBlending)
            glDisable(GL_BLEND);
    }

    void generateNgon(Vertex vertices[], GLuint elements[], const int nSides)
    {
        const float RADIUS = 0.7f;
        vertices[0].position = vec2(0,0);
        vertices[0].color    = vec4(1.0f, 1.0f, 1.0f, 1.0f);
        for (auto i = 1; i <= nSides; i++) {

            float angle = glm::half_pi<float>() + (i - 1) * (2*glm::pi<float>()/nSides);
            float x = RADIUS * cos(angle);
            float y = RADIUS * sin(angle);
            vertices[i].position = vec2(x,y);
            int mod = i % 3;
            switch (mod) {
                case 1:
                    vertices[i].color = vec4(1.0f, 0.0f, 0.0f, 1.0f);
                    break;
                case 2:
                    vertices[i].color = vec4(0.0f, 1.0f, 0.0f, 1.0f);
                    break;
                default:
                    vertices[i].color = vec4(0.0f, 0.0f, 1.0f, 1.0f);
            }
        }

        for (auto i = 1, j = 0; i <= nSides; i++) {

            elements[j] = 0;
            elements[j+1] = i;
            elements[j+2] = i % nSides + 1;
            j+=3;
        }
    }

    void initShapeData()
    {
        glGenVertexArrays(1, &vao_);
        glBindVertexArray(vao_);

        glGenBuffers(1, &vbo_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices_), nullptr, GL_DYNAMIC_DRAW);

        glGenBuffers(1, &ebo_);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements_), nullptr, GL_DYNAMIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
        glVertexAttribPointer(1,4,GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, color)));
        glEnableVertexAttribArray( 0 );
        glEnableVertexAttribArray( 1 );
        glBindVertexArray(0);
    }

    void sceneShape()
    {
        ImGui::Begin("Scene Parameters");
        ImGui::SliderInt("Sides", &nSide_, MIN_N_SIDES, MAX_N_SIDES);
        ImGui::End();

        bool hasNumberOfSidesChanged = nSide_ != oldNSide_;
        if (hasNumberOfSidesChanged)
        {
            oldNSide_ = nSide_;
            generateNgon(vertices_, elements_, nSide_);
        }

        glUseProgram(basicSP_);
        glBindVertexArray(vao_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
        glBufferSubData(GL_ARRAY_BUFFER, 0, (nSide_ + 1) * sizeof(Vertex), vertices_);
        int count = 3 * nSide_;
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, count * sizeof(GLuint), elements_);
        glDrawElements(GL_TRIANGLES, count , GL_UNSIGNED_INT,0);

    }

    void initGround() {
        groundModel_ = glm::mat4(1.0f);
        groundModel_ = glm::translate(groundModel_,glm::vec3(0.0f, -0.1f, 0.0f));
        groundModel_ = glm::scale(groundModel_,glm::vec3(50.0f, 1.0f, 50.0f));
    }

    void drawGround(glm::mat4& projView)
    {

        glm::mat4 mvp = projView * groundModel_;
        glUseProgram(transformSP_);
        glUniformMatrix4fv(mvpUniformLocation_, 1, GL_FALSE, glm::value_ptr(mvp));
        grass_.draw();
    }

    glm::mat4 getViewMatrix()
    {
        mat4 m(1.0f);
        m = glm::rotate(m, -cameraOrientation_.x, glm::vec3(1.0f, 0.0f, 0.0f));
        m = glm::rotate(m, -cameraOrientation_.y, glm::vec3(0.0f, 1.0f, 0.0f));
        m = glm::translate(m, -cameraPosition_);

        return m;
    }

    glm::mat4 getPerspectiveProjectionMatrix()
    {
        return glm::perspective(glm::radians(70.0f), getWindowAspect(), 0.1f, 300.0f);
    }

    void orientCameraTowardMill()
    {
        orientCameraToward(CAMERA_TARGET);
    }

    void orientCameraToward(const glm::vec3& target)
    {
        const glm::vec3 direction = target - cameraPosition_;
        cameraOrientation_.y = std::atan2(-direction.x, -direction.z);
        cameraOrientation_.x = std::atan2(direction.y,
            std::sqrt(direction.x * direction.x + direction.z * direction.z));
    }

    void initBezier()
    {
        glGenVertexArrays(1, &bezierVao_);
        glBindVertexArray(bezierVao_);
        glGenBuffers(1, &bezierVbo_);
        glBindBuffer(GL_ARRAY_BUFFER, bezierVbo_);
        // Une seule allocation pour la résolution maximale, puis sous-mises à jour.
        glBufferData(GL_ARRAY_BUFFER, sizeof(bezierVertices_), nullptr, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
        updateBezier();
    }

    void updateBezier()
    {
        std::size_t count = 0;
        const int segments = bezierNPoints_ + 1;
        for (const auto& curve : BEZIER_CURVES)
            for (int i = 0; i <= segments; ++i)
                bezierVertices_[count++] = evaluateBezier(curve,
                    static_cast<float>(i) / static_cast<float>(segments));

        glBindBuffer(GL_ARRAY_BUFFER, bezierVbo_);
        glBufferSubData(GL_ARRAY_BUFFER, 0, count * sizeof(glm::vec3), bezierVertices_.data());
    }

    void drawBezier(const glm::mat4& projView)
    {
        glUseProgram(bezierSP_);
        glUniformMatrix4fv(bezierMvpLocation_, 1, GL_FALSE, glm::value_ptr(projView));
        glBindVertexArray(bezierVao_);
        const GLsizei pointsPerCurve = bezierNPoints_ + 2;
        for (std::size_t i = 0; i < BEZIER_CURVES.size(); ++i)
            glDrawArrays(GL_LINE_STRIP, static_cast<GLint>(i) * pointsPerCurve, pointsPerCurve);
        glBindVertexArray(0);
    }

    void sceneModels()
    {
        ImGui::Begin("Scene Parameters");
        if (ImGui::SliderInt("Bezier Number Of Points", &bezierNPoints_, 0, MAX_BEZIER_POINTS))
            updateBezier();
        ImGui::Checkbox("Show Bezier Spline", &showBezier_);
        ImGui::Checkbox("Show Grass", &showGrass_);
        ImGui::Checkbox("Show Flour Particles", &showParticles_);
        if (ImGui::Button("View Flour Particles"))
        {
            isAnimatingCamera_ = false;
            const glm::vec3 emitter = windmill_.flourEmitterPosition();
            const glm::vec3 direction = windmill_.flourEmitterDirection();
            cameraPosition_ = emitter + 0.8f * direction + glm::vec3(0.0f, 1.0f, 0.0f)
                + 2.0f * glm::cross(direction, glm::vec3(0.0f, 1.0f, 0.0f));
            orientCameraToward(emitter);
        }
        if (ImGui::Button("Animate Camera"))
        {
            cameraAnimation_ = 0.0f;
            cameraPosition_ = BEZIER_CURVES.front().p0;
            isAnimatingCamera_ = true;
        }
        ImGui::Text("Camera animation: %.1f / %.1f s", cameraAnimation_, CAMERA_DURATION);
        if (ImGui::Button("Reload Shaders"))
        {
            glDeleteProgram(basicSP_);
            glDeleteProgram(transformSP_);
            glDeleteProgram(bezierSP_);
            glDeleteProgram(grassSP_);
            glDeleteProgram(particlesUpdateSP_);
            glDeleteProgram(particlesDrawSP_);
            loadShaderPrograms();
        }
        ImGui::SliderFloat("Wind Speed", &windmill_.windSpeed, 0.0f, 20.0f, "%.2f m/s");
        ImGui::SliderFloat("Wind Angle", &windmill_.windAngle, -glm::pi<float>(), glm::pi<float>(), "%.2f°");
        ImGui::End();

        if (isAnimatingCamera_)
        {
            cameraAnimation_ = std::min(cameraAnimation_ + deltaTime_, CAMERA_DURATION);
            cameraPosition_ = cameraPath_.position(cameraAnimation_ / CAMERA_DURATION);
            orientCameraTowardMill();
            isAnimatingCamera_ = cameraAnimation_ < CAMERA_DURATION;
        }
        else
            updateCameraInput();
        windmill_.update(deltaTime_);

        glm::mat4 projView = getPerspectiveProjectionMatrix() * getViewMatrix();
        glUseProgram(transformSP_);
        drawGround(projView);
        windmill_.draw(projView);
        if (showGrass_)
            drawGrass(projView);
        if (showBezier_)
            drawBezier(projView);

        updateParticles();
        if (showParticles_)
            drawParticles();
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
        particleInput_ = 1 - particleInput_;
    }

private:
    static constexpr unsigned int MAX_PARTICLES = 64;
    ShaderStorageBuffer particles_[2];
    unsigned int particleInput_ = 0;
    unsigned int nParticles_ = 0;
    float totalTime_ = 0.0f, timerParticles_ = 0.0f;
    bool showParticles_ = true;
    GLuint particlesVao_ = 0, particleTexture_ = 0;
    GLuint particlesUpdateSP_ = 0, particlesDrawSP_ = 0;
    GLint particlesTimeLocation_ = -1, particlesDeltaTimeLocation_ = -1;
    GLint emitterPositionLocation_ = -1, emitterDirectionLocation_ = -1, particleCountLocation_ = -1;
    GLint particlesViewLocation_ = -1, particlesProjectionLocation_ = -1, particlesTextureLocation_ = -1;

    GLuint grassVao_ = 0, grassVbo_ = 0, grassTexture_ = 0, grassSP_ = 0;
    GLint grassMvpLocation_ = -1, grassModelViewLocation_ = -1, grassTextureLocation_ = -1;
    GLsizei grassVertexCount_ = 0;
    bool showGrass_ = true;

    static constexpr int MAX_BEZIER_POINTS = 16;
    static constexpr float CAMERA_DURATION = 15.0f;
    const glm::vec3 CAMERA_TARGET{0.0f, 2.0f, -10.0f};
    BezierPath cameraPath_;
    std::array<glm::vec3, BEZIER_CURVES.size() * (MAX_BEZIER_POINTS + 2)> bezierVertices_{};
    GLuint bezierVao_ = 0, bezierVbo_ = 0, bezierSP_ = 0;
    GLint bezierMvpLocation_ = -1;
    int bezierNPoints_ = 3;
    bool showBezier_ = true;
    bool isAnimatingCamera_ = false;
    float cameraAnimation_ = 0.0f;

    // Shaders
    GLuint basicSP_;
    GLuint transformSP_;
    GLuint mvpUniformLocation_;

    GLuint vbo_, ebo_, vao_;

    static constexpr unsigned int MIN_N_SIDES = 5;
    static constexpr unsigned int MAX_N_SIDES = 12;

    Vertex vertices_[MAX_N_SIDES + 1];
    GLuint elements_[MAX_N_SIDES * 3];

    int nSide_, oldNSide_;

    Model grass_;

    glm::mat4 groundModel_;

    Windmill windmill_;

    glm::vec3 cameraPosition_;
    glm::vec2 cameraOrientation_;

    // Imgui var
    const char* const SCENE_NAMES[2] = {
        "Introduction",
        "3D Model & transformation",
    };
    const int N_SCENE_NAMES = sizeof(SCENE_NAMES) / sizeof(SCENE_NAMES[0]);
    int currentScene_;

    bool isMouseMotionEnabled_;
};


int main(int argc, char* argv[])
{
    WindowSettings settings = {};
    settings.fps = 60;
    settings.context.depthBits = 24;
    settings.context.stencilBits = 8;
    settings.context.antiAliasingLevel = 4;
    settings.context.majorVersion = 4;
    settings.context.minorVersion = 3;
    settings.context.attributeFlags = sf::ContextSettings::Attribute::Core;

    App app;
    app.run(argc, argv, "TP3 - Bezier, herbe et particules", settings);
}
