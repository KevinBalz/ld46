#pragma once
#include "Tako.hpp"
#include "Position.hpp"
#include "World.hpp"
#include "Level.hpp"
#include <algorithm>

struct RigidBody
{
    tako::Vector2 size;
};

namespace Physics
{
    void Step(tako::World& world, Level* level, float dt)
    {
        world.IterateComps<Position, RigidBody>([&](Position& pos, RigidBody& rigid)
        {
            Rect n(pos.x, pos.y, rigid.size.x, rigid.size.y);
            n.y -= dt * 8;
            if (level->Overlap(n))
            {
                return;
            }
            pos.x = n.x;
            pos.y = n.y;
            //pos.y = std::max(rigid.size.y/2, pos.y);
        });
    }
}