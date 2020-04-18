#pragma once
#include "Tako.hpp"
#include "World.hpp"
#include "Position.hpp"
#include "Renderer.hpp"

class Game
{
public:
    void Setup(tako::PixelArtDrawer* drawer)
    {
        drawer->SetTargetSize(240, 135);
        drawer->AutoScale();

        //Create Player
        {
            auto player = m_world.Create<Position, RectangleRenderer>();
            Position& pos = m_world.GetComponent<Position>(player);
            pos.x = 0;
            pos.y = 0;
            RectangleRenderer& renderer = m_world.GetComponent<RectangleRenderer>(player);
            renderer.size = { 16, 16};
            renderer.color = { 255, 255, 255, 255 };
        }
    }

    void Update(tako::Input* input, float dt)
    {

    }

    void Draw(tako::PixelArtDrawer* drawer)
    {
        m_world.IterateComps<Position, RectangleRenderer>([&](Position& pos, RectangleRenderer& rect)
        {
            drawer->DrawRectangle(pos.x - rect.size.x / 2, pos.y - rect.size.y / 2, rect.size.x, rect.size.y,  rect.color);
        });
    }
private:
    tako::World m_world;
    tako::Vector2 m_cameraPos;
};