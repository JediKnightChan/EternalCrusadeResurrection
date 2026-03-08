from pydub import AudioSegment
from tqdm import tqdm
import os
from concurrent.futures import ThreadPoolExecutor

ROOT_DIR = "C:/Users/JediKnight/Documents/Unreal Projects/ECR/Script/Python/Sound/Files/Windows/"
SNB_DIR = os.path.join(ROOT_DIR, "English(US)")
CACHE_DIR = os.path.join(SNB_DIR, "batch_cache")
os.makedirs(CACHE_DIR, exist_ok=True)

output_audio = "merged.ogg"
output_text = "timestamps.txt"
N_BATCHES = 20

# Collect all files sorted by size
files = sorted(
    [f for f in os.listdir(SNB_DIR) if f.endswith(".ogg")],
    key=lambda x: os.path.getsize(os.path.join(SNB_DIR, x)),
)

# Split files
batch_size = (len(files) + N_BATCHES - 1) // N_BATCHES
batches = [files[i * batch_size:(i + 1) * batch_size] for i in range(N_BATCHES)]


def load_batch_if_cached(i):
    """Return (audio, timestamps) if existing cache found, else None."""
    audio_path = os.path.join(CACHE_DIR, f"batch_{i}.ogg")
    ts_path = os.path.join(CACHE_DIR, f"batch_{i}.txt")

    if os.path.exists(audio_path) and os.path.exists(ts_path):
        audio = AudioSegment.from_ogg(audio_path)
        with open(ts_path, "r") as f:
            timestamps = f.read().splitlines()
        return audio, timestamps
    return None


def save_batch(i, audio, timestamps):
    audio.export(os.path.join(CACHE_DIR, f"batch_{i}.ogg"), format="ogg")
    with open(os.path.join(CACHE_DIR, f"batch_{i}.txt"), "w") as f:
        f.write("\n".join(timestamps))


def process_batch(i, batch_files, show_progress=False):
    """Compute OR load cached batch result."""
    cached = load_batch_if_cached(i)
    if cached is not None:
        return cached

    combined = AudioSegment.empty()
    timestamps = []
    current_time = 0

    iterator = tqdm(batch_files, desc=f"Batch {i}", leave=True) if show_progress else batch_files

    for fname in iterator:
        audio = AudioSegment.from_ogg(os.path.join(SNB_DIR, fname))
        timestamps.append(f"{current_time/1000:.2f}s - {fname}")
        combined += audio
        current_time += len(audio)

    save_batch(i, combined, timestamps)
    return combined, timestamps


# Process batches in parallel
results = [None] * N_BATCHES
with ThreadPoolExecutor(max_workers=4) as executor:
    futures = {
        executor.submit(process_batch, i, batch, show_progress=(i == 0)): i
        for i, batch in enumerate(batches)
    }

    for future in futures:
        i = futures[future]
        results[i] = future.result()


# Combine batches sequentially
combined = AudioSegment.empty()
timestamps = []
current_time = 0

for i, (audio, ts) in enumerate(results):
    for t in ts:
        sec, fname = t.split("s - ")
        sec = float(sec) + current_time / 1000
        timestamps.append(f"{sec:.2f}s - {fname}")

    combined += audio
    current_time += len(audio)


# Export full merged result
combined.export(os.path.join(SNB_DIR, output_audio), format="ogg")

with open(os.path.join(SNB_DIR, output_text), "w") as f:
    f.write("\n".join(timestamps))

print("Done! Batches cached, merged audio created.")
