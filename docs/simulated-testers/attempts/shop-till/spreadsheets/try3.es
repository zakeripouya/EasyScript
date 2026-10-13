Let coffee beans be 12.50
Let mug be 8.00
Let filter papers be 3.20

Let Total be 0
Let item be ""
Repeat until item is "done":
    Ask "Item?" and call the answer item
    If item is "done", stop
    If item is not coffee beans or mug or filter papers, say "Not on the list"
    Ask "How many?" and call the answer qty
    Set Total to Total plus item times qty
Say "Total: $" plus Total
If Total is above 50, say "Discounted total: $" plus Total times 0.9
