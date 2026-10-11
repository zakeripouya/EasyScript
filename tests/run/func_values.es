note: functions as values: references, calls, functions made inside values, kinds

to double with x (a number):
    give back x times 2.
to greet with who:
    say "Hello, " followed by who followed by ".".
to roll:
    give back 4.
to apply twice with f and start:
    give back call f with (call f with start).
to pick with choice (yes or no), giving back a function taking a number and giving back a number:
    if choice, give back the function double.
    give back a function taking n that gives back n minus 1.

let twice be the function double.
say call twice with 21.
say twice with 4.
say twice of 5.
say twice using 6.
say apply twice with the function double and 5.
say apply twice with (a function taking n that gives back n plus 3) and 1.
say call (call pick with yes) with 10.
say call (call pick with no) with 10.

note: one line, and a block
let shout be a function taking word that gives back word followed by "!".
say shout with "hello".
let introduce be a function taking name (text) and age:
    say name followed by " is " followed by age.
call introduce with "Ann" and 7.
let roller be the function roll.
say call roller.
let answer be a function that gives back 42.
let found be call answer.
say found.
let hello be the function greet.
call hello with "Paris".

note: in lists, maps, records and kinds with variants
let steps be a list of the function double, (a function taking n that gives back n times n) and (a function taking n that gives back 0 minus n).
for each step in steps:
    say call step with 7.
say steps.
let handlers be an empty map from text to functions taking text.
set the value for "hello" in handlers to a function taking who:
    say "hello, " followed by who.
set the value for "bye" in handlers to the function greet.
call the value for "hello" in handlers with "Ann".
for each event and handler in handlers:
    call handler with event.
say handlers.

a button is a kind of thing with:
    label, which is text.
    action, which is a function taking text and giving back text.
let ok be a new button with label "OK" and action (a function taking s that gives back s in uppercase).
say call ok's action with "pressed".
set ok's action to a function taking s that gives back s followed by "?".
say call ok's action with "pressed".
say ok.

a step is one of:
    a fixed with amount (a number).
    a worked out with how (a function taking a number and giving back a number).
let plan be a list of (a new fixed with amount 3) and (a new worked out with how the function double).
for each part in plan:
    check which kind part is:
        when it's a fixed:
            say its amount.
        when it's a worked out:
            say call its how with 100.

note: the same function is equal to itself
let again be the function double.
say twice is again.
let lookalike be a function taking n that gives back n times 2.
say lookalike is twice.
