Let prices be coffee beans $12.50, mug $8, filter papers $3.20.
Let total be 0.
Repeat:
    Ask "Which item?" and call the answer item.
    If item is "done", stop.
    If item is not in prices, say "This item is not in list" and ask again.
    Ask "How many?" and call the answer amount.
    Add price of item times amount to total.
Say total.
If total is more than 50, reduce total by 10 percent and say total.
