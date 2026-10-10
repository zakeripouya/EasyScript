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

There are three kinds: the **lexer** and **parser** catch problems in how sentences are written, the **checker** catches problems with names, and **runtime errors** happen while a program runs. Compile-time errors (the first three) are all reported together, and the program isn't built. A runtime error stops the program.

### From the lexer

These come from the lexer. Run `easyscript tokens yourfile.es` to see them.

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

### From the parser

These come from the parser. Run `easyscript ast yourfile.es` to see them.

**A misspelled first word**: [`err_unknown_statement.es`](../examples/parser/err_unknown_statement.es)

```
Line 2: I don't know a sentence that starts with "sya".
    sya "hello"
    ^^^
Did you mean "say"?
```

**A filler word used as a name**: [`err_filler_name.es`](../examples/parser/err_filler_name.es). The compiler remembers where it dropped `a`, `an`, or `the`, so it can explain what happened instead of complaining about the next word.

```
Line 2: "a" can't be used as a name.
    let a be 5
        ^
EasyScript ignores the words "a", "an" and "the" wherever they appear, so they can't be names.
Try a name that says what it holds, like "total" or "answer".
```

**A word that already means something**: [`err_reserved_name.es`](../examples/parser/err_reserved_name.es)

```
Line 2: "yes" can't be used as a name.
    let yes be 1
        ^^^
"yes" already means something in EasyScript.
Try a different name, like "my_yes".
```

**A comparison with a word missing**: [`err_missing_than.es`](../examples/parser/err_missing_than.es)

```
Line 2: Something is missing after "is greater".
    say score is greater 10
              ^^^^^^^^^^
Did you mean "is greater than"?
```

**Chained comparisons**: [`err_chained_comparison.es`](../examples/parser/err_chained_comparison.es)

```
Line 2: Comparisons can't be chained like this.
    say 1 < x < 10
        ^^^^^^^^^^
Compare one pair at a time and join them with "and": "1 < x and x < 10".
```

**Something that could mean two things**: [`err_ambiguous_call.es`](../examples/parser/err_ambiguous_call.es). This is the "no guessing" rule: the compiler shows both readings instead of picking one.

```
Line 2: "double using 21 plus 1" could mean two things.
    say double using 21 plus 1
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
| A statement missing a word (`let x 5`, `add 5 total`, `ask "Name?"`, `stop`) | "I expected "be" here ..." or "Something is missing after ...", with a correct example of the statement |
| The wrong connecting words (`ask ... and call it x`, `read file ... and call the answer x`) | quotes what was written and what was expected |
| Words after a finished statement (`add 5 to total plus 1`) | "I expected the sentence to end after "total"." |
| `x is 5` or `total = 5` as a sentence | suggests `let x be ...` or `set x to ...` |

After an error the parser skips to the end of that sentence and carries on, so you see one error per sentence and every sentence gets checked. The complete, tested set is in [`tests/ast/`](../tests/ast/) (the `err_*.err` files).

### From the checker

The checker makes sure every name you use has been made with `let` (or by `ask` or `read file`) before it's used, and that no name is made twice. It only runs when the program has no parser errors, so one mistake never causes a flood of follow-on errors.

**A misspelled name**: [`misspelled_variable.es`](../tests/errors/misspelled_variable.es)

```
Line 4: I don't know anything called "totl".
    add 5 to totl.
             ^^^^
Did you mean "total"? You made it on line 1.
```

**A name used before it's made**: [`made_later.es`](../tests/errors/made_later.es)

```
Line 1: I don't know anything called "x".
    say x
        ^
You make "x" later, on line 2. Make it before you use it.
```

**A name made twice**: [`made_twice.es`](../tests/errors/made_twice.es)

```
Line 2: You already made "x" on line 1.
    let x be 2
        ^
To change it, write "set x to ...".
```

Calling a function (`greet using "Ada"`) and `it` are also checker errors for now, because what they mean isn't implemented yet. The full, tested set is in [`tests/errors/`](../tests/errors/).

### While the program runs

Runtime errors are one line, `Line N:` and what went wrong, sometimes followed by a hint. Anything the program printed before the error stays printed.

**Dividing by zero**: [`err_divide_by_zero.es`](../examples/programs/err_divide_by_zero.es)

```
Line 3: You divided by zero.
```

**Mixing up kinds of values**: [`err_subtract_text.es`](../tests/run/err_subtract_text.es)

```
Line 3: I can't subtract text from a number.
```

| Problem | Message (summary) |
|---|---|
| Arithmetic with text, yes/no, or nothing | "I can't add text to a number." With text, the hint suggests `and` or `followed by` |
| Dividing or `mod` by zero | "You divided by zero." |
| `and` between two numbers, or between yes/no and text | explains, and suggests `plus` or `as text` |
| `or` / `not` without yes/no values | says which side wasn't yes or no |
| Ordering text against a number (`1 is less than "2"`) | "I can't compare a number with text." with a hint to use `as a number` |
| `as a number` on text that isn't a number | "I can't turn "12abc" into a number." |
| `length of` something that isn't text | "I can only find the length of text, but this is a number." |
| A file that can't be read or written | "I couldn't read the file "nope.txt": it doesn't exist." |
| A file name that isn't text | "The name of a file has to be text, but this is a number." |

Parts of an expression are worked out left to right, so when two parts would both fail, the error is always about the first one. The tested messages are the `err_*.err` files in [`tests/run/`](../tests/run/).

## Coming soon

**Ambiguous blocks.** Blocks (`if`, loops, functions; coming soon) will follow the same rule as everything else: if a sentence could be read in more than one way, the compiler stops and lists the readings.

**Source lines in runtime errors.** Runtime errors will show the line of your program and point at the part that failed, like compile errors do.
