#pragma once

#include "Shared/NetCode/core.hpp"
#include "Shared/NetCode/vehicle.hpp"
#include "sdk.hpp"

namespace Packet {
struct PlayerFootSync : NetCode::Packet::PlayerFootSync {
    void rewrite(NetworkBitStream& bs)
    {
        bs.reset();
        bs.writeINT8(PacketID);

        bs.writeUINT16(LeftRight);
        bs.writeUINT16(UpDown);
        bs.writeUINT16(Keys);
        bs.writeVEC3(Position);
        bs.writeFLOAT(Rotation.q.w);
        bs.writeFLOAT(Rotation.q.x);
        bs.writeFLOAT(Rotation.q.y);
        bs.writeFLOAT(Rotation.q.z);
        bs.writeUINT8(static_cast<uint8_t>(HealthArmour[0]));
        bs.writeUINT8(static_cast<uint8_t>(HealthArmour[1]));
        bs.writeUINT8(WeaponAdditionalKey);
        bs.writeUINT8(SpecialAction);
        bs.writeVEC3(Velocity);
        bs.writeVEC3(SurfingData.offset);

        switch (SurfingData.type) {
        case PlayerSurfingData::Type::Vehicle:
            bs.writeUINT16(SurfingData.ID);
            break;
        case PlayerSurfingData::Type::Object:
            bs.writeUINT16(VEHICLE_POOL_SIZE + SurfingData.ID);
            break;
        default:
            bs.writeUINT16(0);
            break;
        }

        bs.writeUINT16(AnimationID);
        bs.writeUINT16(AnimationFlags);
    }
};

struct PlayerAimSync : NetCode::Packet::PlayerAimSync {
    void rewrite(NetworkBitStream& bs)
    {
        bs.reset();
        bs.writeINT8(PacketID);

        bs.writeUINT8(CamMode);
        bs.writeVEC3(CamFrontVector);
        bs.writeVEC3(CamPos);
        bs.writeFLOAT(AimZ);
        bs.writeUINT8(ZoomWepState);
        bs.writeUINT8(AspectRatio);
    }
};

struct PlayerSpectatorSync : NetCode::Packet::PlayerSpectatorSync {
    void rewrite(NetworkBitStream& bs)
    {
        bs.reset();
        bs.writeINT8(PacketID);

        bs.writeUINT16(LeftRight);
        bs.writeUINT16(UpDown);
        bs.writeUINT16(Keys);
        bs.writeVEC3(Position);
    }
};

struct PlayerVehicleSync : NetCode::Packet::PlayerVehicleSync {
    void rewrite(NetworkBitStream& bs)
    {
        bs.reset();
        bs.writeINT8(PacketID);

        bs.writeUINT16(VehicleID);
        bs.writeUINT16(LeftRight);
        bs.writeUINT16(UpDown);
        bs.writeUINT16(Keys);
        bs.writeFLOAT(Rotation.q.w);
        bs.writeFLOAT(Rotation.q.x);
        bs.writeFLOAT(Rotation.q.y);
        bs.writeFLOAT(Rotation.q.z);
        bs.writeVEC3(Position);
        bs.writeVEC3(Velocity);
        bs.writeFLOAT(Health);
        bs.writeUINT8(static_cast<uint8_t>(PlayerHealthArmour[0]));
        bs.writeUINT8(static_cast<uint8_t>(PlayerHealthArmour[1]));
        bs.writeUINT8(AdditionalKeyWeapon);
        bs.writeUINT8(Siren);
        bs.writeUINT8(LandingGear);
        bs.writeUINT16(TrailerID);
        bs.writeUINT32(HydraThrustAngle);
    }
};

struct PlayerPassengerSync : NetCode::Packet::PlayerPassengerSync {
    void rewrite(NetworkBitStream& bs)
    {
        bs.reset();
        bs.writeINT8(PacketID);

        bs.writeUINT16(VehicleID);
        bs.writeUINT16(DriveBySeatAdditionalKeyWeapon);
        bs.writeUINT8(static_cast<uint8_t>(HealthArmour[0]));
        bs.writeUINT8(static_cast<uint8_t>(HealthArmour[1]));
        bs.writeUINT16(LeftRight);
        bs.writeUINT16(UpDown);
        bs.writeUINT16(Keys);
        bs.writeVEC3(Position);
    }
};
}