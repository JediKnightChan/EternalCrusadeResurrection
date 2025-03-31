import pandas as pd


def sort_table(df):
    rarity_mapping = {"Grey": 0, "Green": 1, "Blue": 2, "Purple": 3, "Gold": 4}
    df["Rarity Rank"] = df["Rarity"].map(rarity_mapping)

    categories_to_substrings = {
        "PrimaryRangedWeapon": ["Bolt", "Plasma", "Grav", "Melta", "Storm", "Combi"],
        "PrimaryMeleeWeapon": ["Chainsword", "ForceSword", "PowerSword", "Crozius", "Chainaxe", "ForceAxe", "PowerAxe",
                               "Maul", "Fist"],
        "PrimaryHeavyWeapon": ["Bolt", "Autocannon", "Plasma", "Grav", "Melta", "Las"],
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
    return df


if __name__ == '__main__':
    df = pd.read_csv("../data/ecr/gameplay_items/csm.csv", encoding="utf-16")
    df = sort_table(df)
    print(df)
    df.to_csv("../data/ecr/gameplay_items/csm.csv", index=False)