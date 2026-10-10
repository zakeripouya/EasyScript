note: A small notes program: write a few lines to a file, add one more, read it back.
keep notes_file as "notes.txt"
write "Buy milk\nCall Ada\nWater the plants" to file notes_file
append "Write more EasyScript" to file notes_file
read file notes_file and call it notes
say "Your notes:"
say notes
say "That's " followed by length of notes followed by " characters."
