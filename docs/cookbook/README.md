# The EasyScript cookbook

Short programs for real jobs, one page each: what the program is for, the program, the data it reads, and exactly what it shows. Copy one, change the names and the numbers, and run it with `easyscript run NAME.es`.

Every recipe here is run by `make test`, and its page is checked against the files the test runs, so what you see is what EasyScript does today.

## How it's organized

One folder for each kind of work, with 2 or 3 recipes each. A recipe's page shows the beginner way to write it; some end with an "Under the hood" note about how it works or what to watch out for. Data files (`grades.csv`, `bank.csv`, ...) sit beside the recipes that read them.

| Kind of work | Recipes |
|---|---|
| [Teaching](teaching/README.md) | Each student's average and the top student; a quiz on capital cities; letter grades |
| [A small shop](shop/README.md) | A few months of sales; a till for the counter; what needs restocking |
| [A café or restaurant](cafe/README.md) | A week of sales; splitting the bill; drink prices by size |
| [Office admin](office/README.md) | Hours on a timesheet; days until each deadline |
| [Bookkeeping and finance](finance/README.md) | Spending per category from a bank export; savings over five years; large payments |
| [Healthcare admin](health/README.md) | Missed appointments per clinic; supplies to order |
| [Real estate](realestate/README.md) | Average prices per neighborhood; a mortgage calculator |
| [Shipping and logistics](shipping/README.md) | Shipments per country and the late ones; shipping costs by weight |
| [Events](events/README.md) | Guests and their meals; checking guests in |
| [A household budget](household/README.md) | A month's spending against the budget; the start-of-month check |
| [Hobbies and games](hobbies/README.md) | Who wins game night; guess the number; rolling dice |
| [A school science lab](lab/README.md) | Cooling readings; a density calculator; a unit converter |

## Where to go next

- [Your first programs](../language-guide.md#your-first-programs), the gentle start of the language guide.
- [The vocabulary](../vocabulary.md): every sentence EasyScript understands.
- [The examples](../../examples/README.md), including longer programs.
