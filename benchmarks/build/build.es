note: Building a large text one piece at a time: a million pieces, 5 million characters.
let text be ""
repeat 1000000 times:
    set text to text followed by "word "
say length of text
