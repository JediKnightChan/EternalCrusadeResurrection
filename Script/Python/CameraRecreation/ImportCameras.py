import ast
import csv
import os
import unreal


# ============================================================
# CONFIG
# ============================================================

COMBINED_CSV = (
    r"C:\Program Files (x86)\Steam\steamapps\common"
    r"\Warhammer 40,000 - Eternal Crusade\extract\raw_uassets"
    r"\EternalCrusade\Content\Blueprints\Camera\combined2.csv"
)

# Generated Vector Curve CSVs
CURVES_DIR = (
    r"C:\Program Files (x86)\Steam\steamapps\common"
    r"\Warhammer 40,000 - Eternal Crusade\extract\raw_uassets"
    r"\EternalCrusade\Content\Blueprints\Camera\curves"
)

# Parent Camera Mode Blueprint
PARENT_BLUEPRINT = (
    "/Game/Blueprints/ECR/Pawns/Cameras/"
    "ECRCameraMode_ThirdPerson_BP"
)

# Root directory for generated Camera Mode Blueprints
OUTPUT_ROOT = "/Game/Blueprints/ECR/Pawns/Cameras"


# ============================================================
# PARSING
# ============================================================

def parse_float(value, column_name):
    """
    Empty value means:
        inherit from parent.

    Returns:
        float or None
    """

    if value is None:
        return None

    value = str(value).strip()

    if value == "":
        return None

    try:
        return float(value)
    except ValueError as e:
        raise ValueError(
            f"Could not convert '{column_name}' to float: {value!r}"
        ) from e


def parse_vector(value, column_name):
    """
    Empty value means:
        inherit from parent.

    Expected format:
        "(X, Y, Z)"

    Returns:
        unreal.Vector or None
    """

    if value is None:
        return None

    value = str(value).strip()

    if value == "":
        return None

    try:
        vector = ast.literal_eval(value)
    except (ValueError, SyntaxError) as e:
        raise ValueError(
            f"Could not parse '{column_name}': {value!r}"
        ) from e

    if not isinstance(vector, (tuple, list)):
        raise ValueError(
            f"'{column_name}' must be a tuple/list: {value!r}"
        )

    if len(vector) != 3:
        raise ValueError(
            f"'{column_name}' must have 3 components: {value!r}"
        )

    try:
        return unreal.Vector(
            float(vector[0]),
            float(vector[1]),
            float(vector[2]),
        )
    except (TypeError, ValueError) as e:
        raise ValueError(
            f"Invalid values in '{column_name}': {value!r}"
        ) from e


# ============================================================
# PATHS
# ============================================================

def normalize_source_path(file_value):
    """
    Converts:

        Characters\\SM\\Weapons\\Foo.uasset

    into:

        Characters/SM/Weapons/Foo
    """

    path = str(file_value).replace("\\", "/")
    path = os.path.splitext(path)[0]

    return path


def get_blueprint_asset_path(file_value):
    """
    Example:

        Characters\\SM\\Weapons\\SM_CamSettings_Melee.uasset

    becomes:

        /Game/Blueprints/ECR/Pawns/Cameras/
        Characters/SM/Weapons/SM_CamSettings_Melee
    """

    relative_path = normalize_source_path(file_value)

    return f"{OUTPUT_ROOT}/{relative_path}"


def get_curve_csv_path(file_value):
    """
    Example:

        Characters\\SM\\Weapons\\SM_CamSettings_Melee.uasset

    becomes:

        C:/.../Camera/curves/
        Characters/SM/Weapons/SM_CamSettings_Melee.csv
    """

    relative_path = normalize_source_path(file_value)

    return os.path.join(
        CURVES_DIR,
        *relative_path.split("/"),
    ) + ".csv"


def get_curve_asset_path(blueprint_asset_path):
    return blueprint_asset_path + "_TargetOffsetCurve"


# ============================================================
# UNREAL HELPERS
# ============================================================

def ensure_directory(package_path):
    if not unreal.EditorAssetLibrary.does_directory_exist(
        package_path
    ):
        unreal.EditorAssetLibrary.make_directory(
            package_path
        )


def import_vector_curve(csv_path, asset_path):
    """
    Import CSV as Unreal Vector Curve.
    """

    if not os.path.isfile(csv_path):
        raise RuntimeError(
            f"Curve CSV does not exist: {csv_path}"
        )

    package_path = asset_path.rsplit("/", 1)[0]
    asset_name = asset_path.rsplit("/", 1)[1]

    ensure_directory(package_path)

    # Delete existing curve so it is recreated from CSV.
    if unreal.EditorAssetLibrary.does_asset_exist(
        asset_path
    ):
        unreal.EditorAssetLibrary.delete_asset(
            asset_path
        )

    # --------------------------------------------------------
    # Import task
    # --------------------------------------------------------

    import_task = unreal.AssetImportTask()

    import_task.set_editor_property(
        "filename",
        csv_path,
    )

    import_task.set_editor_property(
        "destination_path",
        package_path,
    )

    import_task.set_editor_property(
        "destination_name",
        asset_name,
    )

    import_task.set_editor_property(
        "replace_existing",
        True,
    )

    import_task.set_editor_property(
        "automated",
        True,
    )

    # --------------------------------------------------------
    # CSV settings
    # --------------------------------------------------------

    import_settings = unreal.CSVImportSettings()

    import_settings.set_editor_property(
        "import_type",
        unreal.CSVImportType.ECSV_CURVE_VECTOR,
    )

    import_settings.set_editor_property(
        "import_curve_interp_mode",
        unreal.RichCurveInterpMode.RCIM_LINEAR,
    )

    csv_factory = unreal.CSVImportFactory()

    csv_factory.set_editor_property(
        "automated_import_settings",
        import_settings,
    )

    import_task.set_editor_property(
        "factory",
        csv_factory,
    )

    # --------------------------------------------------------
    # Import
    # --------------------------------------------------------

    asset_tools = (
        unreal.AssetToolsHelpers.get_asset_tools()
    )

    asset_tools.import_asset_tasks(
        [import_task]
    )

    curve = unreal.load_asset(asset_path)

    if not curve:
        raise RuntimeError(
            f"Failed to load imported curve: {asset_path}"
        )

    unreal.log(
        f"Imported CurveVector: {asset_path}"
    )

    return curve


# ============================================================
# BLUEPRINT CREATION
# ============================================================

def create_child_blueprint(
    blueprint_asset_path,
    fov,
    pivot_offset,
    target_offset_curve,
):
    """
    Creates a child Blueprint of PARENT_BLUEPRINT.

    Only explicitly provided properties are overridden.
    None means inherit the parent value.
    """

    package_path = blueprint_asset_path.rsplit(
        "/",
        1,
    )[0]

    asset_name = blueprint_asset_path.rsplit(
        "/",
        1,
    )[1]

    ensure_directory(package_path)

    # --------------------------------------------------------
    # Load parent Blueprint
    # --------------------------------------------------------

    parent_bp = unreal.load_asset(
        PARENT_BLUEPRINT
    )

    if not parent_bp:
        raise RuntimeError(
            f"Could not load parent Blueprint: "
            f"{PARENT_BLUEPRINT}"
        )

    parent_class = parent_bp.generated_class()

    if not parent_class:
        raise RuntimeError(
            f"Could not get generated class from: "
            f"{PARENT_BLUEPRINT}"
        )

    # --------------------------------------------------------
    # Delete existing Blueprint
    # --------------------------------------------------------

    if unreal.EditorAssetLibrary.does_asset_exist(
        blueprint_asset_path
    ):
        unreal.EditorAssetLibrary.delete_asset(
            blueprint_asset_path
        )

    # --------------------------------------------------------
    # Blueprint Factory
    # --------------------------------------------------------

    blueprint_factory = unreal.BlueprintFactory()

    # IMPORTANT:
    #
    # Do NOT set b_skip_class_picker.
    # That property does not exist in this UE version.
    #
    # ParentClass is enough.
    blueprint_factory.set_editor_property(
        "parent_class",
        parent_class,
    )

    # --------------------------------------------------------
    # Create Blueprint
    # --------------------------------------------------------

    asset_tools = (
        unreal.AssetToolsHelpers.get_asset_tools()
    )

    blueprint = asset_tools.create_asset(
        asset_name=asset_name,
        package_path=package_path,
        asset_class=unreal.Blueprint,
        factory=blueprint_factory,
    )

    if not blueprint:
        raise RuntimeError(
            f"Failed to create Blueprint: "
            f"{blueprint_asset_path}"
        )

    unreal.log(
        f"Created Blueprint asset: "
        f"{blueprint_asset_path}"
    )

    # --------------------------------------------------------
    # Get generated class / CDO
    # --------------------------------------------------------

    generated_class = blueprint.generated_class()

    if not generated_class:
        raise RuntimeError(
            f"Failed to get generated class: "
            f"{blueprint_asset_path}"
        )

    cdo = unreal.get_default_object(
        generated_class
    )

    # --------------------------------------------------------
    # Override FOV only if present
    # --------------------------------------------------------

    if fov is not None:
        cdo.set_editor_property(
            "FieldOfView",
            fov,
        )

        unreal.log(
            f"  FieldOfView = {fov}"
        )
    else:
        unreal.log(
            "  FieldOfView: inherited"
        )

    # --------------------------------------------------------
    # Override PivotOffset only if present
    # --------------------------------------------------------

    if pivot_offset is not None:

        # Eternal Crusade -> Lyra adjustment
        pivot_offset = unreal.Vector(
            pivot_offset.x,
            pivot_offset.y,
            pivot_offset.z - 180.0,
        )

        cdo.set_editor_property(
            "PivotOffset",
            pivot_offset,
        )

        unreal.log(
            f"  PivotOffset = {pivot_offset}"
        )

    else:
        unreal.log(
            "  PivotOffset: inherited"
        )

    # --------------------------------------------------------
    # Override TargetOffsetCurve only if present
    # --------------------------------------------------------

    if target_offset_curve is not None:

        cdo.set_editor_property(
            "TargetOffsetCurve",
            target_offset_curve,
        )

        unreal.log(
            f"  TargetOffsetCurve = "
            f"{target_offset_curve.get_name()}"
        )

    else:
        unreal.log(
            "  TargetOffsetCurve: inherited"
        )

    # --------------------------------------------------------
    # Compile
    # --------------------------------------------------------

    unreal.KismetEditorUtilities.compile_blueprint(
        blueprint
    )

    # --------------------------------------------------------
    # Save
    # --------------------------------------------------------

    unreal.EditorAssetLibrary.save_asset(
        blueprint_asset_path
    )

    unreal.log(
        f"Saved Camera Blueprint: "
        f"{blueprint_asset_path}"
    )

    return blueprint


# ============================================================
# MAIN
# ============================================================

def main():

    # --------------------------------------------------------
    # Check input CSV
    # --------------------------------------------------------

    if not os.path.isfile(COMBINED_CSV):
        raise RuntimeError(
            f"Combined CSV does not exist:\n"
            f"{COMBINED_CSV}"
        )

    # --------------------------------------------------------
    # Read CSV
    # --------------------------------------------------------

    with open(
        COMBINED_CSV,
        "r",
        encoding="utf-8-sig",
        newline="",
    ) as file:

        reader = csv.DictReader(file)

        required_columns = {
            "File",
            "FOV",
            "PivotOffset",
            "LookCenterOffset",
            "LookUpOffset",
            "LookDownOffset",
        }

        missing_columns = (
            required_columns
            - set(reader.fieldnames or [])
        )

        if missing_columns:
            raise RuntimeError(
                "Missing columns in combined CSV: "
                + ", ".join(sorted(missing_columns))
            )

        rows = list(reader)

    # --------------------------------------------------------
    # Process rows
    # --------------------------------------------------------

    total = len(rows)

    unreal.log(
        f"Found {total} camera settings."
    )

    for index, row in enumerate(rows, start=1):

        source_file = row["File"]

        unreal.log(
            f"Processing {index}/{total}: "
            f"{source_file}"
        )

        try:

            # =================================================
            # Parse scalar/vector properties
            # =================================================

            fov = parse_float(
                row.get("FOV"),
                "FOV",
            )

            pivot_offset = parse_vector(
                row.get("PivotOffset"),
                "PivotOffset",
            )

            look_center = parse_vector(
                row.get("LookCenterOffset"),
                "LookCenterOffset",
            )

            look_up = parse_vector(
                row.get("LookUpOffset"),
                "LookUpOffset",
            )

            look_down = parse_vector(
                row.get("LookDownOffset"),
                "LookDownOffset",
            )

            # =================================================
            # Target Offset Curve
            # =================================================
            #
            # Only override parent's curve when ALL THREE
            # values are present.
            #
            # Otherwise:
            #
            #     TargetOffsetCurve = inherited
            #
            # =================================================

            target_offset_curve = None

            all_curve_values_present = (
                look_center is not None
                and look_up is not None
                and look_down is not None
            )

            if all_curve_values_present:

                curve_csv_path = (
                    get_curve_csv_path(
                        source_file
                    )
                )

                curve_asset_path = (
                    get_curve_asset_path(
                        get_blueprint_asset_path(
                            source_file
                        )
                    )
                )

                target_offset_curve = (
                    import_vector_curve(
                        curve_csv_path,
                        curve_asset_path,
                    )
                )

            else:

                unreal.log(
                    "  TargetOffsetCurve: inherited "
                    "(one or more LookOffset values empty)"
                )

            # =================================================
            # Blueprint path
            # =================================================

            blueprint_asset_path = (
                get_blueprint_asset_path(
                    source_file
                )
            )

            # =================================================
            # Create Blueprint
            # =================================================

            create_child_blueprint(
                blueprint_asset_path=(
                    blueprint_asset_path
                ),
                fov=fov,
                pivot_offset=pivot_offset,
                target_offset_curve=(
                    target_offset_curve
                ),
            )

        except Exception as e:

            unreal.log_error(
                f"Failed processing "
                f"{source_file}: {e}"
            )

            # Continue with the next camera
            continue

    unreal.log(
        "Finished generating camera Blueprints."
    )


# ============================================================
# RUN
# ============================================================

main()