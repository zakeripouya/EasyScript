note: Read sales from a csv file, add them up per region, and save a report.
note: The first run writes an example sales.csv to read.
if not (file "sales.csv" exists):
    put "date,region,product,amount" in file "sales.csv"
    add "2026-10-01,North,tea,12.50" to the end of file "sales.csv"
    add "2026-10-01,South,coffee,8" to the end of file "sales.csv"
    add "2026-10-02,North,\"cake, lemon\",4.25" to the end of file "sales.csv"
    add "2026-10-03,East,tea,7" to the end of file "sales.csv"
    add "2026-10-03,South,tea,3.75" to the end of file "sales.csv"
    add "2026-10-04,North,coffee,9" to the end of file "sales.csv"

read csv file "sales.csv" and call it sales
say "Read " followed by the number of items in sales followed by " sales."

let totals be an empty map from text to numbers
let counts be an empty map from text to numbers
for each sale in sales:
    let region be the value for "region" in sale
    let amount be (the value for "amount" in sale) as a number
    set the value for region in totals to (the value for region in totals, or 0) plus amount
    set the value for region in counts to (the value for region in counts, or 0) plus 1

let report be an empty list of maps from text to text
for each region and total in totals:
    say region followed by ": " followed by total followed by " from " followed by the value for region in counts followed by " sales"
    let average be (total divided by the value for region in counts) rounded to 2 decimal places
    add (a map of "region" to region, "total" to total as text, "average" to average as text) to the end of report
save report as csv file "report.csv"
say "Saved report.csv:"
say the contents of file "report.csv"
