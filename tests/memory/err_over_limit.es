# Text that keeps growing goes over the test's memory limit, and the check
# stops the program.
let s be ""
repeat 1000000 times:
    set s to s followed by "word "
