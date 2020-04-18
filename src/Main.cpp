#include "Tako.hpp"

void tako::Setup(tako::PixelArtDrawer* drawer)
{
    drawer->SetTargetSize(240, 135);
    drawer->AutoScale();
}

void tako::Update(tako::Input* input, float dt)
{

}

void tako::Draw(tako::PixelArtDrawer* drawer)
{
    drawer->DrawRectangle(0, 0, 16, 16, {255, 255, 255, 255});
}