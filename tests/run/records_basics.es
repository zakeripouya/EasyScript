# Records: defining kinds, making records, reading and changing fields.
a dog is a kind of thing with:
    a name, which is text.
    an age, which is a number, starting at 0.
    a good boy, which is yes or no, starting at yes.
    a nickname, which is text, starting at "none".

a person is a kind of thing with:
    a name, which is text.
    a pet, which is a dog.
    a start time, which is a number, starting at 9.

an order is a kind of thing with:
    a customer, which is a person.
    a total, which is a number.

let rex be a new dog with name "Rex" and age 3.
say rex.
say rex's name.
say rex's age plus 1.
say length of rex's name.

note: changing fields
set rex's age to 4.
add 1 to rex's age.
subtract 2 from rex's age.
increase rex's age by 10.
decrease rex's age by 1.
multiply rex's age by 2.
divide rex's age by 4.
say rex's age.
set rex's name to rex's name followed by "!".
set rex's good boy to not rex's good boy.
say rex.

note: names of several words, and records inside records
let ann be a new person with name "Ann" and pet rex.
say ann's start time.
set ann's start time to 10.
let shop order be a new order with customer ann and total 25.
say the shop order's customer's name.
say shop order's customer's pet's name.
set shop order's customer's pet's age to 7.
say rex's age.
say shop order.

note: the field's value is worked out where it's written
let fido be a new dog with name "Fi" followed by "do" and age 2 times 3.
say fido.

note: sharing: two names, one record
let same dog be rex.
set same dog's name to "Rexy".
say rex's name.
if same dog is rex, say "the same record".
if fido is not rex, say "different records".

note: a copy is a new record whose fields hold the same values
let twin be a copy of fido.
set twin's name to "Twin".
say fido's name followed by " and " followed by twin's name.
if twin is fido, say "wrong".
let ann copy be a copy of ann.
set ann copy's pet's age to 100.
say rex's age.

note: records in parentheses
say (a new dog with name "Pup")'s age.
say (a copy of rex)'s name.

note: as text and joined
let line be "Dog: " followed by fido.
say line.
say fido as text.
