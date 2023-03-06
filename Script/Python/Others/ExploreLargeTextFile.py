import os.path

filepath = "C:/Users/JediKnight/Documents/Unreal Projects/ECRPackaged/Windows/ECR/Saved/Logs/ECR.log"
filesize = os.path.getsize(filepath)
n_bytes = 300 * 1000

position = filesize - 6484 * n_bytes
# position = 0
# position = None


with open(filepath, "rb") as f:
    if position is None:
        position = filesize - n_bytes
    f.seek(position)
    content = f.read(n_bytes)

print(content)
print("Blocks", filesize / n_bytes)