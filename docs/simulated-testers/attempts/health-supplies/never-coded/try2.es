let gloves be 20
let masks be 15
let bandages be 30
let hand gel be 10

ask "How many boxes of gloves do we have now?" and call the answer gloves now
ask "How many boxes of masks do we have now?" and call the answer masks now
ask "How many packs of bandages do we have now?" and call the answer bandages now
ask "How many bottles of hand gel do we have now?" and call the answer hand gel now

if gloves now is less than gloves, say "Please order " and gloves times 2 minus gloves now and " boxes of gloves."
if masks now is less than masks, say "Please order " and masks times 2 minus masks now and " boxes of masks."
if bandages now is less than bandages, say "Please order " and bandages times 2 minus bandages now and " packs of bandages."
if hand gel now is less than hand gel, say "Please order " and hand gel times 2 minus hand gel now and " bottles of hand gel."

if nothing needs ordering, say "Nothing to order this week."
