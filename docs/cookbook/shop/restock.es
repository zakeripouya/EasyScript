a product has a name, a stock and a lowest.
let products be a list of a new product with name "Coffee beans", stock 4 and lowest 10,
    a new product with name "Mug", stock 25 and lowest 5,
    a new product with name "Filter papers", stock 30 and lowest 40,
    a new product with name "Grinder", stock 1 and lowest 2.
a product is low if its stock is below its lowest.
say "Running low:", how many products are low.
for each product in the low products:
    say "  Order [twice its lowest minus its stock] of [its name]."
