#ifndef ES_VERSION_H
#define ES_VERSION_H

// The compiler's version, and the editions of the language it knows: a
// program can say "this program uses EasyScript 2026." on its first line.
// A later edition can change what a wording means, but only for programs that
// ask for it (docs/design-principles.md, "Editions").
#define EASYSCRIPT_VERSION "0.2.0"
#define EASYSCRIPT_EDITION 2026  // the current edition, and the only one so far

#endif
