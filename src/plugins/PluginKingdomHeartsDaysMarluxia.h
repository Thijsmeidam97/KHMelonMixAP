#ifndef KHDAYS_MARLUXIA_PLUGIN_H
#define KHDAYS_MARLUXIA_PLUGIN_H

#include "PluginKingdomHeartsDays.h"
#include "APConnector/Connector.h"

namespace Plugins {
using namespace melonDS;

class PluginKingdomHeartsDaysMarluxia : public PluginKingdomHeartsDays {
public:
    explicit PluginKingdomHeartsDaysMarluxia(u32 gameCode);

    static u32 euGamecode;
    static bool isCart(u32 gameCode) { return gameCode == euGamecode; }

    void loadConfigs(
        std::function<bool(std::string)> getBool,
        std::function<int(std::string)> getInt,
        std::function<std::string(std::string)> getStr
    ) override;

    void onLoadROM() override;
    void onLoadState() override;
    bool shouldRenderFrame() override;

private:
    bool marlux = true;
    bool mmOn = false;
    std::unique_ptr<APConnector> mm;
    void setMarlux();
};
}

#endif
