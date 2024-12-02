# Eternal Crusade: Resurrection

[![Open Source? Yes!](https://badgen.net/badge/Open%20Source%20%3F/Yes%21/blue?icon=github)](https://github.com/JediKnightChan/EternalCrusadeResurrection/)
[![Website eternal-crusade.com](https://img.shields.io/website-up-down-green-red/https/eternal-crusade.com.svg)](https://eternal-crusade.com/)
[![Discord](https://badgen.net/badge/icon/discord?icon=discord&label)](https://discord.gg/Jzs3Bp3WCK)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/1e8058e9e34c44f88a501d0dff789ea0)](https://www.codacy.com/gh/JediKnightChan/EternalCrusadeResurrection/dashboard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=JediKnightChan/EternalCrusadeResurrection&amp;utm_campaign=Badge_Grade)

![](https://eternal-crusade.com/dist/images/ec.webp)

ECR is the resurrection of an online third-person shooter video game Eternal Crusade.

This project can be used as a template for creating a similar genre game, and ECRCommon module contains functionality
that may be useful for a wide range of games.

Check out our [Wiki](https://github.com/JediKnightChan/EternalCrusadeResurrection/wiki) for using this game code "core" and some useful UE dev tips!

## Features

- C++ Unreal Engine 5 project
- Customization system for modular characters (material customization, mesh customization)
- Epic Online Services P2P matchmaking (users can create their own matches), including Linux Dedicated Servers, Friends List, Parties
- Python Scripts for automation of restoring non-exportable or hardly-exportable data for an unpacked UE4 game (socket
  data, maps data, materials, references to materials within meshes)
- Enhanced Input Subsystem is used for input (borrowed from Lyra)
- 3rd person multiplayer shooter and melee combat system based on Lyra's GAS, including combat vehicles
- Plugin for HTTP requests
- Replay Subsystem with all needed functions for Blueprint replay setup
- Gameplay Analytics Subsystem in a separate plugin for collecting them and sending to API endpoint
- Cross-platform code (was built for Windows, Linux, Android)

### GAS differences from Lyra (5.0)

#### Large changes

- Vehicles are supported as pawns (ECRWheeledVehiclePawn)
- [Ability Queue System](https://github.com/JediKnightChan/EternalCrusadeResurrection/commit/d9096523e16c7203f4ac2663be893c6969a5f803)
  allows to grab input while another ability (like stun) is active
- PawnData elements are defined in GameState and on Character as a spawn option, so it can be customizable, unlike Lyra,
  where it's defined by experience and is the same for all players

#### Medium changes

- Several attributes are used to consume damage (First shield, then health, then bleeding health)
- [QuickBar component](https://github.com/JediKnightChan/EternalCrusadeResurrection/blob/master/Source/ECR/Public/Gameplay/Equipment/ECRQuickBarComponent.h)
  has multiple channels (e.g. one for ranged weapons, one for melee), can make many "quick bars"
- ECREquipmentManagerComponent is capable of hiding equipment by visibility channels (e.g. LeftHand, RightHand for
  two-handed weapon)
- Gameplay Abilities can asynchronously load montages for different skeletons using cosmetic tags without having to
  create multiple ability instances for them
- Can temporarily limit yaw and pitch in Pawn Control Component (former Hero Component), Lyra always limits pitch in camera mode
- Advanced interaction system allows interaction options to be set by actors in blueprints, makes possible interactions
  via input tags and overriding input mappings
- Advanced melee combat prediction system

#### Fixes and small changes

- A better nested directory structure
- [Fixed](https://github.com/JediKnightChan/EternalCrusadeResurrection/commit/2990e9dba32ed76332775ed27df2977768a5d257)
  a bug with improper spread cooldown activation on ranged weapons
- Shooting uses advanced tracing to get a point on the camera vector where pawn should aim from weapon actor, unlike
  Lyra, where shooting is done from camera
- GAS debugger correctly works on 3rd page, checking costs on abilities CDOs without producing errors in the log

![](https://eternal-crusade.com/dist/images/github/melee_prediction.jpg)

# GIT policy

In both this repo and `ECRContent` (in private access) `master` branch is locked, all work should be done
after creating a new branch based on `dev`

# Related repositories

## Currently maintained
- [ECRLauncher](https://github.com/JediKnightChan/ECRLauncher) (Public): Launcher for the game, can show news, patch
notes, install the whole game and patches, verify integrity
- [ECRTools](https://github.com/JediKnightChan/ECRTools) (Public): Docker wrapper around Linux dedicated server, discord bot for running / stopping dedicated server,
  launcher data, game versioning data, scripts for statistical processing of gameplay analytics, deployment
- [ECRContent](https://github.com/JediKnightChan/ECRContent/) (Private): Content folder of the game
- [ECRFModSounds](https://github.com/JediKnightChan/ECRFModSounds) (Private): FMod project for the ECR sounds

## Archived
- [ECRSoundSorting](https://github.com/JediKnightChan/ECRSoundSorting) (Public): Code of the site which was used for 
labeling unsorted sounds extracted with basic .bnk .wem converters. Django + Vue.js (CoreUI) + Nginx + Docker compose
- [WWise-IntegrationDemo-EventExtractor](https://github.com/Waagheur/WWise-IntegrationDemo-EventExtactor) (Public): WWise
integration demo turned into an extraction tool (knowing the name of the WWise event, such as Play_Fire in the sound 
bank SNB_DefaultBolter, can extract it)
