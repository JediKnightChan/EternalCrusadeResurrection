import os
import librosa
import numpy as np
import pickle

ROOT_DIR = "C:/Users/JediKnight/Documents/Unreal Projects/ECR/Script/Python/Sound/Files/Windows/"
SNB_DIR = os.path.join(ROOT_DIR, "SNB_MeltaGun_CM")
AUDIO_DIRECTORY = SNB_DIR
AUDIO_DATAFILE = os.path.join(SNB_DIR, "audio_data.pickle")


def dircrawl(dir):
    files = os.listdir(dir)
    for obj in files:

        if os.path.isfile(os.path.join(dir, obj)):
            entries.append(os.path.join(dir, obj))
        elif os.path.isdir(os.path.join(dir, obj)):
            dircrawl(os.path.join(dir, obj))

    return entries


entries = []
files = dircrawl(AUDIO_DIRECTORY)

data = []

for i, file in enumerate(files):
    if (i % 10 == 0):
        print(str(i) + "/" + str(len(files)))
    # Load audio file
    y_ref, sr = librosa.load(file)

    # Calculate spectrogram of audio, on mels scale, with intensity in W
    # y_audio = librosa.power_to_db(librosa.feature.melspectrogram(y = y_ref, sr = sr, n_mels=128))
    y_audio = (librosa.feature.melspectrogram(y=y_ref, sr=sr, n_mels=128))

    # Calculate the highest value index in the spectrogram
    ind = np.unravel_index(np.argmax(y_audio, axis=None), y_audio.shape)

    # Ignore file if max value is 0 (no data in file or bad read)
    if (y_audio[ind] == 0):
        pass
    else:
        # Get max value
        y_max = y_audio[ind]

        # Compute relative intensity to max in spectrogram, set to 0 values below 0.01% of max (we won't check them later during matching)
        inv_y_max = 1 / y_max
        y_audio_relative = y_audio * inv_y_max * 100
        y_audio_relative[y_audio_relative < (1)] = 0

        data.append([file, y_audio_relative, y_max, ind])

with open(AUDIO_DATAFILE, "wb") as f:
    pickle.dump(data, f)
