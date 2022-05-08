/*
 *  Version: MPL 1.1
 *
 *  The contents of this file are subject to the Mozilla Public License Version
 *  1.1 (the "License"); you may not use this file except in compliance with
 *  the License. You may obtain a copy of the License at
 *  http://www.mozilla.org/MPL/
 *
 *  Software distributed under the License is distributed on an "AS IS" basis,
 *  WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 *  for the specific language governing rights and limitations under the
 *  License.
 *
 *  The Original Code is the YSI 2.0 SA:MP plugin.
 *
 *  The Initial Developer of the Original Code is Alex "Y_Less" Cole.
 *  Portions created by the Initial Developer are Copyright (C) 2008
 *  the Initial Developer. All Rights Reserved.
 *
 *  Contributor(s):
 *
 *  Peter Beverloo
 *  Marcus Bauer
 *  MaVe;
 *  Sammy91
 *  Incognito
 *
 *  Special Thanks to:
 *
 *  SA:MP Team past, present and future
 */

#include "scripting.hpp"

#include "main.hpp"
#include "player.hpp"

#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <limits>

#ifdef _WIN32
#include <windows.h>
// Yes - BOTH string versions...
#include <strsafe.h>
#else
#include <algorithm>
#include <dirent.h>
#include <fnmatch.h>
#include <sys/stat.h>
#endif

#include <stdio.h>
#include <string.h>

#define GET_PLAYER(player, param)                                       \
    IPlayer* player = SkyComponent::getCore()->getPlayers().get(param); \
    if (player == nullptr) {                                            \
        return 0;                                                       \
    }

// native SpawnPlayerForWorld(playerid);
static cell AMX_NATIVE_CALL SpawnPlayerForWorld(AMX* amx, cell* params)
{
    CHECK_PARAMS(1, "SpawnPlayerForWorld");

    GET_PLAYER(player, params[1])

    FlatPtrHashSet<IPlayer> players = SkyComponent::getCore()->getPlayers().entries();

    for (IPlayer* other : players) {
        if (other != player && player->isStreamedInForPlayer(*other)) {
            player->streamOutForPlayer(*other);
        }
    }

    return 1;
}

// native SetLastAnimationData(playerid, data)
static cell AMX_NATIVE_CALL SetLastAnimationData(AMX* amx, cell* params)
{
    CHECK_PARAMS(2, "SetLastAnimationData");

    GET_PLAYER(player, params[1])

    // TODO: wait for open.mp to implement a way to to modify animation data without sending rpc
    return 1;
}

// native SendLastSyncData(playerid, toplayerid, animation = 0)
static cell AMX_NATIVE_CALL SendLastSyncData(AMX* amx, cell* params)
{
    CHECK_PARAMS(3, "SendLastSyncData");

    GET_PLAYER(player, params[1])
    GET_PLAYER(toPlayer, params[2])

    GET_SKY_DATA(player, false)

    NetworkBitStream bs = player_data->GetSyncBitStream(PlayerSyncType::E_PLAYER_SYNC);

    Packet::PlayerFootSync syncPacket;
    syncPacket.read(bs);

    syncPacket.PlayerID = player->getID();

    // Make them appear standing still if paused
    if (Time::now() - player_data->GetLastSyncTime() >= Seconds(2)) {
        syncPacket.Velocity = Vector3(0.0f, 0.0f, 0.0f);
    }

    // Set fake data since last packet might not have them.
    float fake_health = player_data->GetFakeHealth();
    if (player_data->GetFakeHealth() != 255) {
        syncPacket.HealthArmour[0] = fake_health;
    }

    float fake_armour = player_data->GetFakeArmour();
    if (fake_armour != 255) {
        syncPacket.HealthArmour[1] = fake_armour;
    }

    /*float fake_angle = player_data->GetFakeFacingAngle();
    if (fake_angle != static_cast<float>(0x7FFFFFFF)) {
        syncPacket.Rotation = GTAQuat(0.0, 0.0, fake_angle);
    }*/

    if (!params[3]) {
        syncPacket.AnimationID = 0;
    }
    PacketHelper::send(syncPacket, *toPlayer);
    
    return 1;
}

// native SendLastSyncPacket(playerid, toplayerid, E_SYNC_TYPES:type = E_PLAYER_SYNC, animation = 0)
static cell AMX_NATIVE_CALL SendLastSyncPacket(AMX* amx, cell* params)
{
    CHECK_PARAMS(4, "SendLastSyncPacket");

    GET_PLAYER(player, params[1])
    GET_PLAYER(toPlayer, params[2])

    GET_SKY_DATA(player, 0)

    if (params[2] < E_PLAYER_SYNC || params[2] > E_LAST_SYNC) {
        return 0;
    }

    PlayerSyncType syncType = static_cast<PlayerSyncType>(params[2]);
    if (syncType == E_LAST_SYNC) {
        syncType = player_data->GetLastSyncType();
    }

    NetworkBitStream bs = player_data->GetSyncBitStream(syncType);

    switch (params[2]) {
    case PlayerSyncType::E_PLAYER_SYNC: {

        Packet::PlayerFootSync syncPacket;
        syncPacket.read(bs);
        syncPacket.PlayerID = player->getID();
        
        // Animations are only sent when they are changed
        if (!params[4]) {
            syncPacket.AnimationID = 0;
        }

        // Make them appear standing still if paused
        if (Time::now() - player_data->GetLastSyncTime() >= Seconds(2)) {
            syncPacket.Velocity = Vector3(0.0f, 0.0f, 0.0f);
        }

        // Set fake data since last packet might not have them.
        float fake_health = player_data->GetFakeHealth();
        if (player_data->GetFakeHealth() != 255) {
            syncPacket.HealthArmour[0] = fake_health;
        }

        float fake_armour = player_data->GetFakeArmour();
        if (fake_armour != 255) {
            syncPacket.HealthArmour[1] = fake_armour;
        }

        /*float fake_angle = player_data->GetFakeFacingAngle();
        if (fake_angle != static_cast<float>(0x7FFFFFFF)) {
            syncPacket.Rotation = GTAQuat(0.0, 0.0, fake_angle);
        }*/

        PacketHelper::send(syncPacket, *toPlayer);
        break;
    }
    case PlayerSyncType::E_AIM_SYNC: {
        Packet::PlayerAimSync syncPacket;
        syncPacket.read(bs);
        syncPacket.PlayerID = player->getID();

        // Fix first-person up/down aim sync
        PlayerWeapon weapon = player_data->GetLastWeapon();
        if (weapon == PlayerWeapon_Sniper || weapon == PlayerWeapon_RocketLauncher || weapon == PlayerWeapon_HeatSeeker || weapon == PlayerWeapon_Camera) {

            syncPacket.AimZ = -syncPacket.CamFrontVector[2];

            if (syncPacket.AimZ > 1.0f) {
                syncPacket.AimZ = 1.0f;
            } else if (syncPacket.AimZ < -1.0f) {
                syncPacket.AimZ = -1.0f;
            }
        }

        if (player_data->GetInfiniteAmmo()) {
            syncPacket.CamZoom = 2;
        }

        PacketHelper::send(syncPacket, *toPlayer);
        break;
    }
    case PlayerSyncType::E_VEHICLE_SYNC: {
        Packet::PlayerVehicleSync syncPacket;
        syncPacket.read(bs);

        // Set fake data since last packet might not have them.
        float fake_health = player_data->GetFakeHealth();
        if (player_data->GetFakeHealth() != 255) {
            syncPacket.PlayerHealthArmour[0] = fake_health;
        }

        float fake_armour = player_data->GetFakeArmour();
        if (fake_armour != 255) {
            syncPacket.PlayerHealthArmour[1] = fake_armour;
        }

        syncPacket.PlayerID = player->getID();
        PacketHelper::send(syncPacket, *toPlayer);
        break;
    }
    case PlayerSyncType::E_PASSENGER_SYNC: {
        Packet::PlayerPassengerSync syncPacket;
        syncPacket.read(bs);

        // Set fake data since last packet might not have them.
        float fake_health = player_data->GetFakeHealth();
        if (player_data->GetFakeHealth() != 255) {
            syncPacket.HealthArmour[0] = fake_health;
        }

        float fake_armour = player_data->GetFakeArmour();
        if (fake_armour != 255) {
            syncPacket.HealthArmour[1] = fake_armour;
        }

        syncPacket.PlayerID = player->getID();
        PacketHelper::send(syncPacket, *toPlayer);
        break;
    }
    }
    return 1;
}

// native SetFakeArmour(playerid, health);
static cell AMX_NATIVE_CALL SetFakeArmour(AMX* amx, cell* params)
{
    CHECK_PARAMS(2, "SetFakeArmour");

    GET_PLAYER(player, params[1])
    GET_SKY_DATA(player, 0)

    player_data->SetFakeArmour(params[2]);
    return 1;
}

// native SetFakeHealth(playerid, health);
static cell AMX_NATIVE_CALL SetFakeHealth(AMX* amx, cell* params)
{
    CHECK_PARAMS(2, "SetFakeHealth");

    GET_PLAYER(player, params[1])
    GET_SKY_DATA(player, 0)

    player_data->SetFakeHealth(params[2]);
    return 1;
}

// native SetFakeFacingAngle(playerid, Float:angle)
static cell AMX_NATIVE_CALL SetFakeFacingAngle(AMX* amx, cell* params)
{
    CHECK_PARAMS(2, "SetFakeFacingAngle");

    GET_PLAYER(player, params[1])
    GET_SKY_DATA(player, 0)

    player_data->SetFakeFacingAngle(params[2]);
    return 1;
}

// native SetKnifeSync(toggle);
static cell AMX_NATIVE_CALL SetKnifeSync(AMX* amx, cell* params)
{
    CHECK_PARAMS(1, "SetKnifeSync");
    static_cast<IEarlyConfig&>(SkyComponent::getCore()->getConfig()).setInt("SKY.enable_knife_sync", static_cast<bool>(params[1]));
    return 1;
}

// native SetDisableSyncBugs(toggle);
static cell AMX_NATIVE_CALL SetDisableSyncBugs(AMX* amx, cell* params)
{
    CHECK_PARAMS(1, "SetDisableSyncBugs");
    static_cast<IEarlyConfig&>(SkyComponent::getCore()->getConfig()).setInt("SKY.disable_sync_bugs", static_cast<bool>(params[1]));
    return 1;
}

// native SetInfiniteAmmoSync(playerid, toggle)
static cell AMX_NATIVE_CALL SetInfiniteAmmoSync(AMX* amx, cell* params)
{
    CHECK_PARAMS(2, "SetInfiniteAmmoSync");

    GET_PLAYER(player, params[1])
    GET_SKY_DATA(player, 0)

    player_data->SetInfiniteAmmo(params[2]);
    return 1;
}

// native SetKeySyncBlocked(playerid, toggle)
static cell AMX_NATIVE_CALL SetKeySyncBlocked(AMX* amx, cell* params)
{
    CHECK_PARAMS(2, "SetKeySyncBlocked");

    GET_PLAYER(player, params[1])
    GET_SKY_DATA(player, 0)

    player_data->SetBlockKeySync(params[2]);
    return 1;
}

// native ClearAnimationsForPlayer(playerid, forplayerid)
static cell AMX_NATIVE_CALL ClearAnimationsForPlayer(AMX* amx, cell* params)
{
    CHECK_PARAMS(2, "ClearAnimationsForPlayer");

    GET_PLAYER(player, params[1])
    GET_PLAYER(forPlayer, params[2])

    NetCode::RPC::ClearPlayerAnimations cpa;
    cpa.PlayerID = player->getID();

    PacketHelper::send(cpa, *forPlayer);
    return 1;
}

// native SendDeath(playerid);
static cell AMX_NATIVE_CALL SendDeath(AMX* amx, cell* params)
{
    CHECK_PARAMS(1, "SendDeath");

    GET_PLAYER(player, params[1])

    // TODO: waiting for open.mp to implement a way to set a players state
    // player->setState(PLAYER_STATE_WASTED);

    NetCode::RPC::PlayerDeath pd;
    pd.PlayerID = player->getID();

    // Death is broadcasted to all players
    PacketHelper::broadcast(pd, SkyComponent::getCore()->getPlayers(), player);
    return 1;
}

// native FreezeSyncData(playerid, bool:toggle)
static cell AMX_NATIVE_CALL FreezeSyncData(AMX* amx, cell* params)
{
    CHECK_PARAMS(2, "FreezeSyncData");

    GET_PLAYER(player, params[1])
    GET_SKY_DATA(player, 0)

    player_data->SetSyncFrozen(E_PLAYER_SYNC, static_cast<bool>(params[2]));
    return 1;
}

// native FreezeSyncPacket(playerid, E_SYNC_TYPES:type = E_PLAYER_SYNC, bool:toggle)
static cell AMX_NATIVE_CALL FreezeSyncPacket(AMX* amx, cell* params)
{
    CHECK_PARAMS(3, "FreezeSyncPacket");

    GET_PLAYER(player, params[1])
    GET_SKY_DATA(player, 0)

    PlayerSyncType syncType = static_cast<PlayerSyncType>(params[2]);
    bool toggle = static_cast<bool>(params[3]);

    player_data->SetSyncFrozen(syncType, toggle);

    if (!toggle) {
        return 1;
    }

    switch (syncType) {
    case PlayerSyncType::E_PLAYER_SYNC: {
        NetworkBitStream bs = player_data->GetSyncBitStream(PlayerSyncType::E_PLAYER_SYNC);
        Packet::PlayerFootSync data;
        data.read(bs);

        data.Velocity = Vector3(0.0f, 0.0f, 0.0f);
        data.SpecialAction = SpecialAction_None;
        data.Keys = 0;
        data.LeftRight = 0;
        data.UpDown = 0;

        data.rewrite(bs);
        break;
    }

    case PlayerSyncType::E_VEHICLE_SYNC: {
        NetworkBitStream bs = player_data->GetSyncBitStream(PlayerSyncType::E_VEHICLE_SYNC);
        Packet::PlayerVehicleSync data;
        data.read(bs);

        data.Velocity = Vector3(0.0f, 0.0f, 0.0f);
        data.Keys = 0;
        data.LeftRight = 0;
        data.UpDown = 0;

        data.rewrite(bs);
        break;
    }

    case PlayerSyncType::E_PASSENGER_SYNC: {
        NetworkBitStream bs = player_data->GetSyncBitStream(PlayerSyncType::E_PASSENGER_SYNC);
        Packet::PlayerPassengerSync data;
        data.read(bs);

        data.Keys = 0;
        data.LeftRight = 0;
        data.UpDown = 0;

        data.rewrite(bs);
        break;
    }

    case PlayerSyncType::E_SPECTATING_SYNC: {
        NetworkBitStream bs = player_data->GetSyncBitStream(PlayerSyncType::E_SPECTATING_SYNC);
        Packet::PlayerSpectatorSync data;
        data.read(bs);

        data.Keys = 0;
        data.LeftRight = 0;
        data.UpDown = 0;

        data.rewrite(bs);
        break;
    }
    default:
        break;
    }
    return 1;
}

// native TextDrawSetPosition(Text:text, Float:fX, Float:fY)
static cell AMX_NATIVE_CALL TextDrawSetPosition(AMX* amx, cell* params)
{
    CHECK_PARAMS(3, "TextDrawSetPosition");

    ITextDraw* td = SkyComponent::getTextDraws()->get(params[1]);
    if (td == nullptr) {
        return 0;
    }

    td->setPosition(Vector2(amx_ctof(params[2]), amx_ctof(params[3])));
    return 1;
}

// native PlayerTextDrawSetPosition(playerid, PlayerText:text, Float:fX, Float:fY)
static cell AMX_NATIVE_CALL PlayerTextDrawSetPosition(AMX* amx, cell* params)
{
    CHECK_PARAMS(4, "PlayerTextDrawSetPosition");

    GET_PLAYER(player, params[1])

    IPlayerTextDrawData* td_data = queryExtension<IPlayerTextDrawData>(*player);
    if (td_data == nullptr) {
        return 0;
    }

    IPlayerTextDraw* ptd = td_data->get(params[2]);
    if (ptd == nullptr) {
        return 0;
    }

    ptd->setPosition(Vector2(amx_ctof(params[3]), amx_ctof(params[4])));
    return 1;
}

// native TextDrawSetStringForPlayer(Text:text, playerid, const string[])
static cell AMX_NATIVE_CALL TextDrawSetStrForPlayer(AMX* amx, cell* params)
{
    CHECK_PARAMS(3, "TextDrawSetStrForPlayer");

    GET_PLAYER(player, params[1])

    ITextDraw* td = SkyComponent::getTextDraws()->get(params[1]);
    if (td == nullptr) {
        return 0;
    }

    char* text;
    amx_StrParam(amx, params[3], text);

    NetCode::RPC::PlayerTextDrawSetString tdp;
    tdp.TextDrawID = td->getID();
    tdp.Text = text;

    PacketHelper::send(tdp, *player);
    return 1;
}

static AMX_NATIVE_INFO native_list[] = {
    { "SpawnPlayerForWorld", SpawnPlayerForWorld },
    { "SetFakeHealth", SetFakeHealth },
    { "SetFakeArmour", SetFakeArmour },
    { "SetFakeFacingAngle", SetFakeFacingAngle },
    { "FreezeSyncData", FreezeSyncData },
    { "FreezeSyncPacket", FreezeSyncPacket },
    { "SetKnifeSync", SetKnifeSync },
    { "SendDeath", SendDeath },
    { "SetLastAnimationData", SetLastAnimationData },
    { "SendLastSyncData", SendLastSyncData },
    { "SendLastSyncPacket", SendLastSyncPacket },
    { "SetDisableSyncBugs", SetDisableSyncBugs },
    { "ClearAnimationsForPlayer", ClearAnimationsForPlayer },
    { "SetKeySyncBlocked", SetKeySyncBlocked },
    { "SetInfiniteAmmoSync", SetInfiniteAmmoSync },
    { "TextDrawSetPosition", TextDrawSetPosition },
    { "PlayerTextDrawSetPosition", PlayerTextDrawSetPosition },
    { "TextDrawSetStrForPlayer", TextDrawSetStrForPlayer },
    { 0, 0 }
};

int InitScripting(AMX* amx)
{
    return amx_Register(amx, native_list, -1);
}
