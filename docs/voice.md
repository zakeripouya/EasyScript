# How EasyScript talks

Every message EasyScript shows (errors, warnings, notes, the shell, and later the notebook's replies) follows this page, so it sounds like one helpful person. The [English pass](english-pass.md) is where these rules come from; `tests/tools/plain_words.py` checks them (see "Checks" below).

## The voice

- **First person, about the program in front of you.** "I don't know anything called "totl"." "I can't add text to a number." EasyScript says what *it* can't do, never that the reader is wrong.
- **Short sentences, everyday words.** One idea per sentence. A message a 12-year-old can read: grade 8 or below.
- **Say what happened, then what to type.** The message says what's wrong; the note under it gives one concrete fix, written as a sentence that works in the reader's own program ("Did you mean "total"? You made it on line 1.", "Write it like "let total be 0"").
- **No jargon.** Programmer words have plain replacements; see the glossary. A word EasyScript uses as part of its own language (`a function taking x that gives back ...`, `the function double`) can appear when it names that phrase.
- **Friendly, not chatty.** No "Oops!", no exclamation marks, no apologies, no jokes. No blame ("you forgot"): say what's missing ("Something is missing after "add"").
- **Exact.** The line, the spot (carets), the name as the reader wrote it. Never guess: when two meanings are possible, say both and ask which.
- **Grown-up plain.** Plain doesn't mean childish: no baby talk, no "magic".

## The shape of each kind of reply

| Reply | Shape | Example |
|---|---|---|
| Error | `Line N: ` + what's wrong, in one sentence. Then the source line and carets. Then at most two notes, each a fix or an example | `Line 2: I don't know anything called "totl".` / `Did you mean "total"? You made it on line 1.` |
| Warning | `Line N (a warning): ` + what can never matter, then what to do | `This "otherwise" can never run: ...` / `Take it out, ...` |
| Two meanings | `"X" could mean two things.` + both readings, written out | `Use parentheses to say which: "(a of 1) plus 2" or "a of (1 plus 2)".` |
| A question back (notebook, later) | One short question with the choices | `Do you mean amount or tip?` |
| An answer (notebook, later) | The value, or one sentence for a small result | `Your best month was September, with $6,210.75.` |
| A suggestion | `Did you mean ...?`, `Write it like ...`, `Try ...` | `Write it like "for each score in scores:".` |

## Glossary: one word for each idea

Messages and beginner docs use the word on the left and never the ones on the right.

| Idea | Say | Never say |
|---|---|---|
| What sort of value something is | kind ("a number", "text", "a list of dogs") | type |
| A name that holds a value (`let total be 0`) | name, or the name itself ("total") | variable |
| Something defined with `to` (`to greet with someone:`) | its name ("greet"), "how to greet", a definition | function (except as below) |
| A function kept as a value (`a function taking x ...`, `the function double`) | the EasyScript words themselves: "a function taking a number", "the function made on line 4" | lambda, closure |
| What a definition is given | input | parameter, argument |
| What it hands back | give back, gives back | return (except quoting `return`) |
| Text | text | string |
| Yes or no | yes or no, a yes/no value | boolean, bool |
| Numbers | a number, a whole number | integer, float, double |
| Where an item is in a list | position | index |
| No value | nothing | null, nil, none |
| When something goes wrong while running | "the program stops" | exception, crash, throw |
| Before the program runs | "before the program runs" | compile time, syntax error |
| While it runs | "while the program runs" | runtime |
| A record or a value of a kind | record, value | object, instance |
| Going through a list | go through, for each | iterate, loop over |
| Making a name | make ("You make "x" later") | declare, initialize, define a variable |
| Where a name can be used | "only exists inside that block" | scope |
| A list | list | array |
| A lookup table | map | dictionary, hash |

The banned words (from the English pass's design rules) are: variable, function, parameter, argument, return, string, boolean, integer, float, array, index, null, exception, syntax, compile, runtime, type, object, instance, iterate, initialize, declare, scope. In messages they're allowed only inside quotes (EasyScript the reader can type, like `"return"`) or as EasyScript's own phrases for functions kept as values: "a function taking", "a function giving back", "a function made ...", "functions taking", and a kind named just "a function".

## Checks

`tests/tools/plain_words.py` runs at the end of `make test` (`audit/plain-words`):

- **Banned words:** no message or note in an expected `.err` file, and no message string in `src/` or `runtime/`, uses a banned word outside quotes and the allowed phrases above. The build fails.
- **Reading level:** each error (its message and notes together, without the EasyScript in quotes) has a Flesch-Kincaid grade of 8 or below. The build fails.
- **Beginner docs:** the README, the language guide and this page get a reading-level report, which doesn't fail the build.
