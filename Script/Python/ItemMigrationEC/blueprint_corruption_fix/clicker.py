"""This clicker was created to automate clicking actions to restore hundreds of corrupted blueprints,
caused by bad Unreal Python API implementation"""

import json
import pyperclip
import pyautogui
import time
import keyboard

with open("C:/Users/JediKnight/Documents/Unreal Projects/ECR/Script/Python/ItemMigrationEC/temp.json", "r") as f:
    package_paths = json.load(f)

# mode = "weapon"
mode = "accessory_or_weapon_mod"


def move_and_click(x, y, button="left", ctrl=False, shift=False, alt=False):
    """
    Moves the mouse to (x, y) and performs a click with optional key modifiers.

    :param x: X-coordinate
    :param y: Y-coordinate
    :param button: "left" or "right" mouse button
    :param ctrl: Hold Ctrl key
    :param shift: Hold Shift key
    :param alt: Hold Alt key
    """
    # Move the mouse to the position
    pyautogui.moveTo(x, y, duration=0)

    # Press modifier keys if needed
    if ctrl:
        keyboard.press('ctrl')
    if shift:
        keyboard.press('shift')
    if alt:
        keyboard.press('alt')

    # Perform the click
    pyautogui.click(button=button)

    # Release modifier keys
    if ctrl:
        keyboard.release('ctrl')
    if shift:
        keyboard.release('shift')
    if alt:
        keyboard.release('alt')


def get_code_for_asset_open(package_path):
    return f"""
import unreal

asset = unreal.EditorAssetLibrary.load_asset("{package_path}")
a = unreal.AssetEditorSubsystem()
a.open_editor_for_assets([asset])
"""


def perform_ctrl_v():
    keyboard.press('ctrl')
    keyboard.press('v')
    keyboard.release('ctrl')
    keyboard.release('v')


# Allow to switch to Unreal
time.sleep(3)

for package_path in package_paths:
    code_for_asset_open = get_code_for_asset_open(package_path)

    # Switch to Unreal "Output Log" tab
    move_and_click(219, 55, button="left")
    # Select input box for python code
    move_and_click(475, 1008, button="left")

    # Paste and execute python code for opening asset
    pyperclip.copy(code_for_asset_open)
    perform_ctrl_v()
    keyboard.press('enter')
    keyboard.release('enter')
    # Wait 3 seconds for asset open
    time.sleep(3)

    # Copy equipment fragment class
    move_and_click(1402, 435, button="right", shift=True)
    # Restore defaults for first fragment
    move_and_click(1874, 362, button="left")

    if mode == "weapon":
        # Open sub-tab
        move_and_click(62, 401, button="left")
        # Paste equipment fragment class
        move_and_click(1402, 435, button="left", shift=True)
    elif mode == "accessory_or_weapon_mod":
        # Select fragment dropout
        move_and_click(987, 361, button="left")
        # Select equipment fragment class
        move_and_click(1052, 463, button="left")
        # Open sub-tab
        move_and_click(62, 401, button="left")
        # Paste equipment fragment class
        move_and_click(1402, 435, button="left", shift=True)

    # Compile
    move_and_click(180, 96, button="left")
    # Wait 1 sec
    time.sleep(1)
    # Save
    move_and_click(40, 96, button="left")

    # Close blueprint
    move_and_click(848, 49, button="left")
