note: Every value is a number, text, or yes/no. The compiler works out which
note: before the program runs, mostly without being told.
to average of first and second:
    give back (first plus second) divided by 2
to describe with name and score:
    give back name and " scored " and score
to passed with score (a number), giving back yes or no:
    give back score is at least 50
let alice be average of 70 and 84
say describe of "Alice" and alice
if passed of alice, say "Alice passed."
let bob be average of 30 and 41
say describe of "Bob" and bob
if not passed of bob, say "Bob can try again."
