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
    logger_ = Logger::getLogger("HostGame");

    MessageHub::get()->setGame(this);

    // TODO generalize this
    for (int i = 0; i < 10; i++) {
    	Unit *u = new Unit(1);
    	entities_[u->getID()] = u;
    }
}

HostGame::~HostGame()
{
    MessageHub::get()->setGame(NULL);
}

void HostGame::update(float dt)
{
    // Update players
    for (auto &player : players_)
    {
        player->update(dt);

        PlayerAction action;
        for (;;)
        {
            action = player->getAction();
            if (action["type"] == ActionTypes::NONE)
                break;
            handleAction(player->getPlayerID(), action);
        }
    }

    // Update entities
    std::vector<eid_t> deadEnts;
    for (auto &it : entities_)
    {
        Entity *entity = it.second;
        entity->update(dt);
        // Check for removal
        if (entity->needsRemoval())
            deadEnts.push_back(entity->getID());
    }
    // TODO remove deadEnts

    // Render
    for (auto &renderer : renderers_)
    {
        renderer->startRender(dt);

        renderer->renderMap(map_);

        for (auto &it : entities_)
            renderer->render(it.second);

        renderer->endRender();
    }
}

void HostGame::sendMessage(eid_t to, const Message &msg)
{
    auto it = entities_.find(to);
    if (it == entities_.end())
    {
        logger_->warning() << "Tried to send message to unknown entity:\n" 
            << msg.toStyledString() << '\n';
        return;
    }

    it->second->handleMessage(msg);
}

void HostGame::handleAction(int64_t playerID, const PlayerAction &action)
{
    std::cout << "[" << playerID
        << "] Read action " << action.toStyledString() << '\n';

    if (action["type"] == ActionTypes::MOVE)
    {
        // Generate a message to target entity with move order
        Message msg;
        msg["to"] = action["entity"];
        msg["type"] = MessageTypes::ORDER;
        msg["order_type"] = OrderTypes::MOVE;
        msg["target"] = action["target"];

        MessageHub::get()->sendMessage(msg);
    }
    else if (action["type"] == ActionTypes::ATTACK)
    {
    	// Generate a message to target entity with move order
    	Message msg;
    	msg["to"] = action["entity"];
    	msg["type"] = MessageTypes::ORDER;
    	msg["order_type"] = OrderTypes::ATTACK;
    	msg["target"] = action["target"];
    	msg["enemy_id"] = action["enemy_id"];

    	MessageHub::get()->sendMessage(msg);
    }
    else
    {
        logger_->warning()
            << "Unknown action type " << action["type"].asString() << '\n';
    }
}

