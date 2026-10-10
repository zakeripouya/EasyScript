note: A million lines of text, made one at a time. Each one is freed as soon
note: as the next replaces it, so this uses no more memory than one line.
let longest be ""
repeat 1000000 times:
    let line be "Line " followed by it as text followed by " of a million"
    if length of line is more than length of longest:
        set longest to line
say "The longest line was: " followed by longest
