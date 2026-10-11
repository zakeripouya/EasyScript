note: Make a messy folder, then tidy it: every file goes into a folder named after its ending.
make a folder called "messy"
let names be a list of "notes.txt", "cat.jpg", "budget.csv", "dog.jpg" and "todo.txt"
for each name in names:
    put "Some text in " followed by name in file "messy/" followed by name
say "Before: " followed by the files in "messy"

for each name in the files in "messy":
    let parts be name split by "."
    let ending be the last item of parts
    let place be "messy/" followed by ending
    if not (folder place exists):
        make a folder called place
    move the file ("messy/" followed by name) to the folder place

say "After: " followed by the folders in "messy"
for each ending in the folders in "messy":
    say "  " followed by ending followed by ": " followed by the files in ("messy/" followed by ending)

rename the file "messy/txt/todo.txt" to "messy/txt/done.txt"
copy the file "messy/csv/budget.csv" to "messy/budget backup.csv"
say "Renamed and copied: " followed by the files in "messy/txt" followed by "; " followed by the files in "messy"
say "The backup has " followed by the size of file "messy/budget backup.csv" followed by " bytes."

delete the folder "messy" and everything in it
if not (folder "messy" exists), say "Cleaned up."
