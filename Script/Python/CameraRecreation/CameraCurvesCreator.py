
import ast
import os

import pandas as pd


# ============================================================
# Configuration
# ============================================================

COMBINED_CSV = r"C:\Program Files (x86)\Steam\steamapps\common\Warhammer 40,000 - Eternal Crusade\extract\raw_uassets\EternalCrusade\Content\Blueprints\Camera\combined2.csv"

# All generated curve CSVs will be placed here.
CURVES_DIR = r"C:\Program Files (x86)\Steam\steamapps\common\Warhammer 40,000 - Eternal Crusade\extract\raw_uassets\EternalCrusade\Content\Blueprints\Camera\curves"


# ============================================================
# Helpers
# ============================================================

VECTOR_COLUMNS = [
    "LookCenterOffset",
    "LookUpOffset",
    "LookDownOffset",
]


def parse_vector(value, column_name, row_number):
    """
    Parse a dataframe cell containing something like:

        (70.0, 0.0, 150.0)

    or:

        [70.0, 0.0, 150.0]

    Returns a tuple of 3 floats.
    """

    if pd.isna(value):
        raise ValueError(
            f"Row {row_number}: '{column_name}' is empty"
        )

    # Already a tuple/list
    if isinstance(value, (tuple, list)):
        vector = value

    # CSV values normally arrive as strings
    elif isinstance(value, str):
        try:
            vector = ast.literal_eval(value)
        except (ValueError, SyntaxError) as e:
            raise ValueError(
                f"Row {row_number}: could not parse "
                f"'{column_name}': {value!r}"
            ) from e

    else:
        raise ValueError(
            f"Row {row_number}: unexpected type for "
            f"'{column_name}': {type(value).__name__}"
        )

    # Validate dimensionality
    if not isinstance(vector, (tuple, list)):
        raise ValueError(
            f"Row {row_number}: '{column_name}' is not a tuple/list: "
            f"{vector!r}"
        )

    if len(vector) != 3:
        raise ValueError(
            f"Row {row_number}: '{column_name}' must contain "
            f"exactly 3 values, got {len(vector)}: {vector!r}"
        )

    # Convert to float
    try:
        vector = tuple(float(x) for x in vector)
    except (TypeError, ValueError) as e:
        raise ValueError(
            f"Row {row_number}: '{column_name}' contains "
            f"non-numeric values: {vector!r}"
        ) from e

    return vector


def get_curve_path(source_file):
    """
    Convert:

        Blueprints/Camera/Characters/SM/Weapons/Foo.uasset

    into:

        CURVES_DIR/Blueprints/Camera/Characters/SM/Weapons/Foo.csv
    """

    source_file = source_file.replace("\\", "/")

    # Remove .uasset
    relative_path = os.path.splitext(source_file)[0] + ".csv"

    return os.path.join(
        CURVES_DIR,
        *relative_path.split("/")
    )


# ============================================================
# Main
# ============================================================

df = pd.read_csv(COMBINED_CSV)


# ------------------------------------------------------------
# Validate columns
# ------------------------------------------------------------

required_columns = [
    "File",
    *VECTOR_COLUMNS,
]

missing_columns = [
    column
    for column in required_columns
    if column not in df.columns
]

if missing_columns:
    raise RuntimeError(
        "Missing required columns: "
        + ", ".join(missing_columns)
    )


os.makedirs(CURVES_DIR, exist_ok=True)


generated = 0

for index, row in df.iterrows():
    try:
        row_number = index + 2  # +1 for zero-index, +1 for CSV header

        source_file = row["File"]

        if pd.isna(source_file):
            raise ValueError(
                f"Row {row_number}: 'File' is empty"
            )

        # --------------------------------------------------------
        # Parse and validate vectors
        # --------------------------------------------------------

        look_center = parse_vector(
            row["LookCenterOffset"],
            "LookCenterOffset",
            row_number,
        )

        look_up = parse_vector(
            row["LookUpOffset"],
            "LookUpOffset",
            row_number,
        )

        look_down = parse_vector(
            row["LookDownOffset"],
            "LookDownOffset",
            row_number,
        )

        # --------------------------------------------------------
        # Generate curve CSV
        # --------------------------------------------------------

        curve_df = pd.DataFrame(
            [
                {
                    "Time": -90.0,
                    "X": look_down[0],
                    "Y": look_down[1],
                    "Z": look_down[2],
                },
                {
                    "Time": 0.0,
                    "X": look_center[0],
                    "Y": look_center[1],
                    "Z": look_center[2],
                },
                {
                    "Time": 90.0,
                    "X": look_up[0],
                    "Y": look_up[1],
                    "Z": look_up[2],
                },
            ]
        )

        curve_path = get_curve_path(source_file)

        os.makedirs(
            os.path.dirname(curve_path),
            exist_ok=True,
        )

        curve_df.to_csv(
            curve_path,
            index=False,
            header=False,
            float_format="%.6f",
        )

        print(
            f"Generated: {curve_path}"
        )

        generated += 1
    except Exception as e:
        print(f"Couldn't generate curve: for {row['File']}")
        continue


print()
print(f"Generated {generated} curve CSV files.")
