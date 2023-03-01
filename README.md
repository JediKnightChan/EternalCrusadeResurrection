# Eternal Crusade: Resurrection

[![Open Source? Yes!](https://badgen.net/badge/Open%20Source%20%3F/Yes%21/blue?icon=github)](https://github.com/JediKnightChan/EternalCrusadeResurrection/)
[![Website eternal-crusade.com](https://img.shields.io/website-up-down-green-red/https/eternal-crusade.com.svg)](https://eternal-crusade.com/)
[![Discord](https://badgen.net/badge/icon/discord?icon=discord&label)](https://discord.gg/Jzs3Bp3WCK)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/1e8058e9e34c44f88a501d0dff789ea0)](https://www.codacy.com/gh/JediKnightChan/EternalCrusadeResurrection/dashboard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=JediKnightChan/EternalCrusadeResurrection&amp;utm_campaign=Badge_Grade)

![](https://eternal-crusade.com/dist/images/ec.webp)

ECR is an attempt of resurrection of an online third-person shooter video game Eternal Crusade.

This project can be used as a template for creating a similar genre game, and ECRCommon module contains functionality
that may be useful for a wide range of games.

## Features
-   C++ Unreal Engine 5 project
-   Customization system for modular characters (material customization, mesh customization)
-   Epic Online Services P2P matchmaking (users can create their own matches)
-   Python Scripts for automation of restoring non-exportable or hardly-exportable data for an unpacked UE4 game (socket data, maps data, materials, references to materials within meshes)
-   Enhanced Input Subsystem is used for input (borrowed from Lyra)
-   3rd person multiplayer shooter and melee combat system based on Lyra's GAS

### GAS differences from Lyra (5.0)
-   A better nested directory structure
-   [Fixed](https://github.com/JediKnightChan/EternalCrusadeResurrection/commit/2990e9dba32ed76332775ed27df2977768a5d257) a bug with improper spread cooldown activation on ranged weapons
-   Several attributes are used to consume damage (First shield, then health, then bleeding health)
-   [QuickBar component](https://github.com/JediKnightChan/EternalCrusadeResurrection/blob/master/Source/ECR/Public/Gameplay/Equipment/ECRQuickBarComponent.h) has multiple channels (eg one for ranged weapons, one for melee)
-   ECREquipmentManagerComponent is capable of hiding equipment by visibility channels (eg LeftHand, RightHand for two-handed weapon)
-   PawnData is defined in GameState and on Character as a spawn option, so it can be customizable, unlike Lyra, where it's defined by experience and is the same for all players
-   Shooting uses advanced tracing to get a point on the camera vector where pawn should aim from weapon actor, unlike Lyra, where shooting is done from camera
-   GAS debugger correctly works on 3rd page, checking costs on abilities CDOs without producing errors in the log
-   Gameplay Abilities can asynchronously load montages for different skeletons using cosmetic tags without having to create multiple GA instances
-   Advanced interaction system allows interaction options to be set by actors in blueprints, makes possible interactions via input tags and overriding input mappings
-   Advanced melee combat prediction system

![](https://eternal-crusade.com/dist/images/github/melee_prediction.jpg)
