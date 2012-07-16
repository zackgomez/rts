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
        state_->stop();
        delete state_;
        state_ = new AttackState(order["enemy_id"].asUInt64(), this);
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
    Actor::update(dt);
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
    targetID_(NO_ENTITY),
    target_(target)
{
    // Make sure the unit stands on the terrain
    target_.y += unit_->getRadius(); 
}

MoveState::MoveState(eid_t targetID, Unit *unit) :
    UnitState(unit),
    targetID_(targetID),
    target_(0.f)
{
}

void MoveState::updateTarget()
{
    if (targetID_ != NO_ENTITY)
    {
        const Entity *e = MessageHub::get()->getEntity(targetID_);
        if (!e)
            targetID_ = 0;
        else
            target_ = e->getPosition();
    }
}

void MoveState::update(float dt)
{
    // Calculate some useful values
    updateTarget();
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

    // If we've reached destination point
    if (targetID_ == NO_ENTITY && dist < unit_->getRadius() / 2.f)
        return new NullState(unit_);

    // Or target entity has died
    if (targetID_ != NO_ENTITY
            && MessageHub::get()->getEntity(targetID_) == NULL)
        return new NullState(unit_);

    // Keep on movin'
    return NULL;
}


AttackState::AttackState(eid_t target_id, Unit *unit) :
    UnitState(unit),
    target_id_(target_id),
    moveState_(new MoveState(target_id, unit))
{
}

AttackState::~AttackState()
{
    delete moveState_;
}

void AttackState::update(float dt)
{
    const Entity *target = MessageHub::get()->getEntity(target_id_);
    if (!target)
        return;

    glm::vec3 targetPos = target->getPosition();
    float targetAngle = unit_->angleToTarget(targetPos); 
    // difference between facing and target
    float arc = addAngles(targetAngle, -unit_->angle_);
    float dist = glm::distance(unit_->pos_, targetPos);

    unit_->logger_->debug() << "arc: " << arc << " dist: " << dist << '\n';
    // If we're in range, don't move, and shoot if possible
    if (dist < unit_->attack_range_ && fabs(arc) < unit_->attack_arc_ / 2.f)
    {
        if (unit_->getAttackTimer() <= 0)
        {
            // TODO(zack): this probably belongs in like actor or at least a
            // helper function
            unit_->setAttackTimer(getParam("unit.cooldown"));
            printf("Attacking target %d\n", target_id_);
            Message spawnMsg;
            spawnMsg["type"] = MessageTypes::SPAWN_ENTITY;
            spawnMsg["to"] = (Json::Value::UInt64) NO_ENTITY; // Send to game object
            spawnMsg["entity_type"] = "PROJECTILE"; // TODO(zack) make constant (also in Game.cpp)
            spawnMsg["entity_pid"] = (Json::Value::Int64) unit_->getPlayerID();
            spawnMsg["entity_pos"] = toJson(unit_->getPosition(dt));
            spawnMsg["projectile_target"] = (Json::Value::UInt64) target_id_;
            spawnMsg["projectile_name"] = "projectile"; // TODO(zack) make a param

            MessageHub::get()->sendMessage(spawnMsg);
        }

        unit_->speed_ = 0.f;
        unit_->turnSpeed_ = 0.f;
    }
    // Otherwise move towards target using move state logic
    else
        moveState_->update(dt);
}

UnitState * AttackState::next()
{
    // We're done pursuing when the target is DEAD
    // TODO(zack): incorporate a sight radius
    const Entity *target = MessageHub::get()->getEntity(target_id_);
    if (!target)
        return new NullState(unit_);

    return NULL;
}

void AttackState::stop()
{
    moveState_->stop();
}
