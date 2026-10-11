note: Finds the shortest way between stations with a breadth-first search.
note: The network is a map from each station to a list of the stations next to it.

to link with here and there and network:
    if network does not have here, set the value for here in network to an empty list of text
    if network does not have there, set the value for there in network to an empty list of text
    add there to the end of the value for here in network
    add here to the end of the value for there in network

let network be an empty map from text to lists of text
link with "Ash" and "Birch" and network
link with "Ash" and "Cedar" and network
link with "Birch" and "Dogwood" and network
link with "Cedar" and "Dogwood" and network
link with "Dogwood" and "Elm" and network
link with "Elm" and "Fig" and network
link with "Cedar" and "Hazel" and network
link with "Hazel" and "Fig" and network
set the value for "Gum" in network to an empty list of text

for each station and neighbours in network:
    if neighbours is empty:
        say station followed by ": no stations next to it"
    otherwise:
        say station followed by ": " followed by neighbours joined with ", "

note: Breadth-first: visit the start, then every station one stop away, then
note: two stops away, and so on. The first time a station is reached is the
note: shortest way to it, so "previous" remembers where each one was reached from.
to find way with start and goal and network, giving back a list of text:
    let previous be a map of start to ""
    let queue be a list of start
    let next be 1
    while next is at most the number of items in queue:
        let here be item next of queue
        add 1 to next
        if here is goal:
            let path be a list of goal
            let step be goal
            while the value for step in previous is not "":
                set step to the value for step in previous
                insert step at position 1 of path
            give back path
        for each neighbour in the value for here in network:
            if previous does not have neighbour:
                set the value for neighbour in previous to here
                add neighbour to the end of queue
    give back an empty list of text

say ""
for each goal in a list of "Fig", "Elm" and "Gum":
    let way be find way with "Ash" and goal and network
    if way is empty:
        say "There's no way from Ash to " followed by goal followed by "."
    otherwise:
        say way joined with " -> " followed by " (" followed by (the number of items in way minus 1) followed by " stops)"

note: How many stops every station is from Ash, found the same way.
let stops be a map of "Ash" to 0
let queue be a list of "Ash"
let next be 1
while next is at most the number of items in queue:
    let here be item next of queue
    add 1 to next
    for each neighbour in the value for here in network:
        if stops does not have neighbour:
            set the value for neighbour in stops to (the value for here in stops) plus 1
            add neighbour to the end of queue
say ""
say "Stops from Ash: " followed by stops
for each station in the keys of network:
    if stops does not have station, say station followed by " can't be reached from Ash."
