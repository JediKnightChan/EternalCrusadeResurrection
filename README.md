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
-   Fixed a bug with improper cooldown activation on ranged weapons
-   Several attributes are used to consume damage (First shield, then health, then bleeding health)
-   QuickBar component has multiple channels (eg one for ranged weapons, one for melee) and can hide all actors by channel
-   PawnData is defined in GameState and on Character as a spawn option, so it can be customizable, unlike Lyra, where it's defined by experience and is the same for all players
