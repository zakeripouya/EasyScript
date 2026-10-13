Price list:
| item          | price  |
| coffee beans  | 12.50  |
| mug           | 8.00   |
| filter papers | 3.20   |

Total = 0
Repeat until item = "done":
    Ask "Item?" into item
    If item = "done", stop
    If item is not in Price list, show "Not on the list" and ask again
    Ask "How many?" into qty
    Total = Total + VLOOKUP(item, Price list, price) * qty
Show "Total: $" & Total
If Total > 50 then show "Discounted total: $" & Total * 0.9
