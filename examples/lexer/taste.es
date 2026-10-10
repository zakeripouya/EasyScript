note: Count down from 3, then greet someone.
set count to 3.
repeat while count is greater than 0:
    say count.
    subtract 1 from count.
say "Liftoff!".

to greet using name:
    say "Hello, " + name + "!".

greet using "Ada".
