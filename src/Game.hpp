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
struct Background {};
struct Foreground {};

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

struct Turnip
{
    tako::Vector2 speed;
};

struct Enemy
{
    tako::Vector2 speed;
    float groundTime;
};

struct DeadEnemy
{
    tako::Vector2 speed;
    float groundTime;
};

struct Spawner
{
    int x, y;
    float duration;
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
            m_turnip = drawer->CreateSprite(m_turnipUI, 0, 0, 8, 8);
        }
        {
            auto bitmap = tako::Bitmap::FromFile("/Rabbit.png");
            auto tex = drawer->CreateTexture(bitmap);
            m_rabbit = drawer->CreateSprite(tex, 0, 0, 12, 12);
            m_rabbitJump = drawer->CreateSprite(tex, 12, 0, 12, 12);
            m_rabbitDead = drawer->CreateSprite(tex, 24, 0, 12, 12);
        }
        m_clipStep = new tako::AudioClip("/Step.wav");
        m_harvest = new tako::AudioClip("/Harvest.wav");
        m_clipEat = new tako::AudioClip("/Eat.wav");
        m_clipThrow = new tako::AudioClip("/Throw.wav");
        m_clipBroke = new tako::AudioClip("/Broke.wav");
        std::map<char, std::function<void(int,int)>> levelCallbacks
        {{
            { 'p', [&](int x, int y)
            {
                auto plant = m_world.Create<Position, SpriteRenderer, Plant, Foreground>();
                Position& pos = m_world.GetComponent<Position>(plant);
                pos.x = x * 16 + 8;
                pos.y = y * 16 + 8;
                SpriteRenderer& renderer = m_world.GetComponent<SpriteRenderer>(plant);
                renderer.size = { 16, 16};
                Plant& pl = m_world.GetComponent<Plant>(plant);
                pl.Reset();
                pl.growth = 0;
            }},
            { 'P', [&](int x, int y)
            {
                auto bitmap = tako::Bitmap::FromFile("/Player.png");
                auto playerTex = drawer->CreateTexture(bitmap);
                m_player = drawer->CreateSprite(playerTex, 0, 0, 12, 12);
                auto player = m_world.Create<Position, SpriteRenderer, RigidBody, Player, Foreground>();
                Position& pos = m_world.GetComponent<Position>(player);
                pos.x = x * 16 + 8;
                pos.y = y * 16 + 8;
                SpriteRenderer& renderer = m_world.GetComponent<SpriteRenderer>(player);
                renderer.size = { 12, 12};
                renderer.sprite = m_player;
                RigidBody& rigid = m_world.GetComponent<RigidBody>(player);
                rigid.size = { 12, 12 };
                rigid.entity = player;
                Player& pl = m_world.GetComponent<Player>(player);
                pl.hunger = pl.displayedHunger = 100;
                pl.turnip = std::nullopt;
                pl.lookDirection = 1;
                pl.airTime = 0.0f;
                pl.speed = {0, 0};
            }},
            { 'S', [&](int x, int y)
            {
                auto spawn = m_world.Create<Spawner>();
                auto& sp = m_world.GetComponent<Spawner>(spawn);
                sp.x = x;
                sp.y = y;
                sp.duration = 0;
            }}
        }};
        m_level = new Level("/Level.txt", drawer, levelCallbacks);

        //Create Carrot
        {
            auto bitmap = tako::Bitmap::FromFile("/Carrot.png");
            auto carTex = drawer->CreateTexture(bitmap);
            auto m_carrot = drawer->CreateSprite(carTex, 0, 0, 16, 32);
            auto carrot = m_world.Create<Position, SpriteRenderer, Background>();
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
            player.displayedHunger = std::max(0.0f, player.displayedHunger - dt * 2);
            player.displayedHunger += (player.hunger - player.displayedHunger) * dt * 3;
            bool grounded = Physics::IsGrounded(m_level, pos, rigid);
            player.speed.y = std::max(grounded ? 0 : -160.0f, player.speed.y - dt * 200);
            player.airTime = grounded ? 0 : player.airTime + dt;
            float moveX = 0;
            if (input->GetKey(tako::Key::Left))
            {
                moveX -= speed;
            }
            if (input->GetKey(tako::Key::Right))
            {
                moveX += speed;
            }
            constexpr auto acceleration = 0.2f;
            player.speed.x = moveX = acceleration * moveX + (1 - acceleration) * player.speed.x;
            if (tako::mathf::abs(moveX) > 1)
            {
                player.lookDirection = tako::mathf::sign(moveX);
                if (grounded)
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
            if (input->GetKey(tako::Key::Up) && player.airTime < 0.3f)
            {
                player.speed.y = 80;
            }
            float moveY = grounded ? std::max(0.0f, player.speed.y) : player.speed.y;
            bool hadTurnip = player.turnip.has_value();
            bool throwPressed = input->GetKeyDown(tako::Key::Space);
            bool eatPressed = input->GetKeyDown(tako::Key::Down);
            if (!hadTurnip && (throwPressed || eatPressed))
            {
                Plant* pickup = nullptr;
                float minDistance = 999999999;
                tako::Vector2 pickupPos;
                Rect p(pos.AsVec(), rigid.size);
                m_world.IterateHandle<Position, Plant>([&](tako::EntityHandle handle)
                {
                    auto& pos = m_world.GetComponent<Position>(handle.id);
                    auto& plant = m_world.GetComponent<Plant>(handle.id);
                    if (plant.growth < 10)
                    {
                        return;
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
                });
                if (pickup)
                {
                    pickup->Reset();
                    tako::Audio::Play(*m_harvest);
                    SpawnParticles({pickupPos.x, pickupPos.y - 3}, 5, -15, 15, 5, 40);
                    auto turnip = m_world.Create<Position, SpriteRenderer, Foreground>();
                    auto& tPos = m_world.GetComponent<Position>(turnip);
                    tPos.x = pickupPos.x;
                    tPos.y = pickupPos.y;
                    auto& tRen = m_world.GetComponent<SpriteRenderer>(turnip);
                    tRen.size = {8, 8};
                    tRen.sprite = m_turnip;
                    player.turnip = turnip;
                }
            }
            if (hadTurnip && throwPressed)
            {
                auto turnip = player.turnip.value();
                m_world.AddComponent<RigidBody>(turnip);
                auto& tBody = m_world.GetComponent<RigidBody>(turnip);
                tBody.size = { 8, 8 };
                tBody.entity = turnip;
                m_world.AddComponent<Turnip>(turnip);
                auto& tTur = m_world.GetComponent<Turnip>(turnip);
                tTur.speed = { 130 * player.lookDirection, 20 };
                tako::Audio::Play(*m_clipThrow);
                player.turnip = std::nullopt;
            }
            if (hadTurnip && eatPressed)
            {
                auto turnip = player.turnip.value();
                player.hunger = std::min(100.0f, player.hunger + 20);
                SpawnParticles(m_world.GetComponent<Position>(turnip).AsVec(), 5, -10, 10, 5, 10);
                toRemove.push_back(turnip);
                tako::Audio::Play(*m_clipEat);
                player.turnip = std::nullopt;
            }

            Physics::Move(m_world, m_level, pos, rigid, tako::Vector2(moveX, moveY) * dt);
            if (player.turnip.has_value())
            {
                auto turnip = player.turnip.value();
                auto& tPos = m_world.GetComponent<Position>(turnip);
                auto tVec = tPos.AsVec();
                auto target = pos.AsVec() + tako::Vector2(0, 4 + 8.0f/2);
                tVec += (target - tVec) * std::min(1.0f, dt * 30);
                tPos.x = tVec.x;
                tPos.y = tVec.y;
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

        m_world.IterateComps<Position, Turnip, RigidBody>([&](Position& position, Turnip& turnip, RigidBody& rigid)
        {
            turnip.speed.y += dt * -30;
            bool deleted = false;
            std::optional<tako::Entity> killed;
            Physics::Move(m_world, m_level, position, rigid, turnip.speed * dt,
                [&]()
                {
                    if (!deleted)
                    {
                        toRemove.push_back(rigid.entity);
                        tako::Audio::Play(*m_clipBroke);
                        deleted = true;
                    }
                },
                [&](auto& otherRigid, auto& movement)
                {
                    if (!killed && m_world.HasComponent<Enemy>(otherRigid.entity))
                    {
                        if (!deleted)
                        {
                            toRemove.push_back(rigid.entity);
                            deleted = true;
                        }

                        tako::Audio::Play(*m_clipBroke);
                        killed = otherRigid.entity;
                    }
                }
            );
            if (deleted)
            {
                SpawnParticles(position.AsVec(), 8, turnip.speed.x * 0.5f, turnip.speed.x * 2, turnip.speed.y * 0.5f, turnip.speed.y * 2);
            }
            if (killed)
            {
                auto toKill = killed.value();
                auto speed = m_world.GetComponent<Enemy>(toKill).speed;
                m_world.RemoveComponent<Enemy>(toKill);
                m_world.RemoveComponent<RigidBody>(toKill);
                m_world.GetComponent<SpriteRenderer>(toKill).sprite = m_rabbitDead;
                m_world.AddComponent<DeadEnemy>(toKill);
                m_world.AddComponent<Background>(toKill);
                m_world.RemoveComponent<Foreground>(toKill);
                auto& dead = m_world.GetComponent<DeadEnemy>(toKill);
                dead.speed = speed;
                dead.groundTime = 0;
            }
        });

        m_world.IterateComps<Position, RigidBody, Enemy, SpriteRenderer>([&](Position& position, RigidBody& rigid, Enemy& enemy, SpriteRenderer& sprite)
        {
            auto grounded = Physics::IsGrounded(m_level, position, rigid);
            if (grounded)
            {
                sprite.sprite = m_rabbit;
                if (enemy.groundTime == 0)
                {
                    SpawnParticles(position.AsVec() - tako::Vector2(0.0f, rigid.size.y / 2), 5, -7, 7, 40, 60);
                }
                enemy.groundTime += dt;
                enemy.speed = { 0, 0 };
                if (enemy.groundTime > 2)
                {
                    Physics::Move(m_world, m_level, position, rigid, {0, 0.5f });
                    enemy.speed = { 30, rand() % 20 + 20.0f };
                    auto speedSign = -tako::mathf::sign(enemy.speed.x);
                    enemy.groundTime = 0;
                    SpawnParticles(position.AsVec() - tako::Vector2(0.0f, rigid.size.y / 2), 5, 5 * speedSign, 10 * speedSign, 10, 30);
                }
            }
            else
            {
                sprite.sprite = m_rabbitJump;
                enemy.groundTime = 0;
                enemy.speed.y -= dt * 20;
            }

            Physics::Move(m_world, m_level, position, rigid, enemy.speed * dt);
        });

        m_world.IterateComps<Position, DeadEnemy>([&](Position& pos, DeadEnemy& enm)
        {
            auto target = pos.AsVec() + enm.speed * dt;
            enm.speed.y -= dt * 80;
            Rect tRect(target, {12, 12});
            if (m_level->Overlap(tRect))
            {
                enm.speed /= -4;
            }
            else
            {
                pos.x = target.x;
                pos.y = target.y;
            }
        });
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
        m_world.IterateComps<Spawner>([&](Spawner& spawn)
        {
            spawn.duration -= dt;
            if (spawn.duration <= 0)
            {
                SpawnRabbit(spawn.x, spawn.y);
                spawn.duration = rand() * 1.0f / RAND_MAX * 2 + 4;
            }
        });
        m_world.IterateComps<Position, Player>([&](Position& pos, Player& player)
        {
           m_cameraTarget = FitMapBound(m_level->MapBounds(), pos.AsVec(), m_cameraSize);
        });
        m_cameraPos += (m_cameraTarget - m_cameraPos) * dt * 2;
        m_cameraPos = FitMapBound(m_level->MapBounds(), m_cameraPos, m_cameraSize);
    }

    void SpawnRabbit(int x, int y)
    {
        auto enemy = m_world.Create<Position, SpriteRenderer, RigidBody, Enemy, Foreground>();
        auto& pos = m_world.GetComponent<Position>(enemy);
        pos.x = x * 16 + 8;
        pos.y = y * 16 + 8;
        auto& renderer = m_world.GetComponent<SpriteRenderer>(enemy);
        renderer.size = { 12, 12};
        renderer.sprite = m_rabbit;
        auto& rigid = m_world.GetComponent<RigidBody>(enemy);
        rigid.size = { 12, 12 };
        rigid.entity = enemy;
        auto& en = m_world.GetComponent<Enemy>(enemy);
        en.speed = {0, 0};
        en.groundTime = 0;
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
        m_world.IterateComps<Position, SpriteRenderer, Background>([&](Position& pos, SpriteRenderer& sprite, Background& b)
        {
            drawer->DrawSprite(pos.x - sprite.size.x / 2, pos.y + sprite.size.y / 2, sprite.size.x, sprite.size.y, sprite.sprite);
        });
        m_world.IterateComps<Position, SpriteRenderer, Foreground>([&](Position& pos, SpriteRenderer& sprite, Foreground& f)
        {
            drawer->DrawSprite(pos.x - sprite.size.x / 2, pos.y + sprite.size.y / 2, sprite.size.x, sprite.size.y, sprite.sprite);
        });
        drawer->SetCameraPosition(m_cameraSize/2);
        float playerHunger = 0;
        for (auto [player] : m_world.Iter<Player>())
        {
            playerHunger = player.displayedHunger;
            break;
        }
        drawer->DrawImage(4, m_cameraSize.y - 4, 8, 8, m_turnipUI);
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
    tako::Sprite* m_turnip;
    tako::Sprite* m_player;
    tako::Sprite* m_rabbit;
    tako::Sprite* m_rabbitJump;
    tako::Sprite* m_rabbitDead;
    tako::AudioClip* m_clipStep;
    tako::AudioClip* m_clipEat;
    tako::AudioClip* m_clipThrow;
    tako::AudioClip* m_clipBroke;
    tako::AudioClip* m_harvest;
    std::array<tako::Sprite*, 3> m_plantStates;
    Level* m_level;
};