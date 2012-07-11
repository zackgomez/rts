#pragma once
#include <glm/glm.hpp>
#include <GL/glew.h>
#include <map>
#include "Logger.h"
#include "Entity.h"
#include "Engine.h"

class Map;
class LocalPlayer;
class Game;
struct MapHighlight;

class Renderer
{
public:
    Renderer() : game_(NULL) { }
    virtual ~Renderer() { }

    void setGame(const Game *game) { game_ = game; }

    virtual void renderEntity(const Entity *entity) = 0;
    virtual void renderMap(const Map *map) = 0;

    virtual void startRender(float dt) = 0;
    virtual void endRender() = 0;

protected:
    const Game *game_;
};

class OpenGLRenderer : public Renderer
{
public:
    OpenGLRenderer(const glm::vec2 &resolution);
    ~OpenGLRenderer();

    virtual void renderEntity(const Entity *entity);
    virtual void renderMap(const Map *map);

    virtual void startRender(float dt);
    virtual void endRender();

    const glm::vec2& getResolution() const { return resolution_; }
    const glm::vec3& getCameraPos() const { return cameraPos_; }
    void updateCamera(const glm::vec3 &delta);
    void updateCameraVel(const glm::vec3 &vel);

    // returns 0 if no acceptable entity near click
    uint64_t selectEntity (const glm::vec2 &screenCoord) const;
    void setSelection(eid_t eid);

    // Returns the terrain location at the given screen coord.  If the coord
    // is not on the map returns glm::vec3(HUGE_VAL).
    glm::vec3 screenToTerrain (const glm::vec2 &screenCoord) const;

    void highlight(const glm::vec2 &mapCoord);

private:
    glm::vec3 screenToNDC(const glm::vec2 &screenCoord) const;

    glm::vec3 cameraPos_;
    glm::vec3 cameraVel_;
    glm::vec3 lightPos_;
    glm::vec2 resolution_;
    // Used to interpolate, last tick seen, and dt since last tick
    uint64_t lastTick_;
    float simdt_;
    // For updating purely render aspects
    float renderdt_;


    GLuint mapProgram_;
    GLuint unitProgram_;
    Mesh * unitMesh_;

    std::map<const Entity *, glm::vec3> ndcCoords_;
    eid_t selection_;

    std::vector<MapHighlight> highlights_;


    static LoggerPtr logger_;
};


struct MapHighlight
{
    glm::vec2 pos;
    float remaining;
};

