note: Reads a file, counts its words, and reports the most common ones.

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

note: Sorted, the same words sit next to each other, so each run is one word.
sort words
let distinct be an empty list of text
let counts be an empty list of numbers
for each word in words:
    if distinct is not empty and the last item of distinct is word:
        set the last item of counts to the last item of counts plus 1
    otherwise:
        add word to the end of distinct
        add 1 to the end of counts
say "Of those, " followed by the number of items in distinct followed by " are different."

note: The most common first: take the biggest count, then put it out of the way.
say ""
say "The five most common words:"
count from 1 to 5 as rank:
    let most be the biggest of counts
    let place be the position of most in counts
    let word be item place of distinct
    say rank followed by ". " followed by word followed by " (" followed by most followed by " times)"
    set item place of counts to 0

say ""
let longest be ""
for each word in distinct:
    if length of word is more than length of longest, set longest to word
say "The longest word is \"" followed by longest followed by "\", with " followed by length of longest followed by " letters."
let capital be the first character of longest in uppercase followed by longest without the first character
say "Written as a name: " followed by capital
