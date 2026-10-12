note: Pieces of text in "say", separated by commas, and values in brackets inside text.
let name be "Ann".
let tasks be 3.
say "Hello,", name, "!".
say "You have", tasks, "tasks left.".
say "Done", "(", tasks, "of", 5, ")", ":", "yes", ";", "no", "?".
say "Cost: $", 5, "or", 10, "%".
say "a ", "b", " c", "", "d".
say "line one\n", "line two".
let scores be a list of 72, 95 and 88.
say "Scores:", scores, "and", yes.
let ages be a map of "Ann" to 30.
say "Age:", the value for "Bob" in ages, or 0.
show "show", 1. print "print", 2. display "display", 3.
say "Hello, [name]! You have [tasks plus 1] tasks.".
say "[tasks][tasks]", "[name]".
say "[[not a value]] and ]] alone".
let story be "Once".
set story to "[story] upon a time".
say story.
to shout with word:
    give back "[word in uppercase]!".
say shout with "hi".
if tasks is 3, say "three", "tasks".
