Let coffee beans be 12.50.
Let mug be 8.
Let filter papers be 3.20.
Let total be 0.
Let item be "".
Repeat until item is "done":
    Ask "Which item?" and call the answer item.
    If item is "done", stop.
    If item is not coffee beans and item is not mug and item is not filter papers, say "This item is not in list".
    Ask "How many?" and call the answer amount.
    Add (price of item) times amount to total.
Say total.
If total is more than 50, reduce total by 10 percent and say total.
