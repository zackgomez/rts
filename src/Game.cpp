#include "Game.h"
#include "Renderer.h"
#include "Map.h"
#include "Player.h"
#include "Entity.h"
#include "Unit.h"

Game::Game(Map *map, const std::vector<Player *> &players) :
    map_(map),
    players_(players),
    tick_(0),
    tickOffset_(1)
{
    logger_ = Logger::getLogger("Game");

    MessageHub::get()->setGame(this);

    for (auto player : players)
        player->setGame(this);

    // TODO generalize this
    for (int i = 0; i < 2; i++) {
        glm::vec3 pos(-0.5f + i, 0.5f, 0.f);
    	Unit *u = new Unit(i + 1, pos);
    	entities_[u->getID()] = u;
    }
}

Game::~Game()
{
    MessageHub::get()->setGame(NULL);
    delete map_;
}

void Game::addRenderer(Renderer *renderer)
{
    renderers_.insert(renderer);
    renderer->setGame(this);
}

void Game::update(float dt)
{
    // lock
    std::unique_lock<std::mutex> lock(mutex_);
    // First update players
    for (auto &player : players_)
        player->update(tick_);

    // TODO verify that all players are ready to go
    // It MUST be true that all players have added exactly one action of type
    // none targetting this frame

    // Lock actions, after this point, all actions will go to next tick
    std::unique_lock<std::mutex> actionLock(actionMutex_);
    // Next tick

    // Do actions
    for (auto &player : players_)
    {
        int64_t pid = player->getPlayerID();
        std::queue<PlayerAction> &pacts = actions_[pid];

        PlayerAction act;
        for (;;)
        {
            assert(!pacts.empty());
            act = pacts.front(); pacts.pop();
            assert(act["tick"] == (Json::Value::UInt64) tick_);

            if (act["type"] == ActionTypes::NONE)
                break;

            handleAction(pid, act);
        }
    }
    // Allow more actions
    actionLock.unlock();

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
    // TODO unlock automatically when lock goes out of scope
    
    // Next tick
    tick_++;
}

void Game::render(float dt)
{
    // Render
    for (auto &renderer : renderers_)
        renderer->startRender(dt);

    // lock
    std::unique_lock<std::mutex> lock(mutex_);

    for (auto &renderer : renderers_)
    renderer->renderMap(map_);

    for (auto &it : entities_)
        for (auto &renderer : renderers_)
            renderer->renderEntity(it.second);

    // unlock
    lock.unlock();

    for (auto &renderer : renderers_)
        renderer->endRender();
}

void Game::sendMessage(eid_t to, const Message &msg)
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

void Game::addAction(int64_t pid, const PlayerAction &act)
{
    assert(act.isMember("tick"));
    assert(act.isMember("type"));
    assert(getPlayer(pid));

    std::unique_lock<std::mutex> lock(actionMutex_);
    actions_[pid].push(act);
    lock.unlock();
    // TODO share action with other players
}

const Entity * Game::getEntity(eid_t eid) const
{
    auto it = entities_.find(eid);
    return it == entities_.end() ? NULL : it->second;
}

const Player * Game::getPlayer(int64_t pid) const
{
    for (auto player : players_)
        if (player->getPlayerID() == pid)
            return player;

    return NULL;
}

void Game::handleAction(int64_t playerID, const PlayerAction &action)
{
    std::cout << "[" << playerID
        << "] Read action " << action.toStyledString() << '\n';

    // TODO include player ID in messages
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
    	// Generate a message to target entity with attack order
    	Message msg;
    	msg["to"] = action["entity"];
    	msg["type"] = MessageTypes::ORDER;
    	msg["order_type"] = OrderTypes::ATTACK;
    	msg["target"] = action["target"];
    	msg["enemy_id"] = action["enemy_id"];

    	MessageHub::get()->sendMessage(msg);
    }
    else if (action["type"] == ActionTypes::STOP)
    {
        Message msg;
    	msg["to"] = action["entity"];
    	msg["type"] = MessageTypes::ORDER;
    	msg["order_type"] = OrderTypes::STOP;

    	MessageHub::get()->sendMessage(msg);
    }
    else
    {
        logger_->warning()
            << "Unknown action type from player " << playerID << " : " << action["type"].asString() << '\n';
    }
}

