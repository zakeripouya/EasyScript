# Design principles

The rules every EasyScript phrasing follows. New syntax follows them, or this page says why it's an exception. They describe the language as it is today; [vocabulary.md](vocabulary.md) lists every phrase.

## 1. One sentence, one meaning

- **A sentence reads as plain English and means exactly one thing.** The parser never guesses. When a sentence could mean two things, it's an error that shows both readings, written out (`(double of 21) plus 1` or `double of (21 plus 1)`).
- **Words are only special where they're used.** A statement word (`say`, `add`, `count`) is special only at the start of a sentence; a phrase word (`item`, `value`, `keys`, `length`) only in its phrase. Everywhere else it can be a name. Only a short list of operator and value words (`and`, `or`, `is`, `plus`, `yes`, `nothing`, `it`, ...) can never be names.
- **Filler words don't change meaning.** `the`, `a` and `an` are ignored everywhere outside text, so `set the total to 0` is `set total to 0`.
- **Names are words, as you'd say them.** A name can be several words (`late fee`, `busiest artist`); the longest name the program knows wins. Names never need underscores or capitals.

## 2. Statements and values

- **A statement starts with a verb and does something:** `say X`, `set X to Y`, `sort L`, `remove K from M`. Statements change things in place or have effects.
- **A value is a noun phrase or a value with a describing word after it:** `the length of T`, `the keys of M`, `L reversed`, `T in uppercase`. Values never change anything; they make something new.
- **A statement that changes something in place has a value form that gives a changed copy, where both are useful:** `remove the first item from L` and `L without the first item`; `add X to the end of L` and `L followed by M`. The value form uses the past participle (`reversed`, `added together`) or a describing phrase (`without ...`, `with X replaced by Y`).
- **Text never changes,** so text tools are only values; `set T to T followed by X` is how text grows.
- **Expressions can't stand alone as sentences,** so a sentence that starts with a value is always an error, never silently dropped.

## 3. Inputs

- **A function's inputs come after a connector word,** the way English introduces what a verb acts on: `with` (the usual one), `of`, `using` and `to`. `to area with width and height:`, `to add tax to amount:`.
- **A call uses the same connector as the definition,** or `with`, `of` and `using`, which work for every function: `add tax to 100`, `area with 3 and 4`.
- **Inputs are separated by `and`** (and by commas after `using`). Each input is a single value; anything longer goes in parentheses.
- **The same shape everywhere:** records take their fields the same way (`a new dog with name "Rex" and age 3`), and methods put the record first (`rename rex with "Max"`).
- **A connector only introduces inputs where it can't mean anything else.** `for` before a kind of record makes a method (`to bark for a dog:`); `to` only starts a call's inputs for a function defined with it.

## 4. `of` and `in`

- **`of` takes a part or a property of one thing:** `the length of T`, `item 2 of L`, `the keys of M`, `the code of C`, `the words of T`, `the biggest of L`.
- **`in` looks inside a container to find or count something:** `the number of items in L`, `the position of X in T`, `the value for K in M`, `for each X in L`.
- **Where both read naturally, both work:** `item 2 of L` and `item 2 in L`, `the first item of L` and `the first item in L`.

## 5. Optional values and "not found"

- **Asking for something that must be there is an error when it isn't,** with a message that says what was there instead: `item 9 of L` on a list of 3 items, `the value for "Bob" in ages` with no Bob.
- **Every such lookup has a way to ask first:** `L contains X`, `M has K`, `the number of items in L`.
- **A value that might be missing says so in its kind:** `a dog or nothing`. It must be checked (`if X is not nothing:`) before it's used, and the compiler enforces that.
- **A default is written where the value is used:** `the value for K in M, or D`. D is only worked out when it's needed.
- **New "not found" results use `or nothing`,** not a made-up value like 0 or -1 (see the exception for `the position of` below).

## 6. Errors

- **Every error is a plain first-person English sentence** that says what went wrong, at a line and column, with carets under the exact part ("I don't know anything called "totl"."). No codes, no "Error:" prefixes, no jargon.
- **Notes give a concrete fix,** written as a sentence that would work in the user's program: "Did you mean "total"? You made it on line 1.", "Write "(double of 21) plus 1" or "double of (21 plus 1)"."
- **Suggested names are natural English,** never underscores or invented spellings ("current score", not "each_score").
- **One mistake, one message.** After an error the rest of that sentence is skipped, and a value whose kind is already wrong doesn't cause more errors.
- **Runtime errors look like compile errors,** plus the chain of calls that led there with the values each was given.

## 7. Equality

- **`is` compares values of the same kind;** comparing different kinds is an error before the program runs.
- **Numbers are equal when they're within a relative 1e-12 of each other,** so `0.1 plus 0.2 is 0.3` is yes. The orderings agree: close numbers are neither above nor below each other.
- **Text is equal when it's the same characters,** exactly (capitals matter).
- **Lists are equal when their items are, in order. Maps are equal when they have the same keys with equal values, in any order.**
- **Records are equal only when they're the same record:** two dogs with the same fields are still two dogs.
- **Everything that searches uses the same equality:** `contains`, `the position of`, `has` and map keys, with the exception for map number keys below.

## 8. Sharing and copying

- **Lists, maps and records are shared:** `let b be a` gives the same thing another name, and a function given one can change it.
- **`a copy of X` copies X and the lists and maps inside it, but shares the records and text inside it.** Lists and maps in a list or map belong to it (they can't be shared by two places in a way you could see without records), so copying them keeps the copy independent. Records have an identity (`is` asks whether two are the same record) and can point to each other, so a copy keeps pointing to the same records. Text never changes, so sharing it can't be seen.
- **Fixed values (`keep`) never change,** and wherever one could be changed it's copied first.

## Known exceptions

Each exception exists because following the rule would cost more than it gives. New exceptions are added here with their reason.

- **`is a letter`, `is a digit` and `is a space` need their `a`.** Filler words are ignored everywhere else, but `x is letter` already means "x equals the variable `letter`". Keeping the `a` here lets `letter`, `digit` and `space` stay ordinary names while `x is a letter` still reads naturally. The parser looks at the dropped word only in this one place.
- **`a`, `an` and `the` can't be names.** Because they're filler, `let a be 5` would read as `let be 5`. The error explains this and suggests a name that says what it holds. Other single letters (`b`, `x`, `n`) are ordinary names.
- **`the position of X in T` gives 0 when X isn't there.** Positions start at 1, so 0 can't be a real position, and it was the simplest result before kinds could say "or nothing" outside records. It breaks the "not found" rule above and will be replaced by `a number or nothing` when values other than records can be nothing (a later step).
- **Map number keys are compared by rounding, not by `is`.** Keys are the same when they print the same at 15 significant digits, so `0.1 plus 0.2` finds the value kept for `0.3`, as `is` would expect. A hash table needs every key to have one exact form, and "within 1e-12" isn't transitive (a can be close to b, and b to c, without a being close to c), so the rounding can't be exactly `is`: two numbers that differ only past the 12th digit are equal by `is` but can be different keys if they round differently.
- **`times` is multiplication only when a value follows it,** so `repeat 3 times:` keeps its `times`.
- **`and` joins text as well as combining yes/no values.** `"Hello, " and name` is natural English; the compiler decides which `and` it is from the kinds on both sides, and reports an error when the kinds don't settle it (`and` between two numbers).
- **`add X to L` is an error when L is a list of numbers,** because it could mean adding X to every item or adding it to the end. For other lists it adds to the end. The error shows both phrasings.
- **`every other item` and `every second item` are different:** `every other` starts with the first item (1, 3, 5), `every second` with the second (2, 4, 6), as in everyday English.
- **A word ending in `'s` is always a possessive,** so a name can't end in `'s`, and `it's` means "its" (the field of `it`), not "it is".

## Open questions

Phrasings that may not follow these rules, left as they are until they're decided:

- **Function inputs only take `with`, `of`, `using` and `to`.** Natural connectors like `for` (`to pause for seconds:`) and `from` (`to busiest artist from names:`) aren't accepted yet.
- **`sort L` has no value form.** There's no `L sorted` to get a sorted copy without changing L.
