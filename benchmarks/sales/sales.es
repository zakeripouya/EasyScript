note: Reads a csv file of 100,000 sales (made by setup.sh) and totals the amounts per region.
read csv file "sales.csv" and call it sales
let totals be an empty map from text to numbers
let total be 0
for each sale in sales:
    let amount be (the value for "amount" in sale) as a number
    let region be the value for "region" in sale
    set the value for region in totals to (the value for region in totals, or 0) plus amount
    add amount to total
say the number of items in sales followed by " rows, total " followed by total
say totals
