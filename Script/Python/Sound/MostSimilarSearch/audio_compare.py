import shutil

import numpy as np
import librosa
import sys
import pickle
import os
from threading import Thread

ROOT_DIR = "C:/Users/JediKnight/Documents/Unreal Projects/ECR/Script/Python/Sound/Files/Windows/"
SNB_DIR = os.path.join(ROOT_DIR, "SNB_ChainSword_IM")
AUDIO_DATAFILE = os.path.join(SNB_DIR, "audio_data.pickle")

MP3_TO_COMPARE = "C:/Users/JediKnight/Desktop/157.ogg"


class ThreadWithReturnValue(Thread):
    def __init__(self, group=None, target=None, name=None,
                 args=(), kwargs={}, Verbose=None):
        Thread.__init__(self, group, target, name, args, kwargs)
        self._return = None

    def run(self):
        if self._target is not None:
            self._return = self._target(*self._args,
                                        **self._kwargs)

    def join(self, *args):
        Thread.join(self, *args)
        return self._return


def closest(vectors, loc, n=200):
    indices = np.argpartition(vectors, n)[:n]
    indices = indices[np.argsort(vectors[indices])]
    output = {}
    for i in [int(x) for x in indices]:
        output.setdefault(vectors[i], [loc[i]])
        output[vectors[i]].append(data[i][0])
    return output


def compare_spectrogram(comp, ref):
    # If reference max is sub 0, ignore comparison and return max distance
    if (ref[2] < 0):
        return [sys.float_info.max, 0, 100]

    else:
        spectrogram_comp = comp[1]
        spectrogram_ref = ref[1]
        if ((spectrogram_comp.shape[1]) >= (spectrogram_ref.shape[1])):
            slots = (spectrogram_comp.shape[1]) - (spectrogram_ref.shape[1])
            size = spectrogram_ref.shape[1]
            dist_min = sys.float_info.max
            dist_min_time = 0
            dist_min_relative_intensity = 100

            # Go through spectrogram_comp
            for i in range(0, slots + 1):
                dist = 0
                reduced_comp = spectrogram_comp[:, i:(i + size)]
                reduced_comp_reference = reduced_comp[ref[3]]
                # If the reference point is too low in the compared, ignore view
                # Can be faulty if spectrogram_comp is normalised, as max value is likely reduced
                if (reduced_comp_reference < 0.05 * ref[2]):
                    # print("hello")
                    pass
                else:
                    # Compare relative values with the non null elements of the reference
                    inv_reduced_comp_reference = 1 / reduced_comp_reference * 100
                    dist = np.sum(np.abs(
                        reduced_comp[spectrogram_ref > 0] * inv_reduced_comp_reference - spectrogram_ref[
                            spectrogram_ref > 0]))

                    # Store calculated distance if it is lower than the other ones
                    if (dist < dist_min):
                        dist_min = dist
                        dist_min_time = i
                        dist_min_relative_intensity = reduced_comp_reference / ref[2] * 100

            return [dist_min, dist_min_time, reduced_comp_reference]

        else:
            # TODO - implement comparison with file smaller than reference
            # print("akay")
            return [sys.float_info.max, 0, 100]


def get_closests(file, n=200):
    y_ref, sr = librosa.load(file)
    y_audio = (librosa.feature.melspectrogram(y=y_ref, sr=sr, n_mels=128))
    dists = np.array([])
    loc = []

    i = 0
    while (i < (len(data) - 4)):
        if (i % 300 == 0):
            print(str(i) + "/" + str(len(data)))

        e1 = ThreadWithReturnValue(target=compare_spectrogram, args=([file, y_audio], data[i]))
        e1.start()
        e2 = ThreadWithReturnValue(target=compare_spectrogram, args=([file, y_audio], data[i + 1]))
        e2.start()
        e3 = ThreadWithReturnValue(target=compare_spectrogram, args=([file, y_audio], data[i + 2]))
        e3.start()
        e4 = ThreadWithReturnValue(target=compare_spectrogram, args=([file, y_audio], data[i + 3]))
        e4.start()

        v1 = e1.join()
        v2 = e2.join()
        v3 = e3.join()
        v4 = e4.join()

        dists = np.append(dists, [v1[0], v2[0], v3[0], v4[0]])
        loc.append(v1[1:])
        loc.append(v2[1:])
        loc.append(v3[1:])
        loc.append(v4[1:])

        i = i + 4

    while (i < len(data)):
        v = compare_spectrogram([file, y_audio], data[i])
        dists = np.append(dists, v[0])
        loc.append(v[1:])

        i = i + 1

    return closest(dists, loc, n)


def get_closests_folder(dir, n=200):
    files = os.listdir(dir)
    closests = []
    for file in files:
        if file.endswith(".ogg"):
            closests.append((dir + "/" + file, get_closests(dir + "/" + file, n)))

    return closests


with open(AUDIO_DATAFILE, "rb") as f:
    data = pickle.load(f)

if __name__ == '__main__':
    res = get_closests(MP3_TO_COMPARE, 10)
    closest_fps = []
    for k, v in res.items():
        print(v[1])
        closest_fps.append(v[1])

    candidates_dir = os.path.join(SNB_DIR, "candidates")
    if os.path.exists(candidates_dir):
        shutil.rmtree(candidates_dir)
    os.makedirs(candidates_dir, exist_ok=True)


    for fp in closest_fps:
        new_fp = os.path.join(candidates_dir, os.path.basename(fp))
        shutil.copy(fp, new_fp)
