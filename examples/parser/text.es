note: Parser example, for reading the syntax tree.
note: `easyscript ast` shows how it is understood.
# commas in "say" join pieces with spaces; followed by joins as it is; brackets put values in text
say "Hello,", name, "!"
say "Hello, [name]!"
say "Total: " followed by total as text
say length of name
say contents of file "notes.txt"
say "42" as a number plus 1
