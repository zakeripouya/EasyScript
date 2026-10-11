note: Searching, splitting, joining, changing and checking text.
let word be "unbreakable"
say word contains "break"
say word contains "fix"
say word does not contain "fix"
say word doesn't contain "able"
say word contains ""
say word starts with "un"
say word starts with "re"
say word ends with "able"
say word does not end with "able"
say word doesn't start with "un"
say "" starts with ""
say the position of "break" in word
say the position of "x" in word
say the position of "" in word
say the position of "é" in "café crème"
say the position of "crème" in "café crème"
note: Not there is nothing, so it has to be checked before it's used as a number.
let where be the position of "x" in word
say where is nothing
if where is nothing, say "no x"
let at be the position of "break" in word
if at is not nothing, say "break starts at " followed by at followed by ", ends at " followed by (at plus 4)
note: Positions in lists too.
let scores be a list of 72, 95 and 88
say the position of 95 in scores
say the position of 100 in scores
say the position of "b" in a list of "a", "b" and "b"
note: Splitting.
say the words of "  the quick  brown	fox  "
say number of items in the words of "   "
let crlf be "one\ntwo" followed by (the character with code 13) followed by "\nthree\n"
say the lines of crlf
say number of items in the lines of ""
say the lines of "a\n\nb"
say "a,b,,c" split by ","
say "a, b, c" split by ", "
say number of items in ("" split by ",")
say "no commas" split by ","
say "2024-10-10" split by "-" joined with "/"
note: Joining.
let names be a list of "Ada", "Grace" and "Linus"
say names joined with ", "
say names joined with ""
say (a list of 1, 2.5 and 3) joined with " + "
say (a list of yes and no) joined with "/"
say an empty list of text joined with ", " followed by "|"
note: Changing.
say "Hello, World" in uppercase
say "Hello, World" in lowercase
say "straße café ÿ" in uppercase
say "ÀÉÎÕÜ ΑΒΓ ДЖЯ Ÿ" in lowercase
say "ελληνικά Ёж" in uppercase
say "  padded \t\n" without extra spaces followed by "|"
say "   " without extra spaces followed by "|"
say "no padding" without extra spaces
say "banana" with "a" replaced by "o"
say "banana" with "an" replaced by ""
say "aaaa" with "aa" replaced by "b"
say "banana" with "x" replaced by "y"
let old be "na"
say "banana" with old replaced by "NA"
say "café" with "é" replaced by "e"
note: Checking characters.
say "a" is a letter
say "Z" is a letter
say "é" is a letter
say "Ж" is a letter
say "7" is a letter
say "ab" is a letter
say "" is a letter
say "7" is a digit
say "x" is a digit
say "77" is a digit
say " " is a space
say "\t" is a space
say "x" is not a space
say "x" isn't a letter
say the code of "A"
say the code of "é"
say the code of "日"
say the code of "😀"
say the character with code 65
say the character with code 233
say the character with code 128512
say the character with code (the code of "a" plus 1)
note: A variable called "letter" is still a name: "is letter" compares with it.
let letter be "7"
say "7" is letter
say "7" is a letter
say "7" is the letter
say "7" isn't letter
note: A list written out inside a list form is generated once (no unused C left over).
say item 2 of a list of "a" and "b"
say the position of "b" in a list of "a" and "b"
