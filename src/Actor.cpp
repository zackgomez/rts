#include "Actor.h"
#include "ParamReader.h"
#include "glm.h"
#include "MessageHub.h"
#include "Unit.h"

namespace rts {

LoggerPtr Actor::logger_;

Actor::Actor(const std::string &name, const Json::Value &params,
    bool mobile, bool targetable) :
  Entity(name, params, mobile, targetable)
{
  if (!logger_.get())
    logger_ = Logger::getLogger("Actor");

  health_ = getMaxHealth();

  attack_timer_ = 0.f;
}

Actor::~Actor()
{
}

void Actor::handleMessage(const Message &msg)
{
  if (msg["type"] == MessageTypes::ATTACK)
  {
    assert(msg.isMember("pid"));
    assert(msg.isMember("damage"));

    // TODO(zack) figure out how to deal with this case (friendly fire)
    // when we have from, we can work that in here too
    assert(msg["pid"].asInt64() != getPlayerID());

    // Just take damage for now
    health_ -= msg["damage"].asFloat();
    if (health_ <= 0.f)
      MessageHub::get()->sendRemovalMessage(this);
  }
  else if (msg["type"] == MessageTypes::ORDER)
  {
    handleOrder(msg);
  }
  else
  {
    logger_->warning() << "Actor got unknown message: "
      << msg.toStyledString() << '\n';
  }
}
void Actor::handleOrder(const Message &order)
{
  assert(order["type"] == MessageTypes::ORDER);
  assert(order.isMember("order_type"));
  if (order["order_type"] == OrderTypes::ENQUEUE)
  {
    enqueue(order);
  }
  else
  {
    logger_->warning() << "Actor got unknown order: "
      << order.toStyledString() << '\n';
  }
}

void Actor::enqueue(const Message &queue_order)
{
  assert(queue_order.isMember("prod"));
  std::string prod_name = queue_order["prod"].asString();

  // TODO(zack) assert that prod_name is something this Actor can produce

  Production prod;
  prod.max_time = getParam(prod_name + ".buildTime");
  prod.time = prod.max_time;
  prod.name = prod_name;
  production_queue_.push(prod);
}

void Actor::produce(const std::string &prod_name)
{
  // TODO(zack) generalize this
  Message spawnMsg;
  spawnMsg["type"] = MessageTypes::SPAWN_ENTITY;
  spawnMsg["to"] = toJson(NO_ENTITY);
  spawnMsg["entity_class"] = Unit::TYPE;
  spawnMsg["entity_name"] = prod_name;
  spawnMsg["entity_pid"] = toJson(getPlayerID());
  spawnMsg["entity_pos"] = toJson(pos_ + getDirection());

  MessageHub::get()->sendMessage(spawnMsg);
}

void Actor::update(float dt)
{
  // count down the attack timer
  attack_timer_ = glm::max(0.f, attack_timer_ - dt);

  // Update production
  if (!production_queue_.empty())
  {
    production_queue_.front().time -= dt;
    if (production_queue_.front().time <= 0)
    {
      produce(production_queue_.front().name);
      production_queue_.pop();
    }
  }

  // Move etc
  Entity::update(dt);
}

void Actor::resetAttackTimer()
{
  attack_timer_ = param("cooldown");
}
}; // rts

