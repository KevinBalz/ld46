#pragma once
#include "Tako.hpp"
#include "World.hpp"
#include "Position.hpp"
#include "Renderer.hpp"
#include "Player.hpp"
#include "Physics.hpp"
#include "Level.hpp"
#include <array>
#include <time.h>
#include <stdlib.h>
#include <algorithm>

tako::Vector2 FitMapBound(Rect bounds, tako::Vector2 cameraPos, tako::Vector2 camSize)
{
    cameraPos.x = std::max(bounds.Left() + camSize.x / 2, cameraPos.x);
    cameraPos.x = std::min(bounds.Right() - camSize.x / 2, cameraPos.x);
    cameraPos.y = std::min(bounds.Top() - camSize.y / 2, cameraPos.y);
    cameraPos.y = std::max(bounds.Bottom() + camSize.y / 2, cameraPos.y);

    return cameraPos;
}

struct Plant
{
    float growth;
    float growthRate;

    void Reset()
    {
        growth = 0;
        growthRate = rand() % 100 / 50.0f + 0.8f;
    }
};

class Game
{
public:
    void Setup(tako::PixelArtDrawer* drawer)
    {
        srand(time(NULL));
        drawer->SetTargetSize(240, 135);
        drawer->AutoScale();
        m_cameraSize = drawer->GetCameraViewSize();

        {
            auto bitmap = tako::Bitmap::FromFile("/Plant.png");
            auto plantTex = drawer->CreateTexture(bitmap);
            for (int i = 0; i < m_plantStates.size(); i++)
            {
                m_plantStates[i] = drawer->CreateSprite(plantTex, i * 16, 0, 16, 16);
            }
        }
        {
            auto bitmap = tako::Bitmap::FromFile("/TurnipUI.png");
            m_turnipUI = drawer->CreateTexture(bitmap);
        }
        std::map<char, std::function<void(int,int)>> levelCallbacks
        {{
            { 'p', [&](int x, int y)
            {

                auto plant = m_world.Create<Position, SpriteRenderer, Plant>();
                Position& pos = m_world.GetComponent<Position>(plant);
                pos.x = x * 16 + 8;
                pos.y = y * 16 + 8;
                SpriteRenderer& renderer = m_world.GetComponent<SpriteRenderer>(plant);
                renderer.size = { 16, 16};
                Plant& pl = m_world.GetComponent<Plant>(plant);
                pl.Reset();
                pl.growth = 0;
            }}
        }};
        m_level = new Level("/Level.txt", drawer, levelCallbacks);

        //Create Player
        {
            auto bitmap = tako::Bitmap::FromFile("/Player.png");
            auto playerTex = drawer->CreateTexture(bitmap);
            m_player = drawer->CreateSprite(playerTex, 0, 0, 12, 12);
            auto player = m_world.Create<Position, SpriteRenderer, RigidBody, Player>();
            Position& pos = m_world.GetComponent<Position>(player);
            pos.x = 0;
            pos.y = 100;
            SpriteRenderer& renderer = m_world.GetComponent<SpriteRenderer>(player);
            renderer.size = { 12, 12};
            renderer.sprite = m_player;
            RigidBody& rigid = m_world.GetComponent<RigidBody>(player);
            rigid.size = { 12, 12 };
            Player& pl = m_world.GetComponent<Player>(player);
            pl.hunger = 100;
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
        m_world.IterateComps<Position, Player, RigidBody>([&](Position& pos, Player& player, RigidBody& rigid)
        {
            constexpr auto speed = 64;
            player.hunger = std::max(0.0f, player.hunger - dt * 2);
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
            if (input->GetKeyDown(tako::Key::Space))
            {
                Plant* pickup = nullptr;
                float minDistance = 999999999;
                Rect p(pos.AsVec(), rigid.size);
                for (auto [pos, plant] : m_world.Iter<Position, Plant>())
                {
                    if (plant.growth < 10)
                    {
                        continue;
                    }
                    Rect pl(pos.AsVec(), { 16, 16});
                    if (Rect::Overlap(p, pl))
                    {
                        float distance = tako::mathf::abs((p.Position()-pl.Position()).magnitude());
                        if (distance < minDistance)
                        {
                            pickup = &plant;
                            minDistance = distance;
                        }
                    }
                }
                if (pickup)
                {
                    pickup->Reset();
                    player.hunger = std::min(100.0f, player.hunger + 10);
                }
            }
        });

        m_world.IterateComps<Plant, SpriteRenderer>([&](Plant& plant, SpriteRenderer& sprite)
        {
            plant.growth += dt * plant.growthRate;
            if (plant.growth > 10)
            {
                sprite.sprite = m_plantStates[2];
            }
            else if (plant.growth > 5)
            {
                sprite.sprite = m_plantStates[1];
            }
            else
            {
                sprite.sprite = m_plantStates[0];
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
        drawer->SetCameraPosition(m_cameraSize/2);
        float playerHunger = 0;
        for (auto [player] : m_world.Iter<Player>())
        {
            playerHunger = player.hunger;
            break;
        }
        drawer->DrawImage(4, m_cameraSize.y - 4, 8, 8, m_turnipUI);
        //drawer->DrawRectangle(4, m_cameraSize.y - 4, 8, 8, {255, 255, 255, 255});
        drawer->DrawRectangle(4 + 10, m_cameraSize.y - 4, 32, 8, {255, 255, 255, 255});
        drawer->DrawRectangle(5 + 10, m_cameraSize.y - 5, 30, 6, {0, 0, 0, 255});
        drawer->DrawRectangle(6 + 10, m_cameraSize.y - 6, 28 * playerHunger / 100, 4, {255, 255, 255, 255});
    }
private:
    tako::World m_world;
    tako::Vector2 m_cameraPos;
    tako::Vector2 m_cameraTarget;
    tako::Vector2 m_cameraSize;
    tako::Sprite* m_carrot;
    tako::Texture* m_turnipUI;
    tako::Sprite* m_player;
    std::array<tako::Sprite*, 3> m_plantStates;
    Level* m_level;
};