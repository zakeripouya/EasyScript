note: "as a number" reads powers of ten, and "as exact text" keeps every digit.
say "1e5" as a number.
say "2.5E-3" as a number.
say " -4e+2 " as a number.
say "1e400" as a number.
say 0.1 plus 0.2.
say (0.1 plus 0.2) as exact text.
say (1 divided by 3) as exact text.
say 100000000000000000000 as exact text.
say 0.0000001 as exact text.
say (-0.5) as exact text.
say 42 as exact text.
let third be 1 divided by 3.
let back be (third as exact text) as a number.
say back is third.
say ((third as text) as a number) minus third is 0.
keep tiny as 0.000000123 as exact text.
say tiny.
keep big as "6.02e23" as a number.
say big as exact text.
