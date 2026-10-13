Table Supplies with columns Item, Unit, Minimum:
  gloves, boxes, 20
  masks, boxes, 15
  bandages, packs, 30
  hand gel, bottles, 10

For each row, ask "How many [Item] ([Unit]) do we have?" and put the answer in column On Hand

Add column Order = IF(On Hand < Minimum, Minimum * 2 - On Hand, 0)

Filter rows where Order > 0 and show Item, Order, Unit
If COUNTIF(Order > 0) = 0, show "Nothing to order this week."
