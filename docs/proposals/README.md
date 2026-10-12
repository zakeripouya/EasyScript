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
