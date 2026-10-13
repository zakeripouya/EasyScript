read the table in "shipments.csv" and call it shipments.
say "Shipments per country:".
for each country and its count in the number of shipments per country:
    say "  [country]: [count]".
say "Average weight:", the average weight kg of shipments rounded to 1 decimal, "kg".
say "Heaviest:", the id of the shipment with the highest weight kg.
a shipment's days is the number of days between its shipped and its delivered.
say "Took more than 5 days:".
for each shipment in the shipments whose days is over 5:
    say "  [shipment's id] to [shipment's country]: [shipment's days] days".
