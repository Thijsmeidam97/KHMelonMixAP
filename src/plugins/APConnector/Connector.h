#pragma once
#include <functional>
#include <memory>
#include <string>
namespace melonDS { class NDS; }
namespace Plugins {
class APConnector {
public:
    using MsgCb = std::function<void(const std::string&)>;
    explicit APConnector(MsgCb fn = {});
    ~APConnector();
    void setNds(melonDS::NDS* value);
    void setEnabled(bool value);
    bool enabled() const;
    void loadRom();
    void loadSt();
    void pollFrame();
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
    melonDS::NDS* nds = nullptr;
    bool active = false;
};
using PluginKingdomHeartsDaysConnector = APConnector;
}
