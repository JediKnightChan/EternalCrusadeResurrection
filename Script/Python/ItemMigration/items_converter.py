import json
import os
import typing

import pandas as pd

# df = pd.read_csv("./data/new/items_example.csv")
with open("module_mapping.json", "r") as f:
    module_mapping = json.load(f)


def encode_obj(obj: typing.Any):
    if isinstance(obj, dict):
        if len(obj):
            string = ""
            for k, v in obj.items():
                string += f"{',' if string else ''}{k}={encode_obj(v)}"
            return f"({string})"
        else:
            return ""
    elif isinstance(obj, list):
        if len(obj):
            string = ""
            for el in obj:
                string += f"{',' if string else ''}{encode_obj(el)}"
            return f"({string})"
        else:
            return ""
    elif isinstance(obj, str) and obj.startswith("/Script/"):
        return str(obj)
    elif obj is None:
        return None
    else:
        return f"\"{str(obj)}\""


def make_row(row_i, name, customization_slot, min_level, eagles_cost, cea, desc, is_enabled=True,
             is_granted_by_default=False, is_purchasable=True, do_override_cma=False, cma=None, preview=None,
             preview_color_namespace="None", incompatibility_groups=None, allowed_loadouts=None):
    if cma is None:
        cma = make_cma([], [])
    if preview is None:
        preview = make_preview([], None)
    if allowed_loadouts is None:
        allowed_loadouts = []
    if incompatibility_groups is None:
        incompatibility_groups = []

    cea = encode_obj(cea)
    cma = encode_obj(cma)
    preview = encode_obj(preview)
    allowed_loadouts = encode_obj(allowed_loadouts)
    incompatibility_groups = encode_obj(incompatibility_groups)

    return {
        "---": row_i,
        "Name": name,
        "Customization Slot": customization_slot,
        "Is Enabled": str(is_enabled),
        "Is Granted By Default": str(is_granted_by_default),
        "Is Purchasable": str(is_purchasable),
        "Min Level": str(min_level),
        "Eagles Cost": str(eagles_cost),
        "Allowed Loadouts": allowed_loadouts,
        "CEA": cea,
        "Do Override CMA (Avoid!)": str(do_override_cma),
        "CMA Overrides": cma,
        "Incompatibility Groups": incompatibility_groups,
        "Description": desc,
        "Preview": preview,
        "Preview Color Namespace": preview_color_namespace
    }


def make_rule(assets, tags, key="Assets"):
    return {
        key: assets,
        "RequiredTags":
            {
                "GameplayTags": [
                    {
                        "TagName": tag
                    } for tag in tags
                ]
            }
    }


def make_cea(cea_rules, default_assets):
    return {
        "CeaRules": cea_rules,
        "DefaultAssets": default_assets
    }


def make_cma(cma_rules, default_assets):
    return {
        "CmaRules": cma_rules,
        "DefaultAssets": default_assets
    }


def make_preview(texture_rules, default_texture):
    return {
        "TextureRules": texture_rules,
        "DefaultTexture": default_texture
    }


def cla_to_new_table():
    cla_json = "./data/old/cla/json/CSM_Traitor_MKVI.json"
    common_name = "Mark VI Corvus Power Armour"
    common_index_prefix = "CSM_MKVI_Traitor_"

    with open(cla_json, "r") as f:
        data = json.load(f)
        cea_assets = data["cea"]

    rows = []
    slot_to_cea = {}
    for cea_asset in cea_assets:
        cea_asset_name = cea_asset.split(".")[2].strip("\"'")
        parsed_cea_name = cea_asset_name.replace("CEA_", "")
        module_name, module_type = parsed_cea_name.rsplit("_", 1)
        slot = module_mapping[module_type]
        slot_to_cea.setdefault(slot, []).append(cea_asset)

    for slot, ceas in slot_to_cea.items():
        row = make_row(common_index_prefix + slot,
                       common_name, slot, 1, 0, make_cea([], ceas), "",
                       is_granted_by_default=True, allowed_loadouts=["Any"])
        rows.append(row)

    df = pd.DataFrame(rows)
    # print(df)
    df.to_csv("./data/new/items_example3.csv", index=False, quoting=1)


def get_item_based_on_cla(given_cea_assets, slot, is_jumppack=False):
    given_assets_modules_to_assets = {}
    for given_cea_asset in given_cea_assets:
        given_cea_asset_name = given_cea_asset.split(".")[2].strip("\"'")
        given_parsed_cea_name = given_cea_asset_name.replace("CEA_", "")
        given_module_name, given_module_type = given_parsed_cea_name.rsplit("_", 1)
        given_assets_modules_to_assets[given_module_type] = given_cea_asset

    with open("./data/new/csm_cla.json", "r") as f:
        data = json.load(f)

    if is_jumppack:
        row_name = "CSM_MKVI_JumpPack_" + slot
        met_jumppack = False
        for rec in data:
            if rec["Name"] == row_name:
                met_jumppack = True
        if not met_jumppack:
            row_name = "CSM_MKVI_JumpPack_" + slot
    else:
        row_name = "CSM_MKVI_JumpPack_" + slot

    assets = []
    for rec in data:
        if rec["Name"] == row_name:
            assets = rec["CEA"]["DefaultAssets"]
    new_assets = []
    for cea_asset in assets:
        cea_asset_name = cea_asset.split(".")[2].strip("\"'")
        parsed_cea_name = cea_asset_name.replace("CEA_", "")
        module_name, module_type = parsed_cea_name.rsplit("_", 1)
        if module_type in given_assets_modules_to_assets:
            new_assets.append(given_assets_modules_to_assets[module_type])
        else:
            new_assets.append(cea_asset)

    for given_cea_asset in given_cea_assets:
        if given_cea_asset not in new_assets:
            new_assets.append(given_cea_asset)

    return new_assets


def subfaction_to_new_table():
    temp_slot_mapping = {
        "CEA_ApothecaryHelixStandard_SkeletalAttachment1": "RightShoulder"
    }
    loadout_mapping_fp = "./loadout_mapping_csm.json"
    subfaction_table = "./data/old/subfaction_tables/SubFactionData_WordBearers1.csv"
    subfaction_table2 = "./data/old/subfaction_tables/SubFactionData_WordBearers2.csv"

    subfaction_name = "Mark VI Corvus Power Armour (Word Bearers)"
    common_name = "Mark VI Corvus Power Armour"

    subfaction_index_prefix = "WB_MKVI_"
    common_index_prefix = "CSM_MKVI_"

    subfaction_pattern = "word"

    if os.path.exists(f"./data/new/csm_subfactions_common.csv"):
        common_items_df = pd.read_csv("./data/new/csm_subfactions_common.csv")
        existing_cea = list(common_items_df["CEA"])
        existing_cea_rownames = list(common_items_df["---"])
    else:
        common_items_df = pd.DataFrame()
        existing_cea = []
        existing_cea_rownames = []

    if os.path.exists(f"./data/new/{subfaction_pattern}.csv"):
        subfaction_items_df = pd.read_csv(f"./data/new/{subfaction_pattern}.csv")
        existing_cea += list(subfaction_items_df["CEA"])
        existing_cea_rownames += list(subfaction_items_df["---"])
    else:
        subfaction_items_df = pd.DataFrame()
    rows = []
    common_rows = []
    ceas_hashes = {}

    with open(loadout_mapping_fp, "r") as f:
        loadout_mapping = json.load(f)

    print(len(existing_cea))
    for rowname, cea in zip(existing_cea_rownames, existing_cea):
        ceas_hashes[hash(str(cea))] = rowname

    print(len(ceas_hashes))
    orig_df = pd.read_csv(subfaction_table)
    orig_df_new = pd.read_csv(subfaction_table2)
    loadouts_to_common_items = {}
    loadouts_to_subfaction_items = {}

    for i, row in orig_df.iterrows():
        loadout = row[0]
        loadout_common_items = []
        loadout_subfaction_items = []
        if loadout.lower() in ["terminator", "terminatorstandard"]:
            break
        cea_assets = row[2]
        cea_assets = cea_assets.strip("()").split(",")
        slot_to_cea = {}
        for cea_asset in cea_assets:
            cea_asset_name = cea_asset.split(".")[2].strip("\"'")
            parsed_cea_name = cea_asset_name.replace("CEA_", "")
            module_name, module_type = parsed_cea_name.rsplit("_", 1)

            if module_type in module_mapping:
                slot = module_mapping[module_type]
                slot_to_cea.setdefault(slot, []).append(cea_asset)
            else:
                if cea_asset_name in temp_slot_mapping:
                    slot = temp_slot_mapping[cea_asset_name]
                    slot_to_cea.setdefault(slot, []).append(cea_asset)
                else:
                    print(f"Slot for module type {module_type} from {cea_asset_name} not found!")
                    raise Exception

        for slot, ceas in slot_to_cea.items():
            is_jumppack = "jumppack" in loadout.lower()
            ceas = get_item_based_on_cla(ceas, slot, is_jumppack=is_jumppack)
            subfaction_pattern_met = any([subfaction_pattern.lower() in cea.lower() for cea in ceas])

            prefix = subfaction_index_prefix if subfaction_pattern_met else common_index_prefix
            row_name = prefix + loadout + "_" + slot

            ceas_hash = hash(encode_obj(make_cea([], ceas)))

            if ceas_hash in ceas_hashes:
                if subfaction_pattern_met:
                    loadout_subfaction_items.append(ceas_hashes[ceas_hash])
                else:
                    loadout_common_items.append(ceas_hashes[ceas_hash])
            else:
                if subfaction_pattern_met:
                    loadout_subfaction_items.append(row_name)
                else:
                    loadout_common_items.append(row_name)

            if ceas_hash in ceas_hashes:
                continue

            row = make_row(row_name,
                           subfaction_name if subfaction_pattern_met else common_name, slot,
                           1, 0, make_cea([], ceas), "",
                           is_granted_by_default=True)
            if subfaction_pattern_met:
                rows.append(row)
            else:
                common_rows.append(row)

            ceas_hashes[ceas_hash] = row_name

        loadouts_to_common_items[loadout] = loadout_common_items
        loadouts_to_subfaction_items[loadout] = loadout_subfaction_items

    orig_df = orig_df[orig_df["---"].isin(list(loadout_mapping.keys()))]

    def get_mo_from_new_df(row):
        df = orig_df_new[orig_df_new["---"] == row["---"]]
        if df.empty:
            return row["Materials Override"]
        else:
            return df["Materials Override"].iloc[0]

    def get_lo_from_new_df(row):
        df = orig_df_new[orig_df_new["---"] == row["---"]]
        if df.empty:
            return row["Loadout Override"]
        else:
            return df["Loadout Override"].iloc[0]


    orig_df["Materials Override"] = orig_df.apply(get_mo_from_new_df, axis=1)
    orig_df["Loadout Override"] = orig_df.apply(get_lo_from_new_df, axis=1)
    orig_df["Default Cosmetic Items Override"] = orig_df["---"].apply(lambda loadout_name: encode_obj(loadouts_to_subfaction_items.get(loadout_name, [])))
    orig_df["---"] = orig_df["---"].replace(loadout_mapping)

    df = pd.DataFrame(rows)
    df = pd.concat([subfaction_items_df, df])
    df.to_csv(f"./data/new/{subfaction_pattern}.csv", index=False, quoting=1)

    df2 = pd.DataFrame(common_rows)
    df2 = pd.concat([common_items_df, df2])
    df2.to_csv("./data/new/csm_subfactions_common.csv", index=False, quoting=1)

    with open("./data/new/common.json", "w") as f:
        json.dump(loadouts_to_common_items, f, indent=4)

    with open("./data/new/subfaction.json", "w") as f:
        json.dump(loadouts_to_subfaction_items, f, indent=4)

    orig_df.to_csv("./data/new/subfaction.csv", index=False, quoting=1)

if __name__ == '__main__':
    # struc = make_cea([
    #     make_rule(["/Script/ECRCommon.CustomizationElementaryAsset'\"/Game/A/A\"'"],
    #               ["Cosmetic.ActorSubclass.GameFaction.ChaosSpaceMarines"]),
    #     make_rule(["/Script/ECRCommon.CustomizationElementaryAsset'\"/Game/A/A\"'"],
    #               ["Cosmetic.ActorSubclass.GameFaction.LoyalSpaceMarines"])
    # ], [
    #     cea_assets[0]
    # ])
    pass

    # print(get_item_based_on_cla(["/Script/ECRCommon.CustomizationElementaryAsset'/Game/Characters/SpaceMarine/Customization/Modules/Torso/CEA_NewJumppack_Torso.CEA_NewJumppack_Torso'", "/Script/ECRCommon.CustomizationElementaryAsset'/Game/Characters/SpaceMarine/Customization/Modules/BellyTubes/CEA_SM_MKV_BellyTubes.CEA_SM_MKV_BellyTubes'"], "Torso", is_jumppack=True))

    # cla_to_new_table()
    subfaction_to_new_table()
    # i = get_item_based_on_cla(["/Script/ECRCommon.CustomizationElementaryAsset'/Game/Characters/SpaceMarine/Customization/Modules/LegLT/CEA_CSM_MKVI_LegLT.CEA_CSM_MKVI_LegLT'"], 'LeftLeg', is_jumppack=True)
    # print(i)