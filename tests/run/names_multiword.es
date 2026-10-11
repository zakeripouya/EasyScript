note: names of several words, everywhere a name can go
keep hourly rate as 150.
keep HOURLY_LIMIT as 8
let late fee be 25.
let late fee total be late fee times 2
say late fee
say late fee total
say hourly rate times 2 plus late fee
set late fee to 30
add 5 to late fee
increase late fee by 1
multiply late fee total by 2
say late fee and " " and late fee total
let clientName be "Ada"
say "Hello, " followed by clientname followed by " and " followed by ClientName

note: functions with several words in their names, and inputs introduced by "to"
to add tax to amount:
    give back amount plus amount times 0.25
say add tax to 100
let taxed price be add tax to 80
say taxed price
to bill with hours worked and hourly price:
    give back hours worked times hourly price
say bill with 3 and hourly rate
to show menu:
    say "1. Coffee"
    say "2. Tea"
show menu
call show menu
note: "for", "from" and "in" introduce inputs too, and calls use the same word
to print receipt for customer name:
    say "Receipt for " followed by customer name
print receipt for "Ada"
print receipt with "Bo"
to busiest artist from artist names:
    give back the first item of artist names
say busiest artist from a list of "Cy" and "Di"
to count vowels in word:
    give back the number of items in only (the characters of word) that are "a"
say count vowels in "banana"
note: a sentence starting with a function's name calls it, even when its first word starts sentences
add tax to 4
say add tax to (add tax to 4)

note: loop names and answers of several words
count from 1 to 3 as session number:
    say "session " followed by session number
for each guest name in a list of "Ann" and "Bob":
    say "Welcome, " followed by guest name
say every guest score in (a list of 1 and 2) plus 10
ask "Your name? " and call the answer visitor name
say "[" followed by visitor name followed by "]"
write "notes" to file "n.txt"
read file "n.txt" and call it saved notes
say saved notes
let things be a list of 10, 20 and 30
let pick index be 2
say item pick index of things
say items pick index to the end of things

note: the longest known name wins; similar names stay separate
let total be 1
let total cost be 2
let total cost now be 3
say total plus total cost plus total cost now
let late_fee be 99
say late fee and " " and late_fee
