supplies = [
    {name: "gloves", unit: "boxes", min: 20},
    {name: "masks", unit: "boxes", min: 15},
    {name: "bandages", unit: "packs", min: 30},
    {name: "hand gel", unit: "bottles", min: 10}
]

to_order = []
for item in supplies:
    item.count = ask("How many " + item.name + " (" + item.unit + ") do we have? ")
    if item.count < item.min:
        to_order.append({name: item.name, unit: item.unit, qty: item.min * 2 - item.count})

if to_order is empty:
    print("Nothing to order this week.")
else:
    for o in to_order:
        print(o.name + ": order " + o.qty + " " + o.unit)
