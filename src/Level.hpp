#pragma once
#include "Tako.hpp"
#include <map>
#include <array>
#include <vector>
#include "Rect.hpp"
#include <functional>

namespace
{
    constexpr auto tilesetTileCount = 15;
}

class Level
{
public:
    Level(const char* file, tako::PixelArtDrawer* drawer, std::map<char, std::function<void(int,int)>>& callbackMap)
    {
        auto bitmap = tako::Bitmap::FromFile("/Tileset.png");
        auto tileset = drawer->CreateTexture(bitmap);
        int tilesPerTilesetRow = bitmap.Width() / 16;
        for (int i = 0; i < tilesetTileCount; i++)
        {
            int y = i / tilesPerTilesetRow;
            int x = i - y * tilesPerTilesetRow;
            m_tileSprites[i] = drawer->CreateSprite(tileset, x * 16, y * 16, 16, 16);
        }

        constexpr size_t bufferSize = 1024 * 1024;
        std::array <tako::U8, bufferSize> buffer;
        size_t bytesRead = 0;
        if (!tako::FileSystem::ReadFile(file, buffer.data(), bufferSize, bytesRead))
        {
            LOG_ERR("Could not read level {}", file);
        }

        auto levelStr = reinterpret_cast<const char *>(buffer.data());
        std::vector<char> tileChars;
        tileChars.reserve(bytesRead);
        {
            int maxX = 0;
            int maxY = 0;
            int x = 0;
            int y = 0;

            for (int i = 0; i < bytesRead; i++) {
                if (levelStr[i] != '\n' && levelStr[i] != '\0') {
                    x++;
                    tileChars.push_back(levelStr[i]);
                } else {
                    maxY++;
                    maxX = std::max(maxX, x);
                    x = 0;
                }
            }

            m_width = maxX;
            m_height = maxY;
        }

        for (int i = 0; i < tileChars.size(); i++)
        {
            int tile = 0;
            switch (tileChars[i])
            {
                case '[':
                    tile = 1;
                    break;
                case '=':
                    tile = 2;
                    break;
                case ']':
                    tile = 3;
                    break;
                case '<':
                    tile = 4;
                    break;
                case '#':
                    tile = 5;
                    break;
                case '>':
                    tile = 6;
                    break;
                case ';':
                    tile = 7;
                    break;
                case '-':
                    tile = 8;
                    break;
                case ':':
                    tile = 9;
                    break;
                case '(':
                    tile = 10;
                    break;
                case '_':
                    tile = 11;
                    break;
                case ')':
                    tile = 12;
                    break;
                case 'G':
                    tile = 14;
                    break;
            }
            if (callbackMap.find(tileChars[i]) != callbackMap.end())
            {
                int y = m_height - i / m_width;
                int x = i % m_width;
                callbackMap[tileChars[i]](x, y);
            }

            m_tiles.push_back(tile);
        }
    }

    void Draw(tako::PixelArtDrawer* drawer)
    {
        for (int y = m_height; y >= 0; y--)
        {
            for (int x = 0; x < m_width; x++)
            {
                int i = (m_height - y) * m_width + x;
                int tile = m_tiles[i];
                if (tile == 0)
                {
                    continue;
                }

                drawer->DrawSprite(x * 16, y * 16 + 16, 16, 16, m_tileSprites[tile - 1]);
            }
        }

    }

    std::optional<Rect> Overlap(Rect rect)
    {
        /*
            TL TM TR
            ML MM MR
            BL BM BR
        */
        int tileX = ((int) rect.x) / 16;
        int tileY = ((int) rect.y) / 16;

        Rect r;
        for (int y = -1; y <= 1; y++)
        {
            for (int x = -1; x <= 1; x++)
            {
                int tX = tileX + x;
                int tY = tileY + y;

                if (tX < 0 || tX > m_width || tY < 0 || tY > m_height)
                {
                    continue;
                }
                int i = (m_height - tY) * m_width + tX;
                int tile = m_tiles[i];
                if (tile == 0)
                {
                    continue;
                }

                r = { tX * 16.0f + 8, tY * 16.0f + 8, 16, 16};
                if (Rect::Overlap(r, rect))
                {
                    return r;
                }
            }
        }

        return std::nullopt;
    }

    Rect MapBounds()
    {
        float width = m_width * 16;
        float height = m_height * 16;
        return
        {
            width / 2,
            height / 2,
            width,
            height
        };
    }
private:
    std::array<tako::Sprite*, tilesetTileCount> m_tileSprites;
    std::vector<int> m_tiles;
    int m_width;
    int m_height;
};