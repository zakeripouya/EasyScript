note: Values that can be nothing have to be checked before they're used.
let word be "unbreakable".
let at be the position of "break" in word.
say at plus 1.
say (the position of "b" in word) times 2.
if at is 3, say "three".
if at is more than 2, say "far".
say character at of word.
check which kind at is:
    when it's nothing:
        say "none".
to bigger with n:
    give back n plus 1.
say bigger with at.
to maybe with n:
    if n is 0, give back 1.
    give back nothing.
let flag be "x" is "y".
check which kind flag is:
    when it's nothing:
        say 1.
    otherwise:
        say 2.
