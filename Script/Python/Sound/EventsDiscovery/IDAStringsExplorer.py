import pandas as pd


df = pd.read_csv("ec_strings.tsv", sep="\t")
strings = list(df["String"].unique())


for string in strings:
    if str(string).lower().startswith("play_")  and "jenkins" not in string:
        print(f"\"{string}\",")
