import json
import os

import pandas as pd


def make_ecr_skill_tree_skill_row(raw_name, text_name, lamp_cost, min_level, required_node, granted_gameplay_items,
                                  node_type, image):
    return {
        '---': raw_name,
        'Display Name': text_name,
        'Is Enabled': True,
        'Lamp Cost': lamp_cost,
        'Min Level': min_level,
        'Required Node': required_node,
        'Granted Gameplay Item': granted_gameplay_items,
        'Type': node_type,
        'Icon Image': image
    }


def convert_csv_to_ingame_csv(in_csv, out_csv, faction_dir):
    df = pd.read_csv(in_csv)
    df = df.fillna("")

    new_data = []
    for i, row in df.iterrows():
        row = row.to_dict()
        item_name = row.get("Progression")

        if row.get("ItemReward"):
            node_type = "GameplayItem"
        elif "mastery" in row.get("Progression").lower():
            node_type = "LpCostReduce"
        elif "classnode" in row.get("Progression").lower():
            node_type = "ClassUnlock"
        else:
            node_type = "TimeUnlock"

        rel_texture_fp = row.get("Progression")
        if isinstance(rel_texture_fp, str):
            texture_fp = os.path.join(f"/Game/GUI/Textures/Customization/Advancements/{faction_dir}/",
                                      rel_texture_fp.lstrip("/\\"))
            basename = os.path.splitext(os.path.basename(texture_fp))[0]
            texture_fp = os.path.join(os.path.dirname(texture_fp), f"{basename}.{basename}").replace("\\", "/")
        else:
            texture_fp = ""

        new_row = make_ecr_skill_tree_skill_row(item_name, row.get("Name"), 0, row.get("Rank"),
                                                row.get("Dependency"), row.get("ItemReward"), node_type, texture_fp)
        new_data.append(new_row)

    df = pd.DataFrame(data=new_data)
    df.to_csv(out_csv, index=False)


if __name__ == '__main__':
    in_csv = "../data/ec/advancements/csm.csv"
    out_csv = "../data/ecr/advancements/csm.csv"
    faction_dir = "CSM"
    convert_csv_to_ingame_csv(in_csv, out_csv, faction_dir)
