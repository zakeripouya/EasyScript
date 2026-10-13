Ask "Bill amount?" and put the answer in Bill.
Ask "How many people?" and put the answer in People.
Ask "How was the service? (good, okay or bad)" and put the answer in Service.

TipRate = IF(Service = "good", 20%, IF(Service = "okay", 15%, 10%))
Tip = Bill * TipRate
Total = Bill + Tip
PerPerson = Total / People

Show "Tip: " & DOLLAR(Tip, 2)
Show "Total: " & DOLLAR(Total, 2)
Show "Each person pays: " & DOLLAR(PerPerson, 2)
