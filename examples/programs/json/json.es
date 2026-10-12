note: A JSON parser and printer. "parse json with T" reads JSON text into a
note: json value (a kind with variants), or a parse failure that says what's
note: wrong and on which line and column. "compact json of V" and "pretty
note: json of V and INDENT" write a value back as JSON text, and "quoted json
note: of T" writes text as a JSON string. Everything else is private to this file.

a json value is one of:
    a json null.
    a json boolean with truth (yes or no).
    a json number with amount (a number).
    a json string with content (text).
    a json array with elements (a list of json values).
    a json object with members (a map from text to json values).

a parse result is one of:
    a parsed document with root (a json value).
    a parse failure with message (text), line (a number) and column (a number).

note: The reader goes through the characters one at a time. The first
note: problem it finds is kept, with where it was; everything after that
note: gives up as soon as it sees there's a problem.
a json reader is a kind of thing with:
    characters, which is a list of text.
    position, which is a number, starting at 1.
    problem, which is text, starting at "".
    problem place, which is a number, starting at 0.

privately to current character in r, giving back text:
    if r's position is more than the number of items in r's characters, give back "".
    give back item (r's position) of r's characters.

privately to advance in r:
    add 1 to r's position.

privately to report in r and message:
    if r's problem is "":
        set r's problem to message.
        set r's problem place to r's position.

note: EasyScript text has no "\r" escape, so the carriage return is made from its code.
privately to carriage return, giving back text:
    give back the character with code 13.

privately to skip spaces in r:
    forever:
        let c be current character in r.
        if c is " " or c is "\n" or c is "\t" or c is carriage return:
            advance in r.
        otherwise:
            stop the loop.

note: --- Values ----------------------------------------------------------

privately to parse value in r, giving back a json value:
    skip spaces in r.
    let c be current character in r.
    if c is "{", give back parse object in r.
    if c is "[", give back parse array in r.
    if c is "\"", give back a new json string with content (parse string in r).
    if c is "-" or c is a digit, give back parse number in r.
    if c is "t" and (take word in r and "true"), give back a new json boolean with truth yes.
    if c is "f" and (take word in r and "false"), give back a new json boolean with truth no.
    if c is "n" and (take word in r and "null"), give back a new json null.
    if c is "", report in r and "I expected a value here, but the text ended.".
    report in r and "I expected a value here.".
    give back a new json null.

note: "true", "false" or "null", one character at a time.
privately to take word in r and expected (text), giving back yes or no:
    for each wanted in the characters of expected:
        if current character in r is not wanted:
            report in r and ("I expected \"" followed by expected followed by "\" here.").
            give back no.
        advance in r.
    give back yes.

privately to parse array in r, giving back a json value:
    advance in r.
    let collected be an empty list of json values.
    skip spaces in r.
    if current character in r is "]":
        advance in r.
        give back a new json array with elements collected.
    forever:
        let element be parse value in r.
        if r's problem is not "", give back a new json null.
        add element to the end of collected.
        skip spaces in r.
        let c be current character in r.
        if c is ",":
            advance in r.
        otherwise if c is "]":
            advance in r.
            give back a new json array with elements collected.
        otherwise:
            report in r and "I expected \",\" or \"]\" here.".
            give back a new json null.

privately to parse object in r, giving back a json value:
    advance in r.
    let found be an empty map from text to json values.
    skip spaces in r.
    if current character in r is "}":
        advance in r.
        give back a new json object with members found.
    forever:
        skip spaces in r.
        if current character in r is not "\"":
            report in r and "I expected a name in quotes here.".
            give back a new json null.
        let name be parse string in r.
        skip spaces in r.
        if r's problem is "" and current character in r is not ":":
            report in r and "I expected \":\" after the name.".
        if r's problem is not "", give back a new json null.
        advance in r.
        let member be parse value in r.
        if r's problem is not "", give back a new json null.
        set the value for name in found to member.
        skip spaces in r.
        let c be current character in r.
        if c is ",":
            advance in r.
        otherwise if c is "}":
            advance in r.
            give back a new json object with members found.
        otherwise:
            report in r and "I expected \",\" or \"}\" here.".
            give back a new json null.

note: --- Numbers ---------------------------------------------------------

note: Digits, as long as there are some.
privately to take digits in r, giving back text:
    let digits be "".
    while current character in r is a digit:
        set digits to digits followed by current character in r.
        advance in r.
    give back digits.

note: Infinity minus infinity isn't a number, so it isn't 0.
privately to too big with amount (a number), giving back yes or no:
    give back amount minus amount is not 0.

privately to parse number in r, giving back a json value:
    let start be r's position.
    let sign be 1.
    if current character in r is "-":
        set sign to -1.
        advance in r.
    if current character in r is not a digit:
        report in r and "I expected a digit here.".
        give back a new json null.
    let whole be take digits in r.
    if whole starts with "0" and the number of characters in whole is above 1:
        set r's position to start.
        report in r and "A number can't start with 0, unless it's 0 itself.".
        give back a new json null.
    let written be whole.
    if current character in r is ".":
        advance in r.
        if current character in r is not a digit:
            report in r and "I expected a digit after the decimal point.".
            give back a new json null.
        set written to written followed by "." followed by take digits in r.
    let amount be written as a number.
    if current character in r is "e" or current character in r is "E":
        advance in r.
        let exponent sign be 1.
        if current character in r is "+":
            advance in r.
        otherwise if current character in r is "-":
            set exponent sign to -1.
            advance in r.
        if current character in r is not a digit:
            report in r and "I expected a digit in the exponent.".
            give back a new json null.
        let exponent be (take digits in r) as a number.
        set amount to amount times (10 to the power of (exponent times exponent sign)).
    if too big with amount:
        set r's position to start.
        report in r and "This number is too big.".
        give back a new json null.
    give back a new json number with amount (amount times sign).

note: --- Text ------------------------------------------------------------

privately to hex value of c (text), giving back a number:
    if c is "", give back -1.
    let place be the position of (c in lowercase) in "0123456789abcdef".
    if place is nothing, give back -1.
    give back place minus 1.

note: The four hex digits after "\u": a number from 0 to 65535 (-1 if they're wrong).
privately to take code in r, giving back a number:
    let code be 0.
    repeat 4 times:
        let digit be hex value of (current character in r).
        if digit is below 0:
            report in r and "\"\\u\" needs four hex digits after it, like \"\\u00e9\".".
            give back -1.
        set code to code times 16 plus digit.
        advance in r.
    give back code.

note: After "\u": one character, or two codes for a character outside the first
note: 65536 (a surrogate pair, like "\ud83d\ude00").
privately to take escaped code in r, giving back text:
    let start be r's position minus 2.
    let code be take code in r.
    if code is below 0, give back "".
    if code is at least 56320 and code is at most 57343:
        set r's position to start.
        report in r and "This is the second half of a pair of codes, without the first half.".
        give back "".
    if code is at least 55296 and code is at most 56319:
        if current character in r is not "\\":
            set r's position to start.
            report in r and "This is the first half of a pair of codes, without the second half.".
            give back "".
        advance in r.
        if current character in r is not "u":
            set r's position to start.
            report in r and "This is the first half of a pair of codes, without the second half.".
            give back "".
        advance in r.
        let low be take code in r.
        if low is below 0, give back "".
        if low is below 56320 or low is above 57343:
            set r's position to start.
            report in r and "This is the first half of a pair of codes, without the second half.".
            give back "".
        set code to 65536 plus (code minus 55296) times 1024 plus (low minus 56320).
    if code is 0:
        set r's position to start.
        report in r and "EasyScript text can't hold the character with code 0.".
        give back "".
    give back the character with code code.

privately to parse string in r, giving back text:
    advance in r.
    let content be "".
    forever:
        let c be current character in r.
        if c is "":
            report in r and "This text never ends: it needs a \" at the end.".
            give back "".
        if c is "\"":
            advance in r.
            give back content.
        if c is "\\":
            advance in r.
            let escaped be current character in r.
            let meanings be a map of "\"" to "\"", "\\" to "\\", "/" to "/", "n" to "\n", "t" to "\t", "r" to carriage return, "b" to the character with code 8 and "f" to the character with code 12.
            if meanings has escaped:
                set content to content followed by the value for escaped in meanings.
                advance in r.
            otherwise if escaped is "u":
                advance in r.
                set content to content followed by take escaped code in r.
                if r's problem is not "", give back "".
            otherwise:
                set r's position to r's position minus 1.
                report in r and ("\"\\" followed by escaped followed by "\" isn't something JSON knows after a backslash.").
                give back "".
        otherwise if the code of c is below 32:
            report in r and "This character has to be written with a backslash in JSON text, like \"\\n\".".
            give back "".
        otherwise:
            set content to content followed by c.
            advance in r.

note: --- Reading a whole document ---------------------------------------

note: The line and column (from 1) of position `place` in the characters.
privately to failure from r, giving back a parse result:
    let line be 1.
    let column be 1.
    let place be 1.
    while place is below r's problem place:
        if item place of r's characters is "\n":
            add 1 to line.
            set column to 1.
        otherwise:
            add 1 to column.
        add 1 to place.
    give back a new parse failure with message (r's problem), line line and column column.

to parse json with source (text), giving back a parse result:
    let r be a new json reader with characters (the characters of source).
    let root be parse value in r.
    skip spaces in r.
    if r's problem is "" and current character in r is not "":
        report in r and "There's more after the value.".
    if r's problem is not "", give back failure from r.
    give back a new parsed document with root root.

note: --- Writing JSON -------------------------------------------------------

privately to two hex digits of n (a number), giving back text:
    let digits be "0123456789abcdef".
    give back character ((n divided by 16) rounded down plus 1) of digits followed by character (n mod 16 plus 1) of digits.

to quoted json of t (text), giving back text:
    let result be "\"".
    for each c in the characters of t:
        if c is "\"":
            set result to result followed by "\\\"".
        otherwise if c is "\\":
            set result to result followed by "\\\\".
        otherwise if c is "\n":
            set result to result followed by "\\n".
        otherwise if c is "\t":
            set result to result followed by "\\t".
        otherwise if c is carriage return:
            set result to result followed by "\\r".
        otherwise if the code of c is below 32:
            set result to result followed by "\\u00" followed by two hex digits of (the code of c).
        otherwise:
            set result to result followed by c.
    give back result followed by "\"".

to compact json of v (a json value), giving back text:
    check which kind v is:
        when it's a json null:
            give back "null".
        when it's a json boolean:
            if its truth, give back "true".
            give back "false".
        when it's a json number:
            give back its amount as text.
        when it's a json string:
            give back quoted json of its content.
        when it's a json array:
            let parts be an empty list of text.
            for each element in its elements:
                add compact json of element to the end of parts.
            give back "[" followed by (parts joined with ",") followed by "]".
        when it's a json object:
            let parts be an empty list of text.
            for each name and member in its members:
                add (quoted json of name) followed by ":" followed by (compact json of member) to the end of parts.
            give back "{" followed by (parts joined with ",") followed by "}".

note: Each item of an array or object on its own line, indented two spaces
note: more than the line it's in.
to pretty json of v (a json value) and indent (text), giving back text:
    let inner be indent followed by "  ".
    check which kind v is:
        when it's a json array:
            if its elements is empty, give back "[]".
            let parts be an empty list of text.
            for each element in its elements:
                add inner followed by (pretty json of element and inner) to the end of parts.
            give back "[\n" followed by (parts joined with ",\n") followed by "\n" followed by indent followed by "]".
        when it's a json object:
            if its members is empty, give back "{}".
            let parts be an empty list of text.
            for each name and member in its members:
                add inner followed by (quoted json of name) followed by ": " followed by (pretty json of member and inner) to the end of parts.
            give back "{\n" followed by (parts joined with ",\n") followed by "\n" followed by indent followed by "}".
        otherwise:
            give back compact json of v.
