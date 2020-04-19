#pragma once
#include "Tako.hpp"
#include "World.hpp"
#include <optional>

struct Player
{
    tako::Vector2 speed;
    float hunger;
    float displayedHunger;
    float walkingPart;
    float stepPart;
    bool stepIndex;
    float lookDirection;
    float airTime;
    std::optional<tako::Entity> turnip;
};