#include "EntityFactory.h"
#include "Unit.h"
#include "Projectile.h"
#include "Building.h"

EntityFactory::EntityFactory()
{
    logger_ = Logger::getLogger("EntityFactory");
}

EntityFactory::~EntityFactory()
{
}

EntityFactory * EntityFactory::get()
{
    static EntityFactory instance;
    return &instance;
}

Entity * EntityFactory::construct(const std::string &cl,
        const std::string &name, const Json::Value &params)
{
    // TODO(zack) use map
    if (cl == Unit::TYPE)
        return new Unit(name, params);
    else if (cl == Projectile::TYPE)
        return new Projectile(name, params);
    else if (cl == Building::TYPE)
        return new Building(name, params);
    else
    {
        logger_->warning() << "Trying to spawn unknown class " << cl <<
            " named " << name << " params: " << params << '\n';
        return NULL;
    }
}

