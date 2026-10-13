# EasyScript English Pass

Oct 11, 2026 · @Pouya Zakeri

**How to use this file:** each English-pass step reads this file first. Decisions are under [Decided](#decided-2026-10-11). Examples written with brackets (`"Hello, [name]!"`) mean the comma form. When the plan changes, edit this file and add a row to its [update log](#update-log).

The English pass reshapes EasyScript so someone who has never coded can build real programs, with no AI needed. It runs after step 22 and before v0.2.0, and ends with freezing the core grammar.

## Goal and success test

**Goal:** someone who can't read or write code can make real programs by writing plain English, without an AI model. Programmers can still drop to loops, maps and types whenever they want.

**Success test, run at every milestone:** someone who has never coded builds a working quiz in 30 minutes, with no help.

**Who it's for, from the research:** end-user programmers (people writing programs for their own work, not as their job) outnumber professional programmers roughly 18 to 1. They mostly want to automate repetitive work, analyze data, build a small site or form, make a game, or teach.

**What we keep:** every sentence still means exactly one thing. Ambiguity is an error with suggestions, never a guess.

## Where it fits in the plan

1. **Step 20:** done (functions as values, built-ins taking functions, interfaces).
2. **Principles prompt:** done (rules in [docs/design-principles.md](design-principles.md), docs/english-pass.md, roadmap updated).
3. **Step 21, modules and standard library:** plus dates and times, csv, files and folders, random, clear the screen.
4. **Step 22, system tools:** plus raw keyboard input, a Snake example and a JSON parser.
5. **English pass:** done (parts 1 to 5, tags `english-pass-part-2` to `english-pass-part-5`; see [english-pass-log.md](english-pass-log.md)). Every high-level form is benchmarked against a hand-written loop (`make bench`). What was left for later is in [Later](#later). Charts followed: `save a bar chart of X as F` (and line and pie charts), drawn by `lib/charts.es`, written in EasyScript.
6. **Playground lite (1 to 2 days):** `easyscript playground` opens a local page with an editor, templates and output, running code through the real compiler. Used only for user testing.
7. **Wizard of Oz study and first non-coder test:** 3 to 5 people, think-aloud, 30-minute quiz challenge; fix what they stumble on.
8. **Starter templates,** license, install docs, then **v0.2.0** and **freeze the core grammar** (after this, only additions).
9. **Phase 3, self-hosting.**
10. **Public browser playground:** an EasyScript interpreter written in EasyScript, compiled to WebAssembly, so nothing runs on a server and nothing is installed. Templates, `explain`, `why`, live values, levels, charts. This is the launch for non-coders.
11. **Phase 4:** notebook, native backend, JIT, networking, persistence, the web kit, ending in EasyScript Studio: a standalone app for regular people (templates, editor with live values, built-in chat panel, charts, "make it an app"), first built as a native window with a web view, all in EasyScript. Playground lite is its first prototype.

**Why before the freeze:** after Phase 3 the compiler is written in EasyScript, so every grammar change would mean changing two compilers and possibly the compiler's own source.

## Design rules for every new phrase

| Rule | What it means | Comes from |
| --- | --- | --- |
| Deterministic | A sentence means one thing; ambiguity is an error with both rewrites | EasyScript's founding rule |
| One meaning per word | Synonyms are fine; a word with two meanings needs a documented exception (`and`, `is` are reviewed) | Simplified Technical English |
| Common words only | Keywords come from the \~3,000 most common English words; no idioms unless a plain form is also accepted (`every 2nd item` beside `every other item`) | Plain language, Oxford 3000, non-native speakers |
| Plain reading level | Errors and beginner docs at grade 6 to 8, checked by an audit test | Plain Writing Act, Flesch-Kincaid |
| No jargon for beginners | Errors, beginner docs and the playground never say: variable, function, parameter, argument, return, string, boolean, integer, float, array, index, null, exception, syntax, compile, runtime, type, object, instance, iterate, initialize, declare, scope (audit test) | Natural Programming research |
| Short and flat | Short sentences, nesting no deeper than about 2; name things instead of nesting them | Cognitive load theory |
| Distinct trigger words | Each pattern starts with its own words, so patterns never overlap | Attempto Controlled English |
| Exact meaning in the core | Every high-level phrase has a documented meaning in plain loops, ifs and maps, so programmers can trust it and drop below it | Layered design (Python, SQL, Rust) |
| Intent stays visible | Sorting, top N, grouping and filtering stay their own compiler nodes, so the optimizer can see them | SQL query planners |
| Reviewed before added | Each phrase is checked against the Cognitive Dimensions checklist and goes through docs/proposals/ | Green and Petre |
| Readable aloud | If a line is awkward to say, it's awkward to read | Screen readers, voice programming |
| Reads like text | If a line could appear unchanged in an email to a coworker, it's right. Prefer English that a person would write over symbols, brackets and programmer patterns. The other rules still apply: one meaning, distinct trigger words, and an error with both readings when something is ambiguous | Plain language; "readable aloud" taken one step further |
| Not too many patterns | The six choice patterns, quantifiers, `whose`, `then` and `per` cover most speech; anything more needs a proposal | Inform 7's lesson |

## Why each feature: the evidence

Every feature in this pass traces to a finding; the Research section has the sources.

| Feature | Why | Evidence |
| --- | --- | --- |
| Quantifiers (`how many`, `any`, `every`) and group actions | Non-programmers act on whole groups and almost never write loops | Pane: 95% of operations on many objects were sets, 5% loops |
| Event rules (`when`, `whenever`) | The most common way non-programmers describe behavior | Pane: 54% of statements were rules or events |
| Depends-on tables, adjectives, `must` rules instead of `if` logic | Boolean expressions are rare and error-prone for beginners; match forms beat them | Pane's thesis: Boolean keywords and parentheses tested poorly; match forms did significantly better |
| Asking when `and`/`or` is unclear | People use "and" for "and then" and "or" to restate | Pane: 29% and 24% of uses |
| `then` chains and multi-step requests | People describe steps in order, not nested | Pane: "then" meant sequencing 66% of the time |
| Math words (`half of`, `20 percent of`) | Beginners write arithmetic in words | Pane: 99% natural-language arithmetic |
| Definitions (`a customer is loyal if ...`) | Names hide detail and cut working memory load; teams share one meaning | Cognitive load theory; semantic layers (dbt, LookML) |
| Testing keywords with non-programmers | Familiar programmer keywords can be no better than random ones | Stefik and Siebert 2013: Perl and Java no better than random keywords; `for`, `while`, `foreach` rated least intuitive |
| One meaning per word, common words, short sentences | Readers of every level and native language read it the same way | Simplified Technical English; plain language |
| Fixed interpretation rules (`it` = previous step only) | Keeps English readable but exact | Attempto Controlled English |
| Examples as tests, `why`, `explain` | End users often trust wrong results; asking "why" speeds debugging | Ko et al.: 5 to 23% misjudged values; Whyline: about 8 times faster debugging |
| Starter kits by job, data first | These are the real reasons people program | Ko et al. job examples; Python survey: data analysis 49%; JetBrains: automation 45% |
| Make something real in 5 minutes | Boredom is the top reason people quit | JetBrains: 64% quit a course, 51% of them from boredom |
| Levels in the playground | Introducing syntax gradually works at scale | Hedy: about 500,000 monthly users, 54 languages |
| Only valid next words in autocomplete | Beginners do best when wrong programs can't be built | Scratch's design |
| Code from examples, shown before use | People can give examples more easily than rules | Excel Flash Fill |
| Question and request forms, polite wrappers | People talk to systems in requests and questions | Speech act theory; Wizard of Oz studies |
| Errors that show the closest valid sentences | Users can't see a language's boundaries and rephrase blindly | Habitability (Watt; Hone and Baber: 15 of 22 first-time users erred even with the vocabulary shown) |
| Measuring coverage on real questions | A number to improve, like a benchmark | Spider: 10,181 real database questions |
| Clear, specific errors | Better messages reduce errors and repeated errors | Enhanced compiler error message study (about 200 students) |
| Review with the Cognitive Dimensions checklist | A standard way to judge a notation | Green and Petre |
| Easy to read is not enough; must be easy to write | English-like languages can be readable but hard to guess | Inform 7 |

## Questions and totals

People ask "how many", "any", "all" and "per" far more than they write loops. Each of these replaces a loop with a counter, a flag or a map.

| Proposed phrase | Replaces today | When |
| --- | --- | --- |
| `how many scores are above 70` | A loop with a counter | Done (part 2) |
| `if any score is above 90:` | A loop with a flag | Done (part 2) |
| `if all scores are at least 50:`, `if every order's profit is above 0` | A loop with a flag | Done (part 2) |
| `if no score is below 0:` | A loop with a flag | Done (part 2) |
| `the average of scores`, `the total of scores`, `the median of scores` | `added together` and division | Done (part 2) |
| `the top 5 scores`, `the lowest 3`, `the 10 most common` | Sort then slice | Done (part 2) |
| `the customer with the highest total`, `the one with the most orders` | A search loop with two variables | Done (part 2), except `the one ...`, which needs part 3's definitions to say which list |
| `the total amount of orders per region` (`per` only: `by` already means sorting and steps) | A map and a loop | Done (part 2) |
| `half of total`, `twice price`, `20 percent of price` | Arithmetic | Done (part 2) |
| `price rounded to 2 decimal places` | Already built (step 21) | Done |
| `how many questions there are` | `the number of items in` | Done (part 2) |

## Choices without if: the six patterns

Each pattern has its own trigger words, so the parser recognizes it without guessing. All six compile to the `if`, loops and checks that already exist, so they're mostly parser and checker work. `if` stays for anyone who wants it.

| # | Pattern | Trigger | Example | Effort |
| --- | --- | --- | --- | --- |
| 1 | Defaults | `, or X`, `starting at X`, `if there isn't one` | `the nickname, or "friend" if there isn't one` | Done (part 3) |
| 2 | Adjectives (definitions) | `a NAME is WORD if ...` at the top level | `a customer is loyal if they ordered more than 5 times.` | Done (part 3) |
| 3 | Acting on a group | A verb + `every` / `the ... that` / `the ... whose` | `remove the tasks that are done.`, `give every loyal customer a 10 percent discount.` | Done (part 3), except `give every ... a discount` (see the update log) |
| 4 | Depends-on tables | `... depends on X:` + indented `VALUE when it's CONDITION.` lines and `VALUE otherwise.` (no colon on the line); every case must be covered | `the shipping cost depends on the total:` / `0 when it's over 100.` / `5 when it's over 50.` / `10 otherwise.` | Done (part 3) |
| 5 | Rules | `must`, `must not`, `must be between` | `an age must be between 0 and 150.` | Done (part 3) |
| 6 | Events | `when` / `whenever` starting a sentence, `every N seconds`, `every Monday at 9am` (part 3) | `whenever the score passes 100, level up.` | Done (part 3) |

**Also:** `unless` as the negative of `if`, and definitions of values, not just yes/no: `a customer's profit is their revenue minus their costs.`, `the best customers are the ones with the highest profit.`

**Order:** defaults, adjectives, groups, depends-on, rules, events. Servers' `when someone visits` comes in Phase 4.

## Hiding computer jargon

| Jargon | Plain English | Example | When |
| --- | --- | --- | --- |
| String concatenation | Text templates | `say "Hello, [name]! You have [count] tasks."` | English pass (top priority) |
| Struct / record definition | `has` | `a dog has a name and an age.` | English pass |
| Input validation | `ask for a number`, `ask yes or no` (re-asks until valid) | `ask for a number: "How old are you?"` | English pass |
| Number formatting | `as money`, `as a percent`, `with commas` | `say price as money.` | English pass |
| Exceptions | `try to ... if that fails, ...` | `try to read the table in "sales.csv". if that fails, say "No file yet."` | English pass |
| Null | `or X if there isn't one` (never "missing": `nothing` is the one word) | `the nickname, or "friend" if there isn't one` | English pass |
| Escape characters | Words | `say "Line one", then a new line, then "Line two".` | English pass |
| Variables | `starts at`, `remember` | `the total starts at 0.` | English pass |
| Array index | `the one whose` | `the student whose name is "Ann"` | English pass |
| Map / dictionary | `per`, `a table of` | `a table of prices: coffee 3, tea 2.` | English pass |
| Units of measure | Numbers with units, checked | `5 meters per second`, `3 hours`, `$20` | English pass |
| Dates | Natural dates | `3 days ago`, `is in this month` (built, step 21); `tomorrow at 3pm` | Step 21; times of day in part 3 |
| Scheduling / cron | Natural times | `every Monday at 9am:` | English pass part 3 |
| Randomness | `random`, `pick`, `shuffle`, `roll a die` | `pick one of the colors.` | Built (step 21) |
| File paths | `file F` in every file sentence | `put the tasks in file "tasks.txt".` | Built (step 21) |
| CSV / Excel parsing | `read csv file ... and call it`, `save ... as csv file` (the main forms) | `read csv file "sales.csv" and call it sales.` | csv built (step 21), xlsx Phase 4 |
| Persistence / database | `kept in` | `the tasks are kept in "tasks.txt".` | Phase 4 |
| HTTP | `get the page at`, `send ... to`, `when someone visits` | `get the page at "https://example.com"` | Phase 4 |
| Concurrency | `at the same time`, `in the background` | `download these pages at the same time.` | Phase 4 |
| UI events | `show a button`, `when it's clicked` | `show a button labeled "Go".` | Phase 4 |
| Charts | `show a chart of`, `show ... as a table` | `show a chart of sales per month.` | English pass words, playground display |
| Compiling | Hidden; it just runs |  | Playground |

**Two more for the English pass:**

- **Exact money:** amounts written with a currency (`$19.99`, `19.99 dollars`) are exact decimals, so adding cents never gives rounding errors; prints with `as money` by default. Almost no language has this built in.
- **Values that always stay up to date,** like spreadsheet cells: `the total is always the sum of the prices.` Whenever a price changes, the total is current. It's how non-programmers already think about data (spreadsheets), and it compiles to working the value out when it's read (cached until something it depends on changes).

**Borrowed from Rust, for the English pass:**

- **Match guards:** `when it's a circle whose radius is over 10:` inside `check which kind`.
- **Named kinds of text or number with rules** (Rust's newtypes): `an email is text that must contain "@".` An email can't be mixed up with other text and is always valid; built from `has` and `must`.
- **Failures you can't ignore:** anything that can fail (reading a file, a missing key) offers a short fallback in the same sentence (`read the table in "x.csv", or say "No file yet." and stop`) alongside `try to ... if that fails`.
- **Editions, decided at v0.2.0:** an optional first line `this program uses EasyScript 2027.` lets later versions change wording without breaking older programs. This makes the grammar freeze safer: old programs keep their edition's meaning.

## Chains, references and acting on groups

| Proposed phrase | What it does | Rule that keeps it deterministic |
| --- | --- | --- |
| `take scores, sort them biggest first, then show the first 3.` | Steps in a row instead of nested values; fused into one pass | `them` and `the ones` mean only the previous step's value |
| `the dogs whose age is over 3`, `the orders where the amount is above 1000` (`where` and `is over` already built) | Filter by a field | `whose`/`where` start the condition |
| `set every monster's color to blue` | Change a whole group | `every` + a list's items |
| `remove the tasks that are done.` | Remove a filtered group | `the ... that` names the group |
| `the name of rex` | Same as `rex's name`; removes parentheses like `(item 1 of dogs)'s name` | `of` after a field name |
| `for each score:` (no `in scores`) | Plural inferred | Allowed only when exactly one plural name matches; otherwise an error |

**Notebook only (later):** `that` and `the result` mean the last value shown; `again`, `undo`; questions as answers (`total?`, `is x bigger than y?`).

### Multi-step requests

People give several steps in one sentence: "I want to make a new file called groceries.txt, then write milk in it, then add eggs and bread to it, and then show me what's in it."

| Piece | Words | Rule |
| --- | --- | --- |
| Openers | `I want to`, `I'd like to`, `let's`, `can you`, `please` | Stripped; the request inside is what runs |
| Step words | `then`, `and then`, `after that`, `next`, `first`, `finally` | Each starts a new step, in order |
| Making things | `make a new file called X`, `make a folder called X`, `make a list called X with ...` | Creates and names it |
| Naming | `called X`, `and call it X`, `named X` | The name is used in later steps |
| `it`, `them` | Pronouns | Only the thing the previous step made or used; two candidates means a question, not a guess |
| Contents | `what's in it` | The contents of the file, list or table |

The example above means exactly:

```
make a new file called "groceries.txt".
write "milk" in file "groceries.txt".
add "eggs" and "bread" to the end of file "groceries.txt", one per line.
show the contents of file "groceries.txt".
```

**File and folder words:** built in step 21: `make a new file called`, `make a folder called`, `put ... in file`, `write ... in file`, `add ... to the end of file`, `file|folder F exists`, `rename|copy|move the file ...`, `delete the file|folder`, `the files in the folder`. Still to come: `show me what's in`, `move every file older than 30 days to`, `show me the files in ... that end with ".jpg"`. File sentences always say `file` (a design-principles exception), because `add X to the end of F` without it is the list sentence.

**Safety:** deleting or replacing a file asks first in the notebook ("This will replace groceries.txt. Go ahead?"); `it` never reaches back more than one step.

**Multi-step requests in program files, not just the notebook.** This must work in a .es file:

```
read the table in "sales.csv", then total the amount per month,
then sort it biggest first, then save it as "best months.csv",
and then make a bar chart of it called "best months.svg".
```

It needs four things:

1. **Line continuation:** a line ending with a comma, `then` or `and` continues onto the next line (today a new line always ends a sentence).
2. **Action forms of value phrases:** every value form gets a matching action (`the total amount per month` / `total the amount per month`; `L sorted biggest first` / `sort it biggest first`).
3. **The `it` rule:** inside a `, then` chain, `it` is the previous step's result; elsewhere it's the loop's or match's item; where both could apply it's an error asking for a name. Chain steps need `, then` (comma first), so `if C then S` keeps its meaning.
4. **Charts:** a small charts module that saves bar, line and pie charts as SVG, written in EasyScript (an SVG is just text). Planned right after the English pass; PNG output waits for Phase 4.

The example reads as exactly: read the table into step 1; step 2 = the total amount of step 1 per month; step 3 = step 2 sorted biggest first; save step 3 as "best months.csv"; save a bar chart of step 3 as "best months.svg".

## Checking and understanding

Research shows non-programmers misjudge whether values are right 5 to 23% of the time, and trust wrong values more than they doubt right ones. Making programs easy to check matters as much as making them easy to write.

| Feature | Example | Why | When |
| --- | --- | --- | --- |
| Examples as tests | `for example, the price of a large coffee is 5.` runs automatically and warns when it stops being true | Testing without the word "test" | English pass |
| Why | `why is total 47?` gives "total started at 0, then had 12, 20 and 15 added on lines 4 to 6." | The Whyline cut debugging time about 8 times in a study | Playground |
| Explain this program | Reads the code back as plain English, line by line | Lets people check the program says what they meant | Playground |
| Explain how | Shows the strategy the compiler picked ("one pass, grouped by customer") | Like SQL's EXPLAIN; teaches algorithms | Phase 4 middle end |
| Live values | Values shown next to lines as the program runs | Makes the thinking visible, like a spreadsheet | Playground |
| One-key fixes | Tab accepts an error's suggestion; never automatic | Fixes without guessing | Playground |
| Friendly common errors | Missing quotes (#1 beginner error), indentation (#2), colons, periods each get a fix | Error message research | English pass |

## Talking to it: groundwork for the notebook

The chat notebook is Phase 4, but its grammar has to be designed now: after the freeze, the core can only grow by additions, and words like `what`, `me` and `?` must not be taken by something else first.

**Requests and questions people use with assistants, mapped to deterministic meanings**

| People say | Means | When |
| --- | --- | --- |
| `what is total?`, `what's total?`, `total?` | Show it | English pass (grammar), playground (display) |
| `show me X`, `give me X`, `list X`, `tell me X` | Show X | English pass |
| `how many orders are big?` | Count and show | English pass |
| `is total above 100?` | Show yes or no | English pass |
| `which customer spent the most?`, `who ...?` | Find and show | English pass |
| `can you show me X`, `could you ...`, `I want X`, `please ...` | Polite wrappers: the request inside them | English pass |
| `that`, `it`, `the result` | The last value shown, only in the notebook | Notebook |
| `again`, `undo`, `now sort it biggest first` | Repeat or change the last request | Notebook |
| `what does this do?`, `explain this` | Read the code back in plain English | Playground |
| `why is total 47?` | Show where the value came from | Playground |
| `make me a quiz`, `start a budget tracker` | Open a starter template | Playground |
| Anything vague (`make something cool`) | Optional AI writes EasyScript, shown before it runs | Last, optional |

**Do now (English pass):** the `?` mark and question forms; `show me` / `give me` / `tell me` / `list`; `what`, `which`, `who`, `how many` questions; stripping polite wrappers; reserving `me` and `undo` (`again` and `the result` are notebook-only words, not reserved in programs, which already use them as names); conversational error wording ("I don't know X yet. Did you mean Y?").

**Later (Phase 4):** sessions that keep variables between messages, multi-turn references, the chat interface, tables and charts in replies, the JIT, the AI fallback.

**Rule:** a question or request means exactly what the same words would mean in a program. `what is total?` is `say total`. Nothing in the notebook depends on guessing. Later, the notebook may accept any text through an interpreter (normalizing, nearest match, a small local model, then an optional LLM), but anything it infers is shown as EasyScript before it runs, and what's saved is always the explicit code.

**Example notebook session (proposed)**

```
> read the table in "spending.csv" and call it purchases.
  Got it: 214 purchases, with columns date, store, category and amount.
> how many purchases are there?
  214
> what did I spend in total?
  $6,482.15
> show me the total amount per category, biggest first.
  (a table of categories and totals)
> which store did I spend the most at?
  Whole Foods ($780.12)
> a purchase is big if its amount is over 200.
  Okay. 9 purchases are big.
> give me the big ones that aren't rent.
  (a table)
> sort that by amount, biggest first.
  (the same table, re-sorted)
> show me a chart of eating out per month.
  (a line chart)
> why is September so high?
  Eating out in September was $350.00, from 14 purchases. The biggest were Nobu ($96.40) and Sushi Bar ($71.25).
> can you warn me when a purchase is over 300?
  Okay. From now on: whenever a purchase is over 300, I'll say so.
> undo
  Removed the warning.
> what does this do?
  (the session read back in plain English)
> save this as "my budget.es".
  Saved.
```

The saved file is an ordinary program:

```
read the table in "spending.csv" and call it purchases.
a purchase is big if its amount is over 200.
say "Purchases: [how many purchases there are]".
say "Total: [the total amount of purchases as money]".
show the total amount of purchases per category, sorted biggest first, as a table.
say "Top store: [the store with the highest total amount]".
show the big purchases whose category isn't "rent", sorted by amount biggest first, as a table.
show a chart of the total amount of purchases whose category is "eating out", per month.
```

When a word could mean two columns ("what did I spend" with both amount and tip), the notebook asks "Do you mean amount or tip?" and keeps the answer as a definition.

## Error messages

**Today:** about 350 compile errors and 110 runtime errors, all tested. Each shows the line, a caret under the problem, and a fix in first-person plain English. Examples from the current compiler:

```
Line 2: I don't know anything called "totl".
    say totl.
        ^^^^
Did you mean "total"? You made it on line 1.
```

```
Line 1: This text is missing its closing quote.
    say "hello.
        ^^^^^^^
Add a " where the text ends. Text has to finish on the line it starts on.
```

```
Line 2: I can't add text to a number.
    say x plus "a".
        ^^^^^^^^^^
To join text, use "and" or "followed by".
```

Also covered: names made later or inside a block, wrong number of inputs, missing cases in `check which kind`, "could be nothing, so check it first", kind mistakes, division by zero, positions past the end.

**Jargon to remove (found by running the compiler):**

| Today | Better |
| --- | --- |
| I don't know a function called "double". Define it first, like "to double with someone:" | I don't know how to "double" yet. Teach it first, like "to double with a number:". |
| A dog doesn't have a field called "age". Its fields are "name". | A dog doesn't have an age. It only has a name. |

**New errors the English pass needs, in the same style:**

| Situation | Error |
| --- | --- |
| Undefined adjective | I don't know what "loyal" means for a customer. Say what it means, like "a customer is loyal if ...". |
| Defined for another kind | I know what "best" means for customers, but not for products. Say what it means, like "the best products are the ones with the most sales." |
| Missing case in a depends-on table | The shipping cost doesn't say what happens when the total is 50 or less. Add an "otherwise:". |
| A must rule broken (runtime) | Line 12: An age must be between 0 and 150, but this one is 200. |
| Unclear `it` | "it" could mean "a.txt" or "b.txt" here. Use the name instead. |
| Unclear `and` | Did you mean both, or one after the other ("and then")? |
| Unknown column | Line 14: "sales.csv" has no column called "amout". Did you mean "amount"? |
| Word with two possible meanings | I don't know which "spend" you mean: amount or tip? |
| Question that doesn't fit | I can't answer "what is the best?" yet. Try "which customer has the highest profit?" |
| Not understood at all | Show the 2 or 3 closest sentences EasyScript accepts, so people don't rephrase blindly (habitability). |

**Rules:** first person; plain words from the common vocabulary; never the banned jargon list; the exact line and spot; one concrete sentence to type instead; a question rather than a guess when two meanings are possible. Each new error gets a test, as today.

### Voice guide and glossary (write during the English pass)

- **docs/voice.md:** how EasyScript talks, as rules: first person, short sentences, plain everyday words, no jargon, friendly but not chatty, no pile of exclamation marks. A fixed shape for each kind of reply: answer, table, error, clarifying question, suggestion. Every error, notebook reply and doc follows it, so EasyScript sounds like one personality.
- **A glossary:** one word per idea everywhere (always "kind", never "type"; always "list", never "array"). The banned-words audit enforces the beginner side.
- **Sentence answers:** small results of `what is X?` are answered in a sentence ("Your best month was September, with $6,210.75."), part 4.
- **Audits cover replies too:** reading level and banned words are checked on notebook-style replies, not just errors.

Phase 4 builds on this: acknowledging before answering, summaries before big tables, pointing out notable things (outliers, big changes), suggesting a next question, and speaking out loud.

### Errors as structured data (groundwork for questions in the notebook)

Every new error in the English pass is stored as structured data as well as its message: the problem, what's missing, its kind, and the possible choices (for example: missing fields "item" and "quantity" of an order; "map" could mean a lookup table or a picture of places). In a program file it prints as an error, as now. In the Phase 4 notebook, the same data becomes a question with choices ("What did Ann order, and how many?"), the answers are filled in, and the final code is shown. Rules: ask only what matters, state any defaults used, show the final code, never guess silently. Add this to part 1's prompt so every later error follows it.

## Rough spots to fix

| Today | Problem | Fix |
| --- | --- | --- |
| `set the value for word in counts to (the value for word in counts, or 0) plus 1` | Says the lookup twice | `add 1 to the value for word in counts, starting at 0` |
| `(item 1 of dogs)'s name` | Parentheses before `'s` | `the name of item 1 of dogs` (wins only when no function is called `name`; both is an error asking which) |
| `(item 3 of fields) as a number` | Parentheses for a conversion | `item 3 of fields, as a number` |
| `a list of (a new circle with radius 1), (a new point)` | Parentheses around each new value | Allow them without parentheses when unambiguous |
| `a dog is a kind of thing with:` + `a name, which is text.` | Wordy | `a dog has a name (text) and an age (a number).` |
| `"Total: " followed by total followed by "!"` | Clunky joins | Commas: `say "Total:", total, "!"` (only at the top level of `say`/`show`/`print`/`display`/`complain`; a comma before `or`, `sorted`, `as`, `rounded`, `starting` keeps its postfix meaning) |
| `only the scores that ...` vs `every score in scores plus 5` | Two shapes for filter and map | One family: `the scores that ...`, `each score in scores plus 5` |
| `profit by customer` rejected as a name | `by` can't be in names; the suggestion `profit customer` is bad English | Allow `by`/`per` in names when unambiguous, or suggest `customer profits` |
| `for each name in ages` is an error | People expect the keys | Accept it as the keys, or keep the error |
| `call F with X` vs `F with X` | Two ways to call a function value | Decide whether to keep both |
| `to bark, for a dog:` | Methods read like functions | Consider `a dog can bark:` |
| `and`, `is` | Several meanings each | Review against "one meaning per word"; document or split |

**Freeze:** after the pass and the first non-coder test, the core grammar is frozen at v0.2.0. Later changes are additions only (new synonyms or phrases that don't change what existing programs mean).

### Found by step 22's JSON parser (a stress test of the language)

| Awkward today | Fix | Part |
| --- | --- | --- |
| `count from 1 to 0` counts down, so `count from 1 to (the number of items)` runs twice on an empty list | Counting goes up only unless written `count down`; a lower end means zero rounds. Breaking change, decide before the freeze | 1 |
| No small values that compare by value (a two-number position must be a record or a one-variant kind) | Value kinds that compare by contents and copy cheaply, e.g. `a point is a pair of x and y` (also the points/vectors gap from the physics examples) | 1 |
| `as a number` doesn't read exponents (`1e5`); `as text` keeps only 10 significant digits, so numbers don't survive a round trip | Read exponents; add an exact conversion (e.g. `X as exact text`) for JSON and CSV | 1 |
| No raw text, no `\r` escape (JSON inside a program needs `\"` everywhere) | Multi-line text blocks where quotes need no escaping (e.g. `the text below:` plus indented lines), and `\r` | 4 |
| No early exit, so every check is repeated | `give back nothing unless ...`, `stop here if ...`, and the fallback in the same sentence | 1 and 3 |
| Inputs joined only by `and` | Natural joiners where unambiguous (`the column of cell in grid`, `from ... to ...`) | 5 |
| Extra parentheses: an argument followed by `followed by`, new values inside map literals | Already on the rough-spots list above | 5 |
| No infinity test, no whole-number division | `X is infinite`; `X divided by Y, rounded down` and `the remainder` | 2 |
| Helpers in a used file are visible by default | Keep: simpler for beginners; `privately` exists | No change |

**More detail from the repo's step 22 notes:**

- **Early exit:** every parsing function checks `if r's problem is not "", give back ...` after each call; the only way to stop a chain of calls with a problem is `stop the program`.
- **Text escapes:** besides `\r`, there's no `\u` escape; the parser uses `the character with code 13` instead of `\r`. The tests read their JSON from `the text sent to the program` to avoid escaping every quote.
- **Exponents:** the parser works them out by hand with `to the power of`.
- **Positions as records:** `contains` can't find a position kept as a record, because records are never equal to each other; Snake keeps each cell as a single number instead.
- **Infinity:** tested with `amount minus amount is not 0`; whole-number division written `(n divided by 16) rounded down`.
- **Input joiners:** `the column of cell in g` has to be written `column of cell and g`.
- **Parentheses:** `report in r and ("I expected " followed by word)`.
- **Visible helpers:** `json.es` needs `privately` on 18 helpers, and a helper named like a variable in the file that uses it is an error there. Keep visible-by-default, but consider making that name clash a quiet shadow (the using file's own name wins) instead of an error.
- **Counting:** loops over "the first N - 1 items" need a `while` when N can be 1.
- **Filter and map family:** the phrases to unify are `where`, `that pass`, `only`, `every` and `applied to every`.

## How the compiler picks the efficient way

The strategy is chosen once per pattern by us, not per program, and applied every time the phrase appears. It works like a SQL query planner, not a game search like minimax. Compile-time cost: milliseconds.

| Level | How it chooses | Example | When |
| --- | --- | --- | --- |
| 1. Fixed | One best strategy per phrase | `per region`: one pass with a hash map; `how many`: a counter, no list made; `every ... is`: stop at the first failure; `the top 5`: a 5-slot heap | English pass |
| 2. Known at compile time | Simple rules | `the top N` with N near the list size: sort instead; `then` chains fused into one loop | Phase 4 middle end |
| 3. Known only at run time | Both versions plus a cheap check | Scan under 16 items, hash map above (pdqsort and small maps already do this) | Phase 4 |
| 4. Definitions over other data | Cost model, like SQL join planning | `a customer's profit is the total profit of their orders`: group once (about 1M steps) instead of scanning all orders per customer (about 10B steps) | Long term |

- **Same answer guaranteed:** every smart strategy is tested against a simple naive mode on every test program.
- **Visible on request:** `explain how` shows the plan in English.
- **Overridable:** programmers can say `sort orders by date, using a stable sort`.
- **Architecture:** English, then the syntax tree, then a high-level IR that keeps intents like grouping and top N, then the optimizer, then low-level code.

**Speed rule for the English pass:** every new high-level form gets a benchmark against the hand-written loop for the same work, recorded in [performance.md](performance.md), and must stay within 20% of it. Counting, totals, `any`/`every` and top N compile to a single pass with no temporary list; definitions used in a loop are worked out once per item, not rescanned. (Step 20's pipeline ran 0.13 s against 0.09 s for a loop because each step built a temporary list; that's the pattern to avoid.)

**Why it stays fast:** the English is turned into the same C a hand-written loop produces, before the program exists. Units are checked, then erased; templates use the existing in-place text building. Only compile time grows, by milliseconds.

**Why the public playground waits for Phase 3:** compiling the C compiler to WebAssembly only gets EasyScript to C in the browser; running that C needs a C compiler in the browser (a 30 MB+ download) or a server that runs strangers' code (costly and risky). An interpreter is the best fit, and writing it after self-hosting means writing it once, in EasyScript. It's also the first step toward the JIT.

**`easyscript plan FILE`: the execution plan, for programmers** (alias `easyscript build --plan FILE`, matching the existing run/build/emit/tokens/ast commands):

```
[Plan] Line 4: "how many orders are big": counted in 1 pass, no list made.
[Plan] Line 6: "the total profit per customer": 1 pass, grouped in a hash map (0 extra lists).
[Plan] Lines 8-10: "take ..., then ..., then ...": fused into a single loop.
```

- **Makes the speed rule verifiable:** engineers can see that the English compiles to the same loops they'd write.
- **Enforces it:** plan output is golden-tested, so a change that makes a form build a temporary list fails the tests.
- **Truthful by construction:** the plan is produced from the actual code generation, never written separately.
- **Two audiences:** `easyscript plan` uses programmer terms (allocations, passes); `explain how` in the playground says the same in beginner words.
- **When:** a basic version in English pass part 2 (one line per high-level form); the full version (strategies, fusion, allocation counts) grows with the Phase 4 optimizer.

## Starter kits and the playground

Each kit is a template program plus a vocabulary module, matched to what end-user programmers actually build.

| Kit | For | Key words | When |
| --- | --- | --- | --- |
| Automate | Office workers, system admins | `rename every file in "Photos" to ...`, `move files older than 30 days`, `send an email to ...`, `every Monday at 9am` | Step 21 files; email Phase 4 |
| Data | Analysts, accountants, managers, teachers | `read the table in "sales.xlsx"`, `per`, `average`, `show a chart of sales per month` | csv step 21; charts playground; xlsx Phase 4 |
| Website and forms | Small business owners | `when someone visits`, `kept in`, `show a form asking for` | Phase 4 |
| Games and animation | Kids, beginners | `whenever`, `every 0.1 seconds`, keys, later sprites and sound | Snake step 22; graphics Phase 4 |
| Science and teaching | Teachers, scientists | Units, simulation steps, plots | English pass units; plots playground |
| Home and schedules | Hobbyists | `at sunset`, timers, Raspberry Pi and ESP32 | Phase 4 to 5 |
| First programs | Everyone | Quiz, budget tracker, guessing game, to-do list | v0.2.0 |

**Public playground (after Phase 3; a local playground lite comes before v0.2.0 for testing):** type and run in a browser with nothing to install; the templates; `explain this program`; `why`; values shown as it runs; one-key fixes; charts and tables; questions as answers. The first five minutes should make something real, since most people who quit a course say it was boring.

## Example programs

### Sales report: today (runs now, tested on Linux)

```
an order is a kind of thing with:
    a customer, which is text.
    a region, which is text.
    an amount, which is a number.
    a cost, which is a number.

read file "orders.csv" and call it contents.
let orders be an empty list of orders.
for each line in the lines of contents without the first item:
    let fields be line split by ",".
    let amount be (item 3 of fields) as a number.
    let cost be (item 4 of fields) as a number.
    add (a new order with customer (item 1 of fields), region (item 2 of fields), amount amount and cost cost) to orders.

let big orders be 0.
for each o in orders:
    if o's amount is above 1000, add 1 to big orders.
say "Big orders: " followed by big orders.

let customer profits be an empty map from text to numbers.
for each o in orders:
    set the value for (o's customer) in customer profits to (the value for (o's customer) in customer profits, or 0) plus (o's amount minus o's cost).

let best be "".
let best profit be -1.
for each name in the keys of customer profits:
    if (the value for name in customer profits) is above best profit:
        set best to name.
        set best profit to the value for name in customer profits.
say "Best customer: " followed by best followed by " (" followed by best profit followed by " profit)".

let region totals be an empty map from text to numbers.
for each o in orders:
    set the value for (o's region) in region totals to (the value for (o's region) in region totals, or 0) plus o's amount.
for each region and total in region totals:
    say region followed by ": " followed by total.

let all positive be yes.
for each o in orders:
    if o's amount minus o's cost is below 0, set all positive to no.
if all positive, say "Every order made money.".
```

### Sales report: after the English pass (proposed)

```
read the table in "orders.csv" and call it orders.

an order's profit is its amount minus its cost.
a big order is one whose amount is over 1000.
a customer's profit is the total profit of their orders.
the best customer is the one with the highest profit.

say "Big orders: [how many orders are big]".
say "Best customer: [the best customer's name] ([the best customer's profit] profit)".

for each region and total in the total amount of orders per region:
    say "[region]: [total]".

if every order's profit is above 0, say "Every order made money.".
```

About 35 lines, 6 loops and 3 maps become about 12 lines, 1 loop and no maps.

### Coffee shop manager (proposed, uses nearly everything)

```
use the web.

a drink has a name, a size and a price.
an order has a customer, some drinks and a time.
a customer has a name and an email.

the orders are kept in "orders.txt".
the customers are kept in "customers.txt".

the price of a drink depends on its size:
    small: 3.
    medium: 4.
    large: 5.

an order's total is the total price of its drinks.
a customer is a regular if they ordered more than 10 times this month.
a big order is one whose total is over 20.

a customer's email must contain "@".
an order must have at least one drink.

for example, the price of a large drink is 5.

give every regular a 10 percent discount on their orders.

when someone visits "/":
    reply with "Welcome! Today we've served [how many orders are from today] orders."

when someone sends an order to "/order":
    try to add their order to the orders.
    if that fails, reply with "Sorry, that order didn't work: [the reason]".
    otherwise, reply with "Thanks, [their name]! That's [their order's total as money]."

every day at 9pm:
    say "Orders: [how many orders are from today]".
    say "Money made: [the total of today's orders as money]".
    say "Best customer: [the customer with the highest total spent today]'s name".
    for each size and count in the number of drinks per size today, sorted biggest first:
        say "  [size]: [count] sold".
    unless any order is from today:
        say "Quiet day.".

start the website on port 8080.
```

### Also drafted

A quiz, a monthly budget, Snake with `whenever` rules, a gradebook with `any`/`every`/`whose`, word stats with a `then` chain, and a small web server; these become the starter templates.

## Research

The strongest single source is Pane's study of how non-programmers describe programs: they write rules and events, act on whole groups, and rarely loop. Rows marked "checked" were opened and confirmed for this doc; "background" rows are well known but not re-checked here.

### How non-programmers express programs

| Source | What it found | What we take | Status |
| --- | --- | --- | --- |
| [Pane, Ratanamahatana & Myers (2001); Pane's PhD thesis (CMU, 2002)](https://www.csd.cmu.edu/sites/default/files/phd-thesis/CMU-CS-02-127.pdf) | Children describing Pac-Man: 54% of statements were rules or events ("when", "if", "after"); 95% of operations on many objects were sets ("when Pac-Man gets all the dots"), only 5% explicit loops; 99% of arithmetic in natural language; groups referred to as plurals with content-based queries | Event rules, quantifiers, `whose`, group actions, math words | Checked |
| Same thesis | "and" meant "and then" 29% of the time; "or" restated the previous item 24% of the time; Boolean expressions were rare and error-prone; match forms beat textual Boolean queries | Ask when `and`/`or` is unclear; prefer tables and patterns over Boolean logic | Checked |
| [Ko et al., "The State of the Art in End-User Software Engineering" (2011)](https://faculty.washington.edu/ajko/papers/Ko2011EndUserSoftwareEngineering.pdf) | End-user programming is defined by intent: programs for your own use. Examples by job (teachers, accountants, scientists, managers, admins, homeowners, kids). Motivations: automate repetitive work, schedule future actions, tinker, teach | Starter kits by job | Checked |
| Ko et al. (2011), citing Scaffidi, Shaw & Myers (2005) | Projection for the US by 2012: under 3 million professional programmers, over 55 million people using spreadsheets and databases at work | Our real audience is end users | Checked (via Ko) |
| Ko et al. (2011) | End users misjudge values 5 to 23% of the time and trust wrong values more than they doubt right ones; spreadsheet errors have cost firms millions | Examples as tests, `why` | Checked |
| [Ko & Myers, "Designing the Whyline" (CHI 2004)](https://hci.rwth-aachen.de/materials/conferences/CHI2004/1p151.pdf) | Asking "why" questions cut debugging time by a factor of 7.8 (9 participants) | `why is total 47?` | Checked |

### Who learns to code and why

| Source | What it found | What we take | Status |
| --- | --- | --- | --- |
| [JetBrains Computer Science Learning Curve Report 2024](https://lp.jetbrains.com/cs-learning-curve-report-2024) | About 24,000 respondents: 45% like automating processes, 42% want to create a game or website, 53% learn for personal projects. Struggles: 51% abstract concepts, 40% poor documentation, 39% getting stuck. 64% have quit a course, mostly from boredom (51%) | Make something real in the first 5 minutes; concrete over abstract | Checked |
| [Python Developers Survey 2024 (JetBrains and PSF)](https://lp.jetbrains.com/python-developers-survey-2024/) | Uses: data analysis 49%, web development 48%, machine learning 42%, data engineering 33%, web scraping 28%, research 28%, DevOps 26% | Data kit first; charts and tables as normal output | Checked |

### Writing English everyone can read

| Source | What it found | What we take | Status |
| --- | --- | --- | --- |
| [Attempto Controlled English (University of Zurich, since 1995)](https://en.wikipedia.org/wiki/Attempto_Controlled_English) | A subset of English that translates to logic; avoids ambiguous forms and resolves the rest with a few fixed interpretation rules | Reuse its rules for `every`, relative clauses, pronouns | Checked |
| [Simplified Technical English (ASD-STE100)](https://skybrary.aero/articles/simplified-technical-english-ste) | The aerospace standard for manuals read by second-language speakers: one meaning per word, no synonyms, 20 words per instruction and 25 per description | One meaning per word (we allow synonyms, but never overloading) | Checked |
| [Cognitive Dimensions of Notations (Green 1989; Green & Petre 1996)](https://en.wikipedia.org/wiki/Cognitive_dimensions_of_notations) | 14 dimensions for judging a notation: closeness of mapping, viscosity, hidden dependencies, error-proneness, progressive evaluation, role-expressiveness and more | The review checklist for every phrase | Checked |
| [Enhanced compiler error messages study (UCD)](https://core.ac.uk/works/78318990) | About 200 students, almost 50,000 errors: clearer messages led to fewer errors overall and fewer repeated errors | Keep investing in error messages | Checked |
| US Plain Writing Act (2010), plainlanguage.gov | Common words, active voice, short sentences, main point first | Grade 6 to 8 reading level for errors and docs | Background |
| Flesch-Kincaid grade level, Oxford 3000 word list | Measures of reading level and common vocabulary | Automated audits | Background |
| Cognitive load theory (Sweller); working memory of about 4 items (Cowan) | People hold only a few things in mind at once | Short sentences, shallow nesting | Background |
| Inform 7 | The most successful English-like language; easy to read but hard to write, because people can't guess which phrasings are accepted | Generous synonyms, errors that suggest the accepted phrasing | Background |

### How systems pick efficient strategies

| Source | What it found | What we take | Status |
| --- | --- | --- | --- |
| SQL query planners (cost-based optimization, Selinger et al. 1979) | Declarative queries; the database picks indexes, join order and sort strategy with a cost model | The model for our optimizer and `explain how` | Background |
| Semantic and metrics layers (dbt, LookML, Cube) | Business terms like "active customer" defined once and shared | Definitions (`the best customers are ...`) | Background |
| Spreadsheets, Scratch, HyperCard | Succeeded with non-programmers by making the program's state visible and immediate | Live values in the playground | Background |

### Getting to almost no learning curve

| Source | What it found | What we take | Status |
| --- | --- | --- | --- |
| [Stefik & Siebert, "An Empirical Investigation into Programming Language Syntax" (2013)](https://neverworkintheory.org/2014/01/29/stefik-siebert-syntax.html) | Novices using Perl and Java were no more accurate than with a language of randomly chosen keywords; Quorum, Python and Ruby did significantly better. Non-programmers rated `for`, `while` and `foreach` the three least intuitive loop words | Pick keywords by testing them with non-programmers, not by habit | Checked |
| Quorum (Stefik) | A language whose syntax was changed based on studies of novices | The model for an evidence-based EasyScript | Checked (via review) |
| [Hedy (Felienne Hermans, launched 2020)](https://drops.dagstuhl.de/storage/01oasics/oasics-vol122-icpec2024/OASIcs.ICPEC.2024.1/OASIcs.ICPEC.2024.1.pdf) | A gradual language for learners: in the browser, in 54 languages, about 500,000 monthly users. Syntax is introduced level by level | Levels in the playground; keywords kept in a table so other languages are possible later | Checked |
| [Resnick et al., "Scratch: Programming for All" (2009)](https://web.media.mit.edu/~mres/papers/Scratch-CACM-final.pdf) | Low floor (easy start), high ceiling (complex projects), wide walls (many kinds of projects); tinkerable; blocks can't be joined in invalid ways | Autocomplete that offers only valid next words; many kinds of starter kits | Background (page didn't load) |
| [Excel Flash Fill (Gulwani, Microsoft Research; Excel 2013)](https://blog.jonudell.net/2013/02/18/flash-fill-text-wrangling-for-non-programmers/) | Users type the result they want for one row; Excel infers the rule for the rest. Ambiguous examples mean results need checking | Examples that suggest code, which is shown before it's used | Checked |

**What this adds to the plan**

1. **Test every keyword.** Before the freeze, run a short survey: show non-programmers several phrasings for each idea and measure which they guess correctly (Stefik's method).
2. **Levels in the playground,** like Hedy: start with `say`, `ask` and text templates, then add lists, choices and definitions as the learner goes. Errors only mention words from the learner's level.
3. **Only valid next words.** The autocomplete predictor offers only continuations the grammar accepts, giving Scratch's "can't build it wrong" feel in text.
4. **Code from examples.** The user writes `for example, "Ann Lee" becomes "Lee, A."` and the playground suggests a rule, shown as EasyScript before it's used, so it stays deterministic and checkable.
5. **Keywords in one table,** so translations into other human languages stay possible later.

### Talking to computers in natural language

| Source | What it found | What we take | Status |
| --- | --- | --- | --- |
| [Watt (1968), via Hone & Baber, "Designing Habitable Dialogues" (2001)](https://research.birmingham.ac.uk/files/4856389/IJHCSHoneHabitable.pdf) | **Habitability**: whether users can say what they need without straying outside the sentences a system accepts. Showing the vocabulary isn't enough (15 of 22 first-time users still made errors); users chase their own goals, rephrase valid input after a failure, and blame the wrong cause. Adding synonyms helps but never fully closes the gap | Make habitability the measure; offer the next valid options; say exactly why a sentence wasn't understood so people don't rephrase blindly | Checked |
| [Dahlbäck, Jönsson & Ahrenberg, "Wizard of Oz Studies: Why and How" (1993)](https://www.media.mit.edu/gnl/discint00/papers/p193-dahlback.pdf) | A hidden person plays the computer to collect how people really talk to a system. People talk differently to computers than to people: simpler dialogue structure, different use of pronouns like "it" and "that" | Run Wizard of Oz sessions for the notebook before building it; don't copy human-human conversation | Checked |
| [Spider (Yu et al., EMNLP 2018)](https://aclanthology.org/D18-1425/) | 10,181 human-written questions over 200 databases in 138 domains, with matching SQL | A ready-made set of real data questions to measure how many EasyScript can express | Checked |
| Speech act theory (Austin, Searle) and Grice's cooperative principle | Indirect requests ("can you show me") are requests; good replies answer what was asked, no more, no less | Polite wrappers mean the request inside; replies show just the answer | Background |
| Conversation analysis on repair (Schegloff and others) | People fix misunderstandings quickly with short clarifying questions | Errors as one short clarifying question | Background |

**What this adds to the plan**

1. **Wizard of Oz study, now.** You play the notebook in a chat window; 5 to 10 people try to do real tasks ("what did I spend most on?"). Record every sentence. It costs nothing and shows exactly what to support.
2. **A phrase corpus and a coverage number.** Collect those sentences plus questions from Spider, rewritten for EasyScript data. Track "percent of real requests EasyScript understands" at every milestone, like a benchmark.
3. **Habitability over vocabulary size.** When something isn't understood, show the closest valid sentences instead of only adding synonyms.

## Later

Left out of the English pass, each with its reason (details under [Still open](#still-open) and in [the log](english-pass-log.md)):

- **Exact money amounts** (`$19.99` kept exactly) and **units of measure** (`5 meters`, `meters per second`): each needs a new kind of value through the checker, code generation and runtime.
- **Named kinds of text with rules** (`an email is text that must contain "@".`): how a value becomes one isn't settled.
- **`try to` beyond files and folders:** other failures would need a safe way to undo a half-done sentence in the generated C.
- **Actions with inputs in the middle** (`give every loyal customer a 10 percent discount`).
- **The best group** (`the store with the highest total amount`) and `they ordered more than 5 times`.
- **`each drink in drinks's name`** (a field after a transformed list) and other phrasings found while writing the examples.
- **The notebook's own words** (`again`, `the result`, `undo`, `why is total 47?`, `explain this`): reserved or planned, built with the notebook.

## Open questions

### Decided (2026-10-11)

- [x] **File sentences say `file`** (`write "milk" in file "groceries.txt"`), as design-principles.md already requires.
- [x] **Planned breaking changes** (recorded in design-principles.md): joining text with `and` is removed (part 1); inputs get natural joiners (part 5).
- [x] **No "missing":** `nothing` is the one word; defaults read `or X if there isn't one`.
- [x] **`every` means "each one", with distinct triggers:** `every X is/are ...` tests (part 2); `every N seconds` / `every Monday at 9am` schedule (part 3). Transforming is `each X in L ...`; `every X in L ...` stays as a synonym.
- [x] **`it`:** inside a `, then` chain it's the previous step's result; elsewhere the loop's or match's item; where both could apply, an error asks for a name.
- [x] **Grouping uses `per` only;** `by` keeps its meanings (sorting, steps, dividing).
- [x] **Chain steps need `, then`** (comma first); `if C then S` keeps its meaning.
- [x] **`the X of Y`** (a field, or a built-in phrase) wins only when no function is called X; if both exist, it's an error asking which.
- [x] **The program result's `code` field becomes `exit code`.**
- [x] **Commas join pieces only at the top level of `say`/`show`/`print`/`display`/`complain`** (and later `reply`); a comma followed by a postfix word (`or`, `sorted`, `as`, `rounded`, `starting`) keeps its postfix meaning.
- [x] **Depends-on tables** use `VALUE when it's CONDITION.` lines and `VALUE otherwise.` (no colon on the line).
- [x] **Already built:** `rounded to N decimal places`, `is over`, the file sentences, `file X exists`, `N days ago`, `is in this month`, `pick`, `shuffle`, `roll a die`, filtering by a field with `where`, `rounded down`. `read csv file` and `save ... as csv file` stay the main forms.
- [x] **`every Monday at 9am` moves to part 3.** `again` and `the result` are notebook-only words, not reserved in programs.

- [x] **`is` keeps its uses.** `is 5`, `is empty`, `is nothing`, `is a circle`, `is loyal` all mean "states a fact you can check", one meaning in English.
- [x] **`and` has two meanings, chosen by position.** Between values or conditions: "both", or joining a list. Between two actions: "and then" (`make a file called "a.txt" and write "hi" in it`). Joining text with `and` (`"Hello, " and name`) is removed once the new text joining exists; it becomes an error showing the new form.
- [x] **Records: `has` is the main form;** `is a kind of thing with:` stays as a synonym. `a dog has a name (text) and an age (a number).`
- [x] **Plural inference, narrowly.** `for each score:` works only when exactly one list's name is the plural of `score`; zero or two matches is an error asking for `in scores`.
- [x] **One family for filters and maps.** "the X that/whose ..." always filters (`only ...` stays as a synonym); "every/each X in L ..." always transforms.
- [x] **Methods: `a dog can bark:` is the main form,** matching interfaces ("something that can speak"). Inputs: `a dog can fetch with a toy:`. `to bark, for a dog:` stays as a synonym.
- [x] **Reading-level audit:** Flesch-Kincaid grade, computed by a small Python script in tests/tools (or the textstat library), skipping quoted code. Errors beginners see must be grade 8 or below (build fails). Beginner docs: reported, not failed. The banned-words list fails the build everywhere beginners read.
- [x] **Values inside text: commas are the main form.** `say "Hello,", name, "!"` and `say "You have", how many tasks there are, "tasks left."` Pieces are separated by commas, with spaces added automatically except next to punctuation (prints `Hello, Ann!`). Brackets (`"Hello, [name]!"`) stay as an optional shortcut that beginners never need. Examples in this doc written with brackets mean the comma form.

### Decided (2026-10-12): reads like text

- [x] **New design rule, "Reads like text":** if a line could appear unchanged in an email to a coworker, it's right. Prefer English a person would write over symbols, brackets and programmer patterns; the other rules still apply.
- [x] **Actions with inputs in the middle:** in a definition's first line, `a KIND` (a kind the program has) is an input called `the KIND`, `another KIND` a second one called `the other KIND`, and `some NAME` an input called `the NAME` with its kind from use. Every other word is fixed; a call uses exactly those words with a value in each place, and `every [WORD] ITEM` in one place does it once per item. Two definitions fitting one sentence is an error asking which; a fixed word that's a kind is an error with a note. `a`/`an` are found with `Token.filler`, as `is a letter` does. `some` before a field's name after `has` stays dropped. Also `reduce X by N percent` and `increase X by N percent`.
  - *Decided while building:* a line is read this way only when an input comes before any of the usual input words (`with`, `of`, `using`, `to`, `for`, `from`, `in`) and it has no comma, so every existing definition keeps its meaning unless it wrote `a KIND` before them (recorded as breaking). It must start with a fixed word and have a fixed word between any two inputs. A value for `some NAME` may be followed by NAME (`a discount of 10 percent` gives 10), since that's how the example reads. `some NAME` takes one word. These definitions are actions only (no value form yet). `reduce X by N` is the same as `decrease`; `increase|decrease|reduce X by N percent` multiplies by (100 ± N) / 100, the hand-written equivalent.
- [x] **Yes/no fields read as describing words, and one-line choices:** `can be WORD` declares a yes/no field starting as no; `a new WORD KIND with ...` sets it; `mark X as [not] WORD` changes it; any yes/no field reads like a describing word; `A if C, otherwise B` is a value.
- [x] **Adding with a condition:** `, plus N if C` / `, minus N if C` inside a value; anything defined with `depends on` or `KIND's NAME is` reads like a field, including `its NAME`.
- [x] **The best group:** `the X with the highest|lowest total|average|number of Y` groups by the field X and gives the winning X; more than one list with the field is an error asking `... of purchases`; `the top N Xs by total Y`.
- [x] **A field across a list, and counting a list field:** `the names of the drinks`, `each drink's name`; `have|has more than|at least|fewer than|exactly N L`; `they`/`their` mean the item in a description.
- [x] **Files in a chain:** in a `, then` chain, file sentences accept `in it`/`to it`, and `the file` means the file the step before made or named (none or two is an error). Outside chains, file sentences still say `file` (a known exception). `file|folder F doesn't exist` / `does not exist`.
- [x] **Exact money:** `$`, `€`, `£` before a number or `dollars`, `euros`, `pounds` after it make an exact amount, kept as whole cents in 64 bits with an overflow check; commas between thousands after a currency sign. Money ± money (one currency) is money; money × or ÷ a number is money rounded to the cent (half away from zero, shown by `easyscript plan`); money ÷ money is a number. Mixing currencies or adding a plain number is an error with a suggestion. Prints with its currency and 2 decimals; `as money` on a plain number stays dollars. Works in lists, maps, fields, totals, averages, sorting and csv tables.
- [x] **Named kinds of text:** `an email is text that must contain "@".`; a field named after it gets the kind; a value becomes one when stored there or written `X as an email` (the rule checked then, quoted in the error); it's usable anywhere text is; plain text where an email is expected is a compile error suggesting `as an email`.
- [x] **"Still not English":** each line gets the wording that reads most like text; additions that fit the rules are built, and changes to what existing programs mean, or lines with no clear wording, go under Still open.

### Still open

- [ ] **A list of records written as a table** ("Still not English": `let students be a list of a new student with name "Ana", ...`): `the students are:` with one `"Ana" in "Math" scored 72` line each needs a wording for each kind. Suggested: `the students are the rows below:` with a header line and comma-separated rows, read like a csv file.
- [x] **Indenting a line of output**: built (2026-10-12) as `say ..., indented [by N]`. In a program that makes a name `indented`, the bare form keeps its old meaning (a piece), so no program changes meaning.
- [x] **Naming two things at once in a loop**: built (2026-10-12) as `for each class and its average in ...`. `for each X and its Y` already worked with the name `its Y`; the new reading applies only when the program never writes `its Y` anywhere else, so no program changes meaning.
- [ ] **A space after a question's prompt** (`ask "What's your name? " ...`): adding it automatically changes what existing programs print, so it waits for a decision. Suggested: add one space when the prompt ends with `?` or `:` and has none.
- [ ] **`the name is "Ada".`** instead of `let name be "Ada".`: it would clash with comparisons and with `the NAME is the one ...` definitions, so it changes meanings; no clear wording.
- [x] **Quotes inside brackets** (`"([parcel's note, or "no note"])"`): built (2026-10-12). It's an addition: text in quotes inside brackets was always an error before. A quote counts as text inside the value only when its closing quote and then a `]` come on the line, so a forgotten `]` keeps its clear error.
- [x] **`put the text below in file F:`**: built (2026-10-12), for `put` and `write` sentences ending with a colon. An addition: such a line was always an error before.

- [ ] **Actions with inputs in the middle** (`give every loyal customer a 10 percent discount`): methods today take their inputs after their name (`give discount to rex with 10`). A way to define `give KIND a N percent discount:` would make this sentence work; it needs a rule for where inputs can go. (Part 3)

- [ ] **`try to` beyond files** (part 4): `try to` covers sentences that use a file or a folder, checked before they run. Should other failures (dividing by zero, a missing key, `as a number` on text that isn't one) be catchable too? Catching them in the generated C needs a way to undo a half-done sentence's memory; each already has its own form (`, or D`, `if ... is 0`).

- [ ] **Exact money amounts and units of measure** (part 4, items 5 and 8, deferred): `$19.99` kept exactly needs exact decimals as a kind of value (or numbers of cents); units (`5 meters`, `meters per second`) need a unit carried by each number's kind and checked like kinds. Both are a step of their own. Also: should `as money` know other currencies?

- [ ] **Named kinds of text with rules** (part 4, item 11, deferred): `an email is text that must contain "@".` How does a value become an email: `"a@b" as an email`, or any text put where an email is kept (a field, an input)? Can an email be used where text is wanted (`say`, `contains`)? Rules on fields (`a customer's email must contain "@".`) already work.

- [ ] **The best group** (`the store with the highest total amount`, from the budget session): should `the X with the highest total Y` group by X when X is a field rather than the items? Today it needs `the first item of the keys of (the total Y of L per X, sorted biggest first)`.

- [ ] **A table's column the program only shows, compares or sorts** (shell and money fixes): it stays text, so `say purchase's amount` prints `$1200` as written and `sorted by amount` sorts `1200, 249, 96.40` as text. Taking its kind from the file too (as columns used in totals and arithmetic now do) would change what existing programs print. Suggested: do it before the freeze, since sorting numbers as text is rarely what anyone wants.
- [ ] **`how many tasks are left`** (shell and money fixes): not built. "left" is also a side (`how many socks are left`), which a program can define as a describing word or a field, so it's two meanings in plain English, not just in a program. Suggested: build it with the same rule as `are there` (a program's own `left` wins) if user testing shows people type it.
- [ ] **A question without its question mark** (`how many purchases are there` typed on its own line): only sentences ending in `?` are questions, so it's an error (as `how many purchases there are` on its own already was). Suggested: a sentence starting with `how many` is a question with or without the mark, since no other sentence starts with `how`.
- [ ] **`X for each FIELD` as a synonym of `per`** (v0.2 prep, simulated testers: 48 of 120 attempts grouped this way, as `the total hours for each employee` or `... by student`). Not built: `for each` starts loops and `by` sorts, so either would need an exception to "one meaning per word". Built instead: a note showing `per employee`. Suggested: accept `for each FIELD` after a question value (`the total ... for each employee`) as `per`, since a loop can't start mid-sentence; leave `by` alone.
- [ ] **`open the table in F` / `load the table in F`** (v0.2 prep): 58 attempts tried `open`, `load`, `import` or `read F`. Not built (`read the table in` works, and the errors now show it). Suggested: add `open the table in` as a synonym if the first non-coder test shows people still type it after seeing the error.
- [ ] **Greetings and thanks** (v0.2 prep): 21 attempts by people who had never coded wrote `Hello!` and `Thank you!`. Kept as an error with a note ("Every sentence is something for the program to do, ..."): skipping them silently would mean a sentence that does nothing. Suggested: keep.
- [ ] **Other describing words after arithmetic** (v0.2 prep, [proposal 0022](proposals/0022-rounding-after-arithmetic.md)): `a plus b rounded` is now an error showing both readings, because it silently rounded only b. `a plus b as text`, `a plus b in uppercase` and the other describing words have the same shape; they weren't changed (rarely wrong in a way that runs, and changing them would turn working programs into errors). Suggested: review them together before the freeze.
- [ ] **`NAME is VALUE` to make a name** (v0.2 prep): 16+ attempts wrote `Total is 0.` or `score = 0`; the same open question as `the name is "Ada"` above, now with evidence.
- [ ] **A field across a list in a program that reads a table** (v0.2 prep): `the descriptions of the purchases` reads as a column called "descriptions", because in such a program any word can be a column. Suggested: try the plural's singular as a column first (the same rule as `field_plural_at`) when the plural isn't itself a column of the file.
- [ ] **Money in constants** (v0.2 prep): `keep fee as $5` kept the number of cents; it's an error now ("A constant can't be an amount of money yet."). Suggested: let fixed values, lists and maps hold amounts, as `let` does.
- [ ] **`sorted by` a text column** (v0.2 prep, found writing the cookbook): `the tasks sorted by name` is "The top items are picked by a number or a date". Suggested: sort text the way `sort L` does.
- [ ] **`the most common FIELD of L`** (v0.2 prep): `the most common item of orders` counts whole rows; `the item with the highest number of orders` works. Suggested: read `the most common FIELD of L` as the field's most common value when L's items have that field.
- [ ] **`a KIND's FIELD depends on X:`** (v0.2 prep): only `the FIELD of a KIND depends on X:` is accepted. Suggested: accept both, as `KIND's FIELD is ...` already is.
- [ ] **Describing words of several words** (`a product is running low if ...`): only one word works. Suggested: allow them when the words aren't a known name.
- [ ] **Who are the first 3 to 5 non-coder testers?** Aim for variety and no programmers: someone who has never coded, a teacher, a spreadsheet-heavy office or finance worker, a teenager, a non-native English speaker.

## Update log

| Date | Change |
| --- | --- |
| 2026-10-11 | Doc created from the planning conversation; research sources checked |
| 2026-10-11 | Added learning-curve research: Stefik and Quorum, Hedy, Scratch, Flash Fill |
| 2026-10-11 | Step 20 done (functions as values, built-ins taking functions, interfaces). Value forms shipped as "double applied to every number in L" and "L combined using F"; "apply ... to" is the in-place sentence. Revisit in the English pass. |
| 2026-10-11 | Added notebook groundwork (questions and requests now, sessions later) and research on habitability, Wizard of Oz studies and Spider |
| 2026-10-11 | Added: why each feature (evidence table), multi-step requests and file words, the example notebook session |
| 2026-10-11 | Added error messages (today, jargon to remove, new ones), the speed rule, and the new order: playground lite before v0.2.0, public playground after Phase 3 |
| 2026-10-11 | Decided: is, and, has, plural inference, filters/maps, can-methods, reading-level audit. Text joining reopened (commas proposed over brackets). |
| 2026-10-11 | Decided: values inside text use commas (brackets optional) |
| 2026-10-11 | Added: multi-step requests in files (line continuation, action forms, it rule) and a charts-to-SVG module after the pass |
| 2026-10-11 | Added: exact money and always-up-to-date values to the English pass |
| 2026-10-11 | Added from Rust: match guards, named kinds of text with rules, failures you can't ignore, editions at v0.2.0 |
| 2026-10-11 | Added: easyscript plan (execution plan, golden-tested), basic version in English pass part 2 |
| 2026-10-11 | Added: voice guide (docs/voice.md), glossary, sentence answers |
| 2026-10-11 | Added: errors stored as structured data, so the notebook can ask them as questions |
| 2026-10-11 | Added: EasyScript Studio as the Phase 4 goal |
| 2026-10-11 | Added: findings from step 22's JSON parser (count direction, value kinds, exact numbers, raw text, early exit, math words) |
| 2026-10-11 | Merged the repo's step 22 notes (escapes, positions, helper name clashes, workarounds); heading renamed to "Rough spots to fix" so links from CHANGELOG and the roadmap still work |
| 2026-10-11 | Part 1 decisions: file sentences say `file`; text `and` and input joiners recorded as planned changes; no "missing"; `every` triggers; the `it` rule; `per` only; `, then` chains; `the X of Y` rule; `exit code`; the comma rule; depends-on lines; already-built list; error counts and step 21 list updated |
| 2026-10-12 | Part 1 built: `exit code`; `as a number` reads `1e5`, `as exact text`; counting up only (a warning for written counts that never run); `unless`, `give back X if/unless C`, `stop here`; line continuation; pieces in `say` and brackets in text (`[[`/`]]` for brackets), text `and` removed; `and`/`and then` between sentences (a function's name after `and` is a value, so it needs `and then`); `has` records with kinds from use (`some` dropped before a field); `can` methods; `the X of Y`; filters `that`/`whose`/`where` as postfixes and `each`; `for each score:`; `made of` kinds; docs/voice.md with the glossary ("name" for variables, "definition"/"how to X" for `to` functions) and `audit/plain-words`; errors as data (`diag_info`, `easyscript errors`). |
| 2026-10-12 | Part 3 (choices without if), items 1 to 6 built: `, or X if there isn't one` for anything that can be nothing; `, starting at S` on map value changes; adjectives (`KIND is ADJ if ...`, `ADJ KIND is one whose ...`) as methods named "is ADJ", used as `X is ADJ`, `the ADJ Ls`, `are ADJ`, `every ADJ ITEM`; values (`KIND's FIELD is ...`) as methods named "the FIELD", used like fields; named values (`the NAME is the one with ...`, `the NAME are the ones whose ...`) read where they're used; tables (`NAME depends on X:` like `let`, `the FIELD of a KIND depends on X:` like a value), with every case covered by an `otherwise` line; rules (`FIELD must ...`, `KIND's FIELD must ...`) checked in `new`, `set` and changes; `remove the Ls that ...` and `set every ITEM's FIELD to X`. Decided while building: `a ... is one` describes and `the ... is the one` names (by the dropped `the`); a table that makes a name is worked out where it's written; `give every loyal customer a 10 percent discount` waits (it needs a way to name actions with inputs in the middle); `the one whose ...` and `the ones with the highest ...` are errors, since neither says exactly one thing. |
| 2026-10-12 | Part 3 finished: events (`every N seconds:`, `every Monday at 9am:`, `every day at 17:00:`, `whenever C:`, `when C, S`, `whenever a key is pressed:`), run by one loop after the program's other sentences (rules checked each round, keys waited for until the next schedule, then the due schedules; keys from a file come one per round, so tests are repeatable); Snake rewritten with them; `9am` lexed as `9` `am`; sentence definitions written in place at their calls (the speed rule: a describing word in a question was 70% slower as a call); examples coffee_shop.es and shipping.es; benchmarks `adjective` and `table`. |
| 2026-10-12 | Part 4 started: questions (`?`, `what is`, `is X ...?`, `how many`, `which ITEM has the highest`, `which Ls are`), requests (`show me`, `give me`, `tell me`, `list`), polite words, steps in a row with the `it` rule (a step's result kept in a hidden name only when the next step says "it"; `it` where an item could also be meant is an error), actions (`total the FIELD`, `average`, `median`, `count`), tables (`read the table in F`: a kind of row per sentence whose fields are the columns used, kinds from use, columns checked on reading; other columns kept for printing and saving; `per month` uses the `date` column), `save X as F` for maps and records. `me`, `undo` reserved. The "best months" example compiles (without its chart line). Decided: file sentences keep `file` (`write "milk" in file it`); a map saves as `key`,`value` columns. |
| 2026-10-12 | Part 4 continued: `ask for a number`, `ask yes or no` (asking again), `as money`/`as a percent`/`with commas`, `the NAME is always VALUE.`, `the sum of`, `try to S. if that fails, ...` with `the reason` (file and folder sentences, checked before they run: no way to undo a half-done sentence safely in the generated C), `the text below:` blocks and `\r`/`\u{...}` escapes, guards in `when` (they don't count as covering the variant). Exact money and units of measure deferred (each a new kind of value). |
| 2026-10-12 | Charts done: `save a bar chart of X as F [, titled T]` (also line and pie charts) and `make a ... chart of X called F`, drawn by `lib/charts.es` (EasyScript), joined to programs with a chart sentence; the best months example has its chart line. |
| 2026-10-12 | Part 5 done: new values in lists and maps without parentheses, call sentences whose inputs are whole values, joiners between inputs (`to column of cell in grid:`), mixed lists of one interface, a better suggestion for `profit by customer`; the phrase audit (new exceptions in design-principles.md, every vocabulary example parsed); examples README with "Start here" first and older examples in the plain style; the "Your first programs" chapter (checked for banned words, every program in it run by the tests); `make bench` with an English-pass history row, the `sorted by FIELD` form (now 1.00 of its hand-written version) and the 20% check fixed; this file's [Later](#later) list. |
| 2026-10-12 | Part 4: examples as tests (`for example, ...`, `easyscript test`), the closest sentences for a sentence that isn't understood, `as a table`, `sorted by FIELD`, `, one per line`, `what's in file F`/`what's in it`, table rows named after the table; examples `groceries.es` and `budget.es`. Named kinds of text with rules deferred (how a value becomes one isn't settled); `the store with the highest total amount` (best group) left as a rough spot. |
| 2026-10-12 | Reads like text (branch `reads-like-text`): the new design rule, and the decisions above. Built: inputs in the middle of a definition's first line, percent changes. |
| 2026-10-12 | Shell and money fixes: charts of amounts; a table's file decides a column's kind where the program's use leaves it as a number or an amount (columns only shown or sorted: Still open); the shell after a failed line; `how many Ls are there` (`are left` and questions without `?`: Still open). |
| 2026-10-12 | v0.2 prep (simulated testers): built `!` as a sentence end, polite words after step words, answers used as numbers asked for as numbers, one error for a file's name without quotes, notes for opening tables and for words from spreadsheets and other languages, "can't do yet" errors for email, web pages, the internet and Excel files, `rounded to 2 decimals` and `round X to N decimal places`, and an error for `a plus b rounded` (proposals 0016 to 0022). Open: `for each FIELD` as `per`, `open the table in`, greetings, other describing words after arithmetic, `NAME is VALUE`. |
| 2026-10-12 | v0.2 prep (shell-and-money review): built `read the table in F, with amount in dollars [and quantity as a number]` (a stated kind wins over the file), plain numbers among amounts tested on both paths, and blank cells as nothing (totals skip them, `how many` counts rows, shown and saved blank, an error in arithmetic suggesting `, or 0`; proposals 0023 and 0024). Not built, as decided: `how many X are left`, judging shown-or-sorted columns from the file. |
| 2026-10-12 | Part 2 (questions and totals) built: `how many`, `any`/`every`/`no`/`all`/`none`, `the total`/`average`/`median of`, `the top N`/`lowest N`, `the N most common`, `the X with the highest Y`, `per FIELD` and `per month`/`year`/`day`/`weekday` (keys: "October 2026" text, years as numbers, so no date keys were needed), maps sorted by value, math words, the comma before `rounded`/`sorted`/`as`, plural verbs after `that`, and `easyscript plan`. Decided while building: built-in phrases win over fields (`the total of rex` is an error noting `rex's total`); a definition with the word keeps it, and `the` + that word is an error; `the one with the most Y` waits for part 3's definitions (it needs a list to refer to); action forms (`total the amount per month`) wait for part 4's `, then` chains, where `it` exists. |

## Sources

- [Pane, J. F. (2002). A Programming System for Children that is Designed for Usability. PhD thesis, Carnegie Mellon University (includes Pane, Ratanamahatana & Myers 2001)](https://www.csd.cmu.edu/sites/default/files/phd-thesis/CMU-CS-02-127.pdf)
- [Ko, A. J., et al. (2011). The State of the Art in End-User Software Engineering. ACM Computing Surveys](https://faculty.washington.edu/ajko/papers/Ko2011EndUserSoftwareEngineering.pdf)
- [Scaffidi, Shaw & Myers. The 55M End-User Programmers Estimate Revisited (CMU)](https://kilthub.cmu.edu/articles/journal_contribution/The_55M_End-User_Programmers_Estimate_Revisited/6470489/1) (page didn't load; figures taken from Ko et al.)
- [Ko, A. J., & Myers, B. A. (2004). Designing the Whyline. CHI 2004](https://hci.rwth-aachen.de/materials/conferences/CHI2004/1p151.pdf)
- [JetBrains. Computer Science Learning Curve Report 2024](https://lp.jetbrains.com/cs-learning-curve-report-2024)
- [JetBrains and Python Software Foundation. Python Developers Survey 2024](https://lp.jetbrains.com/python-developers-survey-2024/)
- [Attempto Controlled English (Wikipedia)](https://en.wikipedia.org/wiki/Attempto_Controlled_English)
- [Simplified Technical English (SKYbrary)](https://skybrary.aero/articles/simplified-technical-english-ste)
- [Cognitive dimensions of notations (Wikipedia)](https://en.wikipedia.org/wiki/Cognitive_dimensions_of_notations)
- [Effective Compiler Error Message Enhancement for Novice Programming Students (CORE)](https://core.ac.uk/works/78318990)

* [Stefik & Siebert (2013). An Empirical Investigation into Programming Language Syntax (review)](https://neverworkintheory.org/2014/01/29/stefik-siebert-syntax.html)
* [Hedy: an inclusive, multi-lingual, gradual programming language (ICPEC 2024)](https://drops.dagstuhl.de/storage/01oasics/oasics-vol122-icpec2024/OASIcs.ICPEC.2024.1/OASIcs.ICPEC.2024.1.pdf)
* [Resnick et al. (2009). Scratch: Programming for All. CACM](https://web.media.mit.edu/~mres/papers/Scratch-CACM-final.pdf) (page didn't load; summarized from background knowledge)
* [Flash Fill: text wrangling for non-programmers (Jon Udell, 2013)](https://blog.jonudell.net/2013/02/18/flash-fill-text-wrangling-for-non-programmers/)

- [Hone & Baber (2001). Designing Habitable Dialogues for Speech-Based Interaction with Computers (discusses Watt 1968)](https://research.birmingham.ac.uk/files/4856389/IJHCSHoneHabitable.pdf)
- [Dahlbäck, Jönsson & Ahrenberg (1993). Wizard of Oz Studies: Why and How](https://www.media.mit.edu/gnl/discint00/papers/p193-dahlback.pdf)
- [Yu et al. (2018). Spider: A Large-Scale Human-Labeled Dataset for Complex and Cross-Domain Semantic Parsing and Text-to-SQL Task](https://aclanthology.org/D18-1425/)