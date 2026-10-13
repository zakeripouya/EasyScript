# Proposals

Every new phrase, and every change to an existing one, starts here. [english-pass.md](../english-pass.md)'s design rules say so ("Reviewed before added"), and so does CLAUDE.md.

## How it works

1. Copy [TEMPLATE.md](TEMPLATE.md) to `NNNN-short-name.md` (the next free number, then a few words: `0001-file-doesnt-exist.md`).
2. Fill it in: the phrase, real examples, the design rules it was checked against, the Cognitive Dimensions notes, and whether it's an addition or a change.
3. Set its status to **Proposed**. It's discussed, then marked **Accepted** or **Declined** (with the reason, kept in the file).
4. Only an accepted proposal is built. The commit that builds it links the proposal, sets its status to **Built**, and does everything else a language change needs (vocabulary, guide, example, changelog, tests).

A doubt that comes up while building goes back into the proposal, not into the parser.

## Where ideas come from

- [english-pass-log.md's "Still not English"](../english-pass-log.md#still-not-english): lines from the examples that a non-coder would stumble on.
- [english-pass.md's Later list and Open questions](../english-pass.md#later).
- [ideas.md](../ideas.md).

## Proposals

- [0001: inputs in the middle of a definition's first line](0001-inputs-in-the-middle.md) (Built)
- [0002: yes/no fields as describing words, and one-line choices](0002-yes-no-fields-and-choices.md) (Built)
- [0003: adding with a condition, and values read like fields](0003-values-read-like-fields.md) (Built)
- [0004: the best group](0004-best-group.md) (Built)
- [0005: a field across a list, and counting a list field](0005-field-across-a-list.md) (Built)
- [0006: files in steps in a row, and "doesn't exist"](0006-files-in-steps.md) (Built)
- [0007: exact money](0007-exact-money.md) (Built)
- [0008: kinds of text with a rule](0008-kinds-of-text.md) (Built)
- [0009: wordings from the "Still not English" list](0009-still-not-english.md) (Built)
- [0010: text in quotes inside brackets](0010-quotes-in-brackets.md) (Built)
- [0011: put the text below in a file](0011-text-below-in-files.md) (Built)
- [0012: say X, indented](0012-indented.md) (Built)
- [0013: for each K and its V](0013-each-and-its.md) (Built)
- [0014: the file decides a column's kind](0014-file-decides-column-kind.md) (Built)
- [0015: how many Ls are there?](0015-how-many-are-there.md) (Built)
- [0016: "!" ends a sentence, and "please" after a step word](0016-exclamation-and-polite-steps.md) (Built)
- [0017: an answer used as a number is asked for as a number](0017-answers-used-as-numbers.md) (Built)
