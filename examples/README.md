# Examples

Small programs, one per feature. **Every example is checked by `make test`** against its expected output, so if an example is here, it really works.

## `lexer/`: the new syntax, as read by the lexer

The new sentence syntax can't run yet; only its lexer is finished. These examples show how the compiler reads each feature. `NAME.tokens` holds the expected output of `easyscript tokens NAME.es`, and `err_*` examples also have `NAME.err` with the exact error message.

| Example | Shows |
|---|---|
| [taste.es](lexer/taste.es) | The program from the README |
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

## `parser/`: expressions, as understood by the parser

Expressions parse today, but don't run yet. Until statements arrive, each line of these files is one expression. `NAME.ast` holds the expected output of `easyscript ast NAME.es` (the syntax tree), and `err_*` examples also have `NAME.err`.

| Example | Shows |
|---|---|
| [arithmetic.es](parser/arithmetic.es) | Words and symbols, precedence, parentheses |
| [comparisons.es](parser/comparisons.es) | `is at least`, `is greater than or equal to`, `isn't`, `reaches`, `!=` |
| [logic.es](parser/logic.es) | `and`, `or`, `not` |
| [text.es](parser/text.es) | Joining text, `as text`, `as a number`, `length of`, `contents of file` |
| [calls.es](parser/calls.es) | `NAME using X, Y` |
| [err_missing_than.es](parser/err_missing_than.es) | Error: "Did you mean "is greater than"?" |
| [err_chained_comparison.es](parser/err_chained_comparison.es) | Error: `1 < x < 10` |
| [err_ambiguous_call.es](parser/err_ambiguous_call.es) | Error: a sentence that could mean two things |

```bash
./easyscript ast examples/parser/arithmetic.es
```

## `legacy/`: the 2024 prototype syntax

This syntax runs end to end today, but it's being replaced by the new syntax and will be removed once the new parser works. `NAME.out` holds the expected output of `easyscript run NAME.es`. (The prototype has no comments, so the explanations are here instead.)

| Example | Shows |
|---|---|
| [print_text.es](legacy/print_text.es) | `PRINT # "..."` |
| [variables.es](legacy/variables.es) | `MAKE A VARIABLE ... ASSIGN ...` with text and a number |
| [files.es](legacy/files.es) | `FILE OPEN`/`WRITE`/`READ`/`CLOSE`. `FILE READ` prints an extra blank line, a known quirk of the prototype |

```bash
./easyscript run examples/legacy/variables.es
```

## Adding an example

Put it in `lexer/`, `parser/`, or `legacy/` (`tests/run.sh` rejects any other folder) with its expected output, run `make test`, and list it in the table above. Examples for features that don't work yet are listed in [docs/roadmap.md](../docs/roadmap.md#planned-examples) instead.
