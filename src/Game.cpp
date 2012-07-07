#include "Game.h"
#include "Renderer.h"
#include "Map.h"
#include "Player.h"
#include "Entity.h"
#include "Unit.h"

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
    entities_.push_back(new Unit(1));
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
    std::vector<Entity *> deadEnts;
    for (auto &entity : entities_)
    {
        entity->update(dt);
        // Check for removal
        if (entity->needsRemoval())
            deadEnts.push_back(entity);
    }
    // TODO remove entities to remove

    // Render
    for (auto &renderer : renderers_)
    {
        renderer->startRender();

        renderer->renderMap(map_);

        for (auto &entity : entities_)
            renderer->render(entity);

        renderer->endRender();
    }
}

