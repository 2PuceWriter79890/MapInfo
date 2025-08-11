#pragma once

#include "ll/api/mod/NativeMod.h"

namespace map_info {

class MapInfoMod {
public:
    static MapInfoMod& getInstance();

    [[nodiscard]] ll::mod::NativeMod& getSelf() const { return mSelf; }

    bool load();
    bool enable();
    bool disable();

private:
    MapInfoMod() : mSelf(*ll::mod::NativeMod::current()) {}

    ll::mod::NativeMod& mSelf;

    void registerCommand();
};

} // namespace map_info