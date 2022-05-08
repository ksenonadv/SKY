#pragma once

#include "Shared/netcode.hpp"
#include "types.hpp"
#include "values.hpp"

enum PlayerSyncType : uint8_t {
    E_PLAYER_SYNC = 0,
    E_AIM_SYNC,
    E_VEHICLE_SYNC,
    E_PASSENGER_SYNC,
    E_SPECTATING_SYNC,
    E_LAST_SYNC,
    E_ALL_SYNC
};

constexpr size_t MAX_SYNC_TYPES = PlayerSyncType::E_ALL_SYNC + 1;

class PlayerSKYData : public IExtension {
public:
    PROVIDE_EXT_UID(0x2A42FC3C486992BD);

    PlayerSKYData()
        : health_(255)
        , armour_(255)
        , angle_((float)0x7FFFFFFF)
    {
        syncFrozen_.fill(false);
    }

    void SetLastWeapon(PlayerWeapon weapon)
    {
        weapon_ = weapon;
    }

    PlayerWeapon GetLastWeapon() const
    {
        return weapon_;
    }

    void SetFakeHealth(float health)
    {
        health_ = health;
    }

    float GetFakeHealth() const
    {
        return health_;
    }

    void SetFakeArmour(float armour)
    {
        armour_ = armour;
    }

    float GetFakeArmour() const
    {
        return armour_;
    }

    void SetFakeFacingAngle(float angle)
    {
        angle_ = angle;
    }

    bool GetFakeFacingAngle() const
    {
        return angle_;
    }

    TimePoint GetLastSyncTime()
    {
        return lastSyncTime_;
    }

    void SetBlockKeySync(bool value)
    {
        blockKeys_ = value;
    }

    bool GetBlockKeySync() const
    {
        return blockKeys_;
    }

    void SetInfiniteAmmo(bool value)
    {
        infAmmo_ = value;
    }

    bool GetInfiniteAmmo() const
    {
        return infAmmo_;
    }

    bool IsSyncFrozen(PlayerSyncType type) const
    {
        return syncFrozen_[type];
    }

    void SetSyncFrozen(PlayerSyncType type, bool value)
    {
        syncFrozen_[type] = value;
    }

    void SetLastSyncType(PlayerSyncType type)
    {
        lastSyncType_ = type;
        lastSyncTime_ = Time::now();
    }

    PlayerSyncType GetLastSyncType() const
    {
        return lastSyncType_;
    }

    void SetSyncData(PlayerSyncType type, NetworkBitStream& bs)
    {
        syncBitstream_[type] = bs;
    }

    NetworkBitStream& GetSyncBitStream(PlayerSyncType type)
    {
        return syncBitstream_[type];
    }

    void freeExtension() override
    {
        delete this;
    }

    void reset() override
    {
        // Reset data ..
    }

private:
    PlayerWeapon weapon_;
    float health_;
    float armour_;

    float angle_;

    TimePoint lastSyncTime_;
    bool blockKeys_;
    bool infAmmo_;

    std::array<bool, MAX_SYNC_TYPES> syncFrozen_;
    PlayerSyncType lastSyncType_;

    std::array<NetworkBitStream, E_LAST_SYNC> syncBitstream_;
};

inline class PlayerEvents : public PlayerEventHandler {
public:
    void onConnect(IPlayer& player) override
    {
        player.addExtension(new PlayerSKYData(), true);
    }
} player_events_;

#define GET_SKY_DATA(peer, fail_ret)                                 \
    PlayerSKYData* player_data = queryExtension<PlayerSKYData>(peer); \
    if (player_data == nullptr) {                                     \
        return fail_ret;                                             \
    }