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
- **A statement that changes something in place has a value form that gives a changed copy, where both are useful:** `sort L` and `L sorted`; `remove item N from L` and `L without item N` (also `the first item`/`the last item`); `remove K from M` and `M without K`; `add X to the end of L` and `L followed by M`. The value form uses the past participle (`sorted`, `reversed`, `added together`) or a describing phrase (`without ...`, `with X replaced by Y`).
- **Built-ins that take a function follow the same pairs:** `apply F to every X in L` and `F applied to every X in L`; `keep only the Xs in L where ...` and `the Xs in L where ...`; `sort L by F` and `L sorted by F`. Combining (`L combined using F`) only has a value form, because it gives back one value rather than changing the list.
- **Text never changes,** so text tools are only values; `set T to T followed by X` is how text grows.
- **Expressions can't stand alone as sentences,** so a sentence that starts with a value is always an error, never silently dropped.
- **A block belongs to the sentence whose line ends with its colon,** and ends that sentence. A value can only end with a block (`let shout be a function taking word:`) when it's the last thing in the sentence.

## 3. Inputs

- **A function's inputs come after a connector word,** the way English introduces what a verb acts on: `with` (the usual one), `of`, `using`, `to`, `for`, `from` and `in`. `to area with width and height:`, `to add tax to amount:`, `to pause for seconds:`, `to busiest artist from names:`, `to count vowels in word:`.
- **A call uses the same connector as the definition,** or `with`, `of` and `using`, which work for every function: `add tax to 100`, `pause for 2`, `busiest artist from names`, `area with 3 and 4`. Another connector is an error that names the ones it takes.
- **Inputs are separated by `and`** (and by commas after `using`). Each input is a single value; anything longer goes in parentheses.
- **A function kept as a value is called the same way,** with `call`: `call handler with "Ann"`, `call item 1 of steps with 3`. In a value, `handler with "Ann"` works too when it's a name, like any call.
- **An interface asks for what a value can do, not what it says it is:** a kind fits `something that can speak` when it has the methods, and never has to name the interface. Its ways are written like the first lines of methods, so fitting is comparing the two.
- **The same shape everywhere:** records take their fields the same way (`a new dog with name "Rex" and age 3`), values of kinds with variants too (`a new circle with radius 3`), and methods put the record first (`rename rex with "Max"`).
- **A connector only introduces inputs where it can't mean anything else.** `for` before a kind of record makes a method (`to bark for a dog:`), and `for` always ends a function's name. `to`, `for`, `from` and `in` only start a call's inputs for a function defined with them, because elsewhere they belong to the sentence around the call (`subtract X from Y`, `the value for K in M`).

## 4. `of` and `in`

- **`of` takes a part or a property of one thing:** `the length of T`, `item 2 of L`, `the keys of M`, `the code of C`, `the words of T`, `the biggest of L`.
- **`in` looks inside a container to find or count something:** `the number of items in L`, `the position of X in T`, `the value for K in M`, `for each X in L`.
- **Where both read naturally, both work:** `item 2 of L` and `item 2 in L`, `the first item of L` and `the first item in L`.

## 5. Optional values and "not found"

- **Asking for something that must be there is an error when it isn't,** with a message that says what was there instead: `item 9 of L` on a list of 3 items, `the value for "Bob" in ages` with no Bob.
- **Every such lookup has a way to ask first:** `L contains X`, `M has K`, `the number of items in L`.
- **A value that might be missing says so in its kind:** `a dog or nothing`, `a number or nothing`, `text or nothing` (anything but a list or map, which can be empty instead). It must be checked (`if X is not nothing:`) before it's used, and the compiler enforces that.
- **A value that's one of several things says so in its kind,** a kind with variants (`a shape is one of:`), and its fields are only used where the program has checked which variant it is (`if s is a circle:`, `check which kind s is:`). The same checks narrow values that can be nothing and values with variants. `check which kind` must cover every variant (or say `otherwise`), so adding a variant shows every place that has to handle it.
- **A default is written where the value is used:** `the value for K in M, or D`. D is only worked out when it's needed.
- **"Not found" results use `or nothing`,** not a made-up value like 0 or -1: `the position of X in T` is `a number or nothing`.

## 6. Errors

- **Every error is a plain first-person English sentence** that says what went wrong, at a line and column, with carets under the exact part ("I don't know anything called "totl"."). No codes, no "Error:" prefixes, no jargon.
- **Notes give a concrete fix,** written as a sentence that would work in the user's program: "Did you mean "total"? You made it on line 1.", "Write "(double of 21) plus 1" or "double of (21 plus 1)"."
- **Suggested names are natural English,** never underscores or invented spellings ("current score", not "each_score").
- **One mistake, one message.** After an error the rest of that sentence is skipped, and a value whose kind is already wrong doesn't cause more errors.
- **Runtime errors look like compile errors,** plus the chain of calls that led there with the values each was given.
- **Something allowed that can never matter is a warning,** not an error: an `otherwise` after a `when` for every variant can never run. The warning is shown like an error, marked "(a warning)", and the program still runs.

## 7. Equality

- **`is` compares values of the same kind;** comparing different kinds is an error before the program runs.
- **Numbers are equal when they're within a relative 1e-12 of each other,** so `0.1 plus 0.2 is 0.3` is yes. The orderings agree: close numbers are neither above nor below each other. Printing shows at most 10 significant digits, so rounding errors don't show either.
- **Text is equal when it's the same characters,** exactly (capitals matter).
- **Lists are equal when their items are, in order. Maps are equal when they have the same keys with equal values, in any order.**
- **Records are equal only when they're the same record:** two dogs with the same fields are still two dogs. **Functions** likewise: two function values are equal only when they're the same one.
- **Values of kinds with variants are equal when they're the same variant with equal fields:** `s is a new point`. They never change, so they have no identity to compare.
- **Everything that searches uses the same equality:** `contains`, `the position of`, `has` and map keys, with the exception for map number keys below.

## 8. Sharing and copying

- **Lists, maps and records are shared:** `let b be a` gives the same thing another name, and a function given one can change it.
- **`a copy of X` copies X and the lists and maps inside it, but shares the records and text inside it.** Lists and maps in a list or map belong to it (they can't be shared by two places in a way you could see without records), so copying them keeps the copy independent. Records have an identity (`is` asks whether two are the same record) and can point to each other, so a copy keeps pointing to the same records. Text never changes, so sharing it can't be seen.
- **Fixed values (`keep`) never change,** and wherever one could be changed it's copied first.
- **A function made inside a value shares the names it uses from around it,** never a copy: a change made inside it is seen outside, and the other way round, even after the function around it has ended. Each `let` makes a new name, so functions made in different rounds of a loop don't share one.
- **Values of kinds with variants never change** once made: a different value is made instead (`set s to a new circle with radius 4`). Like text, they're shared, and sharing them can't be seen, except through a list or map in a field (which can change); `a copy of s` gives the copy its own lists and maps.

## Rules for new phrases

Every phrase added from now on follows these too. They aren't audited by tests yet; the [English pass](english-pass.md) will add the audits and review the existing phrases against them.

- **One meaning per word.** Synonyms are fine (`say` and `print`). A word with two meanings needs a documented exception below. Current overloads to review in the English pass: **`and`** (between values or conditions: both, or the items of a list or the inputs of a call; between two sentences: one after the other; it no longer joins text) and **`is`** (compares values, and starts `is a circle`, `is empty`, `is anything with`, ...).
- **Keywords come from common everyday English.** No idioms unless a plain form is also accepted: `every other item` is an idiom, so `every 2nd item` works too.
- **Messages beginners see stay at a grade 6 to 8 reading level,** and never use these words: variable, function, parameter, argument, return, string, boolean, integer, float, array, index, null, exception, syntax, compile, runtime, type, object, instance, iterate, initialize, declare, scope. Docs written for programmers ([architecture](architecture.md), [memory](memory.md), comments in the compiler) may use them.
- **Every high-level phrase has an exact meaning in terms of the core language** (loops, `if`s, lists, maps), written down in its docs, so a programmer can trust it and write it out by hand. For example, `the numbers in scores where it is above 50` is a new empty list, then `for each` item of `scores` (as it was when it started), adding the item when the condition is yes.
- **High-level operations stay their own syntax-tree nodes:** sorting, top N, grouping, filtering, counting. The parser never rewrites them into loops, so a later optimizer can see what was meant and pick how to do it.
- **Each pattern starts with its own trigger words,** so two patterns never overlap and the parser never has to try one and fall back to another.
- **The success test,** written down so it can be checked: someone who has never written code builds a working quiz in 30 minutes, with no help.

## Known exceptions

Each exception exists because following the rule would cost more than it gives. New exceptions are added here with their reason.

- **`is a letter`, `is a digit` and `is a space` need their `a`, and so does `is a circle` (any variant, with `a` or `an`).** Filler words are ignored everywhere else, but `x is letter` already means "x equals the variable `letter`". Keeping the `a` here lets `letter`, `digit`, `space` and variant names stay ordinary names while `x is a letter` and `s is a circle` still read naturally. The parser looks at the dropped word only in these places.
- **`a`, `an` and `the` can't be names.** Because they're filler, `let a be 5` would read as `let be 5`. The error explains this and suggests a name that says what it holds. Other single letters (`b`, `x`, `n`) are ordinary names.
- **Map number keys are compared by rounding, not by `is`.** Keys are the same when they're the same to 12 significant digits, so `0.1 plus 0.2` finds the value kept for `0.3`, as `is` (within a relative 1e-12) would expect. A hash table needs every key to have one exact form, and "within 1e-12" isn't transitive (a can be close to b, and b to c, without a being close to c), so rounding can only come close to `is`: right at a rounding boundary, two numbers `is` calls equal can still be different keys. Printing rounds further, to 10 digits, so two different keys can print alike (`1` and `1.00000000001`).
- **`times` is multiplication only when a value follows it,** so `repeat 3 times:` keeps its `times`.
- **Commas in `say` join pieces, but only at the top level of the sentence.** A comma inside a value keeps its own meaning (list items, inputs after `using`, fields of `new`), and so does a comma followed by `or`, `sorted`, `as`, `rounded` or `starting`, so `say the value for K in M, or 0` is one piece. Commas are how people write "Hello, Ann", and the cases where a comma already means something are the ones a reader expects.
- **`has` defines a kind of record at the start of a line, and tests a map elsewhere.** `a dog has a name and an age.` (a sentence) states what a dog has; `ages has "Bob"` (a value) asks whether a map has a key. Both mean "has" in English. A sentence can't start with a value, so the two never meet; the definition's first word can't start sentences (`if ages has ...` stays an `if`). `some` before a field's name is left out there, like `a`.
- **`and` means "both" between values and "and then" between sentences, decided by position.** A sentence word followed by the start of a value can't continue a value, so `add 5 to total and say total` is two sentences while `if ready and count is 3` is one condition. A function's name after `and` reads as a value either way, so the second sentence needs `and then`.
- **Brackets in text hold values** (`"Hello, [name]!"`), so a bracket in the text itself is written twice (`[[`, `]]`). Text in quotes can't go inside the brackets, because its quote would end the text.
- **`add X to L` is an error when L is a list of numbers,** because it could mean adding X to every item or adding it to the end. For other lists it adds to the end. The error shows both phrasings.
- **`every other item` and `every second item` are different:** `every other` starts with the first item (1, 3, 5), `every second` with the second (2, 4, 6), as in everyday English.
- **`ask` and `read file` both take `and call the answer NAME` and `and call it NAME`.** Each sentence reads most naturally with one of them, but accepting both means nobody has to remember which goes where.
- **`L followed by M` needs two lists.** Adding one item as a value (`L followed by X`) would read naturally, but `followed by` with a list and text already joins text (`"Slots: " followed by slots`), so its meaning would depend on kinds in a way that's easy to misread. Write `L followed by (a list of X)`.
- **`a copy of R` is shallow for records** (the records inside are shared) while lists and maps inside a list or map are copied. Records have an identity and can point to each other, so copying them silently would change what `is` says and could copy a whole web of records; a separate deep copy phrase is an [idea](ideas.md#a-deep-copy).
- **Maps have `M without K` but no `M with K as X`** yet: setting a value as a value form waits until its wording is settled. Copy the map and `set the value for K in ...` the copy.
- **A word ending in `'s` is always a possessive,** so a name can't end in `'s`, and `it's` means "its" (the field of `it`), not "it is", except right after `when` in `check which kind`, where `when it's a circle:` reads as English does ("when it is a circle"); `when it is a circle:` works too. `its radius` (without the apostrophe) is also the field of `it`, but only before a field's name, so `its` stays an ordinary name elsewhere.

- **The words naming the items in `every number in scores` and `the numbers in scores where ...` aren't names.** They're there so the sentence reads as English; inside, the item is `it`. Making them names would mean `the scores in scores` couldn't be written, and a condition like `where it is above 50` already says which item it means.
- **An interface's name is every word before `is anything with`,** stop words included, so `something that can speak` reads as English. Other names can't contain words like `that`, because there they'd end a name; here the sentence's own words mark where the name ends. Its plural turns `something`/`anything` into `things` (`a list of things that can speak`).
- **`call F` is a value as well as a sentence** (`let n be call next`), although values don't start with a verb. A name alone is the function itself (`let f be next` keeps it), so a function with no inputs needs a word that calls it, and `call` is the one sentences already use.

- **A file's own definitions come before those of the files it uses.** Two used files defining one name is an error only where it's used (with a note suggesting `helpers's double`), but a file can always define a name a used file also defines: otherwise adding a definition to a shared file could break every file that uses it.
- **Kinds of record, kinds with variants and interfaces share one set of names across files,** unlike functions and constants. A kind's name is used inside other phrases (`a new circle`, `a list of dogs`, `for a dog`) where `helpers's` couldn't go, so two kinds with one name couldn't be told apart.
- **`FILE's NAME` uses the possessive for a file,** so a file's name (one word) is special before `'s` in every file of the program, even where it's also a name: the file wins.

- **`pick one of L` and `roll a die` are values that start with a verb,** because that's how people say them; `a random item of L` and `a random number from 1 to 6` are the noun forms, and work too.
- **`pi`, `today`, `tomorrow` and `yesterday` are names, not reserved words:** they mean the library's value only where the program hasn't made the name itself, so programs written before the library still work. Every other library phrase starts with its own words (`the square root of`, `a random number from`, `file F exists`).
- **File and folder sentences say `file` or `folder`:** `delete the file F`, `the lines of file F`, `add X to the end of file F`. Without the word, `the lines of F` and `add X to the end of F` would mean the text tool or the list sentence, depending on what F is.

- **`run` and `complain` start sentences only where the program hasn't defined a function with that name,** like the file sentences, so programs that already use them as names keep working. `run` is also the compiler's own command (`easyscript run`), but inside a program it only ever means running another program.
- **"Given" and "sent" are different words on purpose:** `the words given to the program` are what it was started with (`./app Paris 19`), and `the text sent to the program` is what's piped into it (`./app < notes.txt`). Each phrase starts with its own words (`words given`, `text sent`, `lines sent`, `word N given`), and `word` is only special before a position and `given to the program`, so `word` stays an ordinary name.
- **`R worked` and `R didn't work` are tests without `is`,** because that's how people say it ("if it worked"). They're fixed phrases after a value, like `is a letter`, and only apply to a program result.
- **`program result` is a kind the language defines,** added only to programs that use `run`, `worked` or the kind's name, so other programs keep the name free. Its fields are read like any record's (`r's output`); a program that defines its own `program result` can't also get one from `run` (an error says so).
- **`the key being pressed` is a value that changes by itself:** it's whatever key has been pressed and not read yet, so two uses in a row can differ. Like `the time now`, it describes the world when it's worked out.

The English pass's plan for new phrases, and the decisions already made about them, are in [english-pass.md](english-pass.md#decided-2026-10-11).

## Planned changes

Decided for the English pass ([english-pass.md](english-pass.md)); each breaks some existing programs, so each will be marked in the changelog when it lands.

- **Done (part 1): joining text with `and` was removed.** Pieces of text are joined with commas in `say` (`say "Hello,", name, "!"`), brackets in text, or `followed by`; `"Hello, " and name` is an error showing them.
- **Inputs will get natural joiners where unambiguous** (part 5): `the column of cell in grid`, `from ... to ...`, besides `and`.

## Open questions

None at the moment. Phrasings that may not follow these rules go here until they're decided.
