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
    bool IsGrounded(Level* level, Position& pos, RigidBody& rigid)
    {
        Rect n = {pos.AsVec() + tako::Vector2(0, -0.000009f), rigid.size};
        return level->Overlap(n).has_value();
    }

    void Move(tako::World& world, Level* level, Position& pos, RigidBody& rigid, tako::Vector2 movement)
    {
        Rect n;
        while (tako::mathf::abs(movement.x) > 0.0000001f || tako::mathf::abs(movement.y) > 0.0000001f)
        {
            auto mov = movement;
            if (movement.magnitude() > 1)
            {
                mov.normalize();
            }
            n = {pos.AsVec() + mov, rigid.size};
            auto overlap = level->Overlap(n);
            if (overlap)
            {
                auto other = overlap.value();
                Rect ny(pos.AsVec() + tako::Vector2(0, mov.y), rigid.size);
                if (Rect::Overlap(other, ny))
                {
                    if (movement.y > 0)
                    {
                        pos.y = other.Bottom() - rigid.size.y / 2 - 0.000001f;
                    }
                    else if (movement.y < 0)
                    {
                        pos.y = other.Top() + rigid.size.y / 2 + 0.000001f;
                    }
                    movement.y = 0;
                    continue;
                }
                Rect nx(pos.AsVec() + tako::Vector2(mov.x, 0), rigid.size);
                if (Rect::Overlap(other, nx))
                {
                    if (movement.x > 0)
                    {
                        pos.x = other.Left() - rigid.size.x / 2 - 0.000001f;
                    }
                    else if (movement.x < 0)
                    {
                        pos.x = other.Right() + rigid.size.x / 2 + 0.000001f;
                    }
                    movement.x = 0;
                    continue;
                }
                movement /= 2;
                continue;
            }
            pos.x += mov.x;
            pos.y += mov.y;
            movement -= mov;
        }
    }
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