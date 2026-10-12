note: Named kinds of text with a rule: "an email is text that must contain "@"."
an email is text that must contain "@".
a customer has a name and an email.
let ann be a new customer with name "Ann" and email "ann@example.com".
let typed be "bob@example.com" as an email.
say ann's email, typed, typed contains "bob", ann's email is "ann@example.com".
to send with message and address (an email):
    say "Sending", message, "to", address.
send with "hi" and typed.
send with "hello" and ann's email.
set ann's email to "ann@work.com".
say ann, the length of typed, typed in uppercase.
let emails be a list of typed and ann's email.
say emails.
a phone number is text that must start with "+".
a contact has a phone number and a label.
let office be a new contact with phone number "+1 555 0100" and label "office".
say office, office's phone number starts with "+1".
let numbers be a list of "+44 20" as a phone number, office's phone number.
say numbers.
