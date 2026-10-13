# Average house prices per neighborhood

Your listings are in a csv file. Show the average price in each neighborhood, the cheapest home, and how many have 3 or more bedrooms.

## The program

`prices-by-area.es`:

```
read the table in "listings.csv", with price in dollars, and call it listings.
for each neighborhood and its average in the average price of listings per neighborhood:
    say neighborhood, "average:", average.
let cheapest be the listing with the lowest price.
say "Cheapest:", cheapest's address, "in", cheapest's neighborhood, "for", cheapest's price.
say "Homes with 3 or more bedrooms:", how many listings have a bedrooms of at least 3.
```

## The data

`listings.csv`, in the same folder:

```
address,neighborhood,price,bedrooms
12 Oak St,Riverside,$350000,3
8 Pine Ave,Riverside,$295000,2
101 Elm Rd,Hilltop,$520000,4
44 Birch Ln,Hilltop,$480000,3
7 Maple Ct,Old Town,$310000,2
19 Cedar Dr,Old Town,$365000,3
```

## Running it

`easyscript run prices-by-area.es` shows:

```
Riverside average: $322,500.00
Hilltop average: $500,000.00
Old Town average: $337,500.00
Cheapest: 8 Pine Ave in Riverside for $295,000.00
Homes with 3 or more bedrooms: 4
```

## Under the hood

EasyScript can't read Excel files yet: save the sheet as a csv file first ("Save As", then "CSV").
