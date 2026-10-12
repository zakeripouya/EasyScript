# Examples

Small programs, one per feature. **Every example is checked by `make test`** against its expected output, so if an example is here, it really works.

## `programs/`: programs you can run

`NAME.out` holds the expected output of `easyscript run NAME.es`. `NAME.in`, if present, is typed in as the program's input. `err_*` examples stop with a runtime error, and `NAME.err` holds the exact message.

| Example | Shows |
|---|---|
| [hello.es](programs/hello.es) | `say` |
| [first_program.es](programs/first_program.es) | The program on the README's front page |
| [variables.es](programs/variables.es) | `let`, `add … to`, `increase`, `multiply`, `set` |
| [text.es](programs/text.es) | Joining text with `and` and `followed by`, `length of`, `as a number` |
| [logic.es](programs/logic.es) | Comparisons, `and`, `or`, `not` |
| [decisions.es](programs/decisions.es) | `if`, `otherwise if`, `otherwise`, and the one-line forms |
| [loops.es](programs/loops.es) | Counting up and down, `repeat ... times`, `while`, `skip this one`, `stop the loop`, `it` |
| [logan.es](programs/logan.es) | The 2024 prototype's loop example (`old/logan.code`), ported to today's syntax |
| [tax.es](programs/tax.es) | A constant (`keep tax_rate as 0.2`) used by a function |
| [factorial.es](programs/factorial.es) | A function that calls itself |
| [fibonacci.es](programs/fibonacci.es) | Recursion with two calls, and joining results into one line |
| [taste.es](programs/taste.es) | The README's bigger example: a loop and a function |
| [fizzbuzz.es](programs/fizzbuzz.es) | A function with several `give back`s, used in a loop |
| [guessing_game.es](programs/guessing_game.es) | `ask` in a loop, `if`/`otherwise if`, constants (input from `guessing_game.in`) |
| [notes.es](programs/notes.es) | Writing, appending, and reading back a file, and its length |
| [hanoi.es](programs/hanoi.es) | The Towers of Hanoi: a recursive function with four inputs |
| [receipt.es](programs/receipt.es) | Constants, a function, a loop, and running totals |
| [kinds.es](programs/kinds.es) | Kinds of values worked out by the compiler, and an input and result with their kinds written down (see [Types](../docs/language-guide.md#9-types)) |
| [grade_statistics.es](programs/grade_statistics.es) | Lists: a class's scores, with the average, median, highest and lowest, slices (`the last 3 items in`, `items 3 to 6 of`), letter grades, `each`, `the scores that are at least 60`, pieces in `say` and values in brackets, `give back "A" if ...`, and a fixed list (`keep all grades as a list of ...`) (see [Lists](../docs/language-guide.md#10-lists)) |
| [top_scores.es](programs/top_scores.es) | Lists: a high-score table kept in order with `insert`, `remove`, `contains`, sorting by length, and joining lists with `followed by` |
| [word_counter.es](programs/word_counter.es) | Text tools and maps: reads a file, cleans up its words (`in lowercase`, `with ... replaced by`, `the words of`), counts them with a map (`the value for word in counts, or 0`), and reports the most common ones by going through a copy of the map and removing each winner; `the first character of`, `in uppercase`, `without the first character` (see [Text](../docs/language-guide.md#11-text) and [Maps](../docs/language-guide.md#13-maps)) |
| [graph.es](programs/graph.es) | Maps: a network of stations as a map from each station to a list of its neighbours, the shortest way between two stations with a breadth-first search (a map of where each station was reached from), and how many stops every station is from the start (see [Maps](../docs/language-guide.md#13-maps)) |
| [linked_list.es](programs/linked_list.es) | Records: a linked list of nodes whose `next` can be nothing (`a node has a value (a number) and a next (a node or nothing).`), with methods (`a chain can push with value:`, `pop`, `reverse`, `describe`), `the head of the chain`, a `while ... is not nothing` walk, and recursion that stops at nothing (see [Records](../docs/language-guide.md#12-records)) |
| [binary_tree.es](programs/binary_tree.es) | Records: a binary search tree with `left` and `right` that can be nothing, inserting, walking in order, searching, and its depth (see [Records](../docs/language-guide.md#12-records)) |
| [grid_walk.es](programs/grid_walk.es) | A kind made of fields: points on a grid (`a point is made of an x (a number) and a y (a number).`) compared by their contents, so `visited contains at` finds a spot seen before; `for each move:`, `unless` (see [Kinds with variants](../docs/language-guide.md#values-made-of-fields)) |
| [shapes.es](programs/shapes.es) | Kinds with variants: an area calculator for circles, rectangles, triangles and points, with `check which kind s is:`, `if s is a circle and s's radius ...`, and equality by value (see [Kinds with variants](../docs/language-guide.md#14-kinds-with-variants)) |
| [calculator.es](programs/calculator.es) | A recursive kind: reads "2 plus 3 times 4" word by word into a tree of sums and products (times and "divided by" before plus and minus), shows it with parentheses, and works it out |
| [lambda.es](programs/lambda.es) | A tiny lambda calculus interpreter: terms and results are kinds with variants, scopes are maps, and Church numerals are read back by applying them to a built-in "add one" |
| [multi_file/](programs/multi_file/main.es) | A program in two files: `main.es` uses `shapes.es` (`use shapes.`), which defines a kind, functions and a private helper; `shapes's summary of bed` names one of its functions (see [Programs in several files](../docs/language-guide.md#17-programs-in-several-files)). A folder holding `main.es` is run as one program |
| [dice.es](programs/dice.es) | Random numbers: two dice rolled a thousand times (`roll a die`), a bar chart with `rounded`, `shuffle`, `pick one of`; `use the same random numbers every time` keeps the output the same (see [the standard library](../docs/stdlib.md)) |
| [countdown.es](programs/countdown.es) | `wait 1 second`, `clear the screen` and `the time now` |
| [tidy_folder.es](programs/tidy_folder.es) | Files and folders: makes a folder of files, sorts them into folders by their endings (`make a folder called`, `move the file ... to the folder`, `the files in`, `the folders in`), renames and copies one, and deletes it all |
| [snake/](programs/snake/play.es) | Snake in the terminal (`play.es`): the keyboard (`the key being pressed`), `clear the screen`, `wait` and random food. Its rules are in `board.es`, tested by a scripted game in `main.es` (the test suite runs that, and only compiles `play.es`) |
| [json/](programs/json/main.es) | A JSON parser and printer written in EasyScript (`json.es`: kinds with variants for values, maps, lists and text tools; errors with their line and column), tested by `main.es` on the documents sent to it in `main.in` (`the text sent to the program`) |
| [gradebook.es](programs/gradebook.es) | Questions and totals: `how many students whose score is at least 60`, `the average score of students`, `the median`, `the student with the highest score`, `any`/`every`/`no`, `the top 3 students by score`, `the average score of students per class, sorted biggest first` (see [Questions and totals](../docs/language-guide.md#21-questions-and-totals)) |
| [shipping.es](programs/shipping.es) | Choices without if: a shipping calculator with tables (`the base price of a parcel depends on its weight:`), describing words (`a parcel is heavy if ...`), values (`a parcel's price is ...`), rules (`a weight must be between 0 and 70.`), a named value and defaults (see [Choices without if](../docs/language-guide.md#22-choices-without-if)) |
| [order_report.es](programs/order_report.es) | The sales report from the English pass plan, with questions: big orders counted, profit per customer, the best customer, totals per region, and `if every order's profit is above 0` |
| [sales_report.es](programs/sales_report.es) | CSV files: writes a sales.csv (if there isn't one), reads it (`read csv file ... and call it`), totals each region with a map, and saves a report (`save ... as csv file`) |
| [orders.es](programs/orders.es) | Dates in records: orders with a day each, this month's orders (`is in this month`), totals per month in a map, the earliest and latest (`the smallest of`, `the biggest of`), and sorting by date (`sort ... by`) |
| [pets.es](programs/pets.es) | Records: a dog and a cat with methods of the same name (`speak`, `describe`); each call picks the one for its record's kind |
| [million_lines.es](programs/million_lines.es) | A million lines of text made one at a time, in flat memory (see [memory](../docs/memory.md)) |
| [ask_name.es](programs/ask_name.es) | `ask … and call the answer …` (input from `ask_name.in`) |
| [files.es](programs/files.es) | `write`, `append`, and `read file` |
| [err_divide_by_zero.es](programs/err_divide_by_zero.es) | A runtime error: "Line 3: You divided by zero." |

```bash
./easyscript run examples/programs/hello.es
```

## `lexer/`: how text is split into words and symbols

These show how the lexer reads each feature. `NAME.tokens` holds the expected output of `easyscript tokens NAME.es`, and `err_*` examples also have `NAME.err` with the exact error message.

| Example | Shows |
|---|---|
| [sentences.es](lexer/sentences.es) | Periods and new lines end sentences; capitals and filler words |
| [text.es](lexer/text.es) | Text in quotes, escapes, apostrophes in words |
| [numbers.es](lexer/numbers.es) | Whole numbers, decimals, arithmetic symbols |
| [comparisons.es](lexer/comparisons.es) | `=` `!=` `<` `>` `<=` `>=` |
| [blocks.es](lexer/blocks.es) | Colons and indented blocks |
| [comments.es](lexer/comments.es) | `#` and `note:` comments |
| [err_missing_quote.es](lexer/err_missing_quote.es) | Error: text without a closing quote |
| [err_bad_indentation.es](lexer/err_bad_indentation.es) | Error: indentation that doesn't line up |
| [err_number_with_letters.es](lexer/err_number_with_letters.es) | Error: `3times` |

```bash
./easyscript tokens examples/lexer/blocks.es
```

## `parser/`: how sentences are understood

These show the syntax tree for each kind of sentence (`easyscript ast`). Many use names they never make, so they're for reading the tree, not for running. `NAME.ast` holds the expected output of `easyscript ast NAME.es` (the syntax tree), and `err_*` examples also have `NAME.err`.

| Example | Shows |
|---|---|
| [variables.es](parser/variables.es) | `let`, `set`, `change`, `add … to` and the other arithmetic sentences |
| [output.es](parser/output.es) | `say`, `print`, `show`, `display`, `write`, `please` |
| [input.es](parser/input.es) | `ask … and call the answer …` |
| [files.es](parser/files.es) | `write … to file`, `append … to file`, `read file … and call it`, `stop the program` |
| [arithmetic.es](parser/arithmetic.es) | Words and symbols, precedence, parentheses |
| [comparisons.es](parser/comparisons.es) | `is at least`, `is greater than or equal to`, `isn't`, `reaches`, `!=` |
| [logic.es](parser/logic.es) | `and`, `or`, `not` |
| [text.es](parser/text.es) | Joining text, `as text`, `as a number`, `length of`, `contents of file` |
| [calls.es](parser/calls.es) | `NAME using X, Y` |
| [err_unknown_statement.es](parser/err_unknown_statement.es) | Error: a misspelled first word |
| [err_filler_name.es](parser/err_filler_name.es) | Error: `let a be 5` |
| [err_reserved_name.es](parser/err_reserved_name.es) | Error: `let yes be 1` |
| [err_missing_than.es](parser/err_missing_than.es) | Error: "Did you mean "is greater than"?" |
| [err_chained_comparison.es](parser/err_chained_comparison.es) | Error: `1 < x < 10` |
| [err_ambiguous_call.es](parser/err_ambiguous_call.es) | Error: a sentence that could mean two things |
| [err_otherwise_misplaced.es](parser/err_otherwise_misplaced.es) | Error: an `otherwise` that doesn't line up with its `if` |

```bash
./easyscript ast examples/parser/variables.es
```

## Adding an example

Put it in `programs/`, `lexer/`, or `parser/` (`tests/run.sh` rejects any other folder) with its expected output, run `make test`, and list it in the table above. Examples for features that don't work yet are listed in [docs/roadmap.md](../docs/roadmap.md#examples) instead.
