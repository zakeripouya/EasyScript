# Answers to ask and text read from a file are freed like any other text.
let answers be 0
repeat 1000 times:
    ask "" and call the answer reply
    if length of reply is at least 8, increase answers by 1
say answers
write "some notes" to file "notes.txt"
let size be 0
repeat 10000 times:
    read file "notes.txt" and call it notes
    set size to size plus length of notes
say size
