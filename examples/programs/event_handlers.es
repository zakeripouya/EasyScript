note: A map from event names to the functions that handle them.

to log with message:
    say "  log: " followed by message.

let handlers be an empty map from text to functions taking text.
let clicks be 0.
let typed be "".

set the value for "click" in handlers to a function taking target:
    add 1 to clicks.
    say "Clicked " followed by target followed by " (click " followed by clicks followed by ")".
set the value for "key" in handlers to a function taking letter:
    set typed to typed followed by letter.
    say "Typed so far: " followed by typed.
set the value for "close" in handlers to the function log.

let unknown be a function taking detail:
    say "Nobody handles that (" followed by detail followed by ")".

to handle with event (text), detail (text), table (a map from text to functions taking text) and fallback (a function taking text):
    call (the value for event in table, or fallback) with detail.

let events be a list of "click", "key", "key", "scroll", "click" and "close".
let details be a list of "OK button", "h", "i", "down", "Cancel button" and "window".
count from 1 to the number of items in events as n:
    handle with item n of events and item n of details and handlers and unknown.

say "Handled: " followed by the keys of handlers.
