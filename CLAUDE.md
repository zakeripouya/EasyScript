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
- **Ordinals:** a whole number directly followed by `st`/`nd`/`rd`/`th` (any case, and no more word characters) is `TOK_ORDINAL` (text as written); a wrong ending ("2th") is an error suggesting the right one ("2nd"). Only `every 3rd item of L` uses it.
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

### Expression grammar (implemented in `src/front/parse_expr.c`, operators; `parse_value.c`, values)

- **Precedence, loosest to tightest:**
  0. `every NAME in L REST` (only at the start of a value: `parse_expression`, which parentheses also use; elsewhere it's an error)
  1. `or`
  2. `and`, which stops before `and call`
  3. `not`
  4. comparison (no chaining), including `contains` / `does not contain` / `doesn't contain` and `is empty` / `is not empty` / `isn't empty` (but `is empty list of ...` compares with an empty list)
  5. `followed by` (its own level, below arithmetic, so `"Sum: " followed by 2 plus 3` is `"Sum: " followed by (2 plus 3)`)
  6. `plus` / `minus` / `+` / `-`
  7. `times` / `multiplied by` / `divided by` / `mod` / `*` / `/` / `%`
  8. unary `-`
  9. postfix `as a number` / `as text`, and `added together` / `reversed`
  10. primary (including the list forms in `parse_list.c` and `only L that ...`)

  Binary levels are left-associative.
- **`and` is a single `BINARY_AND` node.** The checker decides whether it means logical and (both sides yes/no) or text joining. `followed by` is `BINARY_JOIN`.
- **Operator phrases** live in `Phrase` tables, both words and symbols. The longest full match wins.
  - **Missing last word:** if the next tokens spell a phrase minus its last word, and that prefix is longer than any full match, it's a "Something is missing after … / Did you mean …" error. This covers `is greater 5`, `is at 5`, `multiplied 3`, `as numbr`.
  - **No negated or mixed forms:** `is not greater than` and `is above or equal to` are errors that suggest `is at most` / `is at least` (or `<=`/`>=` after a symbol).
  - **Equality:** `is not equal to` and `isn't equal to` are accepted, and `==` is a lexer token.
- **`times` needs a value after it:** it's multiplication only when the next token can start a value, so `repeat 3 times:` keeps its `times`.
- **Reserved words:** these can't start a value or be names: `and or not is isn't equals reaches plus minus times multiplied divided mod followed as using`. The value words `yes no true false nothing it` are literals. Every other word is a name. `length`, `contents` and `file` are only special when followed by `of` / `of file`; the list words are only special in their forms (see "Lists"), so `item`, `first`, `last`, `only`, `every`, `list`, `copy` can still be names.
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
| `add E to the end of L`, `insert E at position P of L`, `remove item P from L` (or `the first item`/`the last item`), `set`/`change item P of L to E`, `sort L …` | `STMT_LIST` with a `ListChange` (parse_list.c; `add`/`set` hand over when `end of` / an item form follows) |

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
- **Evaluation** (`front/consteval.c`, `const_eval(arena, diag, expr, lookup, ctx, out)`): literals (not `nothing`), unary minus and `not`, arithmetic, `followed by`, `and`/`or` (with the runtime's short-circuit and join rules), comparisons (with the 1e-12 tolerance), `as text`/`as a number`, and `length of`. Calls, `it`, `contents of file` and `nothing` are errors with a "fixed before the program starts … use let instead of keep" note. **It mirrors the checker's kind rules and `runtime/es_runtime.h`'s values exactly**, including `const_number_text` (= `es_number_text`) and the error wording (a kind mistake in a constant is reported by consteval, not by inference). `tests/run/constant_matches_runtime.es` prints each of 23 constants next to the same runtime expression, so change both together.
- **Checker:**
  - **Collection:** `checker_collect_constants` runs after `collect_functions`, so constants can be used anywhere. Duplicates are errors, and so is a constant named like a function (reported at the constant).
  - **Values:** `checker_check_constant` rejects constants inside blocks or functions. Otherwise it evaluates the value via `lookup_constant`: only constants with a lower `index` are allowed, and a failed earlier constant adds no extra error. Self, later constants, functions, variables and unknown names get specific errors.
  - **Use:** names resolve to constants everywhere (`check_name_expr` ignores `floor`). `check_target` rejects changing a constant ("… is a constant, so it can't change", with a `let` suggestion). `checker_clashes` also rejects variables, inputs and loop numbers named like a constant. Unknown names can get "Did you mean "rate"? It's a constant, kept on line N.".
- **Kind:** the kind of `folded` (an unworked-out constant is `TYPE_ERROR`).
- **Codegen:** each constant becomes `static const double es_k_NAME = …;` (NAN/HUGE_VAL for special numbers), `static const bool …`, or `static EsText *const es_k_NAME = &es_sN;` (a static immortal text object). `EXPR_NAME` resolves constants first (`es_k_`), and `main` adds a `(void)es_k_NAME;` per constant so unused ones don't warn.

### Functions (implemented in `src/front/parse_func.c`, `check.c`, `codegen_c.c`)

- **Definition:** `to NAME [with | of | using] P [(KIND)] [and | ,] Q [(KIND)] … [[,] giving back KIND]:` plus an indented block. KIND (`parse_kind`) is `number` (after the dropped `a`), `text`, `yes or no` or `yes/no`, and `nothing` only after `giving back`; errors: "I don't know the kind of value …", "I expected ")" after the kind of value…", "Something is missing after "giving back"". The param loop stops at `giving back`; with no inputs, `to roll, giving back a number:` skips the comma. AST: `STMT_FUNCTION {name, params, declared (per input, TYPE_UNKNOWN if none), declared_result, param_types, result (filled by the checker), body}`; `easyscript ast` shows `define area (width: a number) giving back a number`. The name can't be a statement word (`parser_is_statement_word`, which includes `please`): "… starts sentences in EasyScript, so it can't be the name of a function". `to greet a:` (filler before the colon) and `to greet with:` are errors.
- **Return:** `give back E`, `return E`, or bare `return` give `STMT_RETURN` (`returned` is NULL for bare; the parser also makes `give back nothing` bare).
- **Call sentences:** `NAME [with | of | using] A and B`, where NAME was found by `parser_find_functions`, which pre-scans `to WORD` at the start of every line before parsing, or `call NAME …`. Zero arguments is allowed (`greet.`), but `greet with` with nothing after it is an error. AST: `STMT_CALL {call: EXPR_CALL}`. Unknown first words also get the pre-scanned function names as suggestions.
- **Checker:**
  - **Function table:** all top-level functions are collected first; a duplicate is "You already defined …".
  - **Where they can go:** a definition inside an if, a loop or another function (`Checker.blocks` / `function`) is "Functions can only be defined at the top level…", and its body isn't checked.
  - **Function bodies:** a body sees only its parameters and the names it makes. `Checker.floor` hides `made[0..floor)`, and the loop stack is cleared. An unknown name that exists outside gives "A function only sees its own inputs and the names it makes. … pass it in as an input." Duplicate parameters are errors.
  - **Calls:** an unknown function gets "Did you mean "area"? It's defined on line N." or a "Define it first" note; a variable called like a function gets "… is a variable, not a function". A wrong count is "needs N value(s), but this gives it M" (or "doesn't take any values"), with a note quoting `to area with width and height` and the definition line. A bare name that's a function with parameters counts as 0 arguments.
  - **`give back` outside a function** is an error. Kinds of inputs, results and give-backs: see "Types".
  - **Names:** making a variable, parameter or loop number with a function's name is an error (`checker_clashes`).
- **Codegen:**
  - **Signature and order:** each function becomes `static RESULT es_f_NAME(int es_line, TYPE es_vX_P…)` with C types from `param_types`/`result` (`void` for nothing), with prototypes before all function bodies and main.
  - **Each body:** `emit_function` uses a fresh `Codegen` (its own body, temps, vars, loops, depth). It declares typed temporaries, then every variable made in the function (`cg_declare_name` hoists into `vars`, skipping parameters; numbers `= 0`, yes/no `= false`, text `= &es_empty_text`), `(void)` casts for non-text variables and all inputs, then `es_enter(es_line)`.
  - **Returns:** `es_tN = E;` (worked out first), drop text inputs and variables (`emit_drop_all`), `es_leave(); return es_tN;` (or `return;`). Falling off the end: a nothing function drops and calls `es_leave()`; a value function calls `es_no_result(def line, "NAME")` (runtime error "The function "NAME" ended without giving anything back." with a hint).
  - **Calls:** `es_f_NAME(line, A, B)`; when two or more arguments have effects (`cg_has_effects`), all go into typed temporaries first, left to right. A zero-parameter function used as a name is `es_f_NAME(line)`. A call sentence: `es_release(CALL);` for text, `(void)CALL;` for numbers/yes-no, `CALL;` for nothing.
- **Runtime:** `es_enter(line)` counts depth and fails above `ES_MAX_DEPTH` (10000) with "Functions are calling each other too deeply (more than 10000 calls inside each other)." and a hint. `es_leave()` decrements.

### Loops (implemented in `src/front/parse_loop.c`)

- **Forms** (all need `:` plus an indented block via `parser_parse_block`; no one-line loops): `count [down] from A to B [by S | in steps of S] [and call each number N | as N]`, `go from …` (same as `count from`), `for each N from A to B [by S | in steps of S]`, `for each N in L` (`LOOP_EACH`, `list`; see "Lists"), `do this N times` / `repeat N times`, `while C` / `as long as C` / `repeat while C`, `keep doing this until C` / `repeat until C`, `forever`. AST: `STMT_LOOP` with `LoopKind`, `from`/`to`/`step` (NULL = 1)/`down`/`var` (default `"number"` at the verb's position, `var_named`)/`times`/`condition`/`list`/`body`.
- **`as` in count headers:** while parsing A, B and S, `Parser.as_ends_value` makes `parse_postfix` stop at `as`, so `as N` names the loop variable (even `as number`). Parentheses reset the flag (`parse_group`), so `(limit as a number)` converts. B and S stop before `and call` (the `and` rule), `by` and `in` (plain words).
- **`count` followed by `is`/`=`/`equals`/`be`** is "A sentence that starts with "count" is a counting loop…" with a `let count be` hint.
- **`stop`** (`parse_loop_control`): `stop program` (filler "the" dropped) is `STMT_STOP`; `stop loop` or bare `stop` is `STMT_BREAK`; `break` too. `skip [this one]`, `continue`, and `move on` are `STMT_CONTINUE`. Whether they're inside a loop is checked by the checker.
- `parse_loop` sets `ended_with_block` last, like `parse_if`.

### if / otherwise (implemented in `src/front/parse_if.c`)

- **Forms:** `if C:` NEWLINE INDENT block DEDENT, then any number of `otherwise if C:` blocks and at most one final `otherwise:` block. `else` is a synonym (`else if`, `else:`). One-line forms `if C, S` and `if C then S` take exactly one simple statement (not `if`/`otherwise`) and never have an `otherwise`. AST: `STMT_IF` with `Vec(IfBranch) branches` (`condition` NULL for the final `otherwise`, `body`, `pos` spanning the header up to its colon) and `one_line`.
- **Lining up:** after the if's DEDENT, an `otherwise` at the start of the next statement continues it (the lexer guarantees it's in the same column). Anything else starting with `otherwise`/`else` goes to `parse_orphan_otherwise`, which explains, in order: after a one-line if ("can't have an otherwise"), inside an open if's block (`p->open_ifs`, "doesn't line up with its if… The if on line N starts in column C"), or no if at all. A second plain `otherwise` is "already has an otherwise, on line N".
- **Header errors** (missing condition, missing colon/comma/then, text after the colon, nothing indented, broken condition) report once, then `parser_skip_line_and_block` skips the line and parses-and-discards an indented block; the rest of the chain is still consumed so it causes no extra errors. `parse_if` sets `ended_with_block` last, because statements inside its blocks reset it.
- **`:` or `,` right after an operator** (`if x plus:`) is "Something is missing after "plus"" (in `expected_value`).

### Lists (implemented in `src/front/parse_list.c`, `parse_expr.c`, `check_list.c`, `src/back/codegen_list.c`, `runtime/es_list.h`)

- **Values:** `list of A, B and C` (`EXPR_LIST`; items are `parse_join` values; commas separate items until the first `and`, after which only `and` does, so a one-line if's comma survives; an unparenthesized `list of` inside is "A list inside a list needs parentheses."), `empty list of KINDS` (`EXPR_EMPTY_LIST`, `list_type`), `item P of|in L`, `first item of|in L`, `last item of|in L` (`EXPR_ITEM` with `ItemWhere`; the list is a primary), `number of items in L` (`EXPR_LENGTH`), `copy of L`, `biggest of L`, `smallest of L`, postfix `L added together`, `L reversed`, and `L is empty` / `is not empty` / `isn't empty` (`EXPR_LIST_OP` with `ListOp`), `L contains X` / `does not contain` / `doesn't contain` (`BINARY_CONTAINS` / `BINARY_NOT_CONTAINS`).
- **Slices** (`EXPR_SLICE` with `SliceKind`, all new lists): `items A to B of|in L` and `items A to end of|in L` (`at_range`: `items` + a position + `to`, or arithmetic to report), `first N items of|in L` / `last N items ...` (`at_first_last`), postfix `L without first item` / `without last item` (`without_phrases` in parse_expr.c), and `every ORDINAL item of|in L` (`at_every_nth`: `second`...`tenth` or a `TOK_ORDINAL`; step 0 is "There's no item at position 0."). The checker needs a list ("I can only take items from a list") and numbers ("The number of items has to be a number" for counts). Runtime: `es_slice_range` (both positions via `es_index`; backwards is "I can't take items %zu to %zu: the first position is after the last one."), `es_slice_end` (negative or fractional counts are errors; more than the length gives everything), `es_slice_without` (empty is "There's no first item"), `es_slice_every` (positions step, 2·step, ...), all through `es_list_pick` (text retained, inner lists copied).
- **Positions** (`parse_position`): a number, a name (never a call: `item n of L`), `it`, or a primary (`(n plus 1)`, text). `parser_at_item(p, after)` decides whether `item`/`first`/`last` start the form: `item` + number/text/`(`, or `item` + a name + one of `after` or an arithmetic word; `first`/`last` + `item` + one of `after`. Otherwise they're names (`add item to total`). Arithmetic after a position is "A position worked out with arithmetic needs parentheses."; `item of` is "Something is missing after "item"".
- **`every NAME in L REST`** (`EXPR_EVERY`): `parse_every` parses the name and a primary L, sets `Parser.pending_item` to an `EXPR_NAME` for the item, and parses REST with `parse_or`, whose first primary is the item (`parse_unary`/`parse_not` skip their prefixes while it's pending, so `- 1` subtracts). **`only L that are COMPARISON V`** (`EXPR_ONLY`): L is a primary; the condition is `BINARY(op, EXPR_IT, V)` from the `item_comparisons` table (`are` for `is`), or `are empty` / `are not empty` / `aren't empty`; `only` starts the form only before `NAME that` or `(`.
- **Kinds** (`parser_parse_kind`, shared with definitions): `number`, `text`, `yes or no`/`yes/no`, `nothing` (results only), `list of` + plural items `numbers`, `text`, `yes/no values`/`yes or no values`, `lists of ...`.
- **Sentences:** `add E to end of L` (`parser_finish_add_end` turns the STMT_CHANGE into STMT_LIST), `set item P of L to E`, `remove item P from L` (anything else after `remove` is "I expected "item" here"), `insert E at position P of L`, `sort L [by their length] [,] [smallest|biggest|shortest|longest first]` (length words need `by their length` and the other way round: two errors). `for each N in L:` is `LOOP_EACH` with `Stmt.loop.list` (parse_loop.c).
- **Checker** (`check_list.c`): `checker_require_list` unifies with "a list of" a fresh item variable ("<message>, but this is <kind>."); values put into a list unify with its items ("\"scores\" is a list of numbers, so it can't hold text." / "contain"); literal items unify with the first ("All the items of a list have to be the same kind: ..."). `sort`, `biggest of`, `smallest of` need numbers or text (by length: text or lists), `length of`/`is empty` text or a list (`DEFER_ONE_OF`). `added together` needs a list of numbers. `every`/`only` push a `Loop` whose `it` is the item; `every`'s name (and a for-each's) is made by `checker_make_item_name` ("You already made ...", note `each_NAME`), with `Symbol.loop_item` for the "is each item of the loop on line N" note. **`add E to X`** (STMT_CHANGE) where X is a list: if its items are numbers (or undecided) it's "\"scores\" is a list of numbers, so this could mean two things." with both phrasings; otherwise the checker sets `Stmt.change.append`. Lists can't be constants (consteval: "A constant can't use a list, because lists can change.").
- **Codegen** (`codegen_list.c`): a list is `EsList *`; variables `es_vl<depth><n|t|b>_NAME`. Runtime functions with a per-item-kind suffix (`cg_item_suffix`: n/b/t/l): `es_list_from(ES_ITEMS_X, n, (CTYPE[]){...})`, `es_list_new`, `es_get_X(line, L, ES_AT|ES_FIRST|ES_LAST, P)`, `es_put_X`, `es_add_X`, `es_insert_X`, `es_remove`, `es_contains_X`, `es_extreme_n/t`, `es_list_sum`, `es_list_length`, `es_list_is_empty`, `es_text_is_empty`, `es_list_reversed`, `es_list_copy`, `es_list_same`, `es_list_text`, `es_sort(L, ES_SORT_*)`. Parts are worked out in source order (`cg_ordered`); reading list contents counts as an effect in `cg_has_effects` (a call could change the list). `for each` stores `es_list_snapshot(L)` in a hidden variable `0eachN` of the loop's scope (dropped like any variable) and assigns the item (retained) each round; `LoopCode.item` makes `it` the item. `every`/`only` become `static EsList *es_everyN/es_onlyN(EsList *es_list, captured...)` in `Codegen.helpers` (emitted after prototypes, before functions): the captured variables (`collect`: every variable name the body reads, minus constants, functions, and names bound by inner `every`s) are borrowed parameters (`Codegen.captured`); the helper snapshots the list only if the body can call a function.
- **Runtime** (`runtime/es_list.h`): `EsList {header, items, len, cap, at}` (in `es_memory.h`, which frees it and releases counted items), growing by doubling (`es_list_reserve`, counted against `ES_MEMORY_LIMIT` first). Errors: "There's no item %s in the list: it has %zu item%s." / "it's empty." (hint "Positions start at 1: the first item is item 1." below 1), "The position of an item has to be a whole number, but it's %s.", "There's no %s item: the list is empty." (first/last/biggest/smallest), "I can't insert at position %s: the position has to be from 1 to %zu (the list has %zu item%s).". `es_list_snapshot` and `es_list_reversed` reuse a list nobody else holds. `es_list_copy` copies inner lists too. Sorting: `ES_DEFINE_SORT` instantiates a stable bottom-up merge sort (insertion sort for runs of 32, an already-ordered check per merge) per kind with the comparison inlined: numbers `<`/`>` (exact, not `es_close`), text `es_text_less` (bytes), and `(length, item)` pairs for `by their length`. `es_list_text` writes "72, 95 and 88", inner lists in parentheses, an empty list as empty text.
- **Sharing:** lists are references; `let b be a` shares, functions get the list itself, `a copy of` copies. A list can't contain a list of its own kind (the occurs check and kinds), so lists never form cycles.

### Types (implemented in `src/front/check_types.c`, `check_expr.c`, `check.c`)

- **Kinds:** `Type` in `ast.h` is an unsigned number: `TYPE_NUMBER`, `TYPE_TEXT`, `TYPE_YESNO`, `TYPE_NOTHING` (only a function's result), plus `TYPE_UNKNOWN` (undecided) and `TYPE_ERROR` (after a mistake; unifies with anything silently, so one mistake gives one message). A list type is its innermost kind plus `TYPE_LIST` (8) per level (`type_list_of`, `type_is_list`, `type_item`, `type_base`, `type_counted` for text and lists), so equal types are equal numbers. `ast_type_name(arena, type)` gives "a number", "text", "a yes/no value", "nothing", "a list of numbers", "a list of lists of text", "a list" (items undecided); `checker_kind(c, var)` is the same for a variable. The checker writes `Expr.type` for every checked expression and `param_types`/`result` for every function.
- **Inference engine** (`check_types.c`): `TypeVar` indexes into `Checker.types` (union-find with path halving; each root has a type and the line it was decided; a list root has type `TYPE_LIST` and an `item` variable). `types_new` (a list type gets an item variable), `types_new_list`, `types_item`, `types_of` (rebuilds the full `Type`), `types_unify` (checks `compatible` first, with an occurs check, then `join`s: the root that knows more wins, lists join their items; false if two different concrete kinds), `types_require` (unifies with a new variable of that kind), `types_line`. Every checked expression is recorded in `Checker.typed`. `types_defer` queues operators whose meaning depends on kinds (`DEFER_AND`: logical for yes/no, join with text, a number side forces the other to text, a list joins text and nothing else; `DEFER_ORDER`: numbers or text; `DEFER_TO_NUMBER`: number or text; `types_defer_one_of`/`DEFER_ONE_OF`: one of the `KIND_` bits, reported as "<message>, but this is <kind>."), trying to settle them at once. `types_finish(c, first_error)` settles the rest in a fixpoint loop, reports undecided inputs/results (innermost kind undecided: "I can't tell what kind of value "x" is." / "… gives back.", suggesting `(a number)` or, for a list, `(a list of numbers)`) **only if no other error was reported**, then writes kinds into the AST.
- **Rules** (`check_expr.c`; messages keep the old runtime wording, with carets): arithmetic needs numbers (`checker_arithmetic`, also used for change statements with the variable on the left); `or`/`not`/conditions need yes/no; equality unifies both sides ("I can't compare a number with text."); ordering unifies and defers; `followed by`/`as text` give text; `length of` and file names need text; `nothing` as a value is "\"nothing\" can't be used as a value."; constants have the kind of their value. `checker_require(c, var, type, at, message, hint)` reports "MESSAGE, but this is KIND.".
- **Variables** (`check.c`): `Symbol.var`; `let` shares the value's variable; `set`/`ask`/`read file` into an existing variable must match ("\"total\" is a number (made on line 1), but this is text."); loop numbers are numbers; a for-each item has the list's item kind. `Loop.it` is the kind of `it` (a number, or the item).
- **Functions:** `Function.params`/`result` are type variables, from the annotations or fresh; a function with no `give back E` anywhere in its body (`gives_value`) gives back nothing. Calls unify each argument with its input ("\"halve\" needs a number for \"x\", but this is text." with "\"x\" is a number because of line N…" or the definition when it was written down). A nothing-function used as a value is "… doesn't give back a value, so it can't be used here." `give back E` unifies with the result ("… gives back text (since line N), but this is a number."), a bare `return` in a value function and a value in a `giving back nothing` function are errors.
- **Order:** `check_program` checks top-level constants, then top-level function bodies, then the rest of the program, then `types_finish`, then `diag_sort(diag, first_error)` puts the checker's errors into source order. Because constants are checked before the main program, `lookup_constant` finds variables through `checker_find_later`, and a function's "only sees its own inputs" note uses `later` too.

### Runtime semantics (implemented in `runtime/es_memory.h` and `runtime/es_runtime.h`)

- **Three files**, embedded in this order: `es_memory.h` (text and list objects and reference counting, the memory checks, start/finish, output, errors), `es_runtime.h` (text, numbers, comparisons, functions, loops, files and input) and `es_list.h` (lists; see "Lists"). Each later file includes the one before only when its guard isn't defined, so each also works on its own in an editor.
- **No tagged value:** the checker knows every kind, so generated code uses `double`, `bool`, `EsText *` and `EsList *`, and the runtime never checks kinds. Runtime errors are only what kinds can't catch: dividing or `mod` by zero, a count step that isn't more than zero, a times count that's negative/not whole/too big, `as a number` on text that isn't a number, file errors, `es_no_result`, recursion depth, and list positions (see "Lists").
- **Memory: reference counting for text and lists** (full description in `docs/memory.md`). Every heap object starts with `EsObject {kind, refs}`: text (`ES_OBJ_TEXT`, its characters right after the `EsText {header, len, chars}`) and lists (`ES_OBJ_LIST`, `EsList`, owning the text and lists in it). `es_object_release` is the generic release; `es_list_retain`/`es_list_release`/`es_list_set`/`es_list_drop` are the list versions of the text functions, and an empty list variable holds the immortal `es_no_list`. `refs == ES_IMMORTAL` means never counted or freed: text literals (codegen's `static EsText es_sN`), text constants, and the runtime's own words (`ES_STATIC_TEXT`, e.g. `es_lit_yes`, `es_empty_text`). `es_retain`/`es_release` take `EsText *` (never NULL: empty variables hold `&es_empty_text`); `es_free_object` frees by kind and is kept out of line (`noipa` on GCC, `noinline` on Clang), because otherwise GCC sees `free()` reachable from releases of static immortal text and warns `-Wfree-nonheap-object`. `es_set(&slot, t)` stores and then releases the old text; `es_drop(&slot)` releases and stores `&es_empty_text`.
- **Ownership convention (keep it consistent):** every text or list expression yields an owned value. Reading a variable retains it; literals and constants are used directly. Runtime functions that take `EsText *` or `EsList *` **consume** them (release, or return them: `es_join` returns the other side when one is empty), except `es_push_*` and the "only looks" helpers. Functions own their text and list arguments. Numbers and yes/no own nothing: no retain/release is ever generated for them. `es_text_new(len, &chars)` and `es_text_copy` make heap text with a count of 1.
- **Memory checks:** live objects and bytes are always counted (`es_memory`). `ES_DEBUG_MEMORY=1` makes `es_finish(true)` fail with exit 70 ("Memory check: N value(s) was/were still alive at the end of the program (B bytes).") if anything is alive. `ES_MEMORY_LIMIT=N` fails with exit 70 ("... more than N bytes of text and lists alive at once.") as soon as live heap bytes would exceed N (`es_memory_take`, checked before `malloc` and before a list grows, so nothing is left unreferenced for LeakSanitizer). Runtime errors exit without the check.
- **Printing** (`es_number_text`, `es_yesno_text`, `es_list_text`): whole numbers with `|x| < 1e15` print with `%.0f` (no decimals, never `-0`). Other numbers use `%.15g`, so `0.1 plus 0.2` prints `0.3`. NaN prints `not a number`; infinities print `infinity` and `-infinity`. yes/no prints `yes`/`no`. Lists print as "72, 95 and 88".
- **Numbers:** `+ - *` and unary `-` are plain C. `es_div`/`es_mod` fail with "You divided by zero."; `es_mod` uses the integer remainder for whole numbers below 9e15 (same as `fmod`, whose −0 results aren't observable) and `fmod` otherwise.
- **Text:** `es_join(a, b)` (for `followed by` and text `and`), `es_say`, `es_length` (UTF-8 characters, via `es_text_chars`), `es_text_is_empty`, `es_text_number(line, t)` for `as a number` (optional surrounding spaces, an optional `-`, digits, optionally `.digits`; otherwise "I can't turn "…" into a number.").
- **Comparisons:** numbers are equal when `a == b` or `|a - b| <= 1e-12 * max(|a|, |b|)` (`es_close`), so `0.1 plus 0.2 is 0.3` is yes. `es_number_order` returns 0 for numbers that are `es_close`, so `<=`/`>=` agree with equality and `<`/`>` are false for them. Text: `es_text_same`, `es_text_order` (bytewise), both consuming. yes/no: C `==`/`!=`.
- **Files:** `es_write_file` writes the text plus `\n`. `es_read_file` strips one final `\n` (and a `\r` before it). I/O errors read like `I couldn't read the file "x": it doesn't exist.`
- **Input:** `es_ask` prints its prompt without a newline, reads one line (stripping `\n`/`\r\n`), and gives empty text at end of input.
- **Shell support:** all program output goes through `es_out`, which drops the first `ES_SKIP_OUTPUT` bytes. With `ES_ANSWERS` set, `es_read_answer` replays answers from that file before reading stdin, appends new ones, and stdin is unbuffered. Both are set only by the shell.
- **Ending:** `main()` ends by releasing its text variables and calling `es_finish(true)` (flush, then the memory check). `stop the program` in `main()` is `goto es_end` (that same ending); inside a function, the function releases its own text and calls `es_stop()`, which runs `es_finish(false)` (no check: the callers' values are out of reach) and exits 0. Runtime errors flush stdout, print `Line N: message` (plus an optional hint line) to stderr, and exit 1 with no clean-up.
- **Loops:** `es_count_start(line, from, to, step, has_step, down)` checks step > 0, picks the direction (`down` forces -1 and is empty when from < to), and works out `last` (the last round's index, adjusted ±1 for rounding; `LLONG_MAX` for an endless or NaN count) and `at_end` (the first round whose number is `es_close` to `to`; from there to `last` the number is `to`). `es_count_value(&c, i)` is `from + direction * i * step` (from scratch each round, no drift) or `to`. This matches the old round-by-round rule exactly (checked against it on 20,000 random counts). `es_times(line, n)` needs a whole number ≥ 0 (and ≤ 9e18).
- **The runtime is header-only** (all `static`) and silences `-Wunused-function` and (Clang, GCC 12+) `-Winfinite-recursion` (endless recursion is an EasyScript runtime error) with pragmas. Generated programs must compile cleanly under `-std=c11 -Wall -Wextra -Wpedantic` with both GCC and Clang; `make test` checks this (`strict-c/generated-programs`). Fix warnings in the runtime's structure, not with more pragmas.

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
- **Audits** (end of `tests/run.sh`, need `python3`, skipped otherwise):
  - `tests/tools/error_coverage.py` checks that every error format string in `src/` and `runtime/` (`diag_error`, `parser_error`, `es_fail`, consteval `fail`, `usage_error`, `fprintf(stderr, "…")`, found by `tests/tools/messages.py`) matches some expected `.err` file or `expect_failure` line in `run.sh`. Messages that need the OS to fail are in its `EXEMPT` table with a reason.
  - `tests/tools/vocab_coverage.py` checks that every phrase the parser accepts (statement table, phrase tables, `parser_at_word`/`parser_match_words`/`parser_expect_phrase` literals, NULL-terminated word lists, operator words) appears as whole words in a code span of `docs/vocabulary.md`.
  - **Adding an error message means adding a test that produces it; adding a phrase means documenting it.**
- **Message style** (audited at v0.1.0): every error, including CLI and system errors, is a plain first-person English sentence ("I couldn't open the file "x": it doesn't exist.", "I don't know the command "x"."). No "Error:" prefixes, codes or jargon. Notes give a concrete fix or example.
- **README sync checks:** the end of `tests/run.sh` checks that the README's "A first program" block equals `examples/programs/first_program.es`, and its "A bigger example" block equals `examples/programs/taste.es` (which runs and has a `.out`).
- CLI behavior (modes, usage errors, no files left in the cwd, temp-dir cleanup) is checked at the bottom of `tests/run.sh`. Add a check there when changing the CLI.
- `examples/programs/NAME.es` plus `NAME.out` and optional `.in`/`.err` (run like `tests/run`), `examples/lexer/NAME.es` plus `NAME.tokens`, and `examples/parser/NAME.es` plus `NAME.ast` (both with an optional `.err`) back the docs. `tests/run.sh` fails if any other folder or a loose `.es` appears under `examples/`.
- `tests/run.sh` compiles every generated program with `-Werror` (strict C, see Current state). It runs the unit binary, then the run/errors/tokens tests and the examples (each in an empty scratch directory, via the shared `check_run`/`check_errors`/`check_dump` functions), then the CLI checks. It prints PASS/FAIL per test plus one summary and exits 1 if anything fails. The `ES` and `UNIT` env vars choose which binaries are tested.
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
- `docs/performance.md` has the latest full table (machine, tool versions, date) and a **History** table with one EasyScript row per milestone. **Never edit past history rows' results; only add new rows.** A benchmark added later gets a new column, with "(none)" in the rows from before it existed. At each milestone, run `make bench`, replace the "Latest results" table, add a history row, and update the README's compact Performance table to match.

## Keeping this file current

**Every change that affects layout, build, or behavior must update the "Current state" and "Build" sections below in the same commit.** Those sections describe the repo as it is, not as it should be.

## Current state of the repo

**Phase 1 is complete: v0.1.0 (tagged `v0.1.0`, 2026-10-10).** Programs in the sentence syntax compile to C and run, including constants, `if`/`otherwise` blocks, loops and top-level functions. **Phase 2 is in progress.** Step 13, memory management with reference counting, is done: text is freed as soon as nothing uses it, so loops run in flat memory. Step 14, static types with inference, is done: the checker knows every value's kind, kind mistakes are compile errors, and generated code uses plain C types. Step 15, lists, is done: lists of one kind, shared by reference, with 1-based positions, `for each`, `every`/`only`, totals, a stable inlined merge sort, and nesting. Still to come from Phase 1: runtime errors with source lines, `file … exists`.

- **Layout:** `src/main.c` is the driver, and `src/shell.c` the interactive shell. `src/common/` holds arena, util, diag and ast. `src/front/` holds lexer, parse_util, parse_expr, parse_value, parse_stmt, parse_if, parse_loop, parse_func, parse_list, consteval, check, check_expr, check_types, check_const and check_list (with private headers `parse_internal.h` and `check_internal.h`). `src/back/` holds codegen_c, codegen_expr and codegen_list (with `codegen_internal.h`). `runtime/es_memory.h`, `runtime/es_runtime.h` and `runtime/es_list.h` are the runtime, and `tools/embed.c` (`embed OUTPUT NAME INPUT...`, joining the inputs in order) is a build tool.
- **Pipeline** (`generate_c` in `src/main.c`, shared by `run`/`build`/`emit`):
  1. Lex and parse.
  2. If there were no errors, run `check_program`.
  3. If there are any diagnostics, print them all and exit 1.
  4. Otherwise run `codegen_c` and write `program.c` to the temp dir.

  `cc -O2 program.c -o OUT -lm` is run via `fork`/`execvp`, and `run` returns the program's exit status. With `ES_SANITIZE=1` (set by `make test-debug`) the program is built instead with `-O1 -fno-omit-frame-pointer -fsanitize=address,undefined -fno-sanitize-recover=all` (no `-g`: on macOS it leaves `.dSYM` folders behind).
- **Stage boundaries:** these now follow the target design. Codegen trusts the checker and reports nothing; it asserts if it ever sees `EXPR_ERROR` or an `it` with no counting loop around it. `src/main.c` still has file-scope `arena` and `temp_*` pointers for its `atexit` cleanup (the only global state in the compiler).
- `src/main.c`: the CLI (the shell is in `src/shell.c`). `run FILE` / `build FILE -o OUT` / `emit FILE` / `tokens FILE` / `ast FILE`, and `help`. Unknown commands and bad arguments print usage and exit 2.
  - Each invocation creates one `mkdtemp` directory under `$TMPDIR` (or `/tmp`), holding `program.c`, `program` and `session.es`. `atexit` removes it.
  - **The shell** (no arguments) is `src/shell.{c,h}` (`shell_run`). It keeps the accepted source in a `StrBuf`. Per entry (a line, or a `:`-ending line plus following lines until a blank one) it writes `session.es` and runs `argv[0] run session.es` with stdout piped through it, setting `ES_SKIP_OUTPUT` (bytes already shown) and `ES_ANSWERS` (`answers.txt` in the temp dir). It keeps the entry only on exit 0 (and adds the bytes printed to `shown`); otherwise it truncates the answers file back and prints "(That line/block wasn't kept.)". It reads stdin one byte at a time so it never reads lines meant for the program. `exit`, `quit` or `stop the program` leaves.
- `src/common/arena.{c,h}`: arena allocator (`arena_new`, `arena_alloc` returns zeroed, aligned memory, `arena_strdup`, `arena_sprintf`/`arena_vsprintf`, `arena_free`). It grows in 64 KiB blocks and gives oversized requests their own block. All compiler memory comes from here.
- `src/common/util.{c,h}`: `StrBuf` (`sb_*`, including `sb_append_quoted`), `Vec(T)` with `vec_push`/`vec_last`/`vec_pop`, `edit_distance` (optimal string alignment, so an adjacent swap counts 1), and `PRINTF_LIKE`.
- `src/common/diag.{c,h}`: opaque `Diag` made by `diag_new(arena, source, len)`. Report with `diag_error(d, span, fmt, ...)`, then `diag_note(d, fmt, ...)` for help lines attached to the latest error. `Span` is a byte `{offset, length}`. `diag_line` gives the 1-based line and `diag_count` the number of errors. `diag_render` (to a `StrBuf`) and `diag_print` (to a `FILE *`) output every error in report order, separated by blank lines: `Line N: message`, then the source line indented 4 spaces, then carets under the span, then the notes. Carets are clamped to the first line of the span, with at least one. Tabs are copied into the padding, UTF-8 is counted per character, and a trailing `\r` is dropped.
- `src/common/ast.{h,c}`: `Expr` (kinds ERROR, NUMBER (text plus `is_decimal`), TEXT, BOOLEAN, NOTHING, IT, NAME, CALL, LENGTH, FILE_CONTENTS, UNARY, BINARY, CONVERT), `Stmt` (LET, SET, CHANGE + `ChangeOp`, SAY, ASK, WRITE_FILE, APPEND_FILE, READ_FILE, STOP, IF with `IfBranch`es, LOOP, BREAK, CONTINUE, FUNCTION, RETURN, CALL, CONSTANT (with `ConstValue folded`), each with a `verb` span), `Name {text, pos}`, and `Block` (`Vec(Stmt *)`). Every node has a `SourcePos`. It also has `ast_dump_block` for `easyscript ast`.
- `src/front/lexer.{c,h}`: `lex(arena, diag, source, len)` returns a `TokenList` that always ends with `TOK_EOF`; tokens carry a `filler` span. Also `token_kind_name` and `tokens_dump(tokens, source, out)`.
- `src/front/parse.h`, `parse_internal.h`, `parse_util.c`, `parse_expr.c` (operator levels), `parse_value.c` (primaries: literals, names, calls, groups, prefix forms), `parse_stmt.c`, `parse_if.c`, `parse_loop.c`, `parse_func.c`, `parse_list.c`: the parser. Shared statement helpers have `parser_` names (`parser_new_stmt`, `parser_parse_name`, `parser_expect_word`, `parser_expect_phrase`, `parser_looks_like_assignment`, `parser_parse_block`); see the grammar sections above. The if parser was fuzzed with 2,500 inputs under ASan/UBSan (no crashes or hangs; all 74 accepted programs compiled with `-Werror`). The expression and statement parsers were each fuzzed with 3,000 inputs under ASan/UBSan, with no findings.
- `src/front/check.{c,h}` (statements, scopes, the program) plus `check_expr.c` (expressions, calls, kind rules), `check_types.c` (the inference engine; see "Types"), `check_const.c` (constants) and `check_list.c` (list values and sentences; see "Lists"), sharing `check_internal.h` (the `Checker`, `Symbol`, `Loop`, `Function`, `Constant`, `TypeSlot`, `Deferred`, `TypedExpr` structs and the `checker_`/`types_` helpers: `checker_find_made`, `checker_find_later`, `checker_find_function`, `checker_find_constant`, `checker_report_unknown`, `checker_clashes`, `checker_function_header`, `checker_check_expr`, `checker_check_call`, `checker_require`, `checker_arithmetic`, `checker_kind`, `checker_make_item_name`, `checker_require_list`, `checker_check_list_expr`, `checker_check_contains`, `checker_check_list_stmt`, `checker_add_to_list`, `checker_collect_constants`, `checker_check_constant`, `checker_closest_constant`, `types_*`): `check_program(arena, diag, program)`. A `Checker` context holds `made` (names made so far, with lines) and `later` (all names the program makes, for "You make "x" later, on line N").
  - **Making names:** `let` makes a name; making it twice is "You already made …" with a "set" hint. `ask` and `read file … and call it` make the name if it's new and silently reuse it otherwise. `set`/`change`/arithmetic statements and `EXPR_NAME` need an existing name.
  - **Unknown names:** "I don't know anything called "x"." plus one note:
    - "Did you mean "total"? You made it on line 1." for the single closest name (OSA distance, limit 1 for names of 3 or fewer characters, otherwise 2)
    - a list when several are equally close
    - "You make "x" later, on line N."
    - otherwise "Make it first, like "let x be 0"."
  - **Loops:** a `Loop` stack (`has_it` for count and times loops). `EXPR_IT` needs some enclosing loop with `has_it`; otherwise "it doesn't refer to anything here", with a note that's different inside a non-counting loop and outside any loop. `STMT_BREAK`/`STMT_CONTINUE` outside any loop are errors. Loop values (from/to/step/times/condition) are checked in the outer scope. The count variable is made in the loop's scope; if the name exists, it's an error ("This loop calls each number "number", but you already made…" when defaulted). The loop body ends the scope as "the loop"; a loop variable used afterwards gets "is the number of the loop on line N, so it only exists inside that loop" (`Symbol.loop_number`).
  - **Scopes:** each if branch body is a scope (`check_block`): names made inside are dropped from `made` at its end and recorded in `ended` with the if's line, so a later use says "You made "x" inside the "if" on line N, so it only exists inside that block." A name that exists outside can't be made again inside ("You already made …"); different blocks can make the same name.
- `src/back/codegen_c.{c,h}` (statements, loops, functions, constants, the program) plus `codegen_expr.c` (names, literals, expressions) and `codegen_list.c` (lists, `for each`, `every`/`only` helpers; see "Lists") sharing `codegen_internal.h` (the `Codegen`/`LoopCode`/`VarCode`/`NameScope`/`TextPool`/`HelperPool` structs and the `cg_` helpers: `cg_emit_expr`, `cg_emit_text` (any value as owned text), `cg_has_effects`, `cg_ctype`, `cg_retain`/`cg_release`/`cg_set`/`cg_drop`/`cg_empty` (per counted type), `cg_operand`, `cg_ordered`, `cg_is_constant`, `cg_emit_var`, `cg_append_prefixed`, `cg_append_var`, `cg_append_c_string`, `cg_static_text`, `cg_declare_name`, `cg_new_temp`, `cg_find_function`, `cg_emit_declarations`, `cg_item_suffix`, `cg_emit_list_expr`, `cg_emit_contains`, `cg_emit_each_head`, `cg_list_stmt`): `codegen_c(arena, program, out)` writes the embedded runtime (the `es_runtime_source` byte array), the static text objects `es_sN`, constants, prototypes, the `every`/`only` helpers, functions, and `int main(void)`. **No tagged value:** every expression is C of its kind's type (`cg_ctype`: `double`, `bool`, `EsText *`, `EsList *`, `void`), using `Expr.type` from the checker; codegen asserts on an undecided kind.
  - **Names:** variables are C locals of `main()` or of their function, mangled by kind and name: `es_vn_` (number), `es_vt_` (text), `es_vb_` (yes/no), `es_vl<depth><n|t|b>_` (lists) plus the name, with `_` written as `__` and `'` as `_q`, so blocks side by side can make the same name with different kinds (`Codegen.vars` holds name+kind pairs). Locals keep their addresses from escaping, so the C compiler keeps numbers in registers.
  - **Ownership** (text and lists, `type_counted`; see "Runtime semantics" and `docs/memory.md`): reads are `es_retain(es_vt_x)` / `es_list_retain(...)`; stores are `es_set` / `es_list_set`, other stores plain `=` (change statements: `es_vn_x = es_vn_x + AMOUNT;`, or `es_div`). `Codegen.scopes` is a stack of `NameScope`s of text and list variables, one per block (`[0]` the top level); `cg_declare_name` records such a variable in the innermost scope unless it's a parameter (or a helper's captured variable) or already in some scope. A block ends with `es_drop`/`es_list_drop` for its variables. `stop the loop`/`skip this one` drop every scope from the innermost through the loop's body (`LoopCode.scope`). `stop the program` in `main()` is `goto es_end;` (the label is emitted only if used, `Codegen.stops`), and the end of `main()` drops every text and list variable and calls `es_finish(true)`; in a function it drops the function's text and lists and calls `es_stop()`.
  - **Operators:** `+ - *` and unary `-` are C; `/` and `mod` are `es_div`/`es_mod(line, a, b)`; `and`/`or` between yes/no are `&&`/`||` (short-circuit, left to right); `and` with type text and `followed by` are `es_join(text(L), text(R))`; equality is `es_close` (numbers), `es_text_same` (text) or `==` (yes/no); ordering is `es_number_order`/`es_text_order` compared with 0; `as text`/`say`/`ask` prompts/written text go through `cg_emit_text` (`es_number_text`, `es_yesno_text`); `as a number` on text is `es_text_number(line, t)`. Number literals always print with a `.` or exponent (`%.17g`, `HUGE_VAL` if infinite), so C never does integer arithmetic.
  - **Temporaries:** typed `es_tN` (`Codegen.temps` holds each one's kind) declared at the top of `main`/the function. Only when both operands of an operation have effects (`cg_has_effects`: calls, file reads, `/`, `mod`, `as a number` on text, a zero-input function name) does the left one go into a temporary: `(es_tN = L, OP(es_tN, R))`, keeping left-to-right order. Counts store from/to/step in temporaries once; `give back` stores its value before the drops.
  - **Literals:** text becomes a static immortal object (`static EsText es_sN = {{ES_OBJ_TEXT, ES_IMMORTAL}, LEN, "..."};`, collected in a `TextPool` shared with the functions) used as `&es_sN`; its string uses octal escapes and `\?`.
  - **Lines:** each statement gets a `/* line N */` comment, and runtime calls that can fail receive the node's line.
  - **if:** `if (C) { … } else if (C2) { … } else { … }`, with `Codegen.depth` indenting nested blocks.
  - **Loops:** each loop is wrapped in `{ }`. A count is `EsCount es_cN = es_count_start(line, from, to, step, has_step, down)` and `for (long long es_iN = 0; es_iN <= es_cN.last; es_iN++) { es_vn_var = es_count_value(&es_cN, es_iN); … }`. Times is `long long es_nN = es_times(…); for (es_iN = 1; es_iN <= es_nN; es_iN++)`. While/until use `while ([!]C)`, and forever is `for (;;)`. Break and continue are C `break;`/`continue;` (after the drops). `Codegen.loops` resolves `it` to the innermost count's variable or `((double)es_iN)` for times. Names made in blocks are hoisted like the rest; the checker guarantees they're only used in their block, and the block's end drops text.
  - **Checked:** fuzzed once, with 171 mutated programs that passed the checker. All compiled with `-Wall -Wextra -Werror`.
- `runtime/es_memory.h`, `runtime/es_runtime.h`: see "Runtime semantics" above.
- `Makefile`: `CC = cc`, `-std=c11 -Wall -Wextra -Isrc`, `LDLIBS = -lm` on every link (`consteval.c` uses `fmod`/`fmax`; Linux needs it).
  - **Source groups:** `COMMON_SRC`, `FRONT_SRC` (also linked into the unit tests), `BACK_SRC`, and `GEN_RUNTIME`, which is `build/gen/es_runtime_embed.c` generated by `build/tools/embed` from `RUNTIME` (`runtime/es_memory.h runtime/es_runtime.h runtime/es_list.h`, in that order). Never edit generated files.
  - **Builds:** objects go under `build/release/` (`-O2`) or `build/debug/` (ASan/UBSan, `-fno-sanitize-recover=all`), with `-MMD` dependencies.
  - **Targets:** `all`, `test`, `debug`, `test-debug` (the sanitizer suite with `ES_SANITIZE=1`, so generated programs are sanitized too, then leak checking: LeakSanitizer on Linux covers compiler and programs; on macOS `tests/leaks.sh` runs every test input, generated program (built without sanitizers, including `tests/memory/`, except programs with a `.err`, which exit without clean-up by design, so `leaks` results for them depend on register allocation) and shell session under `leaks --atExit`), `bless`, `bench`, and `clean`.
  - Zero warnings in both modes, with GCC and Clang. `src/main.c` defines `_XOPEN_SOURCE 700` and `_DARWIN_C_SOURCE`.
- **CI:** `.github/workflows/ci.yml` (GitHub Actions) runs `make`, `make test` and `make test-debug` on every push, on `ubuntu-latest` (GCC; LeakSanitizer) and `macos-latest` (Clang; `tests/leaks.sh`). The runners install Python 3, Go, and on Linux GNU `time`, and set `vm.mmap_rnd_bits=28` so AddressSanitizer starts. Verified locally with GCC 16 in Docker (Linux) and Clang (macOS).
- `tests/`:
  - 40 unit tests
  - 55 AST tests (32 valid, including `list_slices`, including `if_*`, `loop_*`, `func_*` (with `func_kinds` for written-down kinds, list kinds and a zero-input result), `const_define`, `list_forms` (every list form, and list words used as names); 23 `err_*`, including `err_list_slices`, `err_if_header`, `err_if_otherwise`, `err_loop_header`, `err_func_header`, `err_func_kinds`, `err_const_header`, `err_list_forms` (every list parse error))
  - 23 token tests (15 valid, including `ordinals`; 8 `err_*`, including `err_ordinals`)
  - 68 run tests (42 programs, including `list_slices`, `list_basics`, `list_changes`, `list_sharing`, `list_loops`, `list_nested` (every list form, sharing vs copying, nesting, snapshots in loops, `every`/`only` with captured variables, list kinds inferred through functions), including `types_inference` (kinds from use, calls, `and`, ordering, recursion, sibling blocks), `loop_count_edges`, `memory_ownership` (every way text is shared, handed over, or replaced), `stop_releases` and `stop_in_function`, including `if_*`, `loop_*`, `func_*`, `const_basic`, `constant_matches_runtime`, covering numbers, number equality with tolerance, text (including `followed by` precedence), logic and short-circuiting, every comparison, variables, files, `ask` with and without input, `stop`, sentences and synonyms; plus 26 `err_*` runtime-error tests (5 `err_slice_*`): only what kinds can't catch (dividing by zero, left-to-right error order, count steps, times, `as a number`, files, a function with no result, an error's line inside a loop, endless recursion, and 7 `err_list_*`: past the end, item 0, a fractional position, an empty list, first/biggest of an empty list, inserting past the end))
  - Every run test and example runs with `ES_DEBUG_MEMORY=1`, so a program that ends with heap text alive fails.
  - 9 memory tests (`tests/memory/`, including `list_slices`: loops of up to a million rounds building text (joins, temporaries, functions, rolling text, early exits, `ask` and file reads), and `list_loops` (200,000 rounds making, changing, copying, mapping and dropping lists), run with `ES_DEBUG_MEMORY=1 ES_MEMORY_LIMIT=100000`, and peak RSS under 16 MB where `/usr/bin/time` can tell (not under `ES_SANITIZE=1`); `err_over_limit` must hit the limit, exit 70). `bless.sh` handles them (`bless_run DIR 70`).
  - A control check (`memory/check-catches-a-leak` in `run.sh`): a program with one `es_release` taken out of `es_say` must fail the memory check.
  - 4 shell sessions (`tests/shell/NAME.in` plus `.out`/`.err`: output not repeated, blocks, dropped lines, answers replayed and cut back after a failure, leaving)
  - 92 compile-error tests: 6 `list_*` (`list_slice_errors`, `list_type_errors` with every list kind error, `list_add_two_meanings`, `list_constant`, `list_undecided`, `list_item_names`), 38 `type_*` kind errors (the 25 that used to be runtime errors, plus inference, conflicting calls, written-down kinds, results, `nothing`, undecided kinds) and checker messages including block scopes, loop numbers, `it`, stop/skip outside loops, unknown functions, argument counts, `give back` outside functions, functions in blocks, function scopes and name clashes, every constant rule, plus one parse error)
  - 49 examples, 2 README sync checks, CLI checks, and `strict-c/generated-programs` (every generated program from tests/run, tests/memory, examples/programs and benchmarks compiled with `cc -std=c11 -Wall -Wextra -Wpedantic -Werror -O2`; skipped under `ES_SANITIZE=1` since the C is the same)
- **Benchmarks** (after step 15): fib 0.23 s (C 0.07 s; the recursion guard plus the tolerant comparison stop clang turning the recursion into a loop), count 0.05 s, nested 0.11 s, text 1.6 MB, sort 0.06 s (C `qsort` 0.08 s, Go 0.08 s, Python 0.31 s). `benchmarks/` has `fib` (fib 38), `count` (to 100 million), `nested` (10,000 × 10,000 with `if`/`mod`), `text` (10,000 joins) and `sort` (a million Park–Miller numbers), each in `.es`, `.py` and `.go`, plus `.c` except `text`. See "Performance records".
- **Docs:**
  - `README.md`: front page with "A first program" (runs, and is tested), "A bigger example" (a loop and a function; runs, and is tested), honest status, roadmap with Notebook, design rules, build/CLI, pipeline, links, and "License: TBD".
  - `docs/language-guide.md`: chapters 0–10 available (9 is Types, with the annotation design and the alternatives considered; 10 is Lists), except `file X exists` (coming soon).
  - `docs/vocabulary.md`: statuses Available / Coming soon.
  - Also `docs/errors.md` (lexer, parser, checker and runtime errors), `docs/memory.md` (how memory works, for users and contributors), `docs/performance.md` (benchmarks and history), `docs/architecture.md`, `docs/roadmap.md`, `CHANGELOG.md`, `CONTRIBUTING.md` and `examples/README.md`.
- **Examples:**
  - `examples/programs/` (24 runnable): `grade_statistics` and `top_scores` (lists), `kinds` (inference and written-down kinds), `hello`, `first_program`, `taste` (README), `variables`, `text`, `logic`, `decisions`, `loops`, `logan` (old/logan.code ported), `tax` (a constant used by a function), `factorial`, `fibonacci`, the larger `fizzbuzz`, `guessing_game` (+ `.in`), `notes`, `hanoi`, `receipt`, `million_lines` (a million texts in flat memory), `ask_name` (+ `.in`), `files`, `err_divide_by_zero`.
  - `examples/parser/` (16): syntax-tree demos, many using names they never make, plus `err_otherwise_misplaced`.
  - `examples/lexer/` (9).
- No license has been chosen; the user will pick one.
- `old/`: the 2024 prototype `.code` files. Historical only; they no longer compile. `old/logan.code` is ported as `examples/programs/logan.es`.
- `.gitignore` covers build outputs (`build/`, `easyscript`, stray `*.o`/`*.d`), `myfile*`, `.idea/` (CLion), and `.vscode/` (no longer tracked). `.gitattributes` marks `tests/tokens/**` as `-text`.

## Build

```sh
make                                  # builds ./easyscript (plus build/tools/embed and build/release/unit_tests); no warnings expected
make test                             # builds, then runs tests/run.sh (unit, tokens, ast, run, errors, examples, strict C, docs, CLI)
make debug                            # ASan/UBSan build in build/debug/
make test-debug                       # full suite against the sanitizer build (programs sanitized too), then the leak check
make bless                            # rewrite golden files from current output, then show git diff --stat
./easyscript run FILE.es              # compile + run (generated C/binary go to a temp dir)
./easyscript build FILE.es -o OUT     # compile to an executable at OUT
./easyscript emit FILE.es             # print the generated C
./easyscript tokens FILE.es           # print the lexer's tokens
./easyscript ast FILE.es              # print the syntax tree
./easyscript                          # interactive shell (exit or quit to leave)
make clean
```
