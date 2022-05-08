#pragma once

#include "sdk.hpp"
#include "Shared/NetCode/vehicle.hpp"
#include "Shared/NetCode/core.hpp"

#include "main.hpp"
#include "player.hpp"
#include "packets.hpp"

inline class PlayerSync final : public SingleNetworkInEventHandler
{
    bool received(IPlayer &peer, NetworkBitStream &bs) override;
} player_sync_;

inline class AimSync final : public SingleNetworkInEventHandler
{
    bool received(IPlayer &peer, NetworkBitStream &bs) override;
} aim_sync_;

inline class VehicleSync final : public SingleNetworkInEventHandler
{
    bool received(IPlayer &peer, NetworkBitStream &bs) override;
} vehicle_sync_;

inline class PassengerSync final : public SingleNetworkInEventHandler
{
    bool received(IPlayer &peer, NetworkBitStream &bs) override;
} passenger_sync_;

inline class SpectatorSync final : public SingleNetworkInEventHandler
{
    bool received(IPlayer &peer, NetworkBitStream &bs) override;
} spectator_sync_;