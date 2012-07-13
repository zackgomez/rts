#include "Unit.h"
#include "ParamReader.h"
#include "math.h"
#include "MessageHub.h"

LoggerPtr Unit::logger_;

Unit::Unit(int64_t playerID, const glm::vec3 &pos, const std::string &name) :
    Mobile(playerID, pos),
    Actor(name),
    state_(new NullState(this))
{
    pos_ = pos;
    radius_ = 0.5f;
    angle_ = 0.f;
    speed_ = 0.f;
    turnSpeed_ = 0.f;

    if (!logger_.get())
        logger_ = Logger::getLogger("Unit");

    name_ = name;
}

void Unit::handleMessage(const Message &msg)
{
    if (msg["type"] == MessageTypes::ORDER)
    {
        handleOrder(msg);
    }
    else if (msg["type"] == MessageTypes::ATTACK)
    {
        assert(msg.isMember("pid"));
        assert(msg.isMember("damage"));

        // TODO(zack) figure out how to deal with this case
        assert(msg["pid"].asInt64() != playerID_);

        // Just take damage for now
        health_ -= msg["damage"].asFloat();
    }
    else
    {
        logger_->warning() << "Unit got unknown message: "
            << msg.toStyledString() << '\n';
    }
}

void Unit::handleOrder(const Message &order)
{
    assert(order["type"] == MessageTypes::ORDER);
    assert(order.isMember("order_type"));
    if (order["order_type"] == OrderTypes::MOVE)
    {
        state_->stop();
        delete state_;
        state_ = new MoveState(toVec3(order["target"]), this);
    }
    else if (order["type"] == MessageTypes::ORDER &&
            order["order_type"] == OrderTypes::ATTACK)
    {
        // TODO(zack) change state, don't just shoot...
        Message spawnMsg;
        spawnMsg["type"] = MessageTypes::SPAWN_ENTITY;
        spawnMsg["to"] = (Json::Value::UInt64) NO_ENTITY; // Send to game object
        spawnMsg["entity_type"] = "PROJECTILE"; // TODO(zack) make constant (also in Game.cpp)
        spawnMsg["entity_pid"] = (Json::Value::Int64) playerID_;
        spawnMsg["entity_pos"] = toJson(pos_);
        spawnMsg["projectile_target"] = order["enemy_id"];
        spawnMsg["projectile_name"] = "projectile"; // TODO(zack) make a param

        MessageHub::get()->sendMessage(spawnMsg);
    }
    else if (order["type"] == MessageTypes::ORDER &&
            order["order_type"] == OrderTypes::STOP)
    {
        state_->stop();
        delete state_;
        state_ = new NullState(this);
    }
    else
    {
        logger_->warning() << "Unit got unknown order: " << order << '\n';
    }
}

void Unit::update(float dt)
{
    state_->update(dt);

    UnitState *next;
    if ((next = state_->next()))
    {
        delete state_;
        state_ = next;
    }

    Mobile::update(dt);
}

bool Unit::needsRemoval() const
{
    return health_ <= 0.f;
}

void NullState::update(float dt)
{
    unit_->speed_ = 0.f;
    unit_->turnSpeed_ = 0.f;
}

MoveState::MoveState(const glm::vec3 &target, Unit *unit) :
    UnitState(unit),
    target_(target)
{
    // Make sure the unit stands on the terrain
    target_.y += unit_->getRadius(); 
}

void MoveState::update(float dt)
{
    // Calculate some useful values
    float dist = glm::length(target_ - unit_->pos_);
    float desired_angle = unit_->angleToTarget(target_);
    float delAngle = desired_angle - unit_->angle_;
    float speed = getParam("unit.speed");
    float turnRate = getParam("unit.turnRate");
    // rotate
    // only rotate when not close enough
	if (dist > speed * dt)
    {
        // Get delta in [-180, 180]
        while (delAngle > 180.f) delAngle -= 360.f;
        while (delAngle < -180.f) delAngle += 360.f;
        // Would overshoot, just move directly there
        if (fabs(delAngle) < turnRate * dt)
            unit_->turnSpeed_ = delAngle / dt;
        else
            unit_->turnSpeed_ = glm::sign(delAngle) * turnRate;
    }
    // move
    // Set speed careful not to overshoot
    if (dist < speed * dt)
        speed = dist / dt;
    unit_->speed_ = speed;
}

void MoveState::stop()
{
    unit_->speed_ = 0.f;
    unit_->turnSpeed_ = 0.f;
}

UnitState * MoveState::next()
{
    glm::vec3 diff = target_ - unit_->pos_;
    float dist = glm::length(diff);

    if (dist < unit_->getRadius() / 2.f)
        return new NullState(unit_);
    return NULL;
}

/*
AttackState::AttackState(Entity *target, Unit *unit) :
    UnitState(unit),
    target_(target)
{
}

void AttackState::update(float dt)
{
    if (unit_->attack_timer_ <= 0)
    {
        if (glm::distance(target_->getPosition(), unit_->pos_ <= getParam("unit.attackRange")))
        {
            unit_->attack_timer_ = getParam("unit.attackCooldown");
            printf("Attacking target %d\n", target_->getID());
            //TODO(connor): Spawn projectile, fire it at target_
        }
    }
}
*/
