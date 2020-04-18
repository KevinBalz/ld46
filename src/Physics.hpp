#pragma once
#include "Tako.hpp"
#include "Position.hpp"
#include "World.hpp"

struct RigidBody
{
    tako::Vector2 size;
};

namespace Physics
{
    void Step(tako::World& world, float dt)
    {
        world.IterateComps<Position, RigidBody>([&](Position& pos, RigidBody& rigid)
        {
            pos.y -= dt * 2;
        });
    }
}