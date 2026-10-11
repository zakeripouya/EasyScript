note: csv files: quoted fields, commas and line breaks inside quotes, \r\n, blank lines
let line end be the character with code 13 followed by "\n"
let text be "region,amount,note" followed by line end followed by "North,10,plain" followed by line end
set text to text followed by "\"South\",5,\"with, comma\"" followed by line end
set text to text followed by "North,7,\"two\nlines\"" followed by line end followed by line end
set text to text followed by "East,3,\"say \"\"hi\"\"\"" followed by line end
put text in file "sales.csv"
read csv file "sales.csv" and call it rows
say the number of items in rows
say item 1 of rows
say the value for "note" in item 3 of rows
say the value for "note" in item 4 of rows
let totals be an empty map from text to numbers
for each row in rows:
    let region be the value for "region" in row
    let amount be (the value for "amount" in row) as a number
    set the value for region in totals to (the value for region in totals, or 0) plus amount
say totals
let report be an empty list of maps from text to numbers
for each region and total in totals:
    add (a map of "total" to total) to the end of report
save report as csv file "totals.csv"
say the contents of file "totals.csv"
let people be a list of (a map of "name" to "Ann, the first", "says" to "\"hi\""), (a map of "says" to " spaced ", "name" to "Bo")
save people as csv file "people.csv"
say the contents of file "people.csv"
read csv file "people.csv" and call it again
say again
let flags be a list of (a map of "done" to yes) and (a map of "done" to no)
save flags as csv file "flags.csv"
say the contents of file "flags.csv"
save (an empty list of maps from text to text) as csv file "empty.csv"
say the size of file "empty.csv"
put "only,a,header" in file "header.csv"
read csv file "header.csv" and call it header only
say the number of items in header only
