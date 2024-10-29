import json
import os

import pandas as pd


def make_ecr_gameplay_item_row(raw_name, text_name, category, lp_cost, description, rarity, preview_image,
                               is_granted_by_default=False):
    return {
        '---': raw_name,
        'Name': text_name,
        'Customization Slot Category': category,
        'Is Enabled': bool(text_name),
        'Is Granted By Default': is_granted_by_default,
        'Is Purchasable': '',
        'Min Level': '',
        'LP Cost': lp_cost,
        'Silver Cost': '',
        'Required Quest': '',
        'Required Advancement': '',
        'Allowed Loadouts': '',
        'Prohibited Loadouts': '',
        'Allowed Subfactions': '',
        'Modifier Categories': '',
        'Item Definition': '',
        'Granted Cosmetic Tags': '',
        'Granted Gameplay Tags': '',
        'Actor Selection Tags': '',
        'Description': description,
        'Rarity': rarity,
        'Preview': preview_image
    }


with open("tags_to_category.json", "r") as f:
    tags_to_category = json.load(f)

with open("names_to_categories.json", "r") as f:
    names_to_categories = json.load(f)

with open("items_data.json", "r") as f:
    item_data = json.load(f)


def convert_csv_to_ingame_csv(in_csv, out_csv):
    df = pd.read_csv(in_csv)
    df = df.fillna("")

    new_data = []
    for i, row in df.iterrows():
        row = row.to_dict()

        item_name = row.get("name")

        do_ignore = False
        if item_name in item_data["items_to_ignore_names"]:
            do_ignore = True

        for ignore_prefix in item_data["items_to_ignore_prefixes"]:
            if item_name.startswith(ignore_prefix):
                do_ignore = True

        if do_ignore:
            print("Ignoring", item_name)
            continue

        tier = row.get("tier")
        rel_texture_fp = row.get("image")

        tier_to_rarity = {
            ""
            "EItemTier::MasterCrafted": "Green",
            "EItemTier::Artificer": "Purple",
            "EItemTier::Relic": "Gold"
        }
        rarity = tier_to_rarity.get(tier, "Grey")

        if isinstance(rel_texture_fp, str):
            texture_fp = os.path.join("/Game/GUI/Textures/Customization/GameplayItems/", rel_texture_fp.lstrip("/\\"))
            basename = os.path.splitext(os.path.basename(texture_fp))[0]
            texture_fp = os.path.join(os.path.dirname(texture_fp), f"{basename}.{basename}").replace("\\", "/")
            preview_struc = f'(TextureRules=,DefaultTexture="{texture_fp}")'
        else:
            preview_struc = ""

        category = ""

        tags = row.get("tags", "").split(" ")

        for tag in tags:
            c = tags_to_category.get(tag)
            if c:
                category = c

        if not category:
            for n, c in names_to_categories.items():
                if n in item_name:
                    category = c
                    break

        if not category:
            print(f"! Item {item_name} unknown category! (tags {tags})")

        is_granted_by_default = item_name in item_data["default_granted_items"]
        lp_cost = row.get("lp_cost")
        if lp_cost <= 0:
            print(f"! {item_name} has lp cost {lp_cost}")
        new_row = make_ecr_gameplay_item_row(item_name, row.get("display_name_en"), category, lp_cost,
                                             row.get("description_en"), rarity, preview_struc,
                                             is_granted_by_default=is_granted_by_default)
        new_data.append(new_row)

    df = pd.DataFrame(data=new_data)

    # Sorting items
    rarity_mapping = {"Grey": 0, "Green": 1, "Blue": 2, "Purple": 3, "Gold": 4}
    df["Rarity Rank"] = df["Rarity"].map(rarity_mapping)

    # Order of subcategories in categories
    categories_to_substrings = {
        "PrimaryRangedWeapon": ["Bolt", "Plasma", "Grav", "Melta", "Storm"],
        "PrimaryMeleeWeapon": ["Chain", "PowerSword", "Crozius", "Axe", "Maul", "Fist"],
        "PrimaryHeavyWeapon": ["Bolt", "Plasma", "Grav", "Melta", "Las"],
        "SecondaryRangedWeapon": ["Bolt", "Plasma", "Grav"],
    }

    def get_substring_rank(row):
        value = row["---"]
        substrings = categories_to_substrings.get(row.get("Customization Slot Category"), [])
        res_i = len(substrings)
        for i, substring in enumerate(substrings):
            if substring in value:
                res_i = i
                break
        return res_i  # Non-matching entries get a high rank

    df["Substring Rank"] = df.apply(get_substring_rank, axis=1)
    df = df.sort_values(
        by=["Customization Slot Category", "Rarity Rank", "Substring Rank", "LP Cost", "---"],
        ascending=[True, True, True, True, True]
    )
    df[["---", "Rarity Rank", "LP Cost", "Substring Rank"]].to_csv("../data/temp.csv", index=False)
    df = df.drop(columns=["Rarity Rank", "Substring Rank"])

    df.to_csv(out_csv, index=False)


if __name__ == '__main__':
    in_csv = "../data/ec/gameplay_items/csm.csv"
    out_csv = "../data/ecr/gameplay_items/csm.csv"
    convert_csv_to_ingame_csv(in_csv, out_csv)
