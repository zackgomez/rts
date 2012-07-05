#pragma once
#include <glm/glm.hpp>
#include "Logger.h"

class Map;
class Entity;
class LocalPlayer;

class Renderer
{
public:
    virtual ~Renderer() { }

    virtual void render(Entity *entity) = 0;
    virtual void renderMap(Map *map) = 0;

    virtual void startRender() = 0;
    virtual void endRender() = 0;
};

class OpenGLRenderer : public Renderer
{
public:
    OpenGLRenderer(const glm::vec2 &resolution);
    ~OpenGLRenderer();

    virtual void render(Entity *entity);
    virtual void renderMap(Map *map);

    virtual void startRender();
    virtual void endRender();

    const glm::vec2& getResolution() const { return resolution_; }
    const glm::vec3& getCameraPos() const { return cameraPos_; }
    void updateCamera(const glm::vec3 &delta);

private:
    glm::vec3 cameraPos_;
    glm::vec2 resolution_;

    static LoggerPtr logger_;
};

