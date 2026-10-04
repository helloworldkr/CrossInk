#pragma once

#include <GfxRenderer.h>

namespace wallpaper {

bool isAutoShuffleEnabled();
void setAutoShuffleEnabled(bool enable);
bool drawAsleep(GfxRenderer& renderer);

}  // namespace wallpaper
