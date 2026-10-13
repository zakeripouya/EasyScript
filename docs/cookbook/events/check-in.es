let guest list be a list of "Ana", "Cy", "Eli" and "Fay".
let arrived be an empty list of text.
forever:
    ask "Name (or done): " and call it name.
    if name is "done", stop the loop.
    if guest list contains name:
        add name to the end of arrived.
        say "Welcome, [name]!"
    otherwise:
        say "Sorry, [name] isn't on the list."
say the number of items in arrived, "of", the number of items in guest list, "guests are here."
