# Ideas

Possible features that aren't planned yet. Nothing here works today, and the wording is only a sketch. When an idea is picked up it moves to the [roadmap](roadmap.md), and it must follow the [design principles](design-principles.md).

## A deep copy

`a copy of X` copies the lists and maps inside X but shares the records inside it (see [design principles](design-principles.md#8-sharing-and-copying)). A second phrase could copy everything, records included:

```
let backup be a full copy of orders
```

Open questions: the exact words (`a full copy of`, `a deep copy of`), and what to do with records that point to each other in a circle (copy the circle once, keeping its shape).
