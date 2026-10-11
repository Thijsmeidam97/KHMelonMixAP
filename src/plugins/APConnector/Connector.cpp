#include "Connector.h"
#include "State.h"
namespace Plugins {
using APC::Ctx;
struct APConnector::Impl {
    Ctx ctx;
};
APConnector::APConnector() {
    impl = std::make_unique<Impl>();
}
APConnector::~APConnector() = default;
void APConnector::setNds(melonDS::NDS* value) {
    nds = value;
    impl->ctx.setNds(value);
}
void APConnector::setEnabled(bool value) {
    active = value;
    impl->ctx.setOn(value);
}
void APConnector::loadRom() {
    impl->ctx.setNds(nds);
    impl->ctx.loadRom();
}
void APConnector::loadSt() {
    impl->ctx.loadSt();
}
void APConnector::pollFrame() {
    if (active) {
        impl->ctx.poll();
    }
}
}
