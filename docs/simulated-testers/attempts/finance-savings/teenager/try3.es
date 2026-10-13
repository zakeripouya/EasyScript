ask "start money?" and call the answer money
ask "how much each year?" and call the answer yearly
ask "interest %?" and call the answer interest
let cash be number money
repeat 5 times:
  add number yearly to cash
  add cash times number interest / 100 to cash
  say cash rounded to 2 decimal places
