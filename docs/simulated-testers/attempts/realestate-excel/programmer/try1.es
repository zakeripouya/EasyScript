listings = load("listings.xlsx")
groups = listings.groupBy("neighborhood")

for name, houses in groups:
    avg = average(houses.price)
    cheapest = houses.minBy("price")
    print(name + ": avg price = " + round(avg, 2) + ", cheapest = " + cheapest.address + " (" + cheapest.price + ")")
