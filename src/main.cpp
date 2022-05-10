//----------------------------------------------------------
//
//   SA:MP Multiplayer Modification For GTA:SA
//   Copyright 2004-2007 SA:MP Team
//
//----------------------------------------------------------

#include "main.hpp"
#include "version.hpp"

extern void* pAMXFunctions;

SemanticVersion SkyComponent::componentVersion() const
{
    return SemanticVersion(PROJECT_MAJOR, PROJECT_MINOR, PROJECT_PATCH, 0);
}

void SkyComponent::onLoad(ICore* c)
{
    // make sure bitstream versions match core
    if (NetworkBitStream::Version != c->getNetworkBitStreamVersion()) {
        core_->logLn(LogLevel::Error, "This SKY version was not designed to run with this open.mp build.\n\nPlease update to latest version");
        return;
    }

    core_ = c;
    players_ = &c->getPlayers();

    getCore() = c;

    // packet handlers
    core_->addPerPacketInEventHandler<NetCode::Packet::PlayerFootSync::PacketID>(&player_sync_);
    core_->addPerPacketInEventHandler<NetCode::Packet::PlayerAimSync::PacketID>(&aim_sync_);
    core_->addPerPacketInEventHandler<NetCode::Packet::PlayerVehicleSync::PacketID>(&vehicle_sync_);
    core_->addPerPacketInEventHandler<NetCode::Packet::PlayerPassengerSync::PacketID>(&passenger_sync_);
    core_->addPerPacketInEventHandler<NetCode::Packet::PlayerSpectatorSync::PacketID>(&spectator_sync_);

    // show version
    ShowPluginInfo();
}

void SkyComponent::onInit(IComponentList* components)
{
    if (!core_) {
        return;
    }

    StringView name = componentName();

    pawn_component_ = components->queryComponent<IPawnComponent>();
    if (!pawn_component_) {
        core_->logLn(LogLevel::Error,
            "Error loading component %.*s: Pawn component not loaded",
            name.length(), name.data());

        return;
    }

    textdraw_component_ = components->queryComponent<ITextDrawsComponent>();
    if (!textdraw_component_) {
        core_->logLn(LogLevel::Error,
            "Error loading component %.*s: Textdraw component not loaded",
            name.length(), name.data());

        return;
    }

    pawn_component_->getEventDispatcher().addEventHandler(this);
    pAMXFunctions = (void*)&pawn_component_->getAmxFunctions();
    players_->getEventDispatcher().addEventHandler(&player_events_);
}

void SkyComponent::onAmxLoad(void* amx)
{
    InitScripting((AMX*)amx);
};

void SkyComponent::onAmxUnload(void* amx) {};

void SkyComponent::onFree(IComponent* component)
{
    if (component == pawn_component_ || component == this) {
        pawn_component_ = nullptr;
    }
}

void SkyComponent::reset() { }

void SkyComponent::free()
{
    if (pawn_component_) {
        pawn_component_->getEventDispatcher().removeEventHandler(this);
        players_->getEventDispatcher().addEventHandler(&player_events_);
    }

    // packet handlers
    core_->removePerPacketInEventHandler<NetCode::Packet::PlayerFootSync::PacketID>(&player_sync_);
    core_->removePerPacketInEventHandler<NetCode::Packet::PlayerAimSync::PacketID>(&aim_sync_);
    core_->removePerPacketInEventHandler<NetCode::Packet::PlayerVehicleSync::PacketID>(&vehicle_sync_);
    core_->removePerPacketInEventHandler<NetCode::Packet::PlayerPassengerSync::PacketID>(&passenger_sync_);
    core_->removePerPacketInEventHandler<NetCode::Packet::PlayerSpectatorSync::PacketID>(&spectator_sync_);

    delete this;
}

void SkyComponent::provideConfiguration(ILogger& logger, IEarlyConfig& config, bool defaults)
{
    if (defaults) {
        config.setInt("SKY.enable_knife_sync", 0);
        config.setInt("SKY.disable_sync_bugs", 0);
    } else {
        config.setInt("SKY.enable_knife_sync", 0);
        config.setInt("SKY.disable_sync_bugs", 0);
    }
}

void SkyComponent::Log(LogLevel level, const char* fmt, ...)
{
    auto core = getCore();

    if (!core) {
        return;
    }

    va_list args {};
    va_start(args, fmt);
    core->vlogLn(level, fmt, args);
    va_end(args);
}

ITextDrawsComponent*& SkyComponent::getTextDraws()
{
    static ITextDrawsComponent* textdraws {};
    return textdraws;
}

ICore*& SkyComponent::getCore()
{
    static ICore* core {};
    return core;
}

COMPONENT_ENTRY_POINT()
{
    return new SkyComponent();
}