#include "Renderer.h"
#include <SDL/SDL.h>
#include <iostream>
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include "Engine.h"
#include "Entity.h"
#include "Map.h"

LoggerPtr OpenGLRenderer::logger_;

OpenGLRenderer::OpenGLRenderer(const glm::vec2 &resolution) :
    cameraPos_(0.f, 1.f, 0.f)
,   resolution_(resolution)
{
    if (!logger_.get())
        logger_ = Logger::getLogger("OGLRenderer");

    if (!initEngine(resolution_))
    {
        logger_->fatal() << "Unable to initialize graphics resources\n";
        exit(1);
    }
}

OpenGLRenderer::~OpenGLRenderer()
{
}

void OpenGLRenderer::render(Entity *entity)
{
    std::cout << "Renderering entity ID " << entity->getID() << " and type " <<
        entity->getType() << '\n';
}

void OpenGLRenderer::renderMap(Map *map)
{
    const glm::vec2 &mapSize = map->getSize();
    std::cout << "Rendering map of of size " << mapSize.x << ' ' << mapSize.y << '\n';

    renderRectangleColor(glm::rotate(
                glm::scale(glm::mat4(1.f), 0.1f * glm::vec3(mapSize.x, 1.f, mapSize.y)),
                90.f, glm::vec3(1, 0, 0)),
            glm::vec4(0, 1, 0, 1));
}

void OpenGLRenderer::startRender()
{
    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // TODO clamp camera to map
    std::cout << "Camera pos: " << cameraPos_.x << ' ' << cameraPos_.y << ' '
        << cameraPos_.z << '\n';

    // Set up matrices
    float aspectRatio = resolution_.x / resolution_.y;
    float fov = 45.f;
    getProjectionStack().clear();
    getProjectionStack().current() =
        glm::perspective(fov, aspectRatio, 0.1f, 100.f);
    getViewStack().clear();
    getViewStack().current() =
        glm::lookAt(cameraPos_,
                    glm::vec3(cameraPos_.x, 0, cameraPos_.z),
                    glm::vec3(0, 0, 1));
}

void OpenGLRenderer::endRender()
{
    SDL_GL_SwapBuffers();
}

void OpenGLRenderer::updateCamera(const glm::vec3 &delta)
{
    std::cout << "updating camera\n";
    cameraPos_ += delta;
}

