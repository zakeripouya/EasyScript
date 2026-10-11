note: Reads a file, counts how often each word appears with a map, and reports the most common ones.

note: First, a file to read.
write "The cat sat on the mat. The dog sat on the log." to file "story.txt"
append "A cat and a dog: the best of friends, or the worst?" to file "story.txt"
append "Nobody knows. The cat isn't telling, and the dog can't." to file "story.txt"

read file "story.txt" and call it story
say "The file has " followed by the number of items in the lines of story followed by " lines."

note: Words in lowercase, without the punctuation around them.
let cleaned be story in lowercase
for each mark in a list of ".", ",", ":", "?" and "!":
    set cleaned to cleaned with mark replaced by " "
let words be the words of cleaned
say "It has " followed by the number of items in words followed by " words."

note: A map from each word to how many times it appears. A word seen for
note: the first time isn't in the map yet, so ", or 0" counts it from 0.
let counts be an empty map from text to numbers
for each word in words:
    set the value for word in counts to (the value for word in counts, or 0) plus 1
say "Of those, " followed by the number of entries in counts followed by " are different."
say "\"cat\" appears " followed by the value for "cat" in counts followed by " times, and \"bird\" " followed by (the value for "bird" in counts, or 0) followed by "."

note: The most common first: take the word with the biggest count, then
note: remove it. The map keeps its words in the order they first appeared,
note: so of two words seen equally often, the earlier one comes first.
say ""
say "The five most common words:"
let remaining be a copy of counts
count from 1 to 5 as rank:
    let best be ""
    let most be 0
    for each word and seen in remaining:
        if seen is more than most:
            set best to word
            set most to seen
    say rank followed by ". " followed by best followed by " (" followed by most followed by " times)"
    remove best from remaining

say ""
let longest be ""
for each word in the keys of counts:
    if length of word is more than length of longest, set longest to word
say "The longest word is \"" followed by longest followed by "\", with " followed by length of longest followed by " letters."
let capital be the first character of longest in uppercase followed by longest without the first character
say "Written as a name: " followed by capital
