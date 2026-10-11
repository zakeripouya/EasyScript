note: A high-score table that keeps the three best games, best first.

let games be a list of "Kim", "Lee", "Max", "Nia", "Oz" and "Pam"
let points be a list of 120, 340, 95, 510, 340 and 205

let top names be an empty list of text
let top points be an empty list of numbers

note: where a new score goes: after every score that's at least as good,
note: so a tie keeps the earlier game first
to find a place for score and table:
    let place be 1
    for each entry in table:
        if entry is at least score, increase place by 1
    give back place

count from 1 to the length of games as game:
    let score be item game of points
    let place be find a place for score and top points
    if place is at most 3:
        insert score at position place of top points
        insert item game of games at position place of top names
        if the length of top points is more than 3:
            remove the last item from top points
            remove the last item from top names
    say item game of games followed by " scored " followed by score followed by ": the top is now " followed by top names

say ""
say "High scores:"
count from 1 to the length of top names as rank:
    say rank followed by ". " followed by item rank of top names followed by " with " followed by item rank of top points

say ""
say "Total of the top three: " followed by top points added together
if top names contains "Oz", say "Oz tied Lee, but Lee got there first."
say "Everyone but the winner: " followed by only the games that are not item 1 of top names
say "Shortest name first: " followed by games sorted by their length
say "From the bottom: " followed by top points reversed
let late games be a list of "Quinn" and "Rae"
say "Everyone who played, late arrivals too: " followed by (games followed by late games)
