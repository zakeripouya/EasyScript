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
- **Every message is tested.** `make test` checks that each error message in the compiler and runtime is produced by at least one test.
- **No AI required.** Every message and suggestion comes from deterministic rules in the compiler, such as edit distance for "did you mean". The same program always produces the same errors.

## Errors as data

Each error added since the English pass is also kept as data: what the problem is (`missing`, `unknown`, `two meanings`, `wrong kind` or `not allowed`), what's missing or unclear, the kind of value it should be, and the possible answers. `easyscript errors FILE` prints every error and warning as one JSON object per line, so other tools (and, later, the notebook, which will ask the error as a question with these choices) can use it:

```
{"line": 3, "column": 10, "file": null, "warning": false, "message": "There's no list called \"totals\" for this to go through.", "notes": [...], "problem": "missing", "missing": "the list to go through", "kind": "a list", "choices": ["for each total in scores:", "for each total in names:"]}
```

Examples are in [`tests/data/`](../tests/data/).

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
| An ordinal with the wrong ending (`2th`) | ""2th" isn't how that ordinal is written." and suggests `2nd` |
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
Try a name that says what it holds, like "total" or "answer".
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

**An "otherwise" that doesn't line up with its "if"**: [`err_otherwise_misplaced.es`](../examples/parser/err_otherwise_misplaced.es)

```
Line 4: This "otherwise" doesn't line up with its "if".
        otherwise:
        ^^^^^^^^^
An "otherwise" has to start in the same column as its "if". The "if" on line 2 starts in column 1.
```

Other `if` mistakes the parser explains: a missing colon (or comma, or `then`), sentences on the same line after the colon, nothing indented under an `if`, a missing condition, an `otherwise` with no `if` above it or after a one-line `if`, a second plain `otherwise`, and an `if` inside a one-line `if`. See [`tests/ast/err_if_header.es`](../tests/ast/err_if_header.es) and [`err_if_otherwise.es`](../tests/ast/err_if_otherwise.es).

**A name with a word that already means something**: [`err_names.es`](../tests/ast/err_names.es)

```
Line 1: "plus" can't be part of a name, because it already means something in EasyScript.
    let price plus tax be 5
              ^^^^
Try a name without it, like "price tax".
```

Also: a mistyped name of several words ("I don't understand "fees" here." with "Did you mean "late fee"?"), a kind in parentheses straight after a function's name ("A kind in parentheses goes after an input, and inputs come after "with"."), and a sentence starting with the first word of a function's name ("I don't know a sentence that starts with "greet"." with a note that the function is called "greet someone" and how to give it an input).

**Brackets in text and pieces in `say`** ([`err_text_pieces.err`](../tests/ast/err_text_pieces.err)): "This "[" starts a value inside the text, but there's no "]" to end it." (also when the value has text in quotes, whose quote ends the text), "This "]" doesn't end a value." (write `]]`), "There's no value between these brackets.", "I expected "]" to end the value in the text, but found "total"." and "Something is missing after ","." after the last piece.

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

**A name made inside an `if`, used after it**: [`scope_after_if.es`](../tests/errors/scope_after_if.es)

```
Line 4: I don't know anything called "secret".
    say secret
        ^^^^^^
You made "secret" inside the "if" on line 2, so it only exists inside that block. To use it afterwards, make it before the "if".
```

**"stop the loop" outside a loop**: [`stop_outside_loop.es`](../tests/errors/stop_outside_loop.es)

```
Line 2: "stop the loop" only works inside a loop.
    stop the loop
    ^^^^^^^^^^^^^
It leaves the loop it's in. To end the whole program, write "stop the program".
```

**Two counting loops that both call their number "number"**: [`nested_default_number.es`](../tests/errors/nested_default_number.es)

```
Line 2: This loop calls each number "number", but you already made "number" on line 1.
        count from 1 to 3:
        ^^^^^
Give this loop's number its own name, like "count from 1 to 10 as n".
```

**A misspelled function**: [`unknown_function_suggest.es`](../tests/errors/unknown_function_suggest.es)

```
Line 3: I don't know how to "aera".
    say aera of 3 and 4
        ^^^^^^^^^^^^^^^
Did you mean "area"? It's defined on line 1.
```

**The wrong number of inputs**: [`wrong_arg_count.es`](../tests/errors/wrong_arg_count.es). The note points to the function's definition.

```
Line 7: "area" needs 2 values, but this gives it 1.
    say area of 3
        ^^^^^^^^^
It's defined on line 1: "to area with width and height".
```

**A function using the program's variables**: [`function_sees_no_globals.es`](../tests/errors/function_sees_no_globals.es)

```
Line 3: I don't know anything called "total".
        say total
            ^^^^^
A definition only sees its own inputs and the names it makes. To use "total" here, pass it in as an input.
```

**Changing a constant**: [`const_change.es`](../tests/errors/const_change.es)

```
Line 2: "rate" is a constant, so it can't change.
    set rate to 0.3
        ^^^^
It's kept on line 1. If it needs to change, make it with "let rate be ..." instead of "keep".
```

**A constant that uses a variable**: [`const_uses_variable.es`](../tests/errors/const_uses_variable.es)

```
Line 2: "price" is a name that can change, so its value isn't known before the program runs.
    keep tax as price times 0.2
                ^^^^^
A constant is fixed before the program starts. It can only use numbers, text, yes or no, and constants made above it. If the value is only known later, use "let" instead of "keep".
```

The checker also explains `skip this one` outside a loop, `it` outside a counting loop, using a loop's number after the loop ("it only exists inside that loop"), `give back` outside a function, a function defined inside an `if`, a loop, or another function, two functions with the same name, two inputs with the same name, a variable named like a function, and calling a variable. For constants it also explains calls, `it`, file contents and `nothing` in a constant's value, a constant that uses a later constant or itself, constants inside blocks, duplicate constants, names that clash with a constant, and mistakes in the value itself (like dividing by zero), with the same wording as the runtime errors. The full, tested set is in [`tests/errors/`](../tests/errors/).

**`unless` and leaving early:** `unless` needs yes or no like `if` ("An "unless" needs yes or no to decide, but this is a number."), can't have an `otherwise` ("An "unless" can't have an "otherwise"." with a note to turn it into an `if`), and `stop here` only works inside a function that gives back nothing (""stop here" only works inside a function.", or ""double" gives back a value, so it can't stop without one here."). See [`unless_errors.err`](../tests/errors/unless_errors.err) and [`err_unless.err`](../tests/ast/err_unless.err).

**`for each score:`** with no list called `scores` is "There's no list called "things" for this to go through." with a note to make it or say which list ([`loop_each_plural.err`](../tests/errors/loop_each_plural.err)).

### Kinds of values

The checker knows the kind of every value (number, text, yes/no, or a list of one of these) before the program runs, so mixing them up is a compile error with the line and carets. These used to be runtime errors, and they keep the same wording.

**Mixing up kinds of values**: [`type_subtract_text.es`](../tests/errors/type_subtract_text.es)

```
Line 3: I can't subtract text from a number.
    say total minus "x"
        ^^^^^^^^^^^^^^^
```

**Giving a function the wrong kind of value**: [`type_func_argument.es`](../tests/errors/type_func_argument.es)

```
Line 4: "halve" needs a number for "x", but this is text.
    say halve of "four"
                 ^^^^^^
"x" is a number because of line 2. An input has one kind of value everywhere; to make it clear, write it in the definition, like "to halve with x (a number):".
```

**A kind nothing decides**: [`type_undecided_result.es`](../tests/errors/type_undecided_result.es)

```
Line 1: I can't tell what kind of value "forever_down" gives back.
    to forever_down with n:
       ^^^^^^^^^^^^
Say which kind in the definition, like "to forever_down with n, giving back a number:".
```

| Problem | Message (summary) |
|---|---|
| Arithmetic with text or yes/no | "I can't add text to a number." With text, the hint suggests `followed by` |
| An `if` condition that isn't yes or no (`if count:`) | "An "if" needs yes or no to decide, but this is a number." with a hint to compare it |
| A `while`/`until` condition that isn't yes or no | "A loop needs yes or no to decide whether to keep going, but this is a number." |
| A count, step, or number of times that isn't a number | "A count has to start at a number, but this is text." |
| `and` with text on either side | ""and" doesn't join text: it means both, for yes or no values." with "To join text, use commas in "say" (say "Hello,", name) or "followed by" ("Hello, " followed by name)." ([`and_not_text.err`](../tests/errors/and_not_text.err)) |
| `and` between two numbers, or with something that isn't yes or no | "I can't use "and" between a number and a number." (suggesting `plus`), or ""and" needs yes or no on both sides, but the left side is a list of numbers." |
| `or` / `not` without yes/no values | says which side wasn't yes or no |
| Comparing different kinds, or ordering yes/no values | "I can't compare a number with text." with a hint to use `as a number` |
| `as a number` on a yes/no value | "I can't turn a yes/no value into a number." |
| `as exact text` on something that isn't a number | ""as exact text" needs a number, but this is text." |
| `length of` something that isn't text or a list | "I can only find the length of text or a list, but this is a number." |
| A file name that isn't text | "The name of a file has to be text, but this is a number." |
| Storing another kind in a variable | ""total" is a number (made on line 1), but this is text." |
| A function given different kinds, or the wrong kind for a written-down kind | ""halve" needs a number for "x", but this is text." with where the kind came from |
| A function that gives back two kinds, or a value and nothing | ""describe" gives back text (since line 2), but this is a number." |
| Using a function that gives back nothing as a value | ""greet" doesn't give back a value, so it can't be used here." |
| `nothing` used as a value | ""nothing" can't be used as a value." |
| A kind nothing decides | "I can't tell what kind of value "x" is." with the annotation to write |
| A mistake in a written-down kind (`(banana)`) | "I don't know the kind of value "banana"." |

The full, tested set is the `type_*.err` files in [`tests/errors/`](../tests/errors/).

### Lists

A list holds one kind of value, so putting another kind in it, or using a list where it doesn't fit, is a compile error too.

**Putting the wrong kind in a list**: [`list_type_errors.es`](../tests/errors/list_type_errors.es)

```
Line 5: "scores" is a list of numbers, so it can't hold text.
    add "ninety" to the end of scores
        ^^^^^^^^
A list holds one kind of value. To keep text too, make a separate list for it.
```

**`add X to` a list of numbers**: [`list_add_two_meanings.es`](../tests/errors/list_add_two_meanings.es)

```
Line 2: "scores" is a list of numbers, so this could mean two things.
    add 5 to scores
    ^^^^^^^^^^^^^^^
To put it at the end of the list, write "add ... to the end of scores". To add it to every item, write "set scores to every n in scores plus ...".
```

**Questions about a list**: [`question_errors.es`](../tests/errors/question_errors.es), [`err_questions.es`](../tests/ast/err_questions.es)

```
Line 4: There's no list called "results" for "any result" to look through.
    if any result is above 2, say "yes".
           ^^^^^^
"any result" looks through the list called "results". Make that list first, or say which list, like "any result in scores ...".
```

```
Line 12: I can only add up the items of a list, but this is a dog.
    say the total of rex.
                     ^^^
For a field called "total", write "rex's total".
```

```
Line 24: "per month" could mean more than one date here.
    say the number of trips per month.
                            ^^^^^^^^^
Say which, like "per month of start".
```

```
Line 14: "the total" could mean two things here.
    say the total of scores.
            ^^^^^
"total" is also a definition in this program. To use the definition, write "total of ..." without "the". To add up a list, write it like "scores added together".
```

Also: "I expected "there are" or a test like "are above 70" here, ...", "I expected a test like "are above 50" after "all scores", ...", "I expected a field after "per" here, ...", "I expected "with the highest" here, ...", "I expected "divided by" here, ..." (after `the remainder of`), ""per month" needs a date to group by, but a dog has no date in it.", "The top items are picked from a list of numbers or dates, ...", "I can only count how often numbers or text come up, ...".

**Talking to it**: [`chain_errors.es`](../tests/errors/chain_errors.es), [`err_questions_chains.es`](../tests/ast/err_questions_chains.es)

```
Line 3: "it" could mean what the sentence before gave, or the item here.
    let more be scores, then say the scores where it is above 4.
                                                  ^^
Use a name instead: end the sentence before with "and call it ...", or say the item's name.
```

```
Line 1: I can't answer "who" questions yet.
    who is the best?
    ^^^
Ask with "what", "which", "how many" or "is", like "which customer has the highest total?".
```

Also: "The sentence before this one doesn't give anything that "it" could mean.".

**Choices without if**: [`definition_errors.es`](../tests/errors/definition_errors.es), [`err_definitions.es`](../tests/ast/err_definitions.es)

```
Line 7: I don't know what "loyal" means for a customer.
    if ann is loyal, say "yes".
              ^^^^^
Say what it means, like "a customer is loyal if ...".
```

```
Line 8: I know what "best" means for a customer, but not for a product.
    if pen is best, say "yes".
       ^^^
Say what it means, like "a product is best if ...".
```

```
Line 12: "the fee" doesn't say what it is when the total is 10 or less.
    the fee depends on the total:
        ^^^^^^^^^^^^^^^^^^^^^^^^
Add a last line like "10 otherwise." for that case.
```

```
Line 10: This is a number, so it's never nothing.
    say ann's total, or 0.
        ^^^^^^^^^^^
", or" gives another value when there isn't one. Leave it out here.
```

**Events**: [`event_errors.es`](../tests/errors/event_errors.es), [`err_events.es`](../tests/ast/err_events.es)

```
Line 7: "every" can only be at the top level, not inside an "if" or a loop.
        every 2 seconds:
        ^^^^^
Move it to the start of a line, outside every block. Events run once the rest of the program has.
```

```
Line 1: I expected a time of day here, like "9am", "9:30pm" or "17:00".
    every Monday at 25:00:
                    ^^
Write it like "every Monday at 9am:". Hours go from 1 to 12 with am or pm, or from 0 to 23 with minutes (17:00).
```

Also: ""when a key is pressed" happens more than once, so it's "whenever".", "An event needs yes or no to decide, ...".

Also: "I know how to work out a customer's discount, but not a product's.", ""starting at" only works with a value in a map.", ""the ones with the highest ..." doesn't say how many.", ""the one whose ..." could be more than one.", ""big customers" is worked out from itself.", "A table needs its lines below it, indented, after a ":".", "The "otherwise" line has to be the last one.", "A rule can only be at the top level, ...", "The fields called "weight" aren't all the same kind, so one rule can't cover them.".

**A list inside a list without parentheses**: [`err_list_forms.es`](../tests/ast/err_list_forms.es)

```
Line 1: A list inside a list needs parentheses.
    let rows be a list of a list of 1 and 2, and a list of 3
                            ^^^^^^^
Write it like "a list of (a list of 1 and 2) and (a list of 3 and 4)".
```

| Problem | Message (summary) |
|---|---|
| Items of different kinds in `a list of` | "All the items of a list have to be the same kind: the first is a number, but this is text." |
| Looking for the wrong kind with `contains` | ""scores" is a list of numbers, so it can't contain text." |
| A list operation on something that isn't a list | "I can only take an item from a list, but this is a number." (and likewise for adding, inserting, removing, changing items, sorting, reversing, copying, `for each`, `every`, and `only`) |
| A position that isn't a number | "The position of an item has to be a number, but this is text." |
| `added together` on something other than numbers | "I can only add together a list of numbers, but this is a list of text." |
| `biggest of` / `smallest of` / `sort` on yes/no values or lists | "I can only sort a list of numbers or text, but this is a list of yes/no values." |
| `sort ... by their length` on numbers | "I can only sort a list of text or lists by their length, but this is a list of numbers." |
| `is empty` on something other than text, a list or a map | "I can only check whether text, a list or a map is empty, but this is a number." |
| Working something out from a list in a constant | "A constant can't work things out from a list." (a fixed list written out, and its length, are fine) |
| Joining lists of different kinds with `followed by` | "I can't join a list of numbers with a list of text." with a hint to use `as text` |
| A fixed list holding lists | "A fixed list can't hold lists or maps, because those could change." |
| Changing a fixed list (`add`, `remove`, `set item`, `insert`, `sort`) | ""weekdays" is a constant, so it can't change." with a `let` suggestion |
| A loop's item named like something that exists | "You already made "score" on line 1." with a name that isn't taken |
| A loop's item used after the loop | "I don't know anything called "mark"." with "it only exists inside that loop" |
| A position worked out with arithmetic, without parentheses | "A position worked out with arithmetic needs parentheses." |
| `item` without a position | "Something is missing after "item"." |
| `every` in the middle of a value | ""every" has to start the value, because everything after it is worked out for each item." |
| `only L that` without a comparison | "I expected a comparison like "are at least" here, but found "70"." |
| `sort ... by their length, biggest first` | "With "by their length", say "longest first" or "shortest first"." |
| `sort names, longest first` | ""longest first" sorts by length, so it needs "by their length"." |
| A slice of something that isn't a list | "I can only take items from a list, but this is a number." |
| A count of items that isn't a number | "The number of items has to be a number, but this is text." |
| `every 0th item` | "There's no item at position 0." |
| `items 2 to of scores` | "Something is missing after "to"." |

The full, tested set is the `list_*.err` files in [`tests/errors/`](../tests/errors/) and [`err_list_forms.err`](../tests/ast/err_list_forms.err).

**Filters:** `whose` with no kind of record having that field is ""whose" needs a field after it, but no kind of record has one called "colour"."; `each` (like `every`) inside a longer value has to be in parentheses ([`err_list_filters.err`](../tests/ast/err_list_filters.err)).

### Text

Text tools need text (and lists where they say so), and the forms that take characters say how to take items instead, and the other way round.

**A list form used on text**: [`text_type_errors.es`](../tests/errors/text_type_errors.es)

```
Line 5: I can only take an item from a list, but this is text.
    say item 1 of word
                  ^^^^
To take a character of text, write it with "character", like "character 1 of word".
```

| Problem | Message (summary) |
|---|---|
| A character form on something that isn't text | "I can only take a character from text, but this is a list of numbers." with a note to use `item` for a list |
| Looking for something in a number or yes/no value | "I can only look for something in text or a list, but this is a number." |
| Looking for something other than text in text | "I can only look for text in text, but this is a number." with a hint to use `as text` |
| `starts with` / `ends with` without text on both sides | ""starts with" needs text on both sides, but this is a number." |
| A text tool on something that isn't text | "I can only take the words of text, but this is a number." (and likewise for the characters, the lines, uppercase, lowercase, extra spaces, splitting, replacing, the code, and `is a letter` / `is a digit` / `is a space`) |
| `joined with` on something that isn't a list of numbers, text or yes/no values | "I can only join the items of a list, but this is text." (or "I can only join numbers, text or yes/no values, but this is a list of lists of numbers.") |
| Splitting by, joining with, or replacing with something that isn't text | "Text can only be split by text, but this is a number." / "The items can only be joined with text, but this is a number." / "I can only replace text with text, but this is a number." |
| A character code that isn't a number | "A character code has to be a number, but this is text." |
| A text tool in a constant | "A constant can't take text apart or look inside it." with a `let` suggestion |
| `is letter` without the `a`, and no variable called `letter` | "I don't know anything called "letter"." with "To check whether a character is a letter, write "is a letter", with the "a"." |
| `every 0th character` | "There's no character at position 0." |
| `character` without a position | "Something is missing after "character"." |
| `with X replaced` without `by` | "I expected "by" here, but found the text "b"." |
| `split`, `joined`, `starts`, `without extra` with a word missing | "Something is missing after "split"." with "Did you mean "split by"?" |

The full, tested set is the `text_*.err` files in [`tests/errors/`](../tests/errors/) and [`err_text_forms.err`](../tests/ast/err_text_forms.err).

### Records

Records are checked before the program runs: their fields, the values given to `new`, and every use of a value that could be nothing.

**A value that could be nothing, used without a check**: [`record_nothing.es`](../tests/errors/record_nothing.es)

```
Line 13: "first's next" could be nothing, so check it first.
    say first's next's value.
        ^^^^^^^^^^^^
Use it inside "if first's next is not nothing:".
```

**A field missing from `new`**: [`record_fields.es`](../tests/errors/record_fields.es)

```
Line 11: A new dog needs a value for "name" and "age".
    let odd be a new dog.
               ^^^^^^^^^
Give it after "with", like "with name ...", or give the field a starting value where a dog is defined (line 1), like "..., starting at ...".
```

| Problem | Message (summary) |
|---|---|
| A field the kind doesn't have | "A dog doesn't have a field called "nme"." with "Did you mean "name"?" or the list of its fields |
| A field no kind has (on a value whose kind isn't known) | "No kind of record has a field called "wingspan"." |
| A field of something that isn't a record | "Only a record has fields, but this is a number." |
| A field given a value twice, or the wrong kind | "This gives "name" a value twice." / "The field "name" of a dog is text, but this is a number." |
| Setting a field to the wrong kind | "The field "age" is a number, but this is text." |
| A record or nothing given where a record is needed | ""value" needs a node for "start", but this is a node or nothing." with "It could be nothing. Check it first, like "if first's next is not nothing:"." |
| A value (not a name or field) that could be nothing | "This is a node or nothing, so check it first." with a note to give it a name first |
| Checking a record that can't be nothing | ""first" is a node, so it's never nothing." |
| Comparing something that can't be nothing with nothing | "I can't compare a number with nothing." / "I can't compare nothing with nothing." |
| Something that can be nothing, used as a list or map | "This can be nothing, but it's used as a list of numbers here, which can't be." |
| A field that always holds its own kind | "Every chain link would need another chain link inside it, so none could ever be made." with a suggestion to make it "or nothing" |
| A kind of record defined twice, or inside a block | "You already defined "box" on line 3." / "Kinds of record can only be defined at the top level, not inside an "if" or a loop." |
| A field defined twice | "A box already has a field called "width", on line 4." |
| A starting value that isn't fixed, or of the wrong kind | ""size" isn't a constant, so a field can't start at it." / "The field "weight" is a number, but its starting value is text." / "Only a field that can be nothing can start at nothing." / "A field that holds a record can't have a starting value, because records are made while the program runs." |
| A record in a constant | "A constant can't be a record, because records are made while the program runs." |

Mistakes in how a definition or a field is written are parser errors, like "I expected "," here, but found "which"." (a field without its comma), "A list can't be "or nothing"." (or a map), "A new record inside another one needs parentheses.", ""let" makes a new name, so it can't change a field." and "A method is for a kind of record, but this is a number.". The full set is in [`err_records.err`](../tests/ast/err_records.err) and the `record_*.err` files in [`tests/errors/`](../tests/errors/).

**Kinds of record written with `has`:** a field whose kind nothing decides is "I can't tell what kind of value "code" holds." with a note to write it in parentheses (or "It's only ever given nothing..." for a field only ever given nothing), see [`records_has_undecided.err`](../tests/errors/records_has_undecided.err); the parser's mistakes ("I expected ")" to close the "(" on line 1, but found "and".", "I expected "," or "and" before the next field, but found "handle".") are in [`err_records_has.err`](../tests/ast/err_records_has.err).

### Maps

A map has one kind of key (text or numbers) and one kind of value, so a key or value of another kind is a compile error, and so is using something that isn't a map where one goes.

**A key of the wrong kind**: [`map_type_errors.es`](../tests/errors/map_type_errors.es)

```
Line 4: The keys of "ages" are text, but this is a number.
    say the value for 3 in ages
                      ^
```

**Putting the wrong kind of value in a map**: [`map_type_errors.es`](../tests/errors/map_type_errors.es)

```
Line 6: "ages" is a map from text to numbers, so it can't hold text.
    set the value for "Ada" in ages to "old"
                                       ^^^^^
A map holds one kind of value. To keep text too, make a separate map for it.
```

**Going through a map with one name**: [`map_type_errors.es`](../tests/errors/map_type_errors.es)

```
Line 20: A map has a key and a value in each entry, so going through it takes two names.
    for each name in ages:
                     ^^^^
Name both, like "for each name and age in ages:". To go through only the keys, write "for each name in the keys of ages:".
```

**Arithmetic straight after `, or`**: [`err_map_forms.es`](../tests/ast/err_map_forms.es)

```
Line 12: "the value for "x" in ages, or 0 plus 1" could mean two things.
    say the value for "x" in ages, or 0 plus 1
        ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Use parentheses to say which: "(the value for "x" in ages, or 0) plus 1" or "the value for "x" in ages, or (0 plus 1)".
```

| Problem | Message (summary) |
|---|---|
| A map form on something that isn't a map | "I can only look up a value in a map, but this is a list of numbers." with a note showing the list form (and likewise for `has`, setting, removing, `the number of entries in`, `the keys of`, `the values of`, and `for each` with two names) |
| The value after `, or` of the wrong kind | ""ages" is a map from text to numbers, so the value after ", or" has to be a number, but this is text." |
| Keys or values of different kinds in `a map of` | "All the keys of a map have to be the same kind: the first is text, but this is a number." (and "All the values ...") |
| Keys that aren't text or numbers | "A map's keys have to be text or numbers, but this is a yes/no value." |
| The same key written twice in `a map of` | "The key "tea" is in this map twice." with "Each key has one value, so give each key once." (numbers that are the same to 12 significant digits are one key) |
| `length of` or `contains` on a map | "I can only find the length of text or a list, but this is a map from text to numbers." with "To count the entries of a map, write "the number of entries in ages"." / a note to use `has` |
| `add 5 to` a value that's a list of numbers | "This value is a list of numbers, so this could mean two things." with both ways to say it |
| Changing a fixed map | ""prices" is a constant, so it can't change." with a `let` suggestion |
| Working something out from a map in a constant | "A constant can't work things out from a map." (a fixed map written out, and its number of entries, are fine) |
| A fixed map holding lists or maps | "A fixed map can't hold lists or maps, because those could change." |
| A map whose kinds nothing decides | "I can't tell what kind of value "m" is." with a suggestion like `(a map from text to numbers)` |
| `an empty map of numbers`, `a map from number to text` | "I expected "from" here, but found "of"." / "I expected "text" or "numbers" here, but found "number"." |
| `a map of "x" to a list of 1 and 2` | "A list inside a map needs parentheses." |
| `set the value for "x" in ages, or 0 to 5` | "A value that changes can't have ", or"." |
| `remove from ages` | "Something is missing after "remove"." with both kinds of `remove` |

The full, tested set is the `map_*.err` files in [`tests/errors/`](../tests/errors/) and [`err_map_forms.err`](../tests/ast/err_map_forms.err).

### Values that can be nothing

A number, text or yes/no value that can be nothing (like `the position of X in T`) has to be checked before it's used as one.

**Using it unchecked**: [`maybe_values.es`](../tests/errors/maybe_values.es)

```
Line 4: I can't add a number to a number or nothing.
    say at plus 1.
        ^^^^^^^^^
It could be nothing. Check it first, like "if at is not nothing:".
```

The same note follows every kind mistake where the value could be nothing (comparing it, giving it to a function, using it as a position), and "Give it a name with "let" and check it first with "is not nothing"." when it isn't a name. Others:

| Problem | Message (summary) |
|---|---|
| `check which kind` with only `when it's nothing:` | "This doesn't say what to do when at isn't nothing." with "Add an "otherwise:" at the end, for when it's a number." |
| Giving back nothing from a function that gave back a value before | ""maybe" gives back a number (since line 16), but this is nothing." with "To give back nothing too, say so in the definition, like "giving back a number or nothing"." |

While the program runs, a value reached through a record's field, checked not to be nothing and set to nothing through another name before it's used, stops the program with "This was checked not to be nothing, but something has set it to nothing since then.", as for records.

### Kinds with variants

A value of a kind with variants only has the fields of its variant, so a field can only be used where the program has checked which variant it is, and `check which kind` has to say what to do for every variant.

**A field used without checking the variant**: [`kind_fields.es`](../tests/errors/kind_fields.es)

```
Line 10: "s" might not be a circle, so check which kind it is first.
    say s's radius.
        ^^^^^^^^^^
Use it inside "if s is a circle:", or inside "when it's a circle:" after "check which kind s is:".
```

**A variant left out of `check which kind`**: [`kind_match.es`](../tests/errors/kind_match.es)

```
Line 11: This doesn't say what to do when s is a rectangle or a point.
    check which kind s is:
    ^^^^^^^^^^^^^^^^^^^^^^
Add "when it's a rectangle:" and the others, or an "otherwise:" at the end.
```

| Problem | Message (summary) |
|---|---|
| A field of a value that isn't a name or a field, unchecked | "This might not be a circle, so check which kind it is first." with a note to give it a name with "let" |
| A field the checked variant doesn't have | "A rectangle doesn't have a field called "radius"." with "Only a circle has one." |
| A field no variant has | "A shape doesn't have a field called "colour"." with "Did you mean ...?" or where the kind is defined |
| Changing a field | "The fields of a shape can't change." with "Make a new one instead, like "set s to a new circle with radius ..."." |
| A field missing from `new`, given twice, unknown, or of the wrong kind | "A new circle needs a value for "radius"." / "This gives "radius" a value twice." / "A point doesn't have a field called "size"." / "The field "radius" of a circle is a number, but this is text." |
| Asking for a variant of another kind | ""n" is a number, so it's never a circle." |
| A `when` for a variant of another kind, for nothing on a value that can't be nothing, or twice | "A shape is never a cat." / "A shape is never nothing." / "There's already a "when" for a circle, on line 18." |
| `check which kind` on a value without variants | ""n" is a number, which has no variants to check." |
| A kind or variant named twice, or like a kind of record | "You already defined "shape" on line 2." / ""circle" is already a variant of "shape", on line 3." / ""dog" is already the name of a kind of record, on line 5." |
| A kind that could never be made | "Every variant of "loop" needs another loop inside it, so none could ever be made." |
| A field defined twice, a kind defined in a block | ""circle" already has a field called "radius"." / "Kinds can only be defined at the top level, not inside an "if" or a loop." |
| In a constant, or as a field's starting value | "A constant can't be a shape, because those are made while the program runs." / "A constant can't check which kind a value is, ..." / "A field that holds a shape can't have a starting value, ..." |

Mistakes in how a definition or a `check which kind` is written are parser errors, like "The variants of "shape" go on the lines below, indented.", "Say what kind of value "radius" holds, in parentheses after it.", "I don't know a variant called "circel"." (with "Did you mean "circle"?"), "The "otherwise" on line 12 has to come last.", "Each part of "check which kind" starts with "when it's ..." or "otherwise", ..." and ""make" doesn't make values; a new value is written with "new"." The full set is in [`err_kinds.err`](../tests/ast/err_kinds.err).

**An `otherwise` that can never run** is a warning, not an error: the program still compiles and runs, and the warning is shown first, marked "(a warning)": [`kind_otherwise_never_runs.es`](../tests/warnings/kind_otherwise_never_runs.es)

```
Line 12 (a warning): This "otherwise" can never run: the "when"s above it cover every kind s can be.
        otherwise:
        ^^^^^^^^^^
Take it out, or the sentences in it will never be used.
```

**A count that can never run**, because it's written with a start above its end, is a warning too: [`count_never_runs.es`](../tests/warnings/count_never_runs.es)

```
Line 2 (a warning): This count never runs: it starts above where it ends, and counting goes up.
    count from 10 to 1:
               ^^^^^^^
To count down, write "count down from 10 to 1".
```

**Kinds made of fields** (`a point is made of ...`): a field without a kind is "Say what kind of value "x" holds, in parentheses after it.", a missing separator "I expected "," or "and" before the next field, but found "column"." ([`err_kinds_made_of.err`](../tests/ast/err_kinds_made_of.err)), and changing a field is "The fields of a point can't change." ([`kinds_made_of.err`](../tests/errors/kinds_made_of.err)).

### Functions as values

A function kept as a value has a kind that says what it takes and gives back, so calling one wrongly is caught before the program runs.

**Calling something that isn't a function, or with the wrong inputs**: [`func_value_errors.es`](../tests/errors/func_value_errors.es)

```
Line 5: "n" is a number, so it can't be called.
    say call n with 3.
        ^
Only a definition, or a function made with "a function taking ...", can be called. To keep one in a name, write something like "let double be a function taking x that gives back x times 2".
```

```
Line 7: "f" needs a number for its first input, but this is text.
    say call f with "text".
                    ^^^^^^
It's a function taking a number and giving back a number.
```

**A check that a function made earlier can undo**: [`func_value_errors.es`](../tests/errors/func_value_errors.es)

```
Line 19: "maybe" was checked, but the function made on line 16 can set "maybe" whenever it's called, so the check doesn't last.
        say maybe plus 1.
            ^^^^^
Keep it in a name of its own first, like "let current maybe be maybe", and check that instead.
```

| Problem | Message (summary) |
|---|---|
| The wrong number of inputs | ""f" takes 1 value, but this gives it 2." / ""answer" doesn't take any values, but this gives it 1." with the function's kind |
| A function that gives back nothing, used as a value | ""quiet" doesn't give back a value, so it can't be used here." / "A function called here has to give back a value, but this is nothing." |
| An input named like a name around it | "You already made "total" on line 13." with "Give its input another name, like "new total"." |
| `it` in a function made in a loop | ""it" doesn't refer to anything here." with "A function made inside a loop runs later, when it's called, ..." |
| A function made inside `every` or `only` | ""a function taking ..." can't be made inside "every" or "only"." |
| `the function NAME` for methods several kinds share | "Several kinds of record have a method called "speak", so I can't tell which one this is." |
| In a constant | "A constant can't hold "a function taking ..." or "the function ...", because those are made while the program runs." |
| Inputs nothing decides | "I can't tell what kind of value "thing" is." with "Say which kind where the function is made, like "a function taking thing (a number)"." ([`func_value_undecided.es`](../tests/errors/func_value_undecided.es)) |
| A function inside a function with `to` | "Definitions with "to" can only be at the top level, not inside another definition." with "To make a function inside another one, write "let inside be a function taking ...:" ..." |

**Built-ins given the wrong kind of function**: [`func_builtin_errors.es`](../tests/errors/func_builtin_errors.es)

```
Line 8: This has to be a function taking a number and giving back a yes/no value, but it's a function taking a number and giving back a number.
    say the numbers in scores that pass double.
                                        ^^^^^^
It's given each item, and the items it gives back yes for are kept.
```

| Problem | Message (summary) |
|---|---|
| Not a function, or one that takes the wrong inputs | "This has to be a function taking a number, but it's a number." with what the built-in gives it |
| `apply` with a function that changes the kind | ""scores" is a list of numbers, so what's applied to its items has to give back a number, but this gives back text." with a note suggesting `applied to every` |
| Sorting by something that isn't a number or text | "To sort by it, it has to give back numbers or text, but this is a yes/no value." |
| A function that gives back nothing | "What's applied has to give back a value, but this is nothing." |
| Not a list | "I can only choose items from a list, but this is a number." / "I can only go through the items of a list, ..." |
| Combining an empty list without `starting from` (while the program runs) | "There's nothing to combine: the list is empty." with "Say what to start from, like ", starting from 0"." ([`err_combine_empty.es`](../tests/run/err_combine_empty.es)) |

Mistakes in how they're written are parser errors, like "I expected "to every" here, but found "to scores"." and "I expected "where", "that pass" or "combined using" here, but found "twice"."; the full set is in [`err_func_builtins.err`](../tests/ast/err_func_builtins.err).

Mistakes in how a function value or its kind is written are parser errors, like "I expected "that gives back" or a colon here, but found "gives".", "Nothing is indented under this function." and "A function can't be "or nothing"." The full set is in [`err_func_values.err`](../tests/ast/err_func_values.err).

**A function that keeps itself alive** is reported by the memory check at the end (`ES_DEBUG_MEMORY=1`, exit 70): [`err_closure_cycle.es`](../tests/memory/err_closure_cycle.es)

```
Memory check: 1 value was still alive at the end of the program, because a function keeps itself alive through a name it uses:
    the function made on line 2
A function that uses the name it's kept in (or a record, list or map that holds it) is never freed. Define it with "to" instead, or set that name to something else before the program ends.
```

### Interfaces

A kind fits an interface when it has every way, so giving one that doesn't is caught before the program runs: [`iface_errors.es`](../tests/errors/iface_errors.es)

```
Line 22: A fish can't be used as something that can speak: it has no way to speak.
    add (a new fish with name "Nemo") to the end of zoo.
        ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Give it one, like "to speak, for a fish:".
```

| Problem | Message (summary) |
|---|---|
| A method that doesn't match the way | "A cat can't be used as something that can speak: its way to greet doesn't match." with the way and the method's first line |
| A field of an interface's value | "Something that can speak has no fields, only ways." |
| A way it doesn't have | "Something that can speak has no way to jump." with the ways it has |
| A way given the wrong inputs, or used as a value when it gives back nothing | "The way to greet needs text for "name", but this is a number." / "The way to mix takes 2 values after blender, but this gives it 1." / "The way to speak doesn't give back a value, so it can't be used here." |
| Two kinds in a list written out | "All the items of a list have to be the same kind: ..." with "Both can be used as a speaker. To keep them in one list, start with "an empty list of speakers" and add each one to its end." |
| Definitions | ""speaker" already has a way to speak, on line 5." / "You already defined "speaker" on line 1." / ""dog" is already the name of a kind of record, on line 5." / "Interfaces can only be defined at the top level, not inside an "if" or a loop." ([`iface_definitions.es`](../tests/errors/iface_definitions.es)) |

Mistakes in how a definition is written are parser errors, like "Say what kind of value "name" is, in parentheses after it.", "Each line of an interface is a way, but this starts with "speak"." and "The ways of "speaker" go on the lines below, indented."; the full set is in [`err_iface.err`](../tests/ast/err_iface.err).

### Programs in several files

In a program of several files, every error names its file as well as its line. A name two used files both define is caught where it's used: [`err_names`](../tests/modules/err_names/main.es)

```
Line 4 of main.es: "double" is defined in both helpers.es and math.es.
    say double with 21.
        ^^^^^^^^^^^^^^
Say which one you mean, like "helpers's double" or "math's double".
```

| Problem | Message (summary) |
|---|---|
| A file that isn't there | "I couldn't find the file "nothere.es" in the same folder as main.es." ([`err_missing`](../tests/modules/err_missing/main.es)) |
| Files that use each other | "These files use each other in a circle: first.es uses second.es, second.es uses third.es, and third.es uses first.es." ([`err_circle`](../tests/modules/err_circle/main.es)), and "main.es can't use itself." |
| A used file that does things | "setup.es is used by another file, so it can only define things." ([`err_module_does_things`](../tests/modules/err_module_does_things/main.es)) |
| A private definition used from another file | ""tidy" is private to helpers.es, so only that file can use it." with the line of its "privately" |
| A definition of a file this one doesn't use | ""helper" is defined in tools.es, which this file doesn't use." with "Add "use tools." at the top of this file." |
| `FILE's NAME` mistakes | "This file doesn't use tools.es, so "tools's" doesn't name anything here." / "helpers.es doesn't define anything called "nope"." |
| `use` inside a block | ""use" goes at the top of a file, not inside a block." ([`err_use_in_block`](../tests/modules/err_use_in_block/main.es)) |

Mistakes in how `use` and `privately` are written are parser errors, like "The name of a file to use is one word." and "Only definitions can be kept private, but this is "say"."; the full set is in [`err_modules.err`](../tests/ast/err_modules.err).

### The standard library

Kind mistakes with the library's phrases are caught before the program runs, in the same words: [`library_errors.es`](../tests/errors/library_errors.es)

```
Line 2: "the square root of" needs a number, but this is text.
    say the square root of "nine"
                           ^^^^^^
```

| Problem | Message (summary) |
|---|---|
| A part of a date that isn't one | "A date has no "season"." with the parts it has |
| A date as a map's key | "A map's keys have to be text or numbers, but this is a date." |
| A date that can be nothing | "A date can't be "or nothing" yet." with "Keep a yes/no value next to it that says whether there is one, ..." |
| Saving something that isn't rows | "I can only save a list of maps as csv, but this is a number." / "The values saved in a csv file have to be numbers, text or yes/no values, but this is a list of numbers." |
| A path that isn't text | "The name of a file has to be text, like "notes.txt", but this is a number." |
| A library phrase in a constant | "A constant can't use "square root of", because it's worked out while the program runs." |

While the program runs, each mistake has its own message (one test each, `tests/run/err_lib_*.es`):

```
Line 3: The folder "box" isn't empty.
    delete the folder "box"
                      ^^^^^
To delete it with everything in it, add "and everything in it".
```

| Problem | Message (summary) |
|---|---|
| Math | "I can't take the square root of -4, because it's below zero." / "I can't raise -8 to the power of 0.3333333333: a number below zero can only be raised to a whole power." / "I can't raise 0 to the power of -1, because that divides by zero." / "The number of decimal places has to be a whole number from 0 to 15, but it's 2.5." |
| Random numbers | "I can't pick a random number from 6 to 1: the first number has to be the smaller one." / "A random number from 1 to 6.5 can't be worked out: both have to be whole numbers." / "I can't pick one of the items of an empty list." |
| Dates and waiting | "The number of days has to be a whole number, but it's 1.5." / "I can only wait from 0 seconds to a year, but this is -2 seconds." |
| Files and folders | "I couldn't find the file "nothere.txt"." / ""a.txt" is a file, not a folder." / "There's already a folder called "box"." with "Nothing is ever replaced this way. ..." / "I couldn't make the folder "no/such/folder": the folder it should go in doesn't exist." / "I won't delete "." and everything in it, because that's more than one folder of yours." |
| Reading csv | "In a.csv, line 3 has 3 values, but the header has 2." / "In a.csv, the quote on line 2 never ends." / "In a.csv, line 2 has more after a closing quote." / "The header of a.csv has the column "name" twice." / "a.csv is empty, so it has no header row." |
| Saving csv | "Row 2 of the list has 3 values, but the first row has 2." / "Row 2 of the list has no value for "b", but the first row does." |

Mistakes in how the sentences are written are parser errors, like "Say how long to wait: seconds, milliseconds or minutes.", "Only a folder has everything in it." and ""to the power of" twice could mean two things."; the full set is in [`err_library.err`](../tests/ast/err_library.err).

### System tools and the keyboard

Kind mistakes are caught before the program runs: [`system_errors.es`](../tests/errors/system_errors.es)

```
Line 2: The position of a word has to be a number, but this is text.
    let first be word "one" given to the program
                      ^^^^^
```

| Problem | Message (summary) |
|---|---|
| A word for a program that isn't one | "Each word given to a program has to be text, a number or a list of text, but this is a list of numbers." |
| A program that isn't text | "The program to run has to be text, like "git", but this is a number." |
| `worked` on something else | "Only a program result can have worked, but this is a number." with "Run a program first, like ..." |
| A setting's name or a code of the wrong kind | "The name of a setting has to be text, like "HOME", but this is a number." / "The code has to be a number, but this is text." |
| A program's own `program result` kind ([test](../tests/errors/system_own_program_result.es)) | "This program defines its own kind called "program result", so running a program can't make one." |

While the program runs (one test each, `tests/run/err_sys_*.es` and `err_keys_ran_out.es`):

```
Line 1: I couldn't find a program called "easyscript-no-such-program".
    run "easyscript-no-such-program" with "x".
    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Check its name, and that it's installed. A program in this folder is run with its path, like "./tool".
```

| Problem | Message (summary) |
|---|---|
| A word that wasn't given | "There's no word 1: the program wasn't given any words." / "There's no word 2: the program was given 1 word." with "Check how many there are first, like ..." / "The position of a word has to be a whole number, but it's 1.5." |
| Running a program | "I couldn't run "./notes.txt": it isn't a program I'm allowed to run." / "The name of the program to run is empty." / ""false" didn't work: it finished with code 1." with "To carry on when it doesn't work, write "... and call the result r", then check "if r worked:"." |
| A code that isn't one | "The code has to be a whole number from 0 to 255, but it's 256." |
| No keys left | "There's no key to wait for: the keys sent to the program have run out." |

Parser errors, like "Something is missing after "word"." with "Say which word, like "word 1 given to the program"." and "I expected "saying it failed" here, but found "saying it worked".", are in [`err_system.err`](../tests/ast/err_system.err).

### While the program runs

Runtime errors look like compile errors: `Line N:` and what went wrong, the line with carets under the part that failed, and sometimes a hint. Anything the program printed before the error stays printed. Only mistakes that kinds can't catch are left for this.

**Dividing by zero**: [`err_divide_by_zero.es`](../examples/programs/err_divide_by_zero.es)

```
Line 3: You divided by zero.
    say 12 divided by people
        ^^^^^^^^^^^^^^^^^^^^
```

**Inside functions**, the error also shows the chain of calls that led to it, innermost first: the function being worked out and the values it was given (numbers, yes/no, and text, cut short after 30 characters; records as their fields, cut short after 60 characters; lists aren't shown), then each call back to the main program, whose line is shown: [`err_stack_nested.es`](../tests/run/err_stack_nested.es)

```
Line 4: You divided by zero.
        give back marks added together divided by count
                  ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
while working out "average" (label was "x", loud was no)
called from "report" on line 6 (title was "Quarterly figures for the whol...", region was "North")
called from "start" on line 8
called from line 10: start
```

A function calling itself again and again from the same place is shown once, with a count: [`err_stack_recursion.es`](../tests/run/err_stack_recursion.es)

```
Line 3: You divided by zero.
        if n is 0, say 1 divided by n
                       ^^^^^^^^^^^^^^
while working out "countdown" (n was 0)
called from "countdown" on line 4 (n was 1)
...it called itself 4,998 more times...
called from line 5: countdown with 4999
```

**A map key that isn't there**: [`err_map_missing_key.es`](../tests/run/err_map_missing_key.es)

```
Line 3: There's no value for "Bob" in the map.
    say the value for "Bob" in ages
        ^^^^^^^^^^^^^^^^^^^^^^^^^^^
To check first, use "has". To use another value when it's missing, add ", or" and that value after it.
```

**A list item that isn't there**: [`err_list_position_zero.es`](../tests/run/err_list_position_zero.es)

```
Line 2: There's no item 0 in the list: it has 1 item.
    say item 0 of names
        ^^^^^^^^^^^^^^^
Positions start at 1: the first item is item 1.
```

| Problem | Message (summary) |
|---|---|
| Dividing or `mod` by zero | "You divided by zero." |
| A function that keeps calling itself | "Definitions are using each other too deeply (more than 10000 inside each other)." with a hint to check that it stops |
| A count step that's zero or negative | "The step of a count has to be more than zero, but it's -1." with a hint that counting goes up by itself (or down, with `count down`) |
| `repeat N times` with N that isn't a whole number of zero or more | "The number of times has to be a whole number, but it's 2.5." (or "can't be negative") |
| `as a number` on text that isn't a number | "I can't turn "12abc" into a number." (or "I can't turn empty text into a number."), with the hint "A number is digits, with a "-" before them if it's negative, a "." for decimals and an "e" for powers of ten, like 42, -7, 3.5 or 1e5." Long text is cut short after 60 characters |
| A file that can't be read or written | "I couldn't read the file "nope.txt": it doesn't exist." |
| A function that gives back a value but reaches its end without `give back` | ""sign" ended without giving anything back." with a hint |
| A list item that isn't there | "There's no item 4 in the list: it has 3 items." (or "it's empty"); item 0 adds "Positions start at 1: the first item is item 1." |
| A position that isn't a whole number | "The position of an item has to be a whole number, but it's 1.5." |
| The first, last, biggest or smallest item of an empty list | "There's no first item: the list is empty." |
| Inserting past the end | "I can't insert at position 6: the position has to be from 1 to 5 (the list has 4 items)." |
| A range that goes backwards | "I can't take items 3 to 2: the first position is after the last one." |
| A number of items that's negative or not whole | "The number of items can't be negative, but it's -1." / "... has to be a whole number, but it's 1.5." |
| A character that isn't there | "There's no character 5 in the text: it has 4 characters." (or "it's empty"); character 0 adds "Positions start at 1: the first character is character 1." |
| A character position that isn't a whole number | "The position of a character has to be a whole number, but it's 1.5." |
| The first or last character of empty text (or taking it away with `without`) | "There's no first character: the text is empty." |
| A range of characters that goes backwards | "I can't take characters 3 to 2: the first position is after the last one." |
| A number of characters that's negative or not whole | "The number of characters can't be negative, but it's -2." / "... has to be a whole number, but it's 1.5." |
| Splitting by empty text | "I can't split text by empty text." with a hint to use `the characters of` |
| Replacing empty text | "I can't replace empty text: there's nothing to look for." |
| The code of anything but one character | "The code of a character needs exactly one character, but this text has 2." (or "... but this text is empty.") |
| A character code that isn't one | "There's no character with code 55296." with the hint "Character codes are whole numbers from 1 to 1114111, like 65 for "A"." |
| A map key that isn't there | "There's no value for "Bob" in the map." (keys cut short after 60 characters); changing its value adds "It has to be in the map before it can change. To put it there, write "set the value for ... to ..."." |
| Removing a key that isn't there | "I can't remove "Bob": the map has no value for it." with "To check first, use "has"." |
| A record checked not to be nothing, then set to nothing through another name before it's used | "This was checked not to be nothing, but something has set it to nothing since then." with the hint "Another name for the same record can change it. Check it again just before using it." |
| A record's field checked to be a variant, then set to another one through another name before it's used | "This was checked to be a circle, but something has set it to a point since then." with the same hint |
| The average or median of an empty list | "There's nothing to average: the list is empty." / "There's no middle number: the list is empty." |
| The one with the highest (or lowest) of an empty list | "There's nothing to choose from: the list is empty." with the hint "Check that the list isn't empty first, like "if dogs is not empty:"." |
| The top N with N not whole or below 0 | "How many to take has to be a whole number, 0 or more, but it's 1.5." |
| A table's column that isn't in the file | "sales.csv has no column called "amout"." with "Did you mean "amount"?" when one is close |
| A table's value of the wrong kind | "In sales.csv, line 3, "amount" is "lots", which isn't a number." (or "yes or no", or "a date like 2026-10-31") |
| An event's "every N seconds" with N 0 or less | "How often has to be more than 0 seconds, but it's 0." |
| A field given a value its rule doesn't allow | "An age must be between 0 and 150, but this one is 230." / "A person's email must contain "@", but this one is "bob.example.com"." / for other kinds, "..., but this one doesn't follow it." |

Parts of an expression are worked out left to right, so when two parts would both fail, the error is always about the first one. The tested messages are the `err_*.err` files in [`tests/run/`](../tests/run/).

### Talking to it, part 4

```
Line 1: Say what to do if it fails.
    try to read file "x.txt" and call it notes.
    ^^^
Write it like "try to read file "notes.txt" and call it notes. if that fails, say "No notes yet."".
```

```
Line 2: "try to" only works with a file or a folder.
    try to say "hi". if that fails, say the reason.
           ^^^^^^^^
Use it to read, write, make or delete one, like "try to read file "notes.txt" and call it notes."
```

```
Line 1: A constant can't be a number written as money, a percent or with commas.
    keep price as 5 as money.
                  ^^^^^^^^^^
Make it with "let", or format the number where it's used.
```

```
Line 1: "\u12" isn't a character I know.
    say "\u{110000} \u12 \u{}".
                    ^^^^
Write the character's code in hex, like \u{e9} for "é" or \u{1F600}. Codes go from 1 to 10FFFF.
```

While the program runs, `ask for a number` with no more answers stops with:

```
Line 1: There's no answer to read: the input has run out.
```

## Coming soon

Nothing is planned for error messages right now; new features bring their own.
