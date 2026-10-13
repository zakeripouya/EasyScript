let GlovesMin be 20
let MasksMin be 15
let BandagesMin be 30
let GelMin be 10

ask "How many gloves (boxes) do we have?" and put the answer in GlovesOnHand
ask "How many masks (boxes) do we have?" and put the answer in MasksOnHand
ask "How many bandages (packs) do we have?" and put the answer in BandagesOnHand
ask "How many hand gel (bottles) do we have?" and put the answer in GelOnHand

let GlovesOrder be 0
let MasksOrder be 0
let BandagesOrder be 0
let GelOrder be 0

if GlovesOnHand < GlovesMin, set GlovesOrder to GlovesMin * 2 - GlovesOnHand
if MasksOnHand < MasksMin, set MasksOrder to MasksMin * 2 - MasksOnHand
if BandagesOnHand < BandagesMin, set BandagesOrder to BandagesMin * 2 - BandagesOnHand
if GelOnHand < GelMin, set GelOrder to GelMin * 2 - GelOnHand

if GlovesOrder > 0, say "Gloves: order " plus GlovesOrder plus " boxes"
if MasksOrder > 0, say "Masks: order " plus MasksOrder plus " boxes"
if BandagesOrder > 0, say "Bandages: order " plus BandagesOrder plus " packs"
if GelOrder > 0, say "Hand gel: order " plus GelOrder plus " bottles"

if GlovesOrder + MasksOrder + BandagesOrder + GelOrder is 0, say "Nothing to order this week."
