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

LoggerPtr OpenGLRenderer::logger_;

OpenGLRenderer::OpenGLRenderer(const glm::vec2 &resolution) :
    cameraPos_(0.f, 4.f, 0.f)
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
}

OpenGLRenderer::~OpenGLRenderer()
{
}

void OpenGLRenderer::render(const Entity *entity)
{
    const glm::vec3 &pos = entity->getPosition();
    float rotAngle = 180.f / M_PI * acosf(glm::dot(glm::vec2(0, 1), entity->getDirection()));

    glm::mat4 transform = glm::scale(
            glm::rotate(
                glm::translate(glm::mat4(1.f), pos),
                rotAngle, glm::vec3(0, 1, 0)),
            glm::vec3(entity->getRadius() / 0.5f));

    transform = glm::rotate(transform, 90.f, glm::vec3(1, 0, 0));

    logger_->debug() << "Selection: " << selection_ << '\n';
    // if selected draw as green
    glm::vec4 color = entity->getID() == selection_ ?  glm::vec4(0, 1, 0, 1) : glm::vec4(0, 0, 1, 1);
    renderRectangleColor(transform, color);

    glm::vec4 ndc = getProjectionStack().current() * getViewStack().current() *
        transform * glm::vec4(0, 0, 0, 1);
    ndc /= ndc.w;


    ndcCoords_[entity] = glm::vec3(ndc);
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
}

void OpenGLRenderer::startRender()
{
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
                    glm::vec3(cameraPos_.x, 0, cameraPos_.z + 0.5f),
                    glm::vec3(0, 0, 1));

    // Clear coordinates
    ndcCoords_.clear();
}

void OpenGLRenderer::endRender()
{
    SDL_GL_SwapBuffers();
}

void OpenGLRenderer::updateCamera(const glm::vec3 &delta)
{
    // TODO why does this need to be negative?
    cameraPos_ += -delta;

    glm::vec2 mapSize = game_->getMap()->getSize() / 2.f;
    cameraPos_ = glm::clamp(
            cameraPos_,
            glm::vec3(-mapSize.x, 0.f, -mapSize.y),
            glm::vec3(mapSize.x, 100.f, mapSize.y));
}

uint64_t OpenGLRenderer::selectEntity (const glm::vec2 &screenCoord) const
{
    glm::vec2 pos = screenCoord * 2.f - glm::vec2(1.f);

    // TODO Make this find the BEST instead of the first
    for (auto& pair : ndcCoords_)
    {
        glm::vec2 diff = pos - glm::vec2(pair.second);
        // TODO compute this based on size of object
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

glm::vec3
OpenGLRenderer::screenToTerrain (const glm::vec2 &screenCoord) const
{
    return glm::vec3(HUGE_VAL);
}