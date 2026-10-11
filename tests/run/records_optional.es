# Fields that can be nothing, and checking them before use.
a node is a kind of thing with:
    a value, which is a number.
    a next, which is a node or nothing.

let first be a new node with value 1.
say first.
if first's next is nothing, say "no next yet".
set first's next to a new node with value 2 and next nothing.
if first's next is not nothing:
    say first's next's value.
    set first's next's next to a new node with value 3.
otherwise:
    say "never".
say first.

note: "otherwise" knows the check was false
if first's next is nothing:
    say "never".
otherwise if first's next's value is 2:
    say "second is 2".

note: "and" and "or" know what was checked on their left
if first's next is not nothing and first's next's value is 2, say "and works".
if first's next is nothing or first's next's value is 2, say "or works".
if not (first's next is nothing), say first's next's value.

note: a variable holding a node or nothing
let current be first's next.
while current is not nothing:
    say "at " followed by current's value.
    set current to current's next.
if current is nothing, say "walked off the end".

note: until a check is true
let walker be first's next.
repeat until walker is nothing:
    say "walking " followed by walker's value.
    set walker to walker's next.

note: after "if X is nothing, ..." that leaves, X isn't nothing
to second value of start (a node), giving back a number:
    let after be start's next.
    if after is nothing, give back 0.
    give back after's value

say second value of first.
say second value of (a new node with value 5).

note: giving back nothing, and inputs that can be nothing
to find with wanted and start (a node or nothing), giving back a node or nothing:
    let at be start.
    while at is not nothing:
        if at's value is wanted, give back at.
        set at to at's next.
    give back nothing

let found be find with 3 and first.
if found is not nothing, say "found " followed by found's value.
if find with 9 and first is nothing, say "9 isn't there".
say find with 2 and first.
say find with 7 and nothing.

note: kinds worked out from use
to total of start:
    if start is nothing, give back 0.
    give back start's value plus (total of start's next)

say total of first.

note: a record and a record or nothing are compared by which record they are
let second be a new node with value 2.
if first's next is second, say "wrong".
