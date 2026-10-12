note: keys from a file, one at a time (keys_scripted.in)
wait for a key and call it first
say first
repeat 12 times:
    let key be the key being pressed
    if key is nothing:
        say "(no key)"
    otherwise:
        say key
ask "Name? " and call it name
say "Hello, " followed by name
wait for a key and call it last
say last
let after be the key being pressed
say after
