note: The Towers of Hanoi, solved by a function that calls itself.
note: To move a pile of disks, move all but the bottom one out of the way,
note: move the bottom one, then move the rest back on top of it.
to hanoi with disks and source and target and spare:
    if disks is 0, give back 0
    let before be hanoi of (disks minus 1) and source and spare and target
    say "Move disk " followed by disks followed by " from " followed by source followed by " to " followed by target
    let after be hanoi of (disks minus 1) and spare and target and source
    give back before plus 1 plus after
let moves be hanoi of 3 and "A" and "C" and "B"
say "Done in " followed by moves followed by " moves."
