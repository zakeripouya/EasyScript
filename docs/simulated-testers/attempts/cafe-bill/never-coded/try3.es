ask "What is the bill amount?" and call the answer bill
ask "How many people are sharing the bill?" and call the answer people
ask "How was the service? Please type good, okay or bad." and call the answer service

if the service is good, let the tip be 20% of bill
if the service is okay, let the tip be 15% of bill
if the service is bad, let the tip be 10% of bill

let the total be the bill plus the tip
let share be the total divided by the people

say "The tip is $" and the tip
say "The total including the tip is $" and the total
say "Each person needs to pay $" and share
