# 200,000 rounds of dates kept in records, lists, maps and values of a kind:
# made, sorted, compared, copied, printed and dropped, with live memory kept small.
an order is a kind of thing with:
    item, which is text.
    day, which is a date.
    earlier, which is a list of dates, starting at an empty list of dates.

a when is one of:
    on with moment (a date)
    never

let total be 0.
let latest be an empty map from text to dates.
count from 1 to 200000:
    let sale be a new order with item "item " followed by (it mod 7) and day (it mod 90) days ago.
    add yesterday to the end of sale's earlier.
    add sale's day to the end of sale's earlier.
    let days be sale's earlier sorted.
    if the first item of days is before the last item of days, add 1 to total.
    set the value for sale's item in latest to sale's day.
    let twin be a copy of sale.
    if twin's earlier is sale's earlier, add 1 to total.
    let w be a new on with moment sale's day.
    if w is (a new on with moment twin's day), add 1 to total.
    let shown be days as text.
    add the number of characters in shown to total.
say total.
say the number of entries in latest.
