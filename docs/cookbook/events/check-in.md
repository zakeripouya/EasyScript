# Check guests in at the door

Ask each arriving guest's name until someone types done, welcome the ones on the list, and say how many have arrived.

## The program

`check-in.es`:

```
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
```

## Running it

When it's run with `easyscript run check-in.es` and these answers are typed (`Ana`, `Bob`, `Eli`, `done`), it shows (the questions and answers run together here, because the typed answers aren't shown):

```
Name (or done): Welcome, Ana!
Name (or done): Sorry, Bob isn't on the list.
Name (or done): Welcome, Eli!
Name (or done): 2 of 4 guests are here.
```
