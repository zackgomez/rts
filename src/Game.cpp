#include "Game.h"
#include "Renderer.h"
#include "Map.h"
#include "Player.h"

Game::~Game()
{
    delete map_;
}

void Game::addRenderer(Renderer *renderer)
{
    renderers_.insert(renderer);
    renderer->setGame(this);
}


HostGame::HostGame(Map *map, const std::vector<Player *> &players) :
	Game(map)
,   players_(players)
{
}

HostGame::~HostGame()
{
}

void HostGame::update(float dt)
{
    // Update players
	for (auto &player : players_)
		player->update(dt);
	// TODO read input

    // Update entities

    // Render
    for (auto renderer : renderers_)
    {
        renderer->startRender();
        renderer->renderMap(map_);
        renderer->endRender();
    }
}


/*
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
    localPlayer_->update(dt);

    // Update entities

    // Render
    for (auto renderer : renderers_)
    {
        renderer->startRender();
        renderer->renderMap(map_);
        renderer->endRender();
    }
};
*/

