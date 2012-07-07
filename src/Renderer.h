#pragma once
#include <glm/glm.hpp>
#include "Logger.h"
#include <GL/glew.h>
#include <map>

class Map;
class Entity;
class LocalPlayer;
class Game;

class Renderer
{
public:
    Renderer() : game_(NULL) { }
    virtual ~Renderer() { }

    void setGame(const Game *game) { game_ = game; }

    virtual void render(const Entity *entity) = 0;
    virtual void renderMap(const Map *map) = 0;

    virtual void startRender() = 0;
    virtual void endRender() = 0;

protected:
    const Game *game_;
};

class OpenGLRenderer : public Renderer
{
public:
    OpenGLRenderer(const glm::vec2 &resolution);
    ~OpenGLRenderer();

    virtual void render(const Entity *entity);
    virtual void renderMap(const Map *map);

    virtual void startRender();
    virtual void endRender();

    const glm::vec2& getResolution() const { return resolution_; }
    const glm::vec3& getCameraPos() const { return cameraPos_; }
    void updateCamera(const glm::vec3 &delta);

private:
    glm::vec3 cameraPos_;
    glm::vec2 resolution_;

	GLuint mapProgram_;

    std::map<const Entity *, glm::vec3> ndcCoords_;

    static LoggerPtr logger_;
};

