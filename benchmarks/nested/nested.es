note: Nested loops: 100 million rounds with a test inside.
let total be 0
count from 1 to 10000 as row:
    count from 1 to 10000 as column:
        if (row plus column) mod 3 is 0, add 1 to total
say total
