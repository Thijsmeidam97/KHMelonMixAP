#pragma once
#include <memory>
namespace melonDS { class NDS; }
namespace Plugins {
class APConnector {
public:
    APConnector();
    ~APConnector();
    void setNds(melonDS::NDS* value);
    void setEnabled(bool value);
    void loadRom();
    void loadSt();
    void pollFrame();
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
    melonDS::NDS* nds = nullptr;
    bool active = false;
};
}
