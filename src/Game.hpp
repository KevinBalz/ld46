#pragma once
#include "Tako.hpp"
#include "World.hpp"
#include "Position.hpp"
#include "Renderer.hpp"
#include "Player.hpp"
#include "Physics.hpp"
#include "Level.hpp"

tako::Vector2 FitMapBound(Rect bounds, tako::Vector2 cameraPos, tako::Vector2 camSize)
{
    cameraPos.x = std::max(bounds.Left() + camSize.x / 2, cameraPos.x);
    cameraPos.x = std::min(bounds.Right() - camSize.x / 2, cameraPos.x);
    cameraPos.y = std::min(bounds.Top() - camSize.y / 2, cameraPos.y);
    cameraPos.y = std::max(bounds.Bottom() + camSize.y / 2, cameraPos.y);

    return cameraPos;
}

class Game
{
public:
    void Setup(tako::PixelArtDrawer* drawer)
    {
        drawer->SetTargetSize(240, 135);
        drawer->AutoScale();
        m_level = new Level("/Level.txt", drawer);
        m_cameraSize = drawer->GetCameraViewSize();

        //Create Player
        {
            auto player = m_world.Create<Position, RectangleRenderer, RigidBody, Player>();
            Position& pos = m_world.GetComponent<Position>(player);
            pos.x = 0;
            pos.y = 100;
            RectangleRenderer& renderer = m_world.GetComponent<RectangleRenderer>(player);
            renderer.size = { 16, 16};
            renderer.color = { 255, 255, 255, 255 };
            RigidBody& rigid = m_world.GetComponent<RigidBody>(player);
            rigid.size = { 16, 16 };
        }

        //Create Carrot
        {
            auto bitmap = tako::Bitmap::FromFile("/Carrot.png");
            auto carTex = drawer->CreateTexture(bitmap);
            auto m_carrot = drawer->CreateSprite(carTex, 0, 0, 16, 32);
            auto carrot = m_world.Create<Position, SpriteRenderer>();
            Position& pos = m_world.GetComponent<Position>(carrot);
            pos.x = 16 * 5 + 8;
            pos.y = 3 * 16;
            SpriteRenderer& renderer = m_world.GetComponent<SpriteRenderer>(carrot);
            renderer.size = { 16, 32};
            renderer.sprite = m_carrot;
        }
    }

    void Update(tako::Input* input, float dt)
    {
        m_world.IterateComps<Position, Player>([&](Position& pos, Player& player)
        {
            constexpr auto speed = 64;
            if (input->GetKey(tako::Key::Left))
            {
                pos.x -= dt * speed;
            }
            if (input->GetKey(tako::Key::Right))
            {
                pos.x += dt * speed;
            }
            if (input->GetKey(tako::Key::Up))
            {
                pos.y += dt * speed;
            }
        });

        Physics::Step(m_world, m_level, dt);
        m_world.IterateComps<Position, Player>([&](Position& pos, Player& player)
        {
           m_cameraTarget = FitMapBound(m_level->MapBounds(), pos.AsVec(), m_cameraSize);
        });
        m_cameraPos += (m_cameraTarget - m_cameraPos) * dt * 2;
        m_cameraPos = FitMapBound(m_level->MapBounds(), m_cameraPos, m_cameraSize);
    }

    void Draw(tako::PixelArtDrawer* drawer)
    {
        m_cameraSize = drawer->GetCameraViewSize();
        drawer->Clear();
        drawer->SetCameraPosition(m_cameraSize/2);
        drawer->DrawRectangle(5, 5, 32, 8, {255, 255, 255, 255});
        drawer->SetCameraPosition(m_cameraPos);
        m_level->Draw(drawer);
        m_world.IterateComps<Position, RectangleRenderer>([&](Position& pos, RectangleRenderer& rect)
        {
            drawer->DrawRectangle(pos.x - rect.size.x / 2, pos.y + rect.size.y / 2, rect.size.x, rect.size.y,  rect.color);
        });
        m_world.IterateComps<Position, SpriteRenderer>([&](Position& pos, SpriteRenderer& sprite)
        {
          drawer->DrawSprite(pos.x - sprite.size.x / 2, pos.y + sprite.size.y / 2, sprite.size.x, sprite.size.y, sprite.sprite);
        });
    }
private:
    tako::World m_world;
    tako::Vector2 m_cameraPos;
    tako::Vector2 m_cameraTarget;
    tako::Vector2 m_cameraSize;
    tako::Sprite* m_carrot;
    Level* m_level;
};