note: shapes.es: kinds of shape and what to work out about them.
note: A file another file uses only defines things.

a shape is one of:
    a circle with a radius (a number).
    a rectangle with a width (a number) and a height (a number).

keep full turn at 6.28318

to area of s (a shape), giving back a number:
    check which kind s is:
        when it's a circle:
            give back (its radius times its radius times full turn) divided by 2
        when it's a rectangle:
            give back its width times its height

to describe with s (a shape), giving back text:
    check which kind s is:
        when it's a circle:
            give back "a circle of radius " followed by its radius
        when it's a rectangle:
            give back "a " followed by its width followed by " by " followed by its height followed by " rectangle"

privately to tenths of x:
    give back (((x times 10) plus 0.5) mod 1000000 minus (((x times 10) plus 0.5) mod 1)) divided by 10

to summary of s (a shape), giving back text:
    give back (describe with s) followed by ", area " followed by (tenths of (area of s))
