#pragma once
#include "Tako.hpp"
#include "World.hpp"
#include "Position.hpp"
#include "Renderer.hpp"
#include "Player.hpp"

class Game
{
public:
    void Setup(tako::PixelArtDrawer* drawer)
    {
        drawer->SetTargetSize(240, 135);
        drawer->AutoScale();

        //Create Player
        {
            auto player = m_world.Create<Position, RectangleRenderer, Player>();
            Position& pos = m_world.GetComponent<Position>(player);
            pos.x = 0;
            pos.y = 0;
            RectangleRenderer& renderer = m_world.GetComponent<RectangleRenderer>(player);
            renderer.size = { 16, 16};
            renderer.color = { 255, 255, 255, 255 };
            m_cameraPos = m_cameraTarget = pos.AsVec();
        }
    }

    void Update(tako::Input* input, float dt)
    {
        m_world.IterateComps<Position, Player>([&](Position& pos, Player& player)
        {
            m_cameraTarget = pos.AsVec();

            if (input->GetKey(tako::Key::Left))
            {
                pos.x -= dt * 16;
            }
            if (input->GetKey(tako::Key::Right))
            {
                pos.x += dt * 16;
            }
            if (input->GetKey(tako::Key::Up))
            {
                pos.y += dt * 16;
            }
        });

        m_cameraPos += (m_cameraTarget - m_cameraPos) * dt * 2;
    }

    void Draw(tako::PixelArtDrawer* drawer)
    {
        drawer->Clear();
        drawer->SetCameraPosition(m_cameraPos);
        m_world.IterateComps<Position, RectangleRenderer>([&](Position& pos, RectangleRenderer& rect)
        {
            drawer->DrawRectangle(pos.x - rect.size.x / 2, pos.y + rect.size.y / 2, rect.size.x, rect.size.y,  rect.color);
        });
    }
private:
    tako::World m_world;
    tako::Vector2 m_cameraPos;
    tako::Vector2 m_cameraTarget;
};