note: A small tool that uses the words it's given:
note:   easyscript run main.es Ada Grace
if the number of words given to the program is 0:
    say "Give me some names, like: easyscript run main.es Ada Grace".
    stop the program, saying it failed.
for each name in the words given to the program:
    say "Hello, [name]! Your name has [the number of characters in name] letters."
