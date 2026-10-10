# Error messages

A compiler spends more time talking to people about mistakes than about success. EasyScript's errors are designed to be read by someone who has never programmed before.

## What an error looks like

Every error has the same shape:

```
Line 2: This text is missing its closing quote.
    say "Hello, world!.
        ^^^^^^^^^^^^^^^
Add a " where the text ends. Text has to finish on the line it starts on.
```

1. **`Line N:` and a plain-English sentence** saying what's wrong.
2. **The line of your program**, indented.
3. **Carets (`^`)** under the exact part that's wrong.
4. **A suggestion**: what to write instead, or where the thing you mentioned was defined.

That example is real output from today's compiler. It's [`examples/lexer/err_missing_quote.es`](../examples/lexer/err_missing_quote.es), and `make test` checks that the message stays exactly like this.

## The philosophy

- **Say it in English.** "This text is missing its closing quote", not "unterminated string literal". No jargon, no error codes, no grammar terms.
- **Point at the problem.** Show the line and underline the part that's wrong, so you don't have to count columns.
- **Always suggest a fix.** If the compiler can tell what would work, it says so: the valid indentations, the closest variable name, the right symbol.
- **Never guess.** The compiler never quietly picks a meaning and carries on. If a sentence is ambiguous, that's an error that lists the possible readings. Suggestions are only ever *shown* to you; they're never applied automatically.
- **Report everything at once.** The compiler collects every error it finds in one run and shows them all, separated by blank lines, so you can fix several mistakes before trying again.
- **No AI required.** Every message and suggestion comes from deterministic rules in the compiler, such as edit distance for "did you mean". The same program always produces the same errors.

## Errors you can see today

These come from the new lexer. Run `easyscript tokens yourfile.es` to see them.

**Indentation that doesn't line up**: [`err_bad_indentation.es`](../examples/lexer/err_bad_indentation.es)

```
Line 4: This line is indented 2 spaces, which doesn't line up with any block above it.
      say "done".
    ^^
Use 0 or 4 spaces to match one of the blocks it belongs to.
```

**Letters stuck to a number**: [`err_number_with_letters.es`](../examples/lexer/err_number_with_letters.es)

```
Line 2: A number can't have letters stuck to it: "3times".
    repeat 3times:
           ^^^^^^
Put a space between them, like "3 times".
```

The lexer also explains:

| Problem | Message (summary) |
|---|---|
| Text with no closing quote | "This text is missing its closing quote." |
| Unknown escape such as `\q` | "I don't know the escape "\q"." and lists the valid escapes |
| A lone `!` | "I don't understand "!"." and suggests `!=` |
| An apostrophe outside a word | explains that apostrophes go inside words and text goes in quotes |
| Curly quotes pasted from a word processor | "That's a curly quote. Use straight double quotes (") around text." |
| Letters outside a to z in a name, like `café` | suggests putting other text in double quotes |
| A tab mixed into a bad indentation | adds "(A tab counts as 4 spaces.)" |
| A file that isn't UTF-8, or invisible control characters | names the byte or character |

The complete, tested set is in [`tests/tokens/`](../tests/tokens/) (the `err_*.err` files).

## Errors from the parser

These come from the expression parser. Run `easyscript ast yourfile.es` to see them.

**A comparison with a word missing**: [`err_missing_than.es`](../examples/parser/err_missing_than.es)

```
Line 2: Something is missing after "is greater".
    score is greater 10
          ^^^^^^^^^^
Did you mean "is greater than"?
```

**Chained comparisons**: [`err_chained_comparison.es`](../examples/parser/err_chained_comparison.es)

```
Line 2: Comparisons can't be chained like this.
    1 < x < 10
    ^^^^^^^^^^
Compare one pair at a time and join them with "and": "1 < x and x < 10".
```

**Something that could mean two things**: [`err_ambiguous_call.es`](../examples/parser/err_ambiguous_call.es). This is the "no guessing" rule: the compiler shows both readings instead of picking one.

```
Line 2: "double using 21 plus 1" could mean two things.
    double using 21 plus 1
    ^^^^^^^^^^^^^^^^^^^^^^
Use parentheses to say which: "(double using 21) plus 1" or "double using (21 plus 1)".
```

The parser also explains:

| Problem | Message (summary) |
|---|---|
| Any phrase missing its last word (`is at 5`, `multiplied 3`, `followed "b"`, `as numbr`, `contents of "a.txt"`) | "Something is missing after ..." and lists the complete phrases |
| `is not greater than`, `isn't less than` | "EasyScript doesn't have ..." and suggests `is at most` / `is at least` |
| `is above or equal to`, `< or equal to` | "... isn't a comparison" and suggests `is at least` / `<=` |
| An operator with nothing after it (`1 plus`, `3 times`) | "Something is missing after "plus"." |
| A word that isn't an operator (`x plsu 3`, `x isnt 4`) | "I don't understand ... here." with the closest operator by spelling |
| An operator word used as a value (`plus 5`) | "I expected a value here, but found "plus"." |
| `(` without `)`, or `)` without `(` | points at the unmatched parenthesis |
| `.5` | suggests writing `0.5` |

After an error the parser skips to the end of that sentence and carries on, so you see one error per sentence and every sentence gets checked. The complete, tested set is in [`tests/ast/`](../tests/ast/) (the `err_*.err` files).

## Coming soon

**Unknown names, with "did you mean".** The error format is implemented and unit-tested with exactly this message. The checker that will produce it is coming in Phase 1:

```
Line 4: I don't know anything called "totl".
    add 5 to totl.
             ^^^^
Did you mean "total"? You made it on line 1.
```

**Ambiguous statements.** Statements (coming soon) follow the same rule as expressions: if a sentence could be read in more than one way, the parser stops and lists the readings.

## Legacy prototype errors

The legacy prototype (`PRINT # ...` syntax) still uses its original one-line errors, for example:

```
Error: Variable not found for y
```

It stops at the first error. These go away when the legacy pipeline is removed.
