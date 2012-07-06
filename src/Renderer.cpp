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
    cameraPos_(0.f, 5.f, 0.f)
,   resolution_(resolution)
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

void OpenGLRenderer::render(Entity *entity)
{
    std::cout << "Renderering entity ID " << entity->getID() << " and type " <<
        entity->getType() << '\n';
}

void OpenGLRenderer::renderMap(Map *map)
{
    const glm::vec2 &mapSize = map->getSize();

	const glm::vec4 mapColor(0.25, 0.2, 0.15, 1.0);

	std::cout << "map size: " << mapSize.x << ' ' << mapSize.y << '\n';

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

    // Set up matrices
    float aspectRatio = resolution_.x / resolution_.y;
    float fov = 90.f;
    getProjectionStack().clear();
    getProjectionStack().current() =
        glm::perspective(fov, aspectRatio, 0.1f, 100.f);
    getViewStack().clear();
    getViewStack().current() =
        glm::lookAt(cameraPos_,
                    glm::vec3(cameraPos_.x, 0, cameraPos_.z),
                    glm::vec3(0, 0, -1));
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
            glm::vec3(mapSize.x, 100.f, mapSize.y));
}

