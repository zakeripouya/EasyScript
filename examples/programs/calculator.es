note: A calculator that reads sentences like "2 plus 3 times 4", builds a
note: tree from a recursive kind (times and "divided by" before plus and
note: minus), shows it with parentheses, and works it out.

an expression is one of:
    a number value with an amount (a number).
    a sum with a left (an expression) and a right (an expression).
    a difference with a left (an expression) and a right (an expression).
    a product with a left (an expression) and a right (an expression).
    a quotient with a left (an expression) and a right (an expression).

a reader is a kind of thing with:
    pieces, which is a list of text.
    a position, which is a number, starting at 1.

to next word in r:
    if r's position is more than the number of items in r's pieces, give back "".
    give back item (r's position) of r's pieces.

to take number from r, giving back an expression:
    let word be next word in r.
    add 1 to r's position.
    give back a new number value with amount (word as a number).

note: A product is numbers joined by "times" or "divided by".
to take product from r, giving back an expression:
    let result be take number from r.
    while (next word in r is "times") or (next word in r is "divided"):
        let operator be next word in r.
        add 1 to r's position.
        if operator is "divided", add 1 to r's position.
        let other be take number from r.
        if operator is "times":
            set result to a new product with left result and right other.
        otherwise:
            set result to a new quotient with left result and right other.
    give back result.

note: A sum is products joined by "plus" or "minus".
to take sum from r, giving back an expression:
    let result be take product from r.
    while (next word in r is "plus") or (next word in r is "minus"):
        let operator be next word in r.
        add 1 to r's position.
        let other be take product from r.
        if operator is "plus":
            set result to a new sum with left result and right other.
        otherwise:
            set result to a new difference with left result and right other.
    give back result.

to written form of e, giving back text:
    check which kind e is:
        when it's a number value:
            give back its amount as text.
        when it's a sum:
            give back "(" followed by (written form of its left) followed by " + " followed by (written form of its right) followed by ")".
        when it's a difference:
            give back "(" followed by (written form of its left) followed by " - " followed by (written form of its right) followed by ")".
        when it's a product:
            give back "(" followed by (written form of its left) followed by " * " followed by (written form of its right) followed by ")".
        when it's a quotient:
            give back "(" followed by (written form of its left) followed by " / " followed by (written form of its right) followed by ")".

to evaluate with e, giving back a number:
    check which kind e is:
        when it's a number value:
            give back its amount.
        when it's a sum:
            give back (evaluate with its left) plus (evaluate with its right).
        when it's a difference:
            give back (evaluate with its left) minus (evaluate with its right).
        when it's a product:
            give back (evaluate with its left) times (evaluate with its right).
        when it's a quotient:
            give back (evaluate with its left) divided by (evaluate with its right).

let questions be a list of "2 plus 3 times 4", "2 times 3 plus 4", "10 minus 4 minus 3" and "8 divided by 2 times 3".
for each question in questions:
    let tree be take sum from (a new reader with pieces (the words of question)).
    say question followed by " is " followed by (written form of tree) followed by " = " followed by (evaluate with tree).
