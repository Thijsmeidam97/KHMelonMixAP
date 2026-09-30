#include "PluginKingdomHeartsDaysMarluxia.h"
#include "APConnector/Addr.h"

namespace Plugins {

u32 PluginKingdomHeartsDaysMarluxia::euGamecode = 1346849625;

PluginKingdomHeartsDaysMarluxia::PluginKingdomHeartsDaysMarluxia(u32 gameCode)
    : PluginKingdomHeartsDays(gameCode) {
    mm = std::make_unique<APConnector>(
        [this](const std::string& message) {
            if (postMessageToOsd != nullptr)
                postMessageToOsd(message);
        });
}

void PluginKingdomHeartsDaysMarluxia::loadConfigs(
    std::function<bool(std::string)> getBool,
    std::function<int(std::string)> getInt,
    std::function<std::string(std::string)> getStr
) {
    PluginKingdomHeartsDays::loadConfigs(getBool, getInt, getStr);

    marlux = getBool(
        tomlUniqueIdentifier() + ".EnableMarluxiaMissionPlayer"
    );

    mmOn = !getBool(
        tomlUniqueIdentifier() + ".DisableMelonMixConnector"
    );
    if (mm != nullptr)
        mm->setEnabled(mmOn);
}

void PluginKingdomHeartsDaysMarluxia::onLoadROM() {
    PluginKingdomHeartsDays::onLoadROM();
    if (mm != nullptr) {
        mm->setNds(nds);
        mm->loadRom();
        mm->setEnabled(mmOn);
    }
}

void PluginKingdomHeartsDaysMarluxia::onLoadState() {
    PluginKingdomHeartsDays::onLoadState();
    if (mm != nullptr)
        mm->loadSt();
}

void PluginKingdomHeartsDaysMarluxia::setMarlux() {
    if (marlux && nds != nullptr) {
        if (nds->ARM9Read32(SESSIONUSED) != 0) {
            if (nds->ARM9Read32(SESSIONKIND) != MARLUXIAKIND)
                nds->ARM9Write32(SESSIONKIND, MARLUXIAKIND);
            if (nds->ARM9Read8(MISSIONKIND) != MARLUXIAKIND)
                nds->ARM9Write8(MISSIONKIND, MARLUXIAKIND);
        }
    }
}

bool PluginKingdomHeartsDaysMarluxia::shouldRenderFrame() {
    if (mm != nullptr)
        mm->pollFrame();
    setMarlux();
    return PluginKingdomHeartsDays::shouldRenderFrame();
}

}
