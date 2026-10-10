# Text made and thrown away inside expressions: number to text, joins,
# comparisons, length. None of it outlives the sentence.
let total be 0
count from 1 to 1000000 as n:
    if ("x" followed by n as text) is "x1", say "found x1"
    set total to total plus length of (n as text followed by "-" followed by n as text)
say total
