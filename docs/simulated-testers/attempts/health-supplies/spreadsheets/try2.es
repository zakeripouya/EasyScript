let GlovesMin be 20
let MasksMin be 15
let BandagesMin be 30
let GelMin be 10

let GlovesOnHand be ask "How many gloves (boxes) do we have?"
let MasksOnHand be ask "How many masks (boxes) do we have?"
let BandagesOnHand be ask "How many bandages (packs) do we have?"
let GelOnHand be ask "How many hand gel (bottles) do we have?"

let GlovesOrder be IF(GlovesOnHand < GlovesMin, GlovesMin * 2 - GlovesOnHand, 0)
let MasksOrder be IF(MasksOnHand < MasksMin, MasksMin * 2 - MasksOnHand, 0)
let BandagesOrder be IF(BandagesOnHand < BandagesMin, BandagesMin * 2 - BandagesOnHand, 0)
let GelOrder be IF(GelOnHand < GelMin, GelMin * 2 - GelOnHand, 0)

if GlovesOrder > 0, say "Gloves: order " & GlovesOrder & " boxes"
if MasksOrder > 0, say "Masks: order " & MasksOrder & " boxes"
if BandagesOrder > 0, say "Bandages: order " & BandagesOrder & " packs"
if GelOrder > 0, say "Hand gel: order " & GelOrder & " bottles"

if GlovesOrder + MasksOrder + BandagesOrder + GelOrder is 0, say "Nothing to order this week."
