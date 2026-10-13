# Shipments per country, and the late ones

Show how many shipments went to each country, the average weight, the heaviest one, and the shipments that took more than 5 days.

## The program

`late-shipments.es`:

```
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
```

## The data

`shipments.csv`, in the same folder:

```
id,country,weight kg,shipped,delivered
A100,France,12.5,2026-09-01,2026-09-04
A101,Germany,3.2,2026-09-01,2026-09-08
A102,France,7.8,2026-09-03,2026-09-05
A103,Spain,22.0,2026-09-05,2026-09-12
A104,Germany,1.1,2026-09-06,2026-09-09
A105,Italy,15.4,2026-09-08,2026-09-11
```

## Running it

`easyscript run late-shipments.es` shows:

```
Shipments per country:
  France: 2
  Germany: 2
  Spain: 1
  Italy: 1
Average weight: 10.3 kg
Heaviest: A103
Took more than 5 days:
  A101 to Germany: 7 days
  A103 to Spain: 7 days
```

## Under the hood

`a shipment's days is ...` adds a value to every row, worked out from two date columns, and `whose days is over 5` picks the rows by it.
