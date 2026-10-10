# EasyScript

EasyScript is a programming language with English sentence syntax. The compiler is written in C11. It compiles `.es` source files to C, then builds the generated C with `cc -O2` and runs the result.

## Language syntax (target)

- Statements are lowercase conversational sentences. A **period or a newline** ends a statement.
- A **colon followed by an indented block** opens a block (loops, conditionals, functions). The block ends when indentation returns to the outer level.
- Keywords are **case-insensitive**.
- `the`, `a`, `an` are **filler words**: the lexer/parser ignores them everywhere outside string literals. So they can never be used as identifiers.
- The 2024 uppercase prototype syntax (`MAKE A VARIABLE x ASSIGN 10`, `PRINT # x`) and its pipeline have been **removed**. Only the sentence syntax exists. `old/*.code` are historical files that no longer compile.

### Lexical rules (implemented in `src/front/lexer.c`)

- **Words:** an ASCII letter followed by letters, digits, or `_`. An apostrophe is part of a word only when a letter follows it (`isn't`, `guest's`, `rock'n'roll`). A trailing apostrophe (`guests'`) is an error. Words are lowercased. `the`/`a`/`an` are dropped, and the next token (including NEWLINE/EOF, but never INDENT/DEDENT) records the dropped word's span in `Token.filler`. A filler-only line passes nothing on to the next line. `tokens` shows it as `[after "the"]`. **There is no keyword table:** every word is a `WORD`, and the parser decides what it means, so `count` can be a variable.
- **Numbers:** `digits` or `digits.digits`. A `.` is a decimal point only when a digit follows it, so `5.` is `5` and then a period, `.5` is a period and then `5`, and `1.2.3` is `1.2 . 3`. No sign; `-` is always its own token. Letters stuck to a number (`3times`) are an error suggesting `3 times`.
- **Strings:** double quotes only, on one line, with escapes `\n \t \" \\`. An unknown escape is an error and is kept as written. An unterminated string is an error, and its value runs to the end of the line.
- **Symbols:** `. , : ( ) + - * / % = == < > <= >= !=`. A lone `!` is an error suggesting `!=`.
- **Comments:** `#` to the end of the line (outside strings), and any line whose first word is `note:` (any case, colon directly after it). `notes:` and `note :` are not comments.
- **Layout:**
  - `NEWLINE` ends each line that produced at least one token.
  - Blank lines, comment lines and filler-only lines produce nothing and don't affect indentation.
  - Indentation is measured with a tab as exactly 4 columns (not "next tab stop"). It's compared Python-style: an increase emits one `INDENT`, and a decrease emits a `DEDENT` per closed level.
  - A dedent to a width that matches no open level is an error that lists the valid widths. The line then stays at the nearest enclosing level.
  - An indented first line just emits `INDENT`; the parser rejects it.
  - Parentheses do **not** join lines.
  - At EOF the last line gets its `NEWLINE`, then a `DEDENT` for each open block, then `EOF`.
- **Positions:**
  - Tokens carry `line` and `column`, both 1-based. Columns count characters (UTF-8 aware), and a tab counts as 1.
  - Each token's `Span` covers exactly its source bytes: `NEWLINE` covers `\n` or `\r\n`, `INDENT` covers the indentation, and `DEDENT`/`EOF` are zero-length.
- **Other characters:** anything else is an error, and lexing continues. Curly quotes and curly apostrophes get a "use straight quotes" note. Non-ASCII letters get a note to use quotes. Invalid UTF-8 is reported as a byte, and control characters as `U+XXXX`.

### Expression grammar (implemented in `src/front/parse_expr.c`)

- **Precedence, loosest to tightest:**
  1. `or`
  2. `and`, which stops before `and call`
  3. `not`
  4. comparison (no chaining)
  5. `followed by` (its own level, below arithmetic, so `"Sum: " followed by 2 plus 3` is `"Sum: " followed by (2 plus 3)`)
  6. `plus` / `minus` / `+` / `-`
  7. `times` / `multiplied by` / `divided by` / `mod` / `*` / `/` / `%`
  8. unary `-`
  9. postfix `as a number` / `as text`
  10. primary

  Binary levels are left-associative.
- **`and` is a single `BINARY_AND` node.** The checker decides whether it means logical and (both sides yes/no) or text joining. `followed by` is `BINARY_JOIN`.
- **Operator phrases** live in `Phrase` tables, both words and symbols. The longest full match wins.
  - **Missing last word:** if the next tokens spell a phrase minus its last word, and that prefix is longer than any full match, it's a "Something is missing after … / Did you mean …" error. This covers `is greater 5`, `is at 5`, `multiplied 3`, `as numbr`.
  - **No negated or mixed forms:** `is not greater than` and `is above or equal to` are errors that suggest `is at most` / `is at least` (or `<=`/`>=` after a symbol).
  - **Equality:** `is not equal to` and `isn't equal to` are accepted, and `==` is a lexer token.
- **`times` needs a value after it:** it's multiplication only when the next token can start a value, so `repeat 3 times:` keeps its `times`.
- **Reserved words:** these can't start a value or be names: `and or not is isn't equals reaches plus minus times multiplied divided mod followed as using`. The value words `yes no true false nothing it` are literals. Every other word is a name. `length`, `contents` and `file` are only special when followed by `of` / `of file`.
- **Prefix forms bind tightly:** `length of X` and `contents of file X` take a single *primary*, so `length of x plus 1` means `(length of x) plus 1`. Postfix conversions wrap the whole form.
- **Calls:** a call is a single-word name followed by `of`, `with` or `using`, then arguments (`parser_parse_call_args`). Each argument is a *unary* expression.
  - **Separators:** arguments are separated by `and` (but not `and call`), and also by commas after `using` only, so `if f of x, say y` keeps its comma. The `and`s are taken greedily, and the checker reports a wrong count.
  - **Arithmetic after the arguments** (+ - * / % plus minus times multiplied divided mod followed by) is an ambiguity error that suggests both parenthesized forms, unless the last argument is parenthesized (`fib of (n minus 1) plus 1` adds; `is_parenthesized` looks at the source).
  - **Comparisons and `or` end the call.**
  - **No inputs:** a bare `NAME` is `EXPR_NAME`, and the checker and codegen treat it as a call when it's a function with no parameters (functions and variables share one namespace).
- **Positions:** every node has a `SourcePos`, holding its span and the line and column of its first token. A parenthesized group's span includes its parentheses. Binary, unary and convert nodes also keep `op_span`.
- **Errors and recovery:** at most one error per sentence (`Parser.sentence_failed`). After an error, the parser skips to the sentence end (`.`, NEWLINE, DEDENT, INDENT or EOF) and continues.
- **`.5`** (a period directly followed by a number) is rejected wherever a value or a sentence is expected ("Write 0.5 instead of .5.").

### Statement grammar (implemented in `src/front/parse_stmt.c`)

- **A program is a list of statements** (`parse_statements(p, block, in_block)`, used for the program and for every block; in a block it stops after the closing DEDENT). A simple statement ends at `.`, NEWLINE, or EOF, and several can share a line when separated by periods; a block statement (`p->ended_with_block`) ends with its DEDENT instead. An INDENT nothing opened is rejected ("nothing above it starts a block") and its whole block is parsed and discarded, so its DEDENT can't close an enclosing block. A sentence-initial `.` is an error. Expressions can't stand alone as statements. `p->ends_with_name` (set from the form; a one-line if's inner statement overrides it) chooses the leftover message.
- **Optional `please`:** a leading `please` is skipped. `please` on its own is an error.
- **Statement forms** (table `forms[]`, keyed by the first word; the AST normalizes synonyms but keeps `Stmt.verb` as written):

  | Form | AST |
  |---|---|
  | `let X be E`, `let X equal E` | `STMT_LET` |
  | `set X to E`, `change X to E` | `STMT_SET` |
  | `add E to X`, `subtract E from X` (amount first) | `STMT_CHANGE` with a `ChangeOp` |
  | `increase` / `decrease` / `multiply` / `divide X by E` (target first) | `STMT_CHANGE` with a `ChangeOp` |
  | `say` / `print` / `show` / `display E`, or plain `write E` | `STMT_SAY` |
  | `write E to file F` | `STMT_WRITE_FILE` |
  | `ask E and call the answer X` | `STMT_ASK`. The lexer drops "the", so the phrase is `and call answer`, and the prompt expression stops before `and call`. |
  | `append E to file F` | `STMT_APPEND_FILE` |
  | `read file F and call it X` | `STMT_READ_FILE` |
  | `stop the program` | `STMT_STOP` (`stop` on its own, or `stop the loop`, is `STMT_BREAK`; see loops) |
  | `if …` | `STMT_IF` (parse_if.c) |
  | `count` / `go` / `for` / `do` / `repeat` / `while` / `as long as` / `keep doing this until` / `forever` | `STMT_LOOP` (parse_loop.c) |
  | `stop the loop`, `stop`, `break` / `skip this one`, `skip`, `continue`, `move on` | `STMT_BREAK` / `STMT_CONTINUE` (parse_loop.c) |
  | `to NAME …:` / `give back E`, `return [E]` / `call NAME …`, or a sentence starting with a pre-scanned function name | `STMT_FUNCTION` / `STMT_RETURN` / `STMT_CALL` (parse_func.c) |
  | `keep NAME as E` / `keep NAME at E` (`keep doing …` is the loop) | `STMT_CONSTANT` (parse_func.c) |

- **Statement words are contextual.** They're special only as the first word, so `let say be 2`, `add 1 to add` and `let file be …` are fine.
- **Names:** a name is a single WORD that's neither an operator word nor a value word. `parse_name(p, name, next)` takes the words that follow the name in that form, e.g. `be`/`equal` after `let X`.
  - **Filler as name:** if the current token isn't a valid name, or *is* one of those `next` words, and it carries a `filler` span, the filler was meant as the name. That gives the "can't be used as a name… EasyScript ignores a/an/the" error. `let the total be 0` stays valid.
  - **Reserved words** give "already means something / like my_X".
  - **A `next` word with no filler** (`let be 5`) is "Something is missing after "let"".
- **Missing connectors:** `expect_word`/`expect_phrase` report the missing or wrong connector with the form's example sentence. For a wrong phrase, they quote what was written, e.g. `found "and call it"`. If the connector appears straight after the verb (`add to x`, `write to file …`, `append to file …`), the error is "Something is missing after "add"", because `to` would otherwise parse as a name.
- **Unknown first word:** "I don't know a sentence that starts with …", plus one note:
  - the closest statement word (or `please`) by edit distance, if one is close enough
  - otherwise, if the second token is `is`/`=`/`equals`/`be`, a `let X be …` / `set X to …` hint
  - otherwise, the generic list of starting words

  A non-word start gives "A sentence can't start with …".
- **Leftovers:** forms ending in a name or keyword (`add`, `subtract`, `ask`, `read`, `stop`, `break`, `skip`, `continue`, `move`) report extra words as "I expected the sentence to end after …". Others use the expression leftover reporting. Statements with errors are left out of the returned `Block`.
- **Quoting in messages:** `parser_quoted` renders text spans as `the text "…"` to avoid nested quotes.

### Constants (implemented in `parse_func.c`, `consteval.c`, `check.c`, `codegen_c.c`)

- **Syntax:** `keep NAME (as | at) E`. `parse_keep_form` sends `keep` + `doing` to the loop and everything else to `parse_constant`. AST: `STMT_CONSTANT {name, value, folded}`, where `folded` (a `ConstValue`: number/text/yes-no) is filled in by the checker.
- **Evaluation** (`front/consteval.c`, `const_eval(arena, diag, expr, lookup, ctx, out)`): literals (not `nothing`), unary minus and `not`, arithmetic, `followed by`, `and`/`or` (with the runtime's short-circuit and join rules), comparisons (with the 1e-12 tolerance), `as text`/`as a number`, and `length of`. Calls, `it`, `contents of file` and `nothing` are errors with a "fixed before the program starts … use let instead of keep" note. **It mirrors `runtime/es_runtime.h` exactly**, including `const_number_text` (= `es_number_text`) and the error wording. `tests/run/constant_matches_runtime.es` prints each constant next to the same runtime expression, so change both together.
- **Checker:**
  - **Collection:** `checker_collect_constants` runs after `collect_functions`, so constants can be used anywhere. Duplicates are errors, and so is a constant named like a function (reported at the constant).
  - **Values:** `checker_check_constant` rejects constants inside blocks or functions. Otherwise it evaluates the value via `lookup_constant`: only constants with a lower `index` are allowed, and a failed earlier constant adds no extra error. Self, later constants, functions, variables and unknown names get specific errors.
  - **Use:** names resolve to constants everywhere (`check_name_expr` ignores `floor`). `check_target` rejects changing a constant ("… is a constant, so it can't change", with a `let` suggestion). `checker_clashes` also rejects variables, inputs and loop numbers named like a constant. Unknown names can get "Did you mean "rate"? It's a constant, kept on line N.".
- **Codegen:** after the globals, each constant becomes `static const EsValue es_k_NAME = {ES_…, …};` from `folded` (NAN/HUGE_VAL for special numbers, text as a C string literal). `EXPR_NAME` resolves constants first (`es_k_`), and `main` adds a `(void)es_k_NAME;` per constant so unused ones don't warn.

### Functions (implemented in `src/front/parse_func.c`, `check.c`, `codegen_c.c`)

- **Definition:** `to NAME [with | of | using] P [and | ,] Q …:` plus an indented block. AST: `STMT_FUNCTION {name, params, body}`. The name can't be a statement word (`parser_is_statement_word`, which includes `please`): "… starts sentences in EasyScript, so it can't be the name of a function". `to greet a:` (filler before the colon) and `to greet with:` are errors.
- **Return:** `give back E`, `return E`, or bare `return` give `STMT_RETURN` (`returned` is NULL for bare).
- **Call sentences:** `NAME [with | of | using] A and B`, where NAME was found by `parser_find_functions`, which pre-scans `to WORD` at the start of every line before parsing, or `call NAME …`. Zero arguments is allowed (`greet.`), but `greet with` with nothing after it is an error. AST: `STMT_CALL {call: EXPR_CALL}`. Unknown first words also get the pre-scanned function names as suggestions.
- **Checker:**
  - **Function table:** all top-level functions are collected first; a duplicate is "You already defined …".
  - **Where they can go:** a definition inside an if, a loop or another function (`Checker.blocks` / `function`) is "Functions can only be defined at the top level…", and its body isn't checked.
  - **Function bodies:** a body sees only its parameters and the names it makes. `Checker.floor` hides `made[0..floor)`, and the loop stack is cleared. An unknown name that exists outside gives "A function only sees its own inputs and the names it makes. … pass it in as an input." Duplicate parameters are errors.
  - **Calls:** an unknown function gets "Did you mean "area"? It's defined on line N." or a "Define it first" note; a variable called like a function gets "… is a variable, not a function". A wrong count is "needs N value(s), but this gives it M" (or "doesn't take any values"), with a note quoting `to area with width and height` and the definition line. A bare name that's a function with parameters counts as 0 arguments.
  - **`give back` outside a function** is an error.
  - **Names:** making a variable, parameter or loop number with a function's name is an error (`checker_clashes`).
- **Codegen:**
  - **Signature and order:** each function becomes `static EsValue es_f_NAME(int es_line, EsValue es_v_P…)`, with prototypes before all function bodies and main.
  - **Each body:** `emit_function` uses a fresh `Codegen` (its own body, temps, loops, depth). It declares temporaries, then `EsValue es_v_local = es_nothing();` for every name made in the function (`declare_name` hoists into `locals`, skipping parameters), then `(void)` casts so unused inputs or locals can't warn, then `es_enter(es_line)`.
  - **Returns:** every return is `return es_leave(E)`, and the body ends with `return es_leave(es_nothing());`.
  - **Calls:** `(es_tA = arg1, es_tB = arg2, es_f_NAME(line, es_tA, es_tB))`, so arguments are evaluated left to right. A zero-parameter function used as a name is `es_f_NAME(line)`.
- **Runtime:** `es_enter(line)` counts depth and fails above `ES_MAX_DEPTH` (10000) with "Functions are calling each other too deeply (more than 10000 calls inside each other)." and a hint. `es_leave(v)` decrements and returns `v`.

### Loops (implemented in `src/front/parse_loop.c`)

- **Forms** (all need `:` plus an indented block via `parser_parse_block`; no one-line loops): `count [down] from A to B [by S | in steps of S] [and call each number N | as N]`, `go from …` (same as `count from`), `for each N from A to B [by S | in steps of S]`, `do this N times` / `repeat N times`, `while C` / `as long as C` / `repeat while C`, `keep doing this until C` / `repeat until C`, `forever`. AST: `STMT_LOOP` with `LoopKind`, `from`/`to`/`step` (NULL = 1)/`down`/`var` (default `"number"` at the verb's position, `var_named`)/`times`/`condition`/`body`.
- **`as` in count headers:** while parsing A, B and S, `Parser.as_ends_value` makes `parse_postfix` stop at `as`, so `as N` names the loop variable (even `as number`). Parentheses reset the flag (`parse_group`), so `(limit as a number)` converts. B and S stop before `and call` (the `and` rule), `by` and `in` (plain words).
- **`count` followed by `is`/`=`/`equals`/`be`** is "A sentence that starts with "count" is a counting loop…" with a `let count be` hint.
- **`stop`** (`parse_loop_control`): `stop program` (filler "the" dropped) is `STMT_STOP`; `stop loop` or bare `stop` is `STMT_BREAK`; `break` too. `skip [this one]`, `continue`, and `move on` are `STMT_CONTINUE`. Whether they're inside a loop is checked by the checker.
- `parse_loop` sets `ended_with_block` last, like `parse_if`.

### if / otherwise (implemented in `src/front/parse_if.c`)

- **Forms:** `if C:` NEWLINE INDENT block DEDENT, then any number of `otherwise if C:` blocks and at most one final `otherwise:` block. `else` is a synonym (`else if`, `else:`). One-line forms `if C, S` and `if C then S` take exactly one simple statement (not `if`/`otherwise`) and never have an `otherwise`. AST: `STMT_IF` with `Vec(IfBranch) branches` (`condition` NULL for the final `otherwise`, `body`, `pos` spanning the header up to its colon) and `one_line`.
- **Lining up:** after the if's DEDENT, an `otherwise` at the start of the next statement continues it (the lexer guarantees it's in the same column). Anything else starting with `otherwise`/`else` goes to `parse_orphan_otherwise`, which explains, in order: after a one-line if ("can't have an otherwise"), inside an open if's block (`p->open_ifs`, "doesn't line up with its if… The if on line N starts in column C"), or no if at all. A second plain `otherwise` is "already has an otherwise, on line N".
- **Header errors** (missing condition, missing colon/comma/then, text after the colon, nothing indented, broken condition) report once, then `parser_skip_line_and_block` skips the line and parses-and-discards an indented block; the rest of the chain is still consumed so it causes no extra errors. `parse_if` sets `ended_with_block` last, because statements inside its blocks reset it.
- **`:` or `,` right after an operator** (`if x plus:`) is "Something is missing after "plus"" (in `expected_value`).

### Runtime semantics (implemented in `runtime/es_runtime.h`)

- **Values:** `EsValue` is tagged: nothing, number (`double`), text (pointer and length, always NUL-terminated), or yes/no. Variables are dynamically typed; there are no static types yet.
- **Printing:** whole numbers with `|x| < 1e15` print with `%.0f` (no decimals, never `-0`). Other numbers use `%.15g`, so `0.1 plus 0.2` prints `0.3`. NaN prints `not a number`; infinities print `infinity` and `-infinity`. yes/no prints `yes`/`no`, and nothing prints `nothing`.
- **Arithmetic** (`plus minus times divided by mod`, unary `-`) needs two numbers. `plus` with text is an error that hints at `and`/`followed by`. Division or `mod` by zero gives "You divided by zero."; `mod` is `fmod`.
- **`followed by`** turns both sides into text and joins them.
- **`and`:** if the left side is `no`, the answer is `no` without evaluating the right side (codegen does the short-circuit). Otherwise:
  - two yes/no values: the right side's value
  - yes/no with anything else: an error, with an "as text" hint when text is involved
  - text on either side (with text, a number or nothing): join, converting to text
  - anything else (e.g. two numbers): an error, with a "use plus" hint for numbers
- **`or`** short-circuits on a `yes` on the left. Both sides must be yes/no. **`not`** needs yes/no.
- **Comparisons:** equal / not equal work on any values (different kinds are never equal). Numbers are equal when `a == b` or `|a - b| <= 1e-12 * max(|a|, |b|)` (`es_close`), so `0.1 plus 0.2 is 0.3` is yes. `es_order` returns 0 for numbers that are `es_close`, so `<=`/`>=` agree with equality and `<`/`>` are false for them. The ordering comparisons need two numbers or two texts (texts compare bytewise); anything else is an error, with an "as a number" hint for text against a number.
- **Conversions:** `as a number` accepts optional surrounding spaces, an optional `-`, digits, and optionally `.digits`; anything else is an error. `as text` always works. `length of` counts UTF-8 characters and needs text.
- **Files:** `write`/`append` write the value's text plus `\n`. Reading a file (`read file`, `contents of file`) strips one final `\n` (and a `\r` before it). A file name must be text. I/O errors read like `I couldn't read the file "x": it doesn't exist.`
- **Input:** `ask` prints its prompt without a newline, reads one line (stripping `\n`/`\r\n`), and gives empty text at end of input.
- **`es_if(line, v)`** returns the condition for an `if`/`otherwise if`; anything but yes/no is "An "if" needs yes or no to decide, but this is …" with a "Compare it with something" hint.
- **Shell support:** all program output goes through `es_out`, which drops the first `ES_SKIP_OUTPUT` bytes. With `ES_ANSWERS` set, `es_read_answer` replays answers from that file before reading stdin, appends new ones, and stdin is unbuffered. Both are set only by the shell.
- **`stop the program`** flushes and exits 0. Runtime errors flush stdout, print `Line N: message` (plus an optional hint line) to stderr, and exit 1.
- **Loops:** `es_count_start(line, from, to, step, has_step, down)` checks that from/to/step are numbers and step > 0, and picks the direction (`down` forces -1 and is empty when from < to). `es_count_next(&c, i, &var)` computes `from + direction * i * step` from scratch each round, snaps a value that's `es_close` to `to` onto `to`, and stops past `to`. `es_times(line, n)` needs a whole number ≥ 0 (and ≤ 9e18). `es_loop_condition(line, v)` needs yes/no ("A loop needs yes or no to decide whether to keep going…").
- **Memory:** runtime text comes from a static block allocator, freed at exit via `atexit`. **With loops this grows without bound**: about 20 bytes per round for a loop that builds a short text (21 MB peak for a million rounds, measured). Reclaiming memory during loops is a roadmap item.
- **The runtime is header-only** (all `static`) and silences `-Wunused-function` with a GCC/Clang pragma. Generated programs must compile cleanly under `-std=c11 -Wall -Wextra -Wpedantic`.

## Parser rules

- The parser must be **deterministic**. Never guess what the user meant, and never use heuristics, "best match", or silent fallbacks.
- If a sentence is ambiguous or doesn't match any grammar rule, **report an error**. Include the line and column, what was expected, and concrete suggestions (for example, the closest valid sentence forms). Don't pick one reading.
- Because a period ends a statement, it can't also be part of an identifier. The current lexer accepts `.` in identifiers so that `myfile.txt` works. That conflicts with the new syntax, so filenames have to become string literals.

## Compiler implementation rules

- **Arena allocator for all compiler memory** (tokens, AST nodes, strings, symbol tables). No per-object `malloc`/`free` and no `strdup`. Free the whole arena once at the end. The compiler is leak-free: verified with macOS `leaks --atExit` on every command, including error paths and the shell. Keep it that way.
- **No fixed-size buffers.** No `char buf[256]`, `VarMap var_map[100]`, `char line[1024]`, and so on. Grow storage dynamically from the arena. The same goes for generated C: don't emit `char x[256]` for strings.
- Write portable C11 (`-std=c11 -Wall -Wextra`). Invoke the C compiler as `cc`, not `gcc`.
- Report compile errors with source positions rather than calling `exit(1)` deep inside the lexer or parser.

## Code organization (target)

The compiler follows this pipeline:

- **Front end:** lexer → parser → checker.
- **Middle end:** IR and optimizations (added later).
- **Back end:** C codegen.

Each stage talks to the next only through shared data structures: tokens, the AST, and the symbol table. **The parser never emits C. Codegen never looks at source text.**

Target layout:

```
src/main.c            driver: runs the passes in order
src/front/            lexer.c, parse_expr.c, parse_stmt.c, check.c
src/back/             codegen_c.c
src/common/           arena.c, util.c, diag.c, ast.h
runtime/              runtime support linked into generated programs
```

Rules:

- **Group code by job**, not one file per function. Each module has a small public `.h` and keeps its private details in the `.c`.
- **Keep files under ~600 lines.** Split a file when it passes that, or when it covers two different concerns.
- **Helpers that aren't used elsewhere must be `static`.**
- **No global mutable state.** Pass context structs (`Compiler`, `Parser`, `Checker`, `Codegen`) explicitly.
- **Prefer opaque structs** when other modules don't need the fields.
- **Functions stay short and do one thing.**

Don't move code into this layout separately. Create it as later steps rewrite each stage, and update "Current state" as each piece moves.

## Testing

- **Every feature needs tests in `tests/`.** That includes parser error cases: ambiguous input must produce the expected error and suggestions.
- **Never commit while any test is failing.** Run `make test` and `make test-debug` (sanitizers) before every commit.
- `tests/ast/NAME.es` plus `NAME.out`, and an optional `NAME.err`: the same as `tests/tokens`, but run with `easyscript ast`.
- `tests/tokens/NAME.es` plus `NAME.out`, and an optional `NAME.err`: run with `easyscript tokens`. Stdout must match `NAME.out`. If `NAME.err` exists, the run must exit 1 with exactly that stderr. Otherwise it must exit 0 with empty stderr. Name error cases `err_*`.
- `tests/unit/*.c`: C unit tests for `src/common` and `src/front`. Don't use `a`, `an` or `the` as variable names in `.es` fixtures, because the lexer drops them, linked into `build/*/unit_tests`. Each test is `static void test_x(TestContext *t)`, registered with `unit_run` in that file's `*_tests(TestRunner *)` function. Declare that function in `unit.h` and call it from `main` in `unit.c`. Use `CHECK`, `CHECK_SIZE`, and `CHECK_STR`. `t->arena` is fresh for each test.
- `tests/run/NAME.es` plus `NAME.out`: the program is compiled and run with `easyscript run`, with stdin from `NAME.in` if it exists (otherwise `/dev/null`). Stdout must match `NAME.out`. If `NAME.err` exists, the program must exit 1 with exactly that stderr; these are the runtime-error tests, named `err_*`. Otherwise it must exit 0 with empty stderr.
- `tests/errors/NAME.es` plus `NAME.err`: compiled with `easyscript emit`. It must exit non-zero, and stderr must match `NAME.err` exactly. Mostly checker errors, plus one parse error showing that the checker is skipped.
- `tests/shell/NAME.in`: typed into the interactive shell (`easyscript` with no arguments) in an empty scratch directory. Stdout must match `NAME.out` and stderr `NAME.err` (empty if there's no `.err`); `make bless` writes both.
- **README sync checks:** the end of `tests/run.sh` checks that the README's "A first program" block equals `examples/programs/first_program.es`, and its "A bigger example" block equals `examples/programs/taste.es` (which runs and has a `.out`).
- CLI behavior (modes, usage errors, no files left in the cwd, temp-dir cleanup) is checked at the bottom of `tests/run.sh`. Add a check there when changing the CLI.
- `examples/programs/NAME.es` plus `NAME.out` and optional `.in`/`.err` (run like `tests/run`), `examples/lexer/NAME.es` plus `NAME.tokens`, and `examples/parser/NAME.es` plus `NAME.ast` (both with an optional `.err`) back the docs. `tests/run.sh` fails if any other folder or a loose `.es` appears under `examples/`.
- `tests/run.sh` runs the unit binary, then the run/errors/tokens tests and the examples (each in an empty scratch directory, via the shared `check_run`/`check_errors`/`check_dump` functions), then the CLI checks. It prints PASS/FAIL per test plus one summary and exits 1 if anything fails. The `ES` and `UNIT` env vars choose which binaries are tested.
- Expected files record current behavior, including known quirks (for example, the blank line after `FILE READ` in `run/file_io.out`). When fixing a quirk, update the expected file in the same commit.
- **`make bless`** (`tests/bless.sh`) rewrites every golden file from the current output, then shows `git diff --stat` and any new expected files.
  - It covers `tests/run`, `tests/errors`, `tests/tokens`, `tests/ast`, `examples/programs`, `examples/lexer` and `examples/parser`. Run tests use `.in`; exit 1 writes `.err`.
  - In the tokens/ast folders, `.err` is written on failure and removed on success.
  - It exits 1 with "NOT BLESSED" when no golden file could make a test pass.
  - **Only bless after confirming the new output is intended.** First read every failing diff from `make test`. Bless only if each difference is a deliberate change. Then review `git diff` before committing, and say in the summary which expected files changed and why.
  - Never bless just to get a green suite, and never bless output you haven't read.

## Documentation rules

**Every commit that adds or changes a language feature must, in the same commit:**

1. Update the status in `README.md` (the Status table and "Coming soon" list, and the roadmap table if a phase changes).
2. Update `docs/vocabulary.md` with every new or changed word, sentence pattern and synonym, each with an example and its status.
3. Update the matching chapter of `docs/language-guide.md`, including its status label.
4. Add or update an example in `examples/` with its expected output (`examples/lexer/NAME.tokens`, `examples/parser/NAME.ast`, each with an optional `.err`, or `examples/programs/NAME.out` with optional `.in`/`.err` for anything that runs).
5. Add an entry to `CHANGELOG.md` under `[Unreleased]`.

**A feature isn't done until its docs and example exist and pass `make test`.** Also:

- **Never present unimplemented syntax as working.** Anything that doesn't compile today is labelled "Coming soon" (a proposal whose wording may change). Move an item from the planned-examples list in `docs/roadmap.md` into `examples/` when it ships, and tick its roadmap checkbox.
- **Error text in the docs is copied from tested `.err` files.** If a message changes, update `docs/errors.md` along with the expected file.
- Everything in `docs/vocabulary.md` is implemented and "Available" except `file X exists` (still a **proposal, not a decision**) and the later phases. When implementing them, either implement them as written or change the docs in the same commit. Don't let the docs and the parser disagree.
- Architecture or workflow changes also update `docs/architecture.md` and `CONTRIBUTING.md`. The README's program blocks must stay identical to their example files (checked by `make test`).

## Performance records

- `make bench` (`benchmarks/run.sh`) builds every program in `benchmarks/NAME/` (`NAME.es`, plus `.c`, `.go`, `.py` where present). It runs each 3 times (`BENCH_RUNS`) under `/usr/bin/time`, prints the fastest time and peak RSS, and fails if any language's first output line differs from EasyScript's.
- `docs/performance.md` has the latest full table (machine, tool versions, date) and a **History** table with one EasyScript row per milestone. **Never edit past history rows or past results; only add new rows.** At each milestone, run `make bench`, replace the "Latest results" table, add a history row, and update the README's compact Performance table to match.

## Keeping this file current

**Every change that affects layout, build, or behavior must update the "Current state" and "Build" sections below in the same commit.** Those sections describe the repo as it is, not as it should be.

## Current state of the repo

Phase 1 is in progress. Programs in the sentence syntax compile to C and run, including `if`/`otherwise` blocks, loops and top-level functions.

- **Layout:** `src/main.c` is the driver, and `src/shell.c` the interactive shell. `src/common/` holds arena, util, diag and ast. `src/front/` holds lexer, parse_util, parse_expr, parse_stmt, parse_if, parse_loop, parse_func, consteval, check and check_const (with private headers `parse_internal.h` and `check_internal.h`). `src/back/` holds codegen_c and codegen_expr (with `codegen_internal.h`). `runtime/es_runtime.h` is the runtime, and `tools/embed.c` is a build tool.
- **Pipeline** (`generate_c` in `src/main.c`, shared by `run`/`build`/`emit`):
  1. Lex and parse.
  2. If there were no errors, run `check_program`.
  3. If there are any diagnostics, print them all and exit 1.
  4. Otherwise run `codegen_c` and write `program.c` to the temp dir.

  `cc -O2 program.c -o OUT -lm` is run via `fork`/`execvp`, and `run` returns the program's exit status.
- **Stage boundaries:** these now follow the target design. Codegen trusts the checker and reports nothing; it asserts if it ever sees `EXPR_ERROR` or an `it` with no counting loop around it. `src/main.c` still has file-scope `arena` and `temp_*` pointers for its `atexit` cleanup (the only global state in the compiler).
- `src/main.c`: the CLI (the shell is in `src/shell.c`). `run FILE` / `build FILE -o OUT` / `emit FILE` / `tokens FILE` / `ast FILE`, and `help`. Unknown commands and bad arguments print usage and exit 2.
  - Each invocation creates one `mkdtemp` directory under `$TMPDIR` (or `/tmp`), holding `program.c`, `program` and `session.es`. `atexit` removes it.
  - **The shell** (no arguments) is `src/shell.{c,h}` (`shell_run`). It keeps the accepted source in a `StrBuf`. Per entry (a line, or a `:`-ending line plus following lines until a blank one) it writes `session.es` and runs `argv[0] run session.es` with stdout piped through it, setting `ES_SKIP_OUTPUT` (bytes already shown) and `ES_ANSWERS` (`answers.txt` in the temp dir). It keeps the entry only on exit 0 (and adds the bytes printed to `shown`); otherwise it truncates the answers file back and prints "(That line/block wasn't kept.)". It reads stdin one byte at a time so it never reads lines meant for the program. `exit`, `quit` or `stop the program` leaves.
- `src/common/arena.{c,h}`: arena allocator (`arena_new`, `arena_alloc` returns zeroed, aligned memory, `arena_strdup`, `arena_sprintf`/`arena_vsprintf`, `arena_free`). It grows in 64 KiB blocks and gives oversized requests their own block. All compiler memory comes from here.
- `src/common/util.{c,h}`: `StrBuf` (`sb_*`, including `sb_append_quoted`), `Vec(T)` with `vec_push`/`vec_last`/`vec_pop`, `edit_distance` (optimal string alignment, so an adjacent swap counts 1), and `PRINTF_LIKE`.
- `src/common/diag.{c,h}`: opaque `Diag` made by `diag_new(arena, source, len)`. Report with `diag_error(d, span, fmt, ...)`, then `diag_note(d, fmt, ...)` for help lines attached to the latest error. `Span` is a byte `{offset, length}`. `diag_line` gives the 1-based line and `diag_count` the number of errors. `diag_render` (to a `StrBuf`) and `diag_print` (to a `FILE *`) output every error in report order, separated by blank lines: `Line N: message`, then the source line indented 4 spaces, then carets under the span, then the notes. Carets are clamped to the first line of the span, with at least one. Tabs are copied into the padding, UTF-8 is counted per character, and a trailing `\r` is dropped.
- `src/common/ast.{h,c}`: `Expr` (kinds ERROR, NUMBER (text plus `is_decimal`), TEXT, BOOLEAN, NOTHING, IT, NAME, CALL, LENGTH, FILE_CONTENTS, UNARY, BINARY, CONVERT), `Stmt` (LET, SET, CHANGE + `ChangeOp`, SAY, ASK, WRITE_FILE, APPEND_FILE, READ_FILE, STOP, IF with `IfBranch`es, LOOP, BREAK, CONTINUE, FUNCTION, RETURN, CALL, CONSTANT (with `ConstValue folded`), each with a `verb` span), `Name {text, pos}`, and `Block` (`Vec(Stmt *)`). Every node has a `SourcePos`. It also has `ast_dump_block` for `easyscript ast`.
- `src/front/lexer.{c,h}`: `lex(arena, diag, source, len)` returns a `TokenList` that always ends with `TOK_EOF`; tokens carry a `filler` span. Also `token_kind_name` and `tokens_dump(tokens, source, out)`.
- `src/front/parse.h`, `parse_internal.h`, `parse_util.c`, `parse_expr.c`, `parse_stmt.c`, `parse_if.c`, `parse_loop.c`, `parse_func.c`: the parser. Shared statement helpers have `parser_` names (`parser_new_stmt`, `parser_parse_name`, `parser_expect_word`, `parser_expect_phrase`, `parser_looks_like_assignment`, `parser_parse_block`); see the grammar sections above. The if parser was fuzzed with 2,500 inputs under ASan/UBSan (no crashes or hangs; all 74 accepted programs compiled with `-Werror`). The expression and statement parsers were each fuzzed with 3,000 inputs under ASan/UBSan, with no findings.
- `src/front/check.{c,h}` plus `check_const.c` (constants), sharing `check_internal.h` (the `Checker`, `Symbol`, `Loop`, `Function`, `Constant` structs and the `checker_` helpers: `checker_find_made`, `checker_find_function`, `checker_find_constant`, `checker_report_unknown`, `checker_clashes`, `checker_check_expr`, `checker_collect_constants`, `checker_check_constant`, `checker_closest_constant`): `check_program(arena, diag, program)`. A `Checker` context holds `made` (names made so far, with lines) and `later` (all names the program makes, for "You make "x" later, on line N").
  - **Making names:** `let` makes a name; making it twice is "You already made …" with a "set" hint. `ask` and `read file … and call it` make the name if it's new and silently reuse it otherwise. `set`/`change`/arithmetic statements and `EXPR_NAME` need an existing name.
  - **Unknown names:** "I don't know anything called "x"." plus one note:
    - "Did you mean "total"? You made it on line 1." for the single closest name (OSA distance, limit 1 for names of 3 or fewer characters, otherwise 2)
    - a list when several are equally close
    - "You make "x" later, on line N."
    - otherwise "Make it first, like "let x be 0"."
  - **Loops:** a `Loop` stack (`has_it` for count and times loops). `EXPR_IT` needs some enclosing loop with `has_it`; otherwise "it doesn't refer to anything here", with a note that's different inside a non-counting loop and outside any loop. `STMT_BREAK`/`STMT_CONTINUE` outside any loop are errors. Loop values (from/to/step/times/condition) are checked in the outer scope. The count variable is made in the loop's scope; if the name exists, it's an error ("This loop calls each number "number", but you already made…" when defaulted). The loop body ends the scope as "the loop"; a loop variable used afterwards gets "is the number of the loop on line N, so it only exists inside that loop" (`Symbol.loop_number`).
  - **Scopes:** each if branch body is a scope (`check_block`): names made inside are dropped from `made` at its end and recorded in `ended` with the if's line, so a later use says "You made "x" inside the "if" on line N, so it only exists inside that block." A name that exists outside can't be made again inside ("You already made …"); different blocks can make the same name.
- `src/back/codegen_c.{c,h}` (statements, loops, functions, constants, the program) plus `codegen_expr.c` (names, literals, expressions) sharing `codegen_internal.h` (the `Codegen`/`LoopCode` structs and the `cg_` helpers: `cg_emit_expr`, `cg_emit_call1`, `cg_append_prefixed`, `cg_append_name`, `cg_append_c_string`, `cg_declare_name`, `cg_new_temp`): `codegen_c(arena, program, out)` writes the embedded runtime (the `es_runtime_source` byte array), `static EsValue es_v_<name>;` globals, and `int main(void)`.
  - **Names:** variables are mangled to `es_v_` plus the name, with `_` written as `__` and `'` as `_q`.
  - **Temporaries:** the left operand of every binary operation, and the text of file writes, go into temporaries `es_t1…` declared at the top of `main`. This forces left-to-right evaluation.
  - **Short-circuit:** `and`/`or` compile to `(t = L, es_is_no(t) ? t : es_and(line, t, R))`, and `or` uses `es_is_yes`.
  - **Literals:** numbers are re-printed with `%.17g`, or `HUGE_VAL` if infinite. Text uses octal escapes and `\?`.
  - **Lines:** each statement gets a `/* line N */` comment, and runtime calls receive the node's line.
  - **if:** `if (es_if(line, C)) { … } else if (es_if(line, C2)) { … } else { … }`, with `Codegen.depth` indenting nested blocks.
  - **Loops:** each loop is wrapped in `{ }`. A count stores from/to/step in temporaries (left to right, once), then `EsCount es_cN = es_count_start(…)` and `for (long long es_iN = 0; es_count_next(&es_cN, es_iN, &es_v_var); es_iN++)`. Times is `long long es_nN = es_times(…); for (es_iN = 1; es_iN <= es_nN; es_iN++)`. While/until use `while ([!]es_loop_condition(line, C))`, and forever is `for (;;)`. Break and continue are C `break;`/`continue;`. `Codegen.loops` resolves `it` to the innermost count's variable or `es_num((double)es_iN)` for times. Names made in blocks are ordinary globals; the checker guarantees they're only used in their block.
  - **Checked:** fuzzed once, with 171 mutated programs that passed the checker. All compiled with `-Wall -Wextra -Werror`.
- `runtime/es_runtime.h`: see "Runtime semantics" above.
- `Makefile`: `CC = cc`, `-std=c11 -Wall -Wextra -Isrc`.
  - **Source groups:** `COMMON_SRC`, `FRONT_SRC` (also linked into the unit tests), `BACK_SRC`, and `GEN_RUNTIME`, which is `build/gen/es_runtime_embed.c` generated by `build/tools/embed` from `runtime/es_runtime.h`. Never edit generated files.
  - **Builds:** objects go under `build/release/` (`-O2`) or `build/debug/` (ASan/UBSan, `-fno-sanitize-recover=all`), with `-MMD` dependencies.
  - **Targets:** `all`, `test`, `debug`, `test-debug` (leak checking left at the ASan default, which is on for Linux), `bless`, `bench`, and `clean`.
  - Zero warnings in both modes. `src/main.c` defines `_XOPEN_SOURCE 700` and `_DARWIN_C_SOURCE`.
- `tests/`:
  - 40 unit tests
  - 49 AST tests (29 valid, including `if_*`, `loop_*`, `func_*`, `const_define`; 20 `err_*`, including `err_if_header`, `err_if_otherwise`, `err_loop_header`, `err_func_header`, `err_const_header`)
  - 21 token tests (14 valid, 7 `err_*`)
  - 64 run tests (31 programs, including `if_*`, `loop_*`, `func_*`, `const_basic`, `constant_matches_runtime`, covering numbers, number equality with tolerance, text (including `followed by` precedence), logic and short-circuiting, every comparison, variables, files, `ask` with and without input, `stop`, sentences and synonyms; plus 33 `err_*` runtime-error tests, including left-to-right error order, non-yes/no `if`/loop conditions, bad counts, steps and times, an error's line number inside a loop or function, and endless recursion)
  - 4 shell sessions (`tests/shell/NAME.in` plus `.out`/`.err`: output not repeated, blocks, dropped lines, answers replayed and cut back after a failure, leaving)
  - 47 compile-error tests (checker messages including block scopes, loop numbers, `it`, stop/skip outside loops, unknown functions, argument counts, `give back` outside functions, functions in blocks, function scopes and name clashes, every constant rule, plus one parse error)
  - 40 examples, 2 README sync checks, and CLI checks
- **Benchmarks:** `benchmarks/` has `fib` (fib 38), `count` (to 100 million), `nested` (10,000 × 10,000 with `if`/`mod`) and `text` (10,000 joins), each in `.es`, `.py` and `.go`, plus `.c` except `text`. See "Performance records".
- **Docs:**
  - `README.md`: front page with "A first program" (runs, and is tested), "A bigger example" (a loop and a function; runs, and is tested), honest status, roadmap with Notebook, design rules, build/CLI, pipeline, links, and "License: TBD".
  - `docs/language-guide.md`: chapters 0–8 available, except `file X exists` (coming soon).
  - `docs/vocabulary.md`: statuses Available / Coming soon.
  - Also `docs/errors.md` (lexer, parser, checker and runtime errors), `docs/performance.md` (benchmarks and history), `docs/architecture.md`, `docs/roadmap.md`, `CHANGELOG.md`, `CONTRIBUTING.md` and `examples/README.md`.
- **Examples:**
  - `examples/programs/` (15 runnable): `hello`, `first_program`, `taste` (README), `variables`, `text`, `logic`, `decisions`, `loops`, `logan` (old/logan.code ported), `tax` (a constant used by a function), `factorial`, `fibonacci`, `ask_name` (+ `.in`), `files`, `err_divide_by_zero`.
  - `examples/parser/` (16): syntax-tree demos, many using names they never make, plus `err_otherwise_misplaced`.
  - `examples/lexer/` (9).
- No license has been chosen; the user will pick one.
- `old/`: the 2024 prototype `.code` files. Historical only; they no longer compile. `old/logan.code` is ported as `examples/programs/logan.es`.
- `.gitignore` covers build outputs (`build/`, `easyscript`, stray `*.o`/`*.d`), `myfile*`, `.idea/` (CLion), and `.vscode/` (no longer tracked). `.gitattributes` marks `tests/tokens/**` as `-text`.

## Build

```sh
make                                  # builds ./easyscript (plus build/tools/embed and build/release/unit_tests); no warnings expected
make test                             # builds, then runs tests/run.sh (unit, tokens, ast, run, errors, examples, docs, CLI)
make debug                            # ASan/UBSan build in build/debug/
make test-debug                       # full suite against the sanitizer build
make bless                            # rewrite golden files from current output, then show git diff --stat
./easyscript run FILE.es              # compile + run (generated C/binary go to a temp dir)
./easyscript build FILE.es -o OUT     # compile to an executable at OUT
./easyscript emit FILE.es             # print the generated C
./easyscript tokens FILE.es           # print the lexer's tokens
./easyscript ast FILE.es              # print the syntax tree
./easyscript                          # interactive shell (exit or quit to leave)
make clean
```
