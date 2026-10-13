data = read_csv("shipments.csv")

counts = {}
for row in data:
    counts[row.country] = counts.get(row.country, 0) + 1
print("Shipments per country:", counts)

total = sum(row["weight kg"] for row in data)
print("Average weight:", total / len(data))

heaviest = max(data, key=row["weight kg"])
print("Heaviest:", heaviest.id, heaviest["weight kg"])

print("Took more than 5 days:")
for row in data:
    days = date(row.delivered) - date(row.shipped)
    if days > 5:
        print(row.id, days)
