#include "network.hpp"

bool PlayerSync::received(IPlayer& peer, NetworkBitStream& bs)
{
    GET_SKY_DATA(peer, true)

    Packet::PlayerFootSync packet;
    NetworkBitStream& last_bs = player_data->GetSyncBitStream(E_PLAYER_SYNC);

    if (player_data->IsSyncFrozen(E_PLAYER_SYNC) || player_data->IsSyncFrozen(E_ALL_SYNC)) {
        packet.read(last_bs);
    } else {
        packet.read(bs);

        if (packet.Weapon > PlayerWeapon::PlayerWeapon_Parachute || (packet.Weapon > PlayerWeapon::PlayerWeapon_Moltov && packet.Weapon < PlayerWeapon::PlayerWeapon_Colt45)) {
            packet.Weapon = PlayerWeapon::PlayerWeapon_Fist;
        }

        // Because of detonator crasher - Sends AIM_KEY in this packet and cam mode IDs 7, 8, 34, 45, 46, 51 and 65 in ID_AIM_SYNC
        if (packet.Weapon == PlayerWeapon::PlayerWeapon_Bomb) {
            packet.Keys &= ~128;
        }

        static int* disableSyncBugs = SkyComponent::getCore()->getConfig().getInt("SKY.disable_sync_bugs");

        if (*disableSyncBugs) {
            // Prevent "ghost shooting" bugs
            if ((packet.Weapon >= PlayerWeapon::PlayerWeapon_Colt45 && packet.Weapon <= PlayerWeapon::PlayerWeapon_Sniper) || packet.Weapon == PlayerWeapon::PlayerWeapon_Minigun) {
                switch (packet.AnimationID) {
                // PED_RUN_*
                case 1222:
                case 1223:
                case 1224:
                case 1225:
                case 1226:
                case 1227:
                case 1228:
                case 1229:
                case 1230:
                case 1231:
                case 1232:
                case 1233:
                case 1234:
                case 1235:
                case 1236:
                    // PED_SWAT_RUN
                case 1249:
                    // PED_WOMAN_(RUN/WALK)_*
                case 1275:
                case 1276:
                case 1277:
                case 1278:
                case 1279:
                case 1280:
                case 1281:
                case 1282:
                case 1283:
                case 1284:
                case 1285:
                case 1286:
                case 1287:
                    // FAT_FATRUN_ARMED
                case 459:
                    // MUSCULAR_MUSCLERUN*
                case 908:
                case 909:
                    // PED_WEAPON_CROUCH
                case 1274:
                    // PED_WALK_PLAYER
                case 1266:
                    // PED_SHOT_PARTIAL(_B)
                case 1241:
                case 1242:
                    // Baseball bat
                case 17:
                case 18:
                case 19:
                case 20:
                case 21:
                case 22:
                case 23:
                case 24:
                case 25:
                case 26:
                case 27:
                    // Knife
                case 745:
                case 746:
                case 747:
                case 748:
                case 749:
                case 750:
                case 751:
                case 752:
                case 753:
                case 754:
                case 755:
                case 756:
                case 757:
                case 758:
                case 759:
                case 760:
                    // Sword
                case 1545:
                case 1546:
                case 1547:
                case 1548:
                case 1549:
                case 1550:
                case 1551:
                case 1552:
                case 1553:
                case 1554:
                    // Fight
                case 471:
                case 472:
                case 473:
                case 474:
                case 477:
                case 478:
                case 479:
                case 480:
                case 481:
                case 482:
                case 483:
                case 484:
                case 485:
                case 486:
                case 487:
                case 488:
                case 489:
                case 490:
                case 491:
                case 492:
                case 493:
                case 494:
                case 495:
                case 496:
                case 497:
                case 498:
                case 499:
                case 500:
                case 501:
                case 502:
                case 503:
                case 504:
                case 505:
                case 506:
                case 507:
                case 1135:
                case 1136:
                case 1137:
                case 1138:
                case 1139:
                case 1140:
                case 1141:
                case 1142:
                case 1143:
                case 1144:
                case 1145:
                case 1146:
                case 1147:
                case 1148:
                case 1149:
                case 1150:
                case 1151:
                    // Only remove action key if holding aim
                    if (packet.Keys & 128) {
                        packet.Keys &= ~1;
                    }

                    // Remove fire key
                    packet.Keys &= ~4;

                    // Remove aim key
                    packet.Keys &= ~128;
                    break;
                }
            } else if (packet.Weapon == PlayerWeapon::PlayerWeapon_SprayCan || packet.Weapon == PlayerWeapon::PlayerWeapon_FireExtinguisher || packet.Weapon == PlayerWeapon::PlayerWeapon_FlameThrower) {
                if (packet.AnimationID < 1160 || packet.AnimationID > 1167) {
                    // Only remove action key if holding aim
                    if (packet.Keys & 128) {
                        packet.Keys &= ~1;
                    }

                    // Remove fire key
                    packet.Keys &= ~4;

                    // Remove aim key
                    packet.Keys &= ~128;
                }
            } else if (packet.Weapon == PlayerWeapon::PlayerWeapon_Grenade) {
                if (packet.AnimationID < 644 || packet.AnimationID > 646) {
                    packet.Keys &= ~1;
                }
            }
        }

        // Storing latest packet data.
        packet.rewrite(last_bs);
    }

    if (player_data->GetBlockKeySync()) {
        packet.Keys = 0;
    }

    float fake_health = player_data->GetFakeHealth();
    if (player_data->GetFakeHealth() != 255) {
        packet.HealthArmour[0] = fake_health;
    }

    float fake_armour = player_data->GetFakeArmour();
    if (fake_armour != 255) {
        packet.HealthArmour[1] = fake_armour;
    }

    /*float fake_angle = player_data->GetFakeFacingAngle();
        if (fake_angle != static_cast<float>(0x7FFFFFFF)) {
            Vector3 rot_euler = packet.Rotation.ToEuler();
            packet.Rotation = GTAQuat(rot_euler.x, rot_euler.y, fake_angle);
            edited = true;
        }*/

    static int* knifeSync = SkyComponent::getCore()->getConfig().getInt("SKY.enable_knife_sync");

    if (packet.Weapon == PlayerWeapon_Night_Vis_Goggles || packet.Weapon == PlayerWeapon_Thermal_Goggles) {
        packet.Keys &= ~4;
    } else if (packet.Weapon == PlayerWeapon_Knife && !*knifeSync) {
        packet.Keys &= ~128;
    }

    // Rewriting the incoming packet.
    packet.rewrite(bs);

    player_data->SetLastWeapon(static_cast<PlayerWeapon>(packet.Weapon));
    player_data->SetLastSyncType(E_PLAYER_SYNC);
    return true;
}

bool AimSync::received(IPlayer& peer, NetworkBitStream& bs)
{
    GET_SKY_DATA(peer, true)
    Packet::PlayerAimSync packet;
    NetworkBitStream& last_bs = player_data->GetSyncBitStream(E_AIM_SYNC);

    if (player_data->IsSyncFrozen(E_AIM_SYNC) || player_data->IsSyncFrozen(E_ALL_SYNC)) {
        packet.read(last_bs);
    } else {
        packet.read(bs);

        // Storing latest packet data.
        packet.rewrite(last_bs);
    }

    // Fix first-person up/down aim sync
    PlayerWeapon weapon = player_data->GetLastWeapon();
    if (weapon == PlayerWeapon_Sniper || weapon == PlayerWeapon_RocketLauncher || weapon == PlayerWeapon_HeatSeeker || weapon == PlayerWeapon_Camera) {

        packet.AimZ = -packet.CamFrontVector[2];

        if (packet.AimZ > 1.0f) {
            packet.AimZ = 1.0f;
        } else if (packet.AimZ < -1.0f) {
            packet.AimZ = -1.0f;
        }
    }

    if (player_data->GetInfiniteAmmo()) {
        packet.CamZoom = 2;
    }

    // Rewriting the incoming packet.
    packet.rewrite(bs);

    player_data->SetLastSyncType(E_AIM_SYNC);
    return true;
}

bool VehicleSync::received(IPlayer& peer, NetworkBitStream& bs)
{

    GET_SKY_DATA(peer, true)
    Packet::PlayerVehicleSync packet;
    NetworkBitStream& last_bs = player_data->GetSyncBitStream(E_VEHICLE_SYNC);

    if (player_data->IsSyncFrozen(E_VEHICLE_SYNC) || player_data->IsSyncFrozen(E_ALL_SYNC)) {
        packet.read(last_bs);
    } else {
        packet.read(bs);

        // Storing latest packet data.
        packet.rewrite(last_bs);
    }

    if (packet.WeaponID > PlayerWeapon::PlayerWeapon_Parachute || (packet.WeaponID > PlayerWeapon::PlayerWeapon_Moltov && packet.WeaponID < PlayerWeapon::PlayerWeapon_Colt45)) {
        packet.WeaponID = PlayerWeapon::PlayerWeapon_Fist;
    }

    if (player_data->GetBlockKeySync()) {
        packet.Keys = 0;
    }

    float fake_health = player_data->GetFakeHealth();
    if (player_data->GetFakeHealth() != 255) {
        packet.PlayerHealthArmour[0] = fake_health;
    }

    float fake_armour = player_data->GetFakeArmour();
    if (fake_armour != 255) {
        packet.PlayerHealthArmour[1] = fake_armour;
    }

    player_data->SetLastWeapon(static_cast<PlayerWeapon>(packet.WeaponID));

    // Rewriting the incoming packet.
    packet.rewrite(bs);

    player_data->SetLastSyncType(E_VEHICLE_SYNC);
    return true;
}

bool PassengerSync::received(IPlayer& peer, NetworkBitStream& bs)
{
    GET_SKY_DATA(peer, true)
    Packet::PlayerPassengerSync packet;
    NetworkBitStream& last_bs = player_data->GetSyncBitStream(E_PASSENGER_SYNC);

    if (player_data->IsSyncFrozen(E_PASSENGER_SYNC) || player_data->IsSyncFrozen(E_ALL_SYNC)) {
        packet.read(last_bs);
    } else {
        packet.read(bs);

        // Storing latest packet data.
        packet.rewrite(last_bs);
    }

    if (packet.WeaponID > PlayerWeapon::PlayerWeapon_Parachute || (packet.WeaponID > PlayerWeapon::PlayerWeapon_Moltov && packet.WeaponID < PlayerWeapon::PlayerWeapon_Colt45)) {
        packet.WeaponID = PlayerWeapon::PlayerWeapon_Fist;
    }

    if (player_data->GetBlockKeySync()) {
        packet.Keys = 0;
    }

    float fake_health = player_data->GetFakeHealth();
    if (player_data->GetFakeHealth() != 255) {
        packet.HealthArmour[0] = fake_health;
    }

    float fake_armour = player_data->GetFakeArmour();
    if (fake_armour != 255) {
        packet.HealthArmour[1] = fake_armour;
    }

    player_data->SetLastWeapon(static_cast<PlayerWeapon>(packet.WeaponID));

    // Rewriting the incoming packet.
    packet.rewrite(bs);

    player_data->SetLastSyncType(E_PASSENGER_SYNC);
    return true;
}

bool SpectatorSync::received(IPlayer& peer, NetworkBitStream& bs)
{
    GET_SKY_DATA(peer, true)

    Packet::PlayerPassengerSync packet;
    NetworkBitStream& last_bs = player_data->GetSyncBitStream(E_SPECTATING_SYNC);

    if (player_data->IsSyncFrozen(E_SPECTATING_SYNC) || player_data->IsSyncFrozen(E_ALL_SYNC)) {
        packet.read(last_bs);

        // Rewriting the incoming packet.
        packet.rewrite(bs);
    } else {
        // Storing latest packet data.
        packet.rewrite(last_bs);
    }

    player_data->SetLastSyncType(E_SPECTATING_SYNC);
    return true;
}