read the table in "spending.csv", with amount in dollars, and call it spending.
let budget be a map of "groceries" to $400, "eating out" to $150, "transport" to $120 and "fun" to $100.
for each category and its total in the total amount of spending per category:
    let limit be the value for category in budget.
    if total is over limit:
        say category, "is over budget by", total minus limit.
    otherwise:
        say category, "is within budget, with", limit minus total, "left."
save a bar chart of the total amount of spending per category as "september.svg", titled "September".
