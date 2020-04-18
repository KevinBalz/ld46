#pragma once
#include "Tako.hpp"

struct Position
{
    float x;
    float y;

    tako::Vector2 AsVec()
    {
        return {x, y};
    }
};