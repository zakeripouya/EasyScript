keep rate as 2.
let size be 3.
a box is a kind of thing with:
    a width, which is a number, starting at rate.
    a height, which is a number, starting at size.
    a label, which is text, starting at nothing.
    a lid, which is a box or nothing, starting at nothing.
    a weight, which is a number, starting at "heavy".
    a width, which is text.

a chain link is a kind of thing with:
    a next, which is a chain link.

an egg is a kind of thing with:
    a chicken, which is a chicken.
a chicken is a kind of thing with:
    an egg, which is an egg.

a pair is a kind of thing with:
    a left, which is a box, starting at 0.

a box is a kind of thing with:
    a size, which is a number.

if size is 3:
    a note card is a kind of thing with:
        a words, which is text.

keep favourite as a new box with height 1.
keep favourite width as size's width.
