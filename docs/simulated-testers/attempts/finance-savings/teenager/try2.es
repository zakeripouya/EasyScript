ask "start money?" and call the answer money
ask "how much each year?" and call the answer yearly
ask "interest %?" and call the answer interest
repeat 5 times:
  add yearly to money
  add money times interest / 100 to money
  say money rounded to 2 decimal places
