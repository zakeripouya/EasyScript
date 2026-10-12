note: Tests the JSON parser in json.es. The documents to read are sent to the
note: program (main.in), separated by lines of "---". Each one is shown
note: parsed and written back compactly, or with the line and column of its
note: first problem. Run it with:
note:     easyscript run examples/programs/json/main.es < examples/programs/json/main.in

use json.

to show result of source (text):
    let result be parse json with source.
    check which kind result is:
        when it's a parsed document:
            let written be compact json of its root.
            say "  " followed by written.
            let again be parse json with written.
            if again is not a new parsed document with root (its root):
                say "  (reading it back gives a different value: numbers are written with 10 significant digits)"
        when it's a parse failure:
            say "  Line " followed by its line followed by ", column " followed by its column followed by ": " followed by its message

let count be 0.
for each document in the text sent to the program split by "\n---\n":
    add 1 to count.
    say "Document " followed by count followed by ":".
    show result of document.

note: Pretty-printing a nested document.
say "Pretty:"
let nested be "{\"name\": \"Ada\", \"languages\": [[\"English\", \"French\"]], \"born\": {\"year\": 1815, \"month\": 12}, \"notes\": [[]], \"extra\": {}}"
let parsed be parse json with nested.
if parsed is a parsed document:
    say pretty json of parsed's root and "".

note: Making a value in EasyScript and writing it as JSON.
let scores be a map of "Ada" to (a new json number with amount 95), "Grace" to (a new json number with amount 88.5).
let report be a new json object with members (a map of "class" to (a new json string with content "Room \"7\""), "scores" to (a new json object with members scores) and "passed" to (a new json array with elements (a list of (a new json boolean with truth yes) and (a new json null))))
say compact json of report.

note: Looking inside a parsed value: the total of the numbers in an array.
to total of v (a json value), giving back a number:
    let sum be 0.
    check which kind v is:
        when it's a json array:
            for each element in its elements:
                check which kind element is:
                    when it's a json number:
                        add its amount to sum.
                    otherwise:
                        add 0 to sum.
        otherwise:
            add 0 to sum.
    give back sum.
let numbers be parse json with "[[1, 2.5, \"skip me\", 4]]".
if numbers is a parsed document, say "Total: " followed by total of numbers's root.
