#include "Renderer.h"
#include <SDL/SDL.h>
#include <iostream>
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Engine.h"
#include "Entity.h"
#include "Map.h"
#include "Game.h"
#include "Unit.h"

LoggerPtr OpenGLRenderer::logger_;

OpenGLRenderer::OpenGLRenderer(const glm::vec2 &resolution)
:   cameraPos_(0.f, 5.f, 0.f)
,   resolution_(resolution)
,   selection_(NO_ENTITY)
{
    if (!logger_.get())
        logger_ = Logger::getLogger("OGLRenderer");

    if (!initEngine(resolution_))
    {
        logger_->fatal() << "Unable to initialize graphics resources\n";
        exit(1);
    }

	// Load resources
	mapProgram_ = loadProgram("shaders/map.v.glsl", "shaders/map.f.glsl");
    unitProgram_ = loadProgram("shaders/unit.v.glsl", "shaders/unit.f.glsl"); 
    unitMesh_ = loadMesh("models/soldier.obj");
    projectileMesh_ = loadMesh("models/projectile.obj");
    // unit model is based at 0, height 1, translate to center of model
    glm::mat4 unitMeshTrans =
        glm::scale(
            glm::translate(glm::mat4(1.f), glm::vec3(0, -0.5f, 0)),
            glm::vec3(1, 0.5f, 1));
    setMeshTransform(unitMesh_, unitMeshTrans);
}

OpenGLRenderer::~OpenGLRenderer()
{
}

void OpenGLRenderer::renderEntity(const Entity *entity)
{
    glm::vec3 pos = entity->getPosition(simdt_);
    float rotAngle = entity->getAngle(simdt_);
    const std::string &name = entity->getName();

    // Interpolate if they move
    glm::mat4 transform = glm::scale(
            glm::rotate(
                glm::translate(glm::mat4(1.f), pos),
                // TODO(zack) why does rotAngle need to be negative here?
                // I think openGL may use a "left handed" coordinate system...
                -rotAngle, glm::vec3(0, 1, 0)),
            glm::vec3(entity->getRadius() / 0.5f));

    if (name == "unit")
    {
        const Unit *unit = (const Unit *) entity;
        // if selected draw as green
        glm::vec4 color = entity->getID() == selection_ ?  glm::vec4(0, 1, 0, 1) : glm::vec4(0, 0, 1, 1);

        glUseProgram(unitProgram_);
        GLuint colorUniform = glGetUniformLocation(mapProgram_, "color");
        GLuint lightPosUniform = glGetUniformLocation(mapProgram_, "lightPos");
        glUniform4fv(colorUniform, 1, glm::value_ptr(color));
        glUniform3fv(lightPosUniform, 1, glm::value_ptr(lightPos_));
        renderMesh(transform, unitMesh_);

        glm::vec4 ndc = getProjectionStack().current() * getViewStack().current() *
            transform * glm::vec4(0, 0, 0, 1);
        ndc /= ndc.w;
        ndcCoords_[entity] = glm::vec3(ndc);
        
        // display health bar
        float health = unit->getHealth();
        float maxHealth = unit->getMaxHealth();
        // Put itentity matrix on the transformation matrix stacks
        // so health bars will not shift in game coordinates or
        // in camera coordinates
        getProjectionStack().push();
        getProjectionStack().current() = glm::mat4(1.f);
        getViewStack().push();
        getViewStack().current() = glm::mat4(1.f);
        // Red underneath for max health
        glm::mat4 maxHealthTransform =
            glm::scale(
                    glm::translate(glm::mat4(1.f), glm::vec3(ndc.x, ndc.y + 0.12f, -0.9f)),
                    glm::vec3(0.1f,0.01f,0.01f));
        renderRectangleColor(maxHealthTransform, glm::vec4(1,0,0,1));
        // Green on top for current health
        glm::mat4 healthTransform =
            glm::scale(
                    glm::translate(glm::mat4(1.f), glm::vec3(ndc.x, ndc.y + 0.12f, -1.f)),
                    glm::vec3(health/maxHealth,1.f,1.f) * glm::vec3(0.1f,0.01f,0.01f));
        renderRectangleColor(healthTransform, glm::vec4(0,1,0,1));
        // Restore matrices
        getProjectionStack().pop();
        getViewStack().pop();
    }
    else if (name == "projectile")
    {
    	glm::vec4 color = glm::vec4(0.5, 0.7, 0.5, 1);
    	glUseProgram(unitProgram_);
    	GLuint colorUniform = glGetUniformLocation(mapProgram_, "color");
    	GLuint lightPosUniform = glGetUniformLocation(mapProgram_, "lightPos");
    	glUniform4fv(colorUniform, 1, glm::value_ptr(color));
    	glUniform3fv(lightPosUniform, 1, glm::value_ptr(lightPos_));
    	renderMesh(transform, projectileMesh_);
    }
    else
    {
        logger_->warning() << "Unable to render entity " << entity->getName() << '\n';
    }

    // TODO(zack) use these to fix unit shader lighting
    //glm::vec3 modelPos = applyMatrix(
        //getViewStack().current() * transform,
        //glm::vec3(0.f));
    //logger_->info() << "modelpos: " << modelPos << " lightpos: " << lightPos_ << '\n';
    //logger_->info() << "delta: " << lightPos_ - modelPos << '\n';
}

void OpenGLRenderer::renderMap(const Map *map)
{
    const glm::vec2 &mapSize = map->getSize();

	const glm::vec4 mapColor(0.25, 0.2, 0.15, 1.0);

	glUseProgram(mapProgram_);
    GLuint colorUniform = glGetUniformLocation(mapProgram_, "color");
    GLuint mapSizeUniform = glGetUniformLocation(mapProgram_, "mapSize");
    glUniform4fv(colorUniform, 1, glm::value_ptr(mapColor));
    glUniform2fv(mapSizeUniform, 1, glm::value_ptr(mapSize));

	glm::mat4 transform = glm::rotate(
                glm::scale(glm::mat4(1.f), glm::vec3(mapSize.x, 1.f, mapSize.y)),
                90.f, glm::vec3(1, 0, 0));
    renderRectangleProgram(transform);

    // Render each of the highlights
    for (auto& hl : highlights_)
    {
        hl.remaining -= renderdt_;
        glm::mat4 transform =
            glm::scale(
                    glm::rotate(
                        glm::translate(glm::mat4(1.f),
                            glm::vec3(hl.pos.x, 0.01f, hl.pos.y)),
                        90.f, glm::vec3(1, 0, 0)),
                    glm::vec3(0.33f));
        renderRectangleColor(transform, glm::vec4(1, 0, 0, 1));
    }
    // Remove done highlights
    for (size_t i = 0; i < highlights_.size(); )
    {
        if (highlights_[i].remaining <= 0.f)
        {
            std::swap(highlights_[i], highlights_[highlights_.size() - 1]);
            highlights_.pop_back();
        }
        else
            i++;
    }

}

void OpenGLRenderer::startRender(float dt)
{
    simdt_ = dt;
    renderdt_ = SDL_GetTicks() - lastTick_;

    if (game_->isPaused())
    {
        simdt_ = renderdt_ = 0.f;
        //logger_->info() << "Rendering paused frame @ tick " << tick << '\n';
    }

    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    // Set up matrices
    float aspectRatio = resolution_.x / resolution_.y;
    float fov = 90.f;
    getProjectionStack().clear();
    getProjectionStack().current() =
        glm::perspective(fov, aspectRatio, 0.1f, 100.f);
    getViewStack().clear();
    getViewStack().current() =
        glm::lookAt(cameraPos_,
                    glm::vec3(cameraPos_.x, 0, cameraPos_.z - 0.5f),
                    glm::vec3(0, 0, -1));

    // Set up lights
    lightPos_ = applyMatrix(getViewStack().current(), glm::vec3(-5, 10, -5));

    // Clear coordinates
    ndcCoords_.clear();
    lastTick_ = SDL_GetTicks();
}

void OpenGLRenderer::endRender()
{
    SDL_GL_SwapBuffers();
}

void OpenGLRenderer::updateCamera(const glm::vec3 &delta)
{
    cameraPos_ += delta;

    glm::vec2 mapSize = game_->getMap()->getSize() / 2.f;
    cameraPos_ = glm::clamp(
            cameraPos_,
            glm::vec3(-mapSize.x, 0.f, -mapSize.y),
            glm::vec3(mapSize.x, 20.f, mapSize.y));
}

eid_t OpenGLRenderer::selectEntity (const glm::vec2 &screenCoord) const
{
    glm::vec2 pos = glm::vec2(screenToNDC(screenCoord));

    // TODO(zack) Make this find the BEST instead of the first
    for (auto& pair : ndcCoords_)
    {
        glm::vec2 diff = pos - glm::vec2(pair.second);
        // TODO(zack) compute this based on size of object
        float thresh = 0.02f;
        if (glm::dot(diff, diff) < thresh)
            return pair.first->getID();
    }

    return NO_ENTITY;
}

void OpenGLRenderer::setSelection(eid_t eid)
{
    selection_ = eid;
}

void OpenGLRenderer::highlight(const glm::vec2 &mapCoord)
{
    MapHighlight hl;
    hl.pos = mapCoord;
    hl.remaining = 0.5f;
    highlights_.push_back(hl);
}

glm::vec3 OpenGLRenderer::screenToTerrain(const glm::vec2 &screenCoord) const
{
    glm::vec4 ndc = glm::vec4(screenToNDC(screenCoord), 1.f);

    ndc = glm::inverse(getProjectionStack().current() * getViewStack().current()) * ndc;
    ndc /= ndc.w;

    glm::vec3 dir = glm::normalize(glm::vec3(ndc) - cameraPos_);

    glm::vec3 terrain = glm::vec3(ndc) - dir * ndc.y / dir.y;

    const glm::vec2 mapSize = game_->getMap()->getSize() / 2.f;
    // Don't return a non terrain coordinate
    if (terrain.x < -mapSize.x || terrain.x > mapSize.x
            || terrain.z < -mapSize.y || terrain.z > mapSize.y)
        return glm::vec3(HUGE_VAL);

    return glm::vec3(terrain);
}

glm::vec3 OpenGLRenderer::screenToNDC(const glm::vec2 &screen) const
{
    float z;
    glReadPixels(screen.x, resolution_.y - screen.y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &z);
    return glm::vec3(
            2.f * (screen.x / resolution_.x) - 1.f,
            2.f * (1.f - (screen.y / resolution_.y)) - 1.f,
            z);
}

