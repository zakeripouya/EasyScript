note: Characters and pieces of text, with the same forms as list items.
let word be "café au lait"
say length of word
say the number of characters in word
say character 4 of word
say the first character of word
say the last character in word
say characters 2 to 4 of word
say characters 6 to the end of word
say the first 4 characters of word
say the last 4 characters of word
say the first 100 characters of "abc"
say the first 0 characters of "abc" followed by "|"
say word without the first character
say word without the last character
say every other character of "abcdefg"
say every second character of "abcdefg"
say every 3rd character in "a1b2c3d4e5"
say every other character of "héllo wörld"
say the characters of "héllo"
say number of items in the characters of "日本語"
let n be 2
say character n of "xyz"
say character (n plus 1) of "xyz"
say character 2 of ("abc" with "b" replaced by "B")
note: Empty text.
say the characters of "" is empty
say every other character of "" followed by "|"
say the first 2 characters of "" followed by "|"
note: Every character of a long text, in a loop.
let total be 0
let line be "the quick brown fox jumps over the lazy dog"
count from 1 to length of line as i:
    if character i of line is "o", add 1 to total
say total
for each letter in the characters of "naïve":
    say letter
