#include "Game.h"
#include "Renderer.h"
#include "Map.h"

Game::~Game()
{
    delete map_;
}

void Game::addRenderer(Renderer *renderer)
{
    renderers_.insert(renderer);
}



LocalGame::LocalGame(Map *map, Player *player) :
    Game(map)
,   localPlayer_(player)
{
}

LocalGame::~LocalGame()
{
}

void LocalGame::update(float dt)
{
    // Read input

    // Update entities

    // Render
    for (auto renderer : renderers_)
    {
        renderer->startRender();
        renderer->renderMap(map_);
        renderer->endRender();
    }
};
