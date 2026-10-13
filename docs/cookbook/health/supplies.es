a supply has a name, a unit and a lowest.
let supplies be a list of a new supply with name "gloves", unit "boxes" and lowest 20,
    a new supply with name "masks", unit "boxes" and lowest 15,
    a new supply with name "bandages", unit "packs" and lowest 30,
    a new supply with name "hand gel", unit "bottles" and lowest 10.
let orders be 0.
for each supply in supplies:
    ask "How many [supply's unit] of [supply's name] do we have? " and call it count.
    if count is below supply's lowest:
        say "  Order [twice supply's lowest minus count] [supply's unit] of [supply's name]."
        add 1 to orders.
if orders is 0, say "Nothing to order this week."
