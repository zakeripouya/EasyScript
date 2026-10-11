# Dates kept in lists, maps, records and values of a kind: printed, compared,
# sorted, copied, searched and given to functions.
an order is a kind of thing with:
    item, which is text.
    day, which is a date.
    total, which is a number.

let orders be a list of (a new order with item "tea", day today, total 4), (a new order with item "cake", day yesterday, total 6) and (a new order with item "jam", day 40 days ago, total 3)
say orders
let days be a list of today, 3 days ago and tomorrow
say days
say days sorted
say days sorted biggest first
say the biggest of days
say the smallest of days
say days contains yesterday
say days contains 3 days ago
say the position of tomorrow in days
let copied be a copy of days
sort days
say days
say copied
say days is copied
say days joined with " / "
say days reversed is copied
say the days in days where it is after today
say (a function taking d that gives back the weekday of d) applied to every d in days

note: per month, and this month
let spent be an empty map from numbers to numbers
for each sale in orders:
    if sale's day is in this month:
        say sale's item followed by " this month"
    let m be the month of sale's day
    set the value for m in spent to (the value for m in spent, or 0) plus sale's total
say spent

let firsts be a map of "tea" to today and "cake" to yesterday
say firsts
say the value for "tea" in firsts
say the values of firsts
let schedule be a map of "work" to (a list of today and tomorrow)
add 2 days from now to the end of the value for "work" in schedule
say schedule
say orders sorted by (a function taking o that gives back o's day)

let empty days be an empty list of dates
add today to the end of empty days
say empty days
say empty days is (a list of today)

to latest with stamps (a list of dates), giving back a date:
    give back the biggest of stamps
say latest with copied

a when is one of:
    at with moment (a date)
    never
let w be a new at with moment today
say w is (a new at with moment today)
say w is (a new at with moment yesterday)
say w
say a list of w and (a new never)
let twin be a copy of the first item of orders
say twin's day is (the first item of orders)'s day
