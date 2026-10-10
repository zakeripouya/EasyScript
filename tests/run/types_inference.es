note: kinds worked out from how values are used, with no kinds written down
to halve with x:
    give back x divided by 2
say halve of 9
note: from what the function is given (text), and called before it's defined
say shout of "hello"
to shout with words:
    give back words followed by "!"
note: "and" joins text and is logical for yes/no, decided before the program runs
to label with name and count:
    give back name and ": " and count
say label of "apples" and 3
to both with first and second:
    give back first and second
say both of yes and no
note: comparing numbers and comparing text
to earlier with one and other:
    give back one is less than other
say earlier of 2 and 10
to before_word with one and other:
    give back one is less than other
say before_word of "apple" and "pear"
note: recursion, and a function that gives back nothing
to countdown from_n:
    if from_n is 0:
        say "lift off"
        give back nothing
    say from_n
    countdown of (from_n minus 1)
countdown of 3
note: text a function gives back, ignored when it's called as a sentence
shout "unused"
note: the same name with different kinds in blocks side by side
if yes:
    let thing be 1
    say thing plus 1
otherwise:
    let thing be "one"
    say thing
note: ask and file reads give text; length of gives a number
write "12" to file "n.txt"
read file "n.txt" and call it digits
say digits as a number plus length of digits
