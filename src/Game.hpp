#pragma once
#include "Tako.hpp"
#include "World.hpp"
#include "Position.hpp"
#include "Renderer.hpp"
#include "Player.hpp"
#include "Physics.hpp"

class Game
{
public:
    void Setup(tako::PixelArtDrawer* drawer)
    {
        drawer->SetTargetSize(240, 135);
        drawer->AutoScale();
        {
            auto bitmap = tako::Bitmap::FromFile("/Tileset.png");
            m_tileSet = drawer->CreateTexture(bitmap);
        }
        m_tile = drawer->CreateSprite(m_tileSet, 0, 0, 16, 16);

        //Create Player
        {
            auto player = m_world.Create<Position, RectangleRenderer, RigidBody, Player>();
            Position& pos = m_world.GetComponent<Position>(player);
            pos.x = 0;
            pos.y = 100;
            RectangleRenderer& renderer = m_world.GetComponent<RectangleRenderer>(player);
            renderer.size = { 16, 16};
            renderer.color = { 255, 255, 255, 255 };
            m_cameraPos = m_cameraTarget = pos.AsVec();
            RigidBody& rigid = m_world.GetComponent<RigidBody>(player);
            rigid.size = { 16, 16 };
        }

        // Ground
        for (int i = 0; i < 16; i++)
        {
            auto ground = m_world.Create<Position, SpriteRenderer>();
            Position& pos = m_world.GetComponent<Position>(ground);
            pos.x = i * 16 - 16 * 16 / 2;
            pos.y = 0;
            SpriteRenderer& renderer = m_world.GetComponent<SpriteRenderer>(ground);
            renderer.size = { 16, 16};
            renderer.sprite = m_tile;
        }
    }

    void Update(tako::Input* input, float dt)
    {
        m_world.IterateComps<Position, Player>([&](Position& pos, Player& player)
        {
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

        Physics::Step(m_world, dt);
        m_world.IterateComps<Position, Player>([&](Position& pos, Player& player)
        {
           m_cameraTarget = pos.AsVec();
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
        m_world.IterateComps<Position, SpriteRenderer>([&](Position& pos, SpriteRenderer& sprite)
        {
          drawer->DrawSprite(pos.x - sprite.size.x / 2, pos.y + sprite.size.y / 2, sprite.size.x, sprite.size.y, sprite.sprite);
        });
    }
private:
    tako::World m_world;
    tako::Vector2 m_cameraPos;
    tako::Vector2 m_cameraTarget;
    tako::Texture* m_tileSet;
    tako::Sprite* m_tile;
};