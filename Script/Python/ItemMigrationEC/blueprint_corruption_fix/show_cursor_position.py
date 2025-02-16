"""This script is for showing cursor position for further automation of routine UE tasks with clicker"""

import pyautogui
import time

print("Tracking mouse position for 3 seconds... (Move your mouse around)")

start_time = time.time()

while time.time() - start_time < 3:
    x, y = pyautogui.position()  # Get current mouse position
    print(f"Mouse is at ({x}, {y})")
    time.sleep(0.01)  # Small delay to avoid excessive CPU usage

print("Tracking complete.")