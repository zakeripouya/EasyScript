# Reads a csv file of 100,000 sales (made by setup.sh) and totals the amounts per region.
import csv

totals = {}
total = 0
rows = 0
with open("sales.csv", newline="") as f:
    for sale in csv.DictReader(f):
        amount = int(sale["amount"])
        region = sale["region"]
        totals[region] = totals.get(region, 0) + amount
        total += amount
        rows += 1
print(f"{rows} rows, total {total}")
print(totals)
