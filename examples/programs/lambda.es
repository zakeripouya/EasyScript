note: A tiny lambda calculus interpreter. Terms and results are kinds with
note: variants; numbers are Church numerals (a number n is a function that
note: applies f n times), read back by applying them to a built-in "add one".

a term is one of:
    a variable with a name (text).
    a function with a parameter (text) and a body (a term).
    an application with a callee (a term) and an argument (a term).
    an add one.
    a number literal with an amount (a number).

a result is one of:
    a closure with a parameter (text), a body (a term) and a scope (a map from text to results).
    a number result with an amount (a number).
    an increment.

note: Building terms.
to var with name:
    give back a new variable with name name.
to fn with parameter and body:
    give back a new function with parameter parameter and body body.
to applied with callee and argument:
    give back a new application with callee callee and argument argument.

to evaluate with t and scope, giving back a result:
    check which kind t is:
        when it's a variable:
            give back the value for (its name) in scope.
        when it's a function:
            give back a new closure with parameter (its parameter), body (its body) and scope scope.
        when it's an add one:
            give back a new increment.
        when it's a number literal:
            give back a new number result with amount (its amount).
        when it's an application:
            let callee be evaluate with (its callee) and scope.
            let argument be evaluate with (its argument) and scope.
            give back apply with callee and argument.

to apply with callee and argument, giving back a result:
    check which kind callee is:
        when it's a closure:
            let inner be a copy of its scope.
            set the value for (its parameter) in inner to argument.
            give back evaluate with (its body) and inner.
        when it's an increment:
            if argument is a number result:
                give back a new number result with amount (argument's amount plus 1).
            give back argument.
        otherwise:
            give back callee.

to written form of t, giving back text:
    check which kind t is:
        when it's a variable:
            give back its name.
        when it's a function:
            give back "(fn " followed by its parameter followed by ". " followed by (written form of its body) followed by ")".
        when it's an application:
            give back "(" followed by (written form of its callee) followed by " " followed by (written form of its argument) followed by ")".
        when it's an add one:
            give back "add-one".
        when it's a number literal:
            give back its amount as text.

note: The Church numeral for n: fn f. fn x. f (f (... x)).
to numeral of n, giving back a term:
    let inner be var with "x".
    repeat n times:
        set inner to applied with (var with "f") and inner.
    give back fn with "f" and (fn with "x" and inner).

note: A numeral's number: apply it to "add one" and 0.
to number of t, giving back text:
    let counted be evaluate with (applied with (applied with t and (a new add one)) and (a new number literal with amount 0)) and (an empty map from text to results).
    check which kind counted is:
        when it's a number result:
            give back its amount as text.
        otherwise:
            give back "not a number".

let addition be fn with "m" and (fn with "n" and (fn with "f" and (fn with "x" and (applied with (applied with (var with "m") and (var with "f")) and (applied with (applied with (var with "n") and (var with "f")) and (var with "x")))))).
let multiplication be fn with "m" and (fn with "n" and (fn with "f" and (applied with (var with "m") and (applied with (var with "n") and (var with "f"))))).
let two be numeral of 2.
let three be numeral of 3.

say "two is " followed by (written form of two).
say "two is " followed by (number of two).
say "two plus three is " followed by (number of (applied with (applied with addition and two) and three)).
say "two times three is " followed by (number of (applied with (applied with multiplication and two) and three)).
say "three times three plus two is " followed by (number of (applied with (applied with addition and (applied with (applied with multiplication and three) and three)) and two)).
