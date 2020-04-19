#pragma once
#include "Tako.hpp"
#include "World.hpp"
#include <optional>

struct Player
{
    float hunger;
    float displayedHunger;
    float walkingPart;
    float lookDirection;
    std::optional<tako::Entity> turnip;
};