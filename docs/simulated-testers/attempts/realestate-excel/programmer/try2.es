let listings be read("listings.xlsx")
let neighborhoods be unique(listings, "neighborhood")

for each name in neighborhoods:
    let houses be filter(listings, "neighborhood", name)
    let avg be average(houses, "price")
    let cheapest be min(houses, "price")
    print(name + ": avg price = " + round(avg, 2) + ", cheapest = " + cheapest)
