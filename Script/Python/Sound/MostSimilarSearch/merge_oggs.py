from pydub import AudioSegment
from tqdm import tqdm
import os

# Input folder
ROOT_DIR = "C:/Users/JediKnight/Documents/Unreal Projects/ECR/Script/Python/Sound/Files/Windows/"
SNB_DIR = os.path.join(ROOT_DIR, "SNB_BolterGun_CM")

# Output files
output_audio = "merged.ogg"
output_text = "timestamps.txt"

# Collect all .ogg files sorted by number
files = sorted(
    [f for f in os.listdir(SNB_DIR) if f.endswith(".ogg")],
    key=lambda x: int(os.path.splitext(x)[0])
)

combined = AudioSegment.empty()
timestamps = []

current_time = 0  # milliseconds

for fname in tqdm(files):
    path = os.path.join(SNB_DIR, fname)
    audio = AudioSegment.from_ogg(path)

    duration_sec = len(audio) / 1000
    timestamps.append(f"{current_time/1000:.2f}s - {fname}")

    combined += audio
    current_time += len(audio)

# Export merged audio
combined.export(os.path.join(SNB_DIR, output_audio), format="ogg")

# Write timestamps
with open(os.path.join(SNB_DIR, output_text), "w") as f:
    f.write("\n".join(timestamps))

print("Done! Created merged audio and timestamps file.")
