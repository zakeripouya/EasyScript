Ask "Bill amount?" and call the answer Bill.
Ask "How many people?" and call the answer People.
Ask "How was the service? (good, okay or bad)" and call the answer Service.

Let TipRate be IF(Service = "good", 20%, IF(Service = "okay", 15%, 10%))
Let Tip be Bill * TipRate
Let Total be Bill + Tip
Let PerPerson be Total / People

Show "Tip: $" plus ROUND(Tip, 2)
Show "Total: $" plus ROUND(Total, 2)
Show "Each person pays: $" plus ROUND(PerPerson, 2)
