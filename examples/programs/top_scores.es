note: A high-score table that keeps the three best games, best first.

let games be a list of "Kim", "Lee", "Max", "Nia", "Oz" and "Pam"
let points be a list of 120, 340, 95, 510, 340 and 205

let top_names be an empty list of text
let top_points be an empty list of numbers

note: where a new score goes: after every score that's at least as good,
note: so a tie keeps the earlier game first
to place_for with score and table:
    let place be 1
    for each entry in table:
        if entry is at least score, increase place by 1
    give back place

count from 1 to the length of games as game:
    let score be item game of points
    let place be place_for with score and top_points
    if place is at most 3:
        insert score at position place of top_points
        insert item game of games at position place of top_names
        if the length of top_points is more than 3:
            remove the last item from top_points
            remove the last item from top_names
    say item game of games followed by " scored " followed by score followed by ": the top is now " followed by top_names

say ""
say "High scores:"
count from 1 to the length of top_names as rank:
    say rank followed by ". " followed by item rank of top_names followed by " with " followed by item rank of top_points

say ""
say "Total of the top three: " followed by top_points added together
if top_names contains "Oz", say "Oz tied Lee, but Lee got there first."
say "Everyone but the winner: " followed by only the games that are not item 1 of top_names
let by_length be a copy of games
sort by_length by their length
say "Shortest name first: " followed by by_length
say "From the bottom: " followed by top_points reversed
