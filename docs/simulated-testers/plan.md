# Simulated testers: the plan

Part A of the v0.2 prep ([english-pass-log.md](../english-pass-log.md#v02-prep)). The goal: find what people who don't know EasyScript would naturally write that it doesn't accept, before the grammar freezes at v0.2.0. This is about phrasing, not correctness (`make test` covers that).

## The grid

- **12 sectors**, **2 tasks each**: one data or report task (a csv given in the task: totals, the best, per month, a chart) and one interactive or rules task (asking questions, choices, prices, a quiz, a game, a schedule).
- **5 kinds of people**, and **every task is attempted by all five**, so every kind appears in every sector: 24 tasks × 5 people = **120 attempts**.
- **4 tasks EasyScript can't do yet** (email, an Excel file, the internet, a website). For these, success means a clear, honest error that says it can't do that yet; a confusing error is a failure.

### The people

These descriptions are exactly what the writing agent is given about each person.

| Key | Kind of person | Description given to the agent |
|---|---|---|
| `never-coded` | Has never coded | Has never written a program of any kind. Writes the way they'd write an email or a note to a coworker: whole sentences, polite, describing what they want. |
| `spreadsheets` | Lives in spreadsheets | Works in Excel and Google Sheets every day: thinks in columns, rows, SUM, AVERAGE, COUNTIF, VLOOKUP and pivot tables. Has never written code outside a spreadsheet formula. |
| `teenager` | A teenager | 15 years old. Writes casually and briefly, often lowercase, sometimes slangy. Tried Scratch at school once. Impatient: tries the shortest thing that might work. |
| `non-native` | A non-native English speaker | Learned English as a second language. Writes simple, sometimes ungrammatical English: drops "the" and "a", mixes up verb forms and prepositions, uses short sentences. |
| `programmer` | A programmer from Python or JavaScript | Writes Python and JavaScript every day. Expects variables, `=`, loops, functions, brackets and dot notation, and reaches for them first. |

### How each attempt runs

1. A separate agent writes the program. It's given **only** the task, the person's description, and: "EasyScript is a programming language that understands English sentences. Write the program the way this person would." It never sees the language guide, the vocabulary, the examples or any EasyScript code. One agent per sector writes that sector's 10 attempts, and gets their errors back (so what it learns from errors stays within a sector).
   - **How it's kept blind:** each agent is a separate `claude -p` session started in an empty folder outside the repository, with no tools and a plain system prompt ("You are a helpful assistant."), so it can't read files and loads no project instructions. Retries continue the same session with `--resume`.
   - **A first run was thrown away.** It used subagents started inside the repository, which load the repository's CLAUDE.md (the whole grammar) automatically; their programs used exact EasyScript phrases (`read the table in`, `ask for an amount`) that the person couldn't have known. None of that run is kept.
2. The program is run with `easyscript run` (the compiler as of master `cd03c6a`, kept as a fixed copy so later fixes don't change the results) in an empty folder holding the task's files, with the task's answers as typed input, the test clock (`ES_NOW`) and a fixed random seed, and a 10-second limit.
3. If it fails (an error before or while running), the agent is given only what the person would see (the error message) and tries again, up to 3 tries in all. A program that runs isn't retried, even if its output is wrong: the person may not notice.
4. Every try is kept: `tryN.es`, `tryN.txt` (its output and errors, the exit code, and the files it made).
5. The final result is classified:
   - **(a) worked** and did what the task asked;
   - **(b) failed at first, but an error's suggestion led to a fix** that then worked;
   - **(c) stuck:** the errors or suggestions didn't help within 3 tries (for "can't do yet" tasks: the error didn't say clearly that EasyScript can't do it);
   - **(d) ran but did something different from what was meant:** checked against the task, not just the exit code. The worst case.

   A "can't do yet" task counts as (a) when the first error says clearly that EasyScript can't do it, and (b) when a later one does.
   A program that runs and does most of the task but leaves a part out silently is (d) when what it prints is wrong or misleading, and (a) with a note when it's right but incomplete because the person left something out.

Everything is saved under `attempts/SECTOR-TASK/PERSON/`, with `result.md` giving the class and a one-line reason.

## The tasks

Each task's folder under `tasks/` has `task.md` (the person's words, given to the agent as written), its data files, and `answers.txt` (what the person types) where it asks questions.


### 1. Teaching

**Data: `teaching-report`**

> I have my class's test results in a file called grades.csv (below). I want to see each student's average score, find out who has the best average, count how many test results were below 50, and save a bar chart of the averages as averages.svg.

`grades.csv`:

```
student,subject,score
Amira,Math,82
Amira,Science,91
Amira,English,77
Ben,Math,45
Ben,Science,58
Ben,English,62
Chloe,Math,95
Chloe,Science,88
Chloe,English,93
Dev,Math,38
Dev,Science,49
Dev,English,71
```


**Interactive: `teaching-quiz`**

> I want a little quiz for my class about capital cities. Ask 5 questions one at a time (France, Japan, Kenya, Canada, Brazil), tell the student if each answer is right or wrong (and the right answer if wrong), and at the end say their score out of 5. If they got 4 or more, also say "Well done!". Capitals should count as right whatever capital letters the student uses.

`answers.txt` (typed answers, one per line):

```
paris
Tokyo
Nairobi
toronto
Brasilia
```


### 2. A small shop

**Data: `shop-report`**

> My shop's sales are in sales.csv (below). I want the total money we took, the product that sold the most units, the money taken per month, and a bar chart of the money per month saved as months.svg.

`sales.csv`:

```
date,product,quantity,price
2026-08-03,Coffee beans,4,$12.50
2026-08-03,Mug,2,$8.00
2026-08-17,Filter papers,10,$3.20
2026-09-02,Coffee beans,6,$12.50
2026-09-14,Mug,1,$8.00
2026-09-21,Grinder,1,$45.00
2026-10-01,Coffee beans,3,$12.50
2026-10-05,Filter papers,12,$3.20
```


**Interactive: `shop-till`**

> I want a simple till for my shop counter. Prices: coffee beans $12.50, mug $8, filter papers $3.20. Keep asking for an item and how many, until I type "done". Then show the total. If the total is over $50, take 10% off and show the discounted total. If I type an item that isn't on the list, say so and ask again.

`answers.txt` (typed answers, one per line):

```
mug
3
coffee beans
2
teapot
filter papers
5
done
```


### 3. A restaurant or café

**Data: `cafe-report`**

> Last week's café orders are in orders.csv (below). Show the money taken each day, the most popular item (by number of orders), the average order price, and which day was busiest (most orders).

`orders.csv`:

```
day,item,size,price
Monday,Latte,large,4.50
Monday,Tea,small,2.00
Monday,Latte,small,3.50
Tuesday,Espresso,small,2.50
Tuesday,Latte,large,4.50
Wednesday,Cappuccino,large,4.80
Wednesday,Tea,large,2.80
Wednesday,Latte,small,3.50
Wednesday,Espresso,small,2.50
Thursday,Tea,small,2.00
Friday,Latte,large,4.50
Friday,Cappuccino,small,3.80
```


**Interactive: `cafe-bill`**

> I want a bill splitter for my café's tables. Ask for the bill amount, how many people, and how the service was (good, okay or bad). The tip is 20% for good, 15% for okay and 10% for bad. Show the tip, the total with tip, and how much each person pays, in dollars and cents.

`answers.txt` (typed answers, one per line):

```
86.40
3
good
```


### 4. Office admin

**Data: `office-report`**

> Our timesheets are in timesheet.csv (below). For each employee, show their total hours for the week. List anyone who worked more than 40 hours and how many overtime hours they did (hours over 40). Also show the total hours for the whole team.

`timesheet.csv`:

```
employee,day,hours
Rosa,Mon,9
Rosa,Tue,8.5
Rosa,Wed,9
Rosa,Thu,9
Rosa,Fri,7
Tom,Mon,8
Tom,Tue,8
Tom,Wed,8
Tom,Thu,6
Tom,Fri,8
Lena,Mon,10
Lena,Tue,10
Lena,Wed,9.5
Lena,Thu,8
Lena,Fri,6
```


**Interactive: `office-email`** (can't do yet: email)

> Every Monday at 9am, I want to send the week's meeting room bookings (in bookings.csv, below) by email to team@ourcompany.com, with the subject "Room bookings this week".

`bookings.csv`:

```
room,day,time,booked by
Blue,Monday,10:00,Rosa
Green,Tuesday,14:00,Tom
Blue,Thursday,09:30,Lena
```


### 5. Bookkeeping and finance

**Data: `finance-report`**

> My bank export is in bank.csv (below). Show how much I spent in each category, how much per month, my single biggest expense, and save a pie chart of spending per category as categories.svg. One row has no amount yet (the bank hasn't filled it in); it should just be left out.

`bank.csv`:

```
date,description,category,amount
2026-08-01,Rent,Housing,$1200
2026-08-04,Groceries,Food,$96.40
2026-08-11,Electric bill,Utilities,$71.25
2026-08-19,Groceries,Food,$110.15
2026-09-01,Rent,Housing,$1200
2026-09-06,Restaurant,Food,$58.00
2026-09-15,Internet,Utilities,$45.00
2026-09-22,Pharmacy,Health,
2026-09-28,Groceries,Food,$87.30
```


**Interactive: `finance-savings`**

> I want a savings calculator. Ask how much money I start with, how much I add every year, and the yearly interest rate in percent. Then show my balance at the end of each year for 5 years (interest is added at the end of each year, after that year's deposit), rounded to cents.

`answers.txt` (typed answers, one per line):

```
1000
500
4
```


### 6. Healthcare admin (scheduling and supplies only)

**Data: `health-report`**

> Our clinic's appointments are in appointments.csv (below). For each clinic, show how many appointments were missed, and what percent of its appointments were missed. Also show which day of the week had the most appointments.

`appointments.csv`:

```
date,weekday,clinic,status
2026-10-05,Monday,North,kept
2026-10-05,Monday,North,missed
2026-10-05,Monday,South,kept
2026-10-06,Tuesday,North,kept
2026-10-06,Tuesday,South,missed
2026-10-06,Tuesday,South,missed
2026-10-07,Wednesday,North,cancelled
2026-10-07,Wednesday,South,kept
2026-10-08,Thursday,North,kept
2026-10-08,Thursday,North,kept
2026-10-08,Thursday,North,missed
2026-10-09,Friday,South,kept
```


**Interactive: `health-supplies`**

> I check our clinic's supplies every Friday. The supplies and the lowest number we should have of each are: gloves (boxes) 20, masks (boxes) 15, bandages (packs) 30, hand gel (bottles) 10. Ask me how many of each we have now. Then list the ones that are below their lowest number and how many to order to get back up to twice that number. If nothing needs ordering, say "Nothing to order this week."

`answers.txt` (typed answers, one per line):

```
25
8
30
3
```


### 7. Real estate

**Data: `realestate-excel`** (can't do yet: an Excel file)

> I keep my house listings in an Excel file, listings.xlsx. I want the average price per neighborhood and the cheapest house in each.

`listings.xlsx`: a stand-in for an Excel workbook (starts with the zip signature `PK`).


**Interactive: `realestate-mortgage`**

> I want a mortgage calculator for clients. Ask for the house price, the deposit, the yearly interest rate in percent, and the number of years. Show the loan amount and the monthly payment (standard formula: payment = loan × r / (1 − (1 + r)^(−n)), where r is the monthly rate and n the number of months), rounded to cents, and the total paid over the whole loan.

`answers.txt` (typed answers, one per line):

```
350000
70000
6
25
```


### 8. Shipping and logistics

**Data: `shipping-report`**

> Our shipments are in shipments.csv (below). Show how many shipments went to each country, the average weight, the heaviest shipment, and list the shipments that took more than 5 days from shipped to delivered.

`shipments.csv`:

```
id,country,weight kg,shipped,delivered
A100,France,12.5,2026-09-01,2026-09-04
A101,Germany,3.2,2026-09-01,2026-09-08
A102,France,7.8,2026-09-03,2026-09-05
A103,Spain,22.0,2026-09-05,2026-09-12
A104,Germany,1.1,2026-09-06,2026-09-09
A105,Italy,15.4,2026-09-08,2026-09-11
```


**Interactive: `shipping-tracking`** (can't do yet: the internet)

> For each tracking number in tracking.txt, look it up on the carrier's website (https://track.example-carrier.com/?id=NUMBER) and tell me which parcels are delivered.

`tracking.txt`:

```
EC1029384
EC1029391
EC1029402
```


### 9. Events

**Data: `events-report`**

> The guest list for our event is in guests.csv (below). Show how many people said yes, the total headcount (people who said yes plus their plus-ones), how many of each meal we need for the people coming (plus-ones eat the same meal as their guest), and save a pie chart of the meals as meals.svg.

`guests.csv`:

```
name,rsvp,meal,plus ones
Ana,yes,fish,1
Bo,no,veggie,0
Cy,yes,veggie,0
Dee,maybe,fish,1
Eli,yes,chicken,2
Fay,yes,veggie,1
Gus,no,chicken,0
```


**Interactive: `events-website`** (can't do yet: a website)

> I want a web page where guests can type their name and choose a meal to sign up for our event, and the sign-ups get saved to signups.csv.


### 10. A household budget

**Data: `household-report`**

> Our household spending is in spending.csv (below), and our monthly budget is: groceries $400, eating out $150, transport $120, fun $100. For each month, show how much we spent in each category, and say which categories went over budget that month and by how much. Also save a bar chart of the total spent per month as months.svg.

`spending.csv`:

```
date,category,amount
2026-08-02,groceries,$120.50
2026-08-09,groceries,$98.20
2026-08-12,eating out,$64.00
2026-08-15,transport,$45.00
2026-08-20,eating out,$102.30
2026-08-28,fun,$35.00
2026-09-03,groceries,$210.75
2026-09-10,groceries,$205.10
2026-09-14,transport,$130.00
2026-09-19,eating out,$48.60
2026-09-25,fun,$120.00
```


**Interactive: `household-check`**

> At the start of each month I want to check our budget. Ask for our income for the month and then each regular bill: rent, utilities, insurance and phone. Show what's left after the bills. If what's left is less than $500, warn us. Then ask how much we want to save each month and tell us how many months it takes to reach a $5,000 emergency fund.

`answers.txt` (typed answers, one per line):

```
4200
1650
180
95
60
400
```


### 11. Hobbies and games

**Data: `hobbies-report`**

> Our board game nights are recorded in games.csv (below). Who has won the most games? What's the average winning score for each game? And show the top 3 winning scores with who got them and in which game.

`games.csv`:

```
date,game,winner,score
2026-09-04,Catan,Maya,10
2026-09-04,Ticket to Ride,Jon,124
2026-09-11,Catan,Jon,11
2026-09-11,Azul,Maya,78
2026-09-18,Ticket to Ride,Maya,141
2026-09-18,Azul,Priya,85
2026-09-25,Catan,Maya,10
2026-09-25,Ticket to Ride,Priya,118
```


**Interactive: `hobbies-game`**

> Make a number guessing game: the computer picks a random whole number from 1 to 10, then keeps asking me to guess. After each wrong guess it says "Too high" or "Too low". When I get it right, it says "You got it in N guesses!"

`answers.txt` (typed answers, one per line):

```
1
2
3
4
5
6
7
8
9
10
```


### 12. A school science lab

**Data: `lab-report`**

> Our class measured how fast water cools. The readings are in readings.csv (below), in degrees Celsius. For each group, show the average temperature and the biggest drop between two readings in a row. Also show every reading in Fahrenheit (F = C × 9 / 5 + 32) and save a line chart of group A's temperatures as cooling.svg.

`readings.csv`:

```
group,minute,temperature
A,0,80
A,5,71
A,10,64
A,15,59
B,0,82
B,5,70
B,10,62
B,15,57
```


**Interactive: `lab-density`**

> For our density lab: ask for an object's name, its mass in grams and its volume in cubic centimeters. Work out its density (mass divided by volume), rounded to 2 decimal places, and say whether it floats or sinks in water (it floats if the density is less than 1). Then ask if there's another object; stop when the answer is no.

`answers.txt` (typed answers, one per line):

```
cork
4.8
20
yes
steel bolt
31.4
4
no
```

