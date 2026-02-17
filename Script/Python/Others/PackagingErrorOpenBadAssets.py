"""Script to open all failed assets to recompile them"""

import unreal

error_text = """
LogProperty: Error: FStructProperty::Serialize Loading: Property 'StructProperty /Game/Blueprints/ECR/Data/Missions/Implementation/B_GM_MasterGameMission.B_GM_MasterGameMission_C:Get Faction Name:CallFunc_Get_Faction_Data_Faction_Data'. Unknown structure.
LogProperty: Error: FStructProperty::Serialize Loading: Property 'StructProperty /Game/Blueprints/ECR/Data/Missions/Implementation/B_GM_MasterGameMission.B_GM_MasterGameMission_C:Spend Spawn Point Spawn Resource:CallFunc_Get_Faction_Data_Faction_Data'. Unknown structure.
LogProperty: Error: FStructProperty::Serialize Loading: Property 'StructProperty /Game/Blueprints/ECR/Data/Missions/Implementation/B_GM_MasterGameMission.B_GM_MasterGameMission_C:Spend Spawn Point Spawn Resource:CallFunc_Get_Faction_Data_Faction_Data_1'. Unknown structure.
LogProperty: Error: FStructProperty::Serialize Loading: Property 'StructProperty /Game/Blueprints/ECR/Controllers/InMatchBotController.InMatchBotController_C:Spawn Bot Character:CallFunc_Get_Faction_Data_Faction_Data'. Unknown structure.
LogProperty: Error: FStructProperty::Serialize Loading: Property 'StructProperty /Game/Blueprints/ECR/Infos/InMatchGameState.InMatchGameState_C:Best Players Achievements.Best Players Achievements'. Unknown structure.
LogProperty: Error: FStructProperty::Serialize Loading: Property 'StructProperty /Game/Blueprints/ECR/Infos/InMatchGameState.InMatchGameState_C:Get Best Player Info:App'. Unknown structure.
LogProperty: Error: FStructProperty::Serialize Loading: Property 'StructProperty /Game/Blueprints/ECR/Infos/InMatchGameState.InMatchGameState_C:Set Best Players Roles:Result.Result'. Unknown structure.
LogProperty: Error: FStructProperty::Serialize Loading: Property 'StructProperty /Game/Blueprints/ECR/Infos/InMatchGameState.InMatchGameState_C:Set Best Players Roles:CallFunc_Get_Best_Player_Info_App'. Unknown structure.
LogProperty: Error: FStructProperty::Serialize Loading: Property 'StructProperty /Game/Blueprints/ECR/Infos/InMatchGameState.InMatchGameState_C:Set Best Players Roles:CallFunc_Get_Best_Player_Info_App_1'. Unknown structure.
LogProperty: Error: FStructProperty::Serialize Loading: Property 'StructProperty /Game/Blueprints/ECR/Infos/InMatchGameState.InMatchGameState_C:Set Best Players Roles:CallFunc_Get_Best_Player_Info_App_2'. Unknown structure.
LogProperty: Error: FStructProperty::Serialize Loading: Property 'StructProperty /Game/Blueprints/ECR/Infos/InMatchGameState.InMatchGameState_C:Set Best Players Roles:CallFunc_Get_Best_Player_Info_App_3'. Unknown structure.
LogProperty: Error: FStructProperty::Serialize Loading: Property 'StructProperty /Game/Blueprints/ECR/Infos/InMatchGameState.InMatchGameState_C:Set Best Players Roles:CallFunc_Get_Best_Player_Info_App_4'. Unknown structure.
LogProperty: Error: FStructProperty::Serialize Loading: Property 'StructProperty /Game/Blueprints/ECR/Infos/InMatchGameState.InMatchGameState_C:Set Best Players Roles:CallFunc_Get_Best_Player_Info_App_5'. Unknown structure.
LogProperty: Error: FStructProperty::Serialize Loading: Property 'StructProperty /Game/Blueprints/ECR/Infos/InMatchGameState.InMatchGameState_C:Set Best Players Roles:K2Node_MakeStruct_StructMatchEndBestPlayers'. Unknown structure.
LogProperty: Error: FStructProperty::Serialize Loading: Property 'StructProperty /Game/GUI/Widgets/InMatch/HUD/CharacterAttributes/W_IM_DeathScreen.W_IM_DeathScreen_C:ExecuteUbergraph_W_IM_DeathScreen:CallFunc_Get_Faction_Data_Faction_Data'. Unknown structure.
LogProperty: Error: FStructProperty::Serialize Loading: Property 'StructProperty /Game/GUI/Widgets/InMatch/Menus/BattleReport/W_BR_HallOfFame.W_BR_HallOfFame_C:ExecuteUbergraph_W_BR_HallOfFame:CallFunc_Array_Get_Item_1'. Unknown structure.
LogProperty: Error: FStructProperty::Serialize Loading: Property 'StructProperty /Game/GUI/Widgets/InMatch/Menus/BattleReport/W_HF_Item.W_HF_Item_C:ExecuteUbergraph_W_HF_Item:CallFunc_Get_Faction_Data_Faction_Data'. Unknown structure.
LogProperty: Error: FStructProperty::Serialize Loading: Property 'StructProperty /Game/GUI/Widgets/InMatch/Menus/BattleReport/W_IM_VictoryDefeatScreen.W_IM_VictoryDefeatScreen_C:Get Best Players Animation Duration:CallFunc_Array_Get_Item_1'. Unknown structure.
LogProperty: Error: FStructProperty::Serialize Loading: Property 'StructProperty /Game/GUI/Widgets/InMatch/Menus/PlayersTableMenu/W_C_PT_AllianceStats.W_C_PT_AllianceStats_C:Update Factions Text:CallFunc_Get_Faction_Data_Faction_Data'. Unknown structure.
LogProperty: Error: FStructProperty::Serialize Loading: Property 'StructProperty /Game/GUI/Widgets/InMatch/Menus/PlayersTableMenu/W_C_AS_PlayerStats.W_C_AS_PlayerStats_C:Update Faction Image:CallFunc_Get_Faction_Data_Faction_Data'. Unknown structure.
LogProperty: Error: FStructProperty::Serialize Loading: Property 'StructProperty /Game/GUI/Widgets/InMatch/Menus/SpawnMenu/W_C_SM_LoadoutSelection.W_C_SM_LoadoutSelection_C:ExecuteUbergraph_W_C_SM_LoadoutSelection:CallFunc_Get_Faction_Data_Faction_Data'. Unknown structure.
LogProperty: Error: FStructProperty::Serialize Loading: Property 'StructProperty /Game/GUI/Widgets/Menus/MainMenu/MainPanel/Components/W_C_CA_FactionResult.W_C_CA_FactionResult_C:ExecuteUbergraph_W_C_CA_FactionResult:CallFunc_Get_Faction_Data_Faction_Data'. Unknown structure.
LogProperty: Error: FStructProperty::Serialize Loading: Property 'StructProperty /Game/GUI/Widgets/InMatch/HUD/ControlPoints/W_C_MS_ControlPoint.W_C_MS_ControlPoint_C:Refresh Data:CallFunc_Get_Faction_Data_Faction_Data'. Unknown structure.
LogProperty: Error: FStructProperty::Serialize Loading: Property 'StructProperty /Game/GUI/Widgets/Common/Factions/W_C_Common_FactionsBox.W_C_Common_FactionsBox_C:Refresh Images:CallFunc_Get_Faction_Data_Faction_Data'. Unknown structure.
LogProperty: Error: FStructProperty::Serialize Loading: Property 'StructProperty /Game/GUI/Widgets/Menus/MainMenu/MainPanel/Components/W_C_MM_DailyTask.W_C_MM_DailyTask_C:ExecuteUbergraph_W_C_MM_DailyTask:CallFunc_Get_Daily_Task_Data_Res'. Unknown structure.
LogProperty: Error: FStructProperty::Serialize Loading: Property 'StructProperty /Game/GUI/Widgets/Menus/MainMenu/Advancement/FactionSkillTrees/W_M_C_AD_SkillTree_Abstract.W_M_C_AD_SkillTree_Abstract_C:ExecuteUbergraph_W_M_C_AD_SkillTree_Abstract:CallFunc_Get_Faction_Data_Faction_Data'. Unknown structure.
LogProperty: Error: FStructProperty::Serialize Loading: Property 'StructProperty /Game/GUI/Widgets/Menus/MainMenu/Matches/Matchmaking/W_M_C_MM_PartyInvitePopup.W_M_C_MM_PartyInvitePopup_C:ExecuteUbergraph_W_M_C_MM_PartyInvitePopup:CallFunc_Get_Faction_Data_Faction_Data'. Unknown structure.
LogProperty: Error: FStructProperty::Serialize Loading: Property 'StructProperty /Game/GUI/Widgets/Menus/MainMenu/Matches/Matchmaking/W_M_C_MM_PartyMember.W_M_C_MM_PartyMember_C:Update Faction :CallFunc_Get_Faction_Data_Faction_Data'. Unknown structure.
LogProperty: Error: FStructProperty::Serialize Loading: Property 'StructProperty /Game/GUI/Widgets/Menus/MainMenu/Matches/P2P/MatchCreation/W_M_C_MW_CreateMatch_New.W_M_C_MW_CreateMatch_New_C:Get Factions to Short Names:CallFunc_Get_Faction_Data_Faction_Data'. Unknown structure.
LogProperty: Error: FStructProperty::Serialize Loading: Property 'StructProperty /Game/GUI/Widgets/Menus/MainMenu/Matches/P2P/MatchCreation/W_M_C_MC_Step4Factions_New.W_M_C_MC_Step4Factions_New_C:Perform Contains Char Faction Check:CallFunc_Get_Faction_Data_Faction_Data'. Unknown structure.
LogProperty: Error: FStructProperty::Serialize Loading: Property 'StructProperty /Game/GUI/Widgets/Menus/MainMenu/Matches/P2P/MatchCreation/Step4Factions/W_C_S4F_FactionCandidate_New.W_C_S4F_FactionCandidate_New_C:Faction Data'. Unknown structure.
LogProperty: Error: FStructProperty::Serialize Loading: Property 'StructProperty /Game/GUI/Widgets/Menus/MainMenu/Matches/P2P/MatchCreation/Step4Factions/W_C_S4F_DefaultFactionsList_New.W_C_S4F_DefaultFactionsList_New_C:Create And Add Faction Candidate:Data'. Unknown structure.
LogProperty: Error: FStructProperty::Serialize Loading: Property 'StructProperty /Game/GUI/Widgets/Menus/MainMenu/Matches/P2P/MatchCreation/Step4Factions/W_C_S4F_DefaultFactionsList_New.W_C_S4F_DefaultFactionsList_New_C:Refresh Available Factions:CallFunc_Get_Faction_Data_Faction_Data'. Unknown structure.
LogProperty: Error: FStructProperty::Serialize Loading: Property 'StructProperty /Game/GUI/Widgets/Menus/MainMenu/Matches/P2P/MatchCreation/Step4Factions/W_C_S4F_FactionCandidate_Full_New.W_C_S4F_FactionCandidate_Full_New_C:ExecuteUbergraph_W_C_S4F_FactionCandidate_Full_New:CallFunc_Get_Faction_Data_Faction_Data'. Unknown structure.
LogProperty: Error: FStructProperty::Serialize Loading: Property 'StructProperty /Game/GUI/Widgets/Menus/MainMenu/Matches/P2P/MatchCreation/W_M_C_MC_Step5Settings_New.W_M_C_MC_Step5Settings_New_C:Setup Capacity Slider:Faction Data'. Unknown structure.
LogProperty: Error: FStructProperty::Serialize Loading: Property 'StructProperty /Game/GUI/Widgets/Menus/MainMenu/Matches/P2P/MatchCreation/W_M_C_MC_Step5Settings_New.W_M_C_MC_Step5Settings_New_C:Setup Capacity Slider:CallFunc_Get_Faction_Data_Faction_Data'. Unknown structure.
LogProperty: Error: FStructProperty::Serialize Loading: Property 'StructProperty /Game/GUI/Widgets/Menus/MainMenu/Matches/P2P/MatchCreation/W_M_C_MC_Step5Settings_New.W_M_C_MC_Step5Settings_New_C:Setup Capacity Slider:CallFunc_Get_Faction_Data_Faction_Data_1'. Unknown structure.
LogProperty: Error: FStructProperty::Serialize Loading: Property 'StructProperty /Game/GUI/Widgets/Menus/MainMenu/Matches/P2P/MatchSearch/W_M_C_MS_Faction.W_M_C_MS_Faction_C:Manual Refresh:CallFunc_Get_Faction_Data_Faction_Data'. Unknown structure.
LogProperty: Error: FStructProperty::Serialize Loading: Property 'StructProperty /Game/GUI/Widgets/InMatch/Menus/BattleReport/W_BR_Icon.W_BR_Icon_C:ExecuteUbergraph_W_BR_Icon:CallFunc_Get_Faction_Data_Faction_Data'. Unknown structure.
LogProperty: Error: FStructProperty::Serialize Loading: Property 'StructProperty /Game/GUI/Widgets/InMatch/HUD/W_IM_AllyWatchScreen.W_IM_AllyWatchScreen_C:Update Faction Images:CallFunc_Get_Faction_Data_Faction_Data'. Unknown structure.
LogProperty: Error: FStructProperty::Serialize Loading: Property 'StructProperty /Game/GUI/Widgets/InMatch/HUD/GameplayNotifiesCenter/W_C_GNCE_Item.W_C_GNCE_Item_C:Manual Refresh:CallFunc_Get_Faction_Data_Faction_Data'. Unknown structure.
LogProperty: Error: FStructProperty::Serialize Loading: Property 'StructProperty /Game/GUI/Widgets/InMatch/HUD/Minimap/W_C_HUD_Minimap.W_C_HUD_Minimap_C:ExecuteUbergraph_W_C_HUD_Minimap:CallFunc_Get_Faction_Data_Faction_Data'. Unknown structure.
LogProperty: Error: FStructProperty::Serialize Loading: Property 'StructProperty /Game/GUI/Widgets/InMatch/HUD/WeaponInterface/Character/Ammo/W_C_WI_Ammo.W_C_WI_Ammo_C:ExecuteUbergraph_W_C_WI_Ammo:CallFunc_Get_Faction_Data_Faction_Data'. Unknown structure.
"""

wanted_prefix = "LogProperty: Error: FStructProperty::Serialize Loading: Property"
for line in error_text.splitlines():
    if line.startswith(wanted_prefix):
        line = line.replace(wanted_prefix, "")
        line = line.replace("'StructProperty ", "")
        asset_path = line.split(".")[0].strip()

        asset = unreal.EditorAssetLibrary.load_asset(asset_path)
        a = unreal.AssetEditorSubsystem()
        a.open_editor_for_assets([asset])
