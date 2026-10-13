let gloves_min be 20
let masks_min be 15
let bandages_min be 30
let gel_min be 10

let gloves be ask "How many gloves (boxes) do we have? "
let masks be ask "How many masks (boxes) do we have? "
let bandages be ask "How many bandages (packs) do we have? "
let gel be ask "How many hand gel (bottles) do we have? "

let ordered be false

if gloves < gloves_min:
    say "gloves: order " + (gloves_min * 2 - gloves) + " boxes"
    set ordered to true
if masks < masks_min:
    say "masks: order " + (masks_min * 2 - masks) + " boxes"
    set ordered to true
if bandages < bandages_min:
    say "bandages: order " + (bandages_min * 2 - bandages) + " packs"
    set ordered to true
if gel < gel_min:
    say "hand gel: order " + (gel_min * 2 - gel) + " bottles"
    set ordered to true

if ordered == false:
    say "Nothing to order this week."
