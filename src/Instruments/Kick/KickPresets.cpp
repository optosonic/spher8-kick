#include "KickParams.h"

namespace atrium::kick
{
    namespace
    {
        struct Name
        {
            const char* category;
            const char* name;
        };

        constexpr Name kNames[kPresetCount] = {
            { "Utility", "INIT" },
            { "House", "Deep House" },
            { "House", "Classic House" },
            { "House", "Minimal House" },
            { "House", "Punchy House" },
            { "Techno", "Raw Techno" },
            { "Techno", "Industrial Techno" },
            { "Techno", "Hypnotic Techno" },
            { "Techno", "Rumble Foundation" },
            { "EDM", "Festival Punch" },
            { "EDM", "Big Room" },
            { "EDM", "Progressive" },
            { "EDM", "Electro" },
            { "Trance", "Classic Trance" },
            { "Trance", "Psytrance" },
            { "Trance", "Hard Trance" },
            { "808 / Bass", "Classic 808" },
            { "808 / Bass", "Long 808" },
            { "808 / Bass", "Distorted 808" },
            { "808 / Bass", "Sub Drop" },
            { "Hard", "Hardstyle Punch" },
            { "Hard", "Hardcore Distortion" },
            { "Hard", "Gabber" },
            { "Hard", "Industrial Impact" },
            { "Experimental", "Metallic Kick" },
            { "Experimental", "FM Kick" },
            { "Experimental", "Glitch Kick" },
            { "Experimental", "Cinematic Boom" },
            { "Experimental", "Reverse Impact" },
            { "Variation", "Tight Click" },
            { "Variation", "Soft Acoustic" },
            { "Variation", "Sub Only" },
            { "808 / Bass", "Trap 808" },
            { "Variation", "Hip Hop Dust" },
            { "Variation", "Room Thud" },
            { "Experimental", "Noise Burst" },
            { "Techno", "Rumble Whoop" },
            { "Experimental", "Boom Whoop" },
            { "Experimental", "Double Whoop" },
        };
    }

    const char* presetName (int index) noexcept
    {
        if (index < 0 || index >= kPresetCount)
            return "INIT";
        return kNames[index].name;
    }

    const char* presetCategory (int index) noexcept
    {
        if (index < 0 || index >= kPresetCount)
            return "Utility";
        return kNames[index].category;
    }

    Targets presetTargets (int index) noexcept
    {
        Targets t = defaults();
        switch (index)
        {
            case 0:
                break;
            case 1:
                t.tune = 46.0f; t.decay = 520.0f; t.clickLevel = 0.22f; t.subLevel = 0.78f;
                t.drive = 0.08f; t.punch = 0.20f; t.impactSt = 28.0f; t.bodySt = 10.0f; t.bodyMs = 140.0f;
                t.atkEmph = 0.10f; t.clickEnh = 0.05f;
                break;
            case 2:
                t.tune = 55.0f; t.decay = 240.0f; t.clickLevel = 0.52f; t.drive = 0.28f; t.punch = 0.58f;
                t.bodyWave = 2.0f; t.impactSt = 32.0f; t.impactMs = 5.0f; t.bodySt = 12.0f; t.satMode = 1.0f;
                break;
            case 3:
                t.tune = 58.0f; t.decay = 145.0f; t.clickLevel = 0.16f; t.subLevel = 0.32f; t.drive = 0.04f;
                t.punch = 0.22f; t.impactSt = 16.0f; t.bodyMs = 48.0f; t.bodyLevel = 0.75f; t.clickMs = 2.2f;
                break;
            case 4:
                t.tune = 52.0f; t.decay = 185.0f; t.clickLevel = 0.72f; t.punch = 0.88f; t.drive = 0.36f;
                t.bodyWave = 3.0f; t.atkEmph = 0.62f; t.clickType = 2.0f; t.clickHz = 2400.0f; t.impactMs = 4.0f;
                break;
            case 5:
                t.tune = 48.0f; t.decay = 310.0f; t.drive = 0.55f; t.satMode = 2.0f; t.bodyWave = 1.0f;
                t.clickLevel = 0.48f; t.punch = 0.42f; t.bodyHarm = 0.55f; t.evenOdd = 0.15f;
                break;
            case 6:
                t.tune = 43.0f; t.decay = 390.0f; t.drive = 0.74f; t.satMode = 3.0f; t.clickType = 4.0f;
                t.clickLevel = 0.82f; t.fold = 0.48f; t.subLevel = 0.84f; t.hp = 42.0f; t.clip = 0.28f;
                break;
            case 7:
                t.tune = 50.0f; t.decay = 680.0f; t.clickLevel = 0.10f; t.drive = 0.05f; t.subLevel = 0.72f;
                t.impactSt = 12.0f; t.bodyMs = 210.0f; t.punch = 0.12f; t.bodyLevel = 0.70f; t.clickMs = 6.0f;
                break;
            case 8:
                t.tune = 38.0f; t.decay = 980.0f; t.subLevel = 0.96f; t.bodyLevel = 0.42f; t.clickLevel = 0.06f;
                t.drive = 0.10f; t.bodyMs = 280.0f; t.impactSt = 18.0f; t.subDecay = 1100.0f; t.punch = 0.08f;
                break;
            case 9:
                t.tune = 50.0f; t.decay = 165.0f; t.punch = 0.96f; t.clickLevel = 0.88f; t.clickType = 2.0f;
                t.drive = 0.42f; t.impactSt = 44.0f; t.impactMs = 3.8f; t.atkEmph = 0.75f; t.clickHz = 3600.0f;
                break;
            case 10:
                t.tune = 44.0f; t.decay = 275.0f; t.drive = 0.64f; t.satMode = 1.0f; t.punch = 0.76f;
                t.clickLevel = 0.66f; t.impactSt = 40.0f; t.bodySt = 18.0f; t.bodyMs = 120.0f; t.clip = 0.18f;
                break;
            case 11:
                t.tune = 53.0f; t.decay = 360.0f; t.drive = 0.14f; t.clickLevel = 0.28f; t.punch = 0.32f;
                t.bodyMs = 170.0f; t.subLevel = 0.58f; t.impactSt = 22.0f; t.tone = 0.05f;
                break;
            case 12:
                t.tune = 66.0f; t.decay = 110.0f; t.clickLevel = 0.78f; t.clickType = 5.0f; t.punch = 0.64f;
                t.drive = 0.34f; t.bodyWave = 3.0f; t.impactSt = 24.0f; t.impactMs = 2.8f; t.clickHz = 4100.0f;
                break;
            case 13:
                t.tune = 50.0f; t.decay = 255.0f; t.clickLevel = 0.40f; t.drive = 0.22f; t.punch = 0.46f;
                t.impactSt = 30.0f; t.bodySt = 16.0f; t.bodyMs = 100.0f; t.subLevel = 0.50f;
                break;
            case 14:
                t.tune = 49.0f; t.decay = 135.0f; t.clickLevel = 0.64f; t.clickHz = 4700.0f; t.punch = 0.72f;
                t.drive = 0.30f; t.impactMs = 3.2f; t.bodyMs = 42.0f; t.impactSt = 34.0f; t.clickMs = 2.4f;
                break;
            case 15:
                t.tune = 46.0f; t.decay = 195.0f; t.drive = 0.60f; t.punch = 0.82f; t.satMode = 1.0f;
                t.clickLevel = 0.70f; t.clip = 0.36f; t.impactSt = 38.0f; t.bodyHarm = 0.40f;
                break;
            case 16:
                t.tune = 55.0f; t.decay = 1150.0f; t.clickLevel = 0.04f; t.subLevel = 0.28f; t.drive = 0.03f;
                t.impactSt = 19.0f; t.bodySt = 7.0f; t.bodyMs = 190.0f; t.punch = 0.08f; t.bodyWave = 0.0f;
                t.clickOn = 0.0f; t.subDecay = 1200.0f;
                break;
            case 17:
                t.tune = 40.0f; t.decay = 2900.0f; t.clickOn = 0.0f; t.clickLevel = 0.0f; t.subLevel = 0.48f;
                t.drive = 0.0f; t.impactSt = 14.0f; t.bodyMs = 340.0f; t.bodySt = 6.0f; t.punch = 0.0f;
                t.release = 120.0f; t.comp = 0.0f; t.clip = 0.0f;
                break;
            case 18:
                t.tune = 46.0f; t.decay = 1450.0f; t.drive = 0.82f; t.satMode = 3.0f; t.clip = 0.58f;
                t.clickLevel = 0.18f; t.fold = 0.22f; t.subLevel = 0.42f; t.clipHard = 1.0f; t.evenOdd = 0.25f;
                break;
            case 19:
                t.tune = 33.0f; t.decay = 1850.0f; t.impactSt = 52.0f; t.impactMs = 28.0f; t.bodySt = 26.0f;
                t.bodyMs = 720.0f; t.subLevel = 0.88f; t.clickLevel = 0.12f; t.drive = 0.08f; t.subDecay = 2000.0f;
                break;
            case 20:
                t.tune = 51.0f; t.decay = 155.0f; t.punch = 1.0f; t.clickLevel = 0.96f; t.clickType = 2.0f;
                t.drive = 0.78f; t.satMode = 3.0f; t.impactSt = 54.0f; t.impactMs = 5.5f; t.clip = 0.42f;
                t.atkEmph = 0.92f; t.clickHz = 2800.0f;
                break;
            case 21:
                t.tune = 47.0f; t.decay = 210.0f; t.drive = 0.94f; t.satMode = 4.0f; t.fold = 0.82f;
                t.clip = 0.72f; t.clipHard = 1.0f; t.clickLevel = 0.74f; t.bodyWave = 1.0f; t.asym = 0.35f;
                break;
            case 22:
                t.tune = 57.0f; t.decay = 85.0f; t.drive = 0.88f; t.punch = 0.92f; t.clickLevel = 0.92f;
                t.clickType = 3.0f; t.satMode = 3.0f; t.impactSt = 46.0f; t.impactMs = 2.2f; t.clip = 0.64f;
                t.bodyLevel = 0.90f;
                break;
            case 23:
                t.tune = 36.0f; t.decay = 740.0f; t.clickType = 4.0f; t.clickLevel = 0.86f; t.drive = 0.66f;
                t.satMode = 2.0f; t.subLevel = 0.82f; t.bodyHarm = 0.72f; t.punch = 0.48f; t.bodyMs = 250.0f;
                t.bodyWave = 5.0f;
                break;
            case 24:
                t.tune = 110.0f; t.decay = 70.0f; t.bodyWave = 4.0f; t.clickType = 4.0f; t.clickLevel = 1.0f;
                t.bodyHarm = 1.0f; t.drive = 0.72f; t.satMode = 4.0f; t.fold = 0.70f; t.impactSt = 8.0f;
                t.bodySt = 2.0f; t.subLevel = 0.05f; t.clickHz = 6800.0f; t.clickMs = 12.0f; t.punch = 0.15f;
                break;
            case 25:
                t.tune = 60.0f; t.decay = 205.0f; t.bodyWave = 4.0f; t.bodyHarm = 0.95f; t.impactSt = 42.0f;
                t.bodySt = 22.0f; t.drive = 0.24f; t.clickType = 2.0f; t.clickLevel = 0.34f; t.bodyMs = 68.0f;
                break;
            case 26:
                t.tune = 92.0f; t.decay = 42.0f; t.bodyWave = 1.0f; t.clickType = 5.0f; t.clickLevel = 0.74f;
                t.clickMs = 1.1f; t.impactSt = 64.0f; t.impactMs = 1.8f; t.drive = 0.52f; t.bodySt = 0.0f;
                t.subLevel = 0.10f; t.punch = 0.28f;
                break;
            case 27:
                t.tune = 30.0f; t.decay = 3600.0f; t.attack = 22.0f; t.clickLevel = 0.22f; t.clickType = 1.0f;
                t.subLevel = 0.92f; t.drive = 0.06f; t.impactSt = 20.0f; t.bodyMs = 520.0f; t.bodySt = 12.0f;
                t.punch = 0.16f; t.release = 480.0f; t.subDecay = 3400.0f;
                break;
            case 28:
                t.tune = 48.0f; t.attack = 150.0f; t.decay = 680.0f; t.bodySt = -28.0f; t.bodyMs = 460.0f;
                t.impactSt = 0.0f; t.clickLevel = 0.12f; t.drive = 0.04f; t.sustain = 0.18f; t.release = 220.0f;
                t.punch = 0.0f; t.bodyShape = 1.0f;
                break;
            case 29:
                t.tune = 73.0f; t.decay = 78.0f; t.clickLevel = 0.90f; t.clickMs = 1.4f; t.clickHz = 5400.0f;
                t.subLevel = 0.18f; t.bodyLevel = 0.52f; t.punch = 0.68f; t.impactSt = 14.0f; t.impactMs = 1.6f;
                break;
            case 30:
                t.tune = 82.0f; t.decay = 640.0f; t.drive = 0.0f; t.clickType = 1.0f; t.clickLevel = 0.08f;
                t.clickHz = 900.0f; t.punch = 0.0f; t.subLevel = 0.62f; t.bodyLevel = 0.38f; t.satMode = 0.0f;
                t.tone = -0.70f; t.cutoff = 1400.0f; t.impactSt = 4.0f; t.bodyMs = 280.0f; t.atkEmph = 0.0f;
                break;
            case 31:
                t.tune = 41.0f; t.bodyLevel = 0.08f; t.subLevel = 1.0f; t.clickOn = 0.0f; t.clickLevel = 0.0f;
                t.drive = 0.0f; t.decay = 860.0f; t.impactSt = 6.0f; t.bodyMs = 30.0f; t.punch = 0.0f;
                t.subDecay = 900.0f; t.clip = 0.0f;
                break;
            case 32:
                t.tune = 45.0f; t.decay = 1680.0f; t.drive = 0.46f; t.satMode = 2.0f; t.clickLevel = 0.16f;
                t.subLevel = 0.58f; t.impactSt = 22.0f; t.bodySt = 10.0f; t.bodyMs = 210.0f; t.clip = 0.26f;
                break;
            case 33:
                t.tune = 70.0f; t.decay = 110.0f; t.clickType = 1.0f; t.clickLevel = 0.62f; t.clickMs = 22.0f;
                t.drive = 0.12f; t.satMode = 2.0f; t.punch = 0.08f; t.tone = -0.45f; t.cutoff = 2200.0f;
                t.subLevel = 0.78f; t.bodyLevel = 0.34f; t.impactSt = 6.0f; t.bodyMs = 24.0f;
                break;
            case 34:
                t.tune = 67.0f; t.decay = 175.0f; t.clickLevel = 0.34f; t.drive = 0.09f; t.punch = 0.40f;
                t.hp = 70.0f; t.bodyMs = 55.0f; t.impactSt = 20.0f; t.subLevel = 0.36f; t.bodyWave = 1.0f;
                break;
            case 35:
                t.tune = 96.0f; t.decay = 95.0f; t.bodyLevel = 0.22f; t.clickLevel = 1.0f; t.clickType = 0.0f;
                t.clickMs = 28.0f; t.clickHz = 1800.0f; t.drive = 0.28f; t.subOn = 0.0f; t.subLevel = 0.0f;
                t.punch = 0.18f; t.clickHp = 400.0f;
                break;
            case 36:
                t.tune = 40.0f; t.decay = 1500.0f; t.subDecay = 1700.0f; t.subLevel = 0.92f; t.bodyLevel = 0.48f;
                t.clickLevel = 0.16f; t.clickMs = 3.2f; t.impactSt = 24.0f; t.impactMs = 6.0f; t.bodySt = 6.0f;
                t.bodyMs = 70.0f; t.drive = 0.34f; t.punch = 0.22f; t.hp = 22.0f; t.cutoff = 9000.0f;
                t.followSub = 0.92f; t.followDrive = 0.80f; t.followLevel = 0.55f; t.followRes = 0.62f;
                t.followMs = 1100.0f; t.followShape = 1.0f; t.followT1 = 0.22f; t.followT2 = 0.58f;
                t.followSt0 = 0.0f; t.followSt1 = 5.0f; t.followSt2 = -6.0f; t.followSt3 = 9.0f;
                break;
            case 37:
                t.tune = 53.0f; t.decay = 980.0f; t.subDecay = 1100.0f; t.subLevel = 0.88f; t.bodyLevel = 0.70f;
                t.clickLevel = 0.38f; t.impactSt = 46.0f; t.impactMs = 5.0f; t.bodySt = 4.0f; t.bodyMs = 36.0f;
                t.drive = 0.42f; t.punch = 0.48f; t.satMode = 1.0f; t.followSub = 1.0f; t.followDrive = 0.78f;
                t.followLevel = 0.72f; t.followRes = 0.74f; t.followMs = 680.0f; t.followShape = 1.0f;
                t.followT1 = 0.24f; t.followT2 = 0.56f; t.followSt0 = -3.0f; t.followSt1 = 11.0f;
                t.followSt2 = -7.0f; t.followSt3 = 8.0f; t.subHarm = 0.16f;
                break;
            case 38:
                t.tune = 33.0f; t.decay = 2600.0f; t.subDecay = 2800.0f; t.bodyLevel = 0.22f; t.subLevel = 1.0f;
                t.clickLevel = 0.06f; t.clickOn = 1.0f; t.impactSt = 16.0f; t.impactMs = 8.0f; t.bodyMs = 180.0f;
                t.bodySt = 3.0f; t.drive = 0.24f; t.punch = 0.06f; t.followSub = 1.0f; t.followDrive = 0.48f;
                t.followLevel = 0.84f; t.followRes = 0.46f; t.followMs = 2100.0f; t.followShape = 1.0f;
                t.followT1 = 0.18f; t.followT2 = 0.62f; t.followSt0 = 0.0f; t.followSt1 = 8.0f;
                t.followSt2 = -3.0f; t.followSt3 = 12.0f; t.hp = 18.0f;
                break;
            default:
                break;
        }
        return t;
    }
}
