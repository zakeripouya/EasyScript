note: settings from the environment
let home be the setting called "HOME"
if home is nothing:
    say "HOME isn't set"
otherwise:
    say "HOME is set: " followed by (the length of home is above 0)
let missing be the setting called "EASYSCRIPT_NO_SUCH_SETTING"
say missing is nothing
say missing
let name be "PATH"
if the setting called name is not nothing, say "PATH is set"
