Ask "Bill amount?" and call the answer Bill.
Ask "How many people?" and call the answer People.
Ask "How was the service? (good, okay or bad)" and call the answer Service.

Let TipRate be 10%
If Service = "good", set TipRate to 20%
If Service = "okay", set TipRate to 15%

Let Tip be Bill * TipRate
Let Total be Bill + Tip
Let PerPerson be Total / People

Show "Tip: $" plus Tip rounded to 2 decimal places
Show "Total: $" plus Total rounded to 2 decimal places
Show "Each person pays: $" plus PerPerson rounded to 2 decimal places
