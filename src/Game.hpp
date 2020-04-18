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

struct Temporary
{
    float left;
};

struct Particle
{
    tako::Vector2 speed;
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
        m_clipStep = new tako::AudioClip("/Step.wav");
        m_harvest = new tako::AudioClip("/Harvest.wav");
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

    void SpawnParticles(tako::Vector2 origin, int amount, float minX, float maxX, float minY, float maxY)
    {
        for (int i = 0; i < amount; i++)
        {
            auto particle = m_world.Create<Position, RectangleRenderer, Temporary, Particle>();
            auto& pPos = m_world.GetComponent<Position>(particle);
            pPos.x = origin.x;
            pPos.y = origin.y;
            auto& pRen = m_world.GetComponent<RectangleRenderer>(particle);
            pRen.size = { 1, 1 };
            pRen.color = { 255, 255, 255, 255};
            auto& pTmp = m_world.GetComponent<Temporary>(particle);
            pTmp.left = 30 + rand() % 100 / 10.0f;
            auto& pPar = m_world.GetComponent<Particle>(particle);
            pPar.speed = tako::Vector2(((float)rand()) / RAND_MAX * (maxX - minX) + minX, ((float)rand()) / RAND_MAX * (maxY - minY) + minY);
        }
    }

    void Update(tako::Input* input, float dt)
    {
        static std::vector<tako::Entity> toRemove;
        for (auto ent : toRemove)
        {
            m_world.Delete(ent);
        }
        toRemove.clear();
        m_world.IterateHandle<Temporary>([&](tako::EntityHandle& handle)
        {
            Temporary& tmp = m_world.GetComponent<Temporary>(handle.id);
            tmp.left -= dt;
            if (tmp.left < 0)
            {
                toRemove.push_back(handle.id);
            }
        });
        for (auto ent : toRemove)
        {
            m_world.Delete(ent);
        }
        toRemove.clear();
        m_world.IterateComps<Position, Player, RigidBody>([&](Position& pos, Player& player, RigidBody& rigid)
        {
            constexpr auto speed = 64;
            player.hunger = std::max(0.0f, player.hunger - dt * 2);
            float moveX = 0;
            if (input->GetKey(tako::Key::Left))
            {
                moveX -= dt * speed;
            }
            if (input->GetKey(tako::Key::Right))
            {
                moveX += dt * speed;
            }
            pos.x += moveX;
            if (tako::mathf::abs(moveX) > 0)
            {
                static float spawnInterval = 0.6f;
                player.walkingPart += dt;
                if (tako::mathf::abs(player.walkingPart) > spawnInterval)
                {
                    tako::Audio::Play(*m_clipStep);
                    auto sign = tako::mathf::sign(moveX);
                    SpawnParticles(pos.AsVec() - tako::Vector2(0, 5), 1, -5 * sign, -10 * sign, 10, 20);
                    player.walkingPart = 0;
                    spawnInterval = rand() * 1.0f / RAND_MAX * 0.4f + 0.4f;
                }
                else if (player.walkingPart < 0)
                {
                    player.walkingPart = 0;
                }
            }
            else
            {
                player.walkingPart = std::min(0.0f, player.walkingPart - dt / 2);
            }
            if (input->GetKey(tako::Key::Up))
            {
                pos.y += dt * speed;
            }
            if (input->GetKeyDown(tako::Key::Space))
            {
                Plant* pickup = nullptr;
                float minDistance = 999999999;
                tako::Vector2 pickupPos;
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
                            pickupPos = pl.Position();
                        }
                    }
                }
                if (pickup)
                {
                    pickup->Reset();
                    tako::Audio::Play(*m_harvest);
                    SpawnParticles({pickupPos.x, pickupPos.y - 3}, 5, -15, 15, 5, 40);
                    player.hunger = std::min(100.0f, player.hunger + 10);
                    SpawnParticles(pos.AsVec(), 5, -10, 10, 5, 10);
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
        m_world.IterateComps<Position, Particle>([&](Position& pos, Particle& part)
        {
            auto target = pos.AsVec() + part.speed * dt;
            part.speed.y -= dt * 50;
            Rect tRect(target, {1, 1});
            if (m_level->Overlap(tRect))
            {
                part.speed /= -4;
            }
            else
            {
                pos.x = target.x;
                pos.y = target.y;
            }
        });
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
    tako::AudioClip* m_clipStep;
    tako::AudioClip* m_harvest;
    std::array<tako::Sprite*, 3> m_plantStates;
    Level* m_level;
};