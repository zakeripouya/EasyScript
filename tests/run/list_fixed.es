note: fixed lists ("keep"): used directly where they are only looked at, copied where they could be changed
keep weekdays as a list of "Mon", "Tue" and "Wed".
keep primes at a list of 2, 3, 5 and 7
keep none_yet as an empty list of numbers
keep day_count as length of weekdays
say weekdays
say day_count followed by " " followed by length of primes followed by " " followed by (none_yet is empty) as text
say item 2 of weekdays followed by " " followed by the last item in primes
say weekdays contains "Tue"
for each d in weekdays:
    say d
say every p in primes times 10
say only the primes that are above 2
say items 2 to 3 of weekdays
say primes added together followed by " " followed by the biggest of primes
let mine be weekdays
add "Thu" to the end of mine
say mine
say weekdays
to count_items of things:
    add "x" to the end of things
    give back length of things
say count_items of weekdays
say weekdays
say a copy of primes
say primes reversed
say primes
note: a fixed list used inside a function, and in another fixed value
to first_day:
    give back the first item in weekdays
say first_day
keep flags as a list of yes, no and yes
say flags
say every f in flags is yes
