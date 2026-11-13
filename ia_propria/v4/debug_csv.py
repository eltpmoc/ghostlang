import pandas as pd
import ast

df = pd.read_csv("v4/dados/train.csv")
row = df.iloc[0]
val = row['conversations']
print("RAW:", val[:100])
try:
    conv = ast.literal_eval(val)
    print("Parsed! Length:", len(conv))
except Exception as e:
    print("Error:", e)
