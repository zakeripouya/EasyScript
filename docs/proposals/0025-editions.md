# 0025: editions

**Status:** Built
**Kind:** Addition. A sentence starting with `this program uses` was an error before.

## The phrase

```
this program uses EasyScript 2026.
```

An optional first line that records which edition of EasyScript a program is written for. Capitals don't matter (`EasyScript`, `easyscript`).

- **No such line:** the program uses the current edition, 2026.
- **A known edition:** the program is read the way that edition reads it. Today there is one edition, 2026.
- **An unknown edition** is an error naming the ones this compiler knows:

  ```
  Line 1: I don't know EasyScript 2031. This version of EasyScript knows the 2026 edition.
      this program uses EasyScript 2031.
                                   ^^^^
  Write "this program uses EasyScript 2026." or leave the line out.
  ```

- **Anywhere but the first sentence of a file** it's an error: "The edition goes on the first line of the program." In a program of several files, each file can say its edition on its own first line.

What it means in core-language terms: nothing at all for the 2026 edition; it's a note to later versions of the compiler.

## The promise

After v0.2.0 the core grammar is frozen per edition. A later edition (say 2027) can change what a wording means, but only for programs that say `this program uses EasyScript 2027.`; a program that says 2026, or says nothing and was written for 2026, keeps its meaning in every later version of the compiler. Additions (new phrases that were errors before) can come to every edition, because they can't change what a working program does. This is written down in [design-principles.md](../design-principles.md#editions) and the language guide.

## Checked against the design rules

- One meaning per word: `this program uses` only ever starts this sentence.
- Common words only, readable aloud: "this program uses EasyScript 2026" is a plain sentence.
- Distinct trigger words: `this` doesn't start any other sentence.
- Deterministic: the edition is written down or it's the current one; never guessed from the program.
- Errors: grade 8 or below, one concrete fix.

## Cognitive Dimensions notes

- Viscosity: lets the language change its wording later without breaking anyone, which keeps the freeze from being permanent.
- Hidden dependencies: the line is optional, so most programs never see it.

## Open questions

- What a program without the line means once a second edition exists: the current edition (as today), or the one it was written for. A program written before the line existed has no way to say which, so the second edition will need a rule (Rust reads such a program as its oldest edition). Decided when the second edition is made; until then there's nothing to decide.
