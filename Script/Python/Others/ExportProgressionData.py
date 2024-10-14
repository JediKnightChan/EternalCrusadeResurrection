import unreal

table_paths = [
    "/Game/Blueprints/ECR/Data/Factions/GameplayItems/GameplayItems_LSM"
]

for table_path in table_paths:
    table = unreal.EditorAssetLibrary.find_asset_data(table_path).get_asset()
    row_names = unreal.DataTableFunctionLibrary.get_data_table_row_names(table)
    column_values = unreal.DataTableFunctionLibrary.get_data_table_column_as_string(table, "LP Cost")
    print(table, dict(zip(row_names, column_values)))

item_struct = unreal.EditorAssetLibrary.find_asset_data("/Game/Blueprints/ECR/Data/Factions/GameplayItems/StrucGameplayItem").get_asset()
