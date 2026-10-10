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
| [factorial.es](programs/factorial.es) | A function that calls itself |
| [fibonacci.es](programs/fibonacci.es) | Recursion with two calls, and joining results into one line |
| [taste.es](programs/taste.es) | The README's bigger example: a loop and a function |
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
