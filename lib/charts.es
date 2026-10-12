note: Charts, written in EasyScript. A program that says "save a bar chart
note: of X as F" (or a line chart, or a pie chart) has this file joined to it,
note: and the sentence writes what "numbers chart", "text map chart" or
note: "number map chart" gives back into F. Each chart is an SVG picture:
note: text that browsers and drawing programs show as an image.

privately keep colors as a list of "#0072B2", "#E69F00", "#009E73", "#CC79A7", "#56B4E9", "#D55E00", "#F0E442" and "#999999".
privately keep plot height as 300.
privately keep slot as 64.
privately keep pi value as 3.14159265358979.

note: Text put inside the picture: &, < and > written the way SVG wants them.
privately to escaped with words (text), giving back text:
    give back words with "&" replaced by "&amp;" with "<" replaced by "&lt;" with ">" replaced by "&gt;".

note: A number written in the picture, with at most one decimal.
privately to place with number, giving back text:
    give back (number rounded to 1 decimal place) as text.

note: The sine and cosine with an angle (in radians), worked out by adding up
note: the first terms of their series; plenty for drawing.
privately to sine with angle, giving back a number:
    let x be angle.
    while x is above pi value:
        set x to x minus 2 times pi value.
    while x is below 0 minus pi value:
        set x to x plus 2 times pi value.
    let term be x.
    let sum be x.
    count from 1 to 12 as k:
        set term to 0 minus term times x times x divided by ((2 times k) times (2 times k plus 1)).
        add term to sum.
    give back sum.

privately to cosine with angle, giving back a number:
    give back sine with (angle plus pi value divided by 2).

privately to color for position, giving back text:
    give back item ((position minus 1) mod (the number of items in colors) plus 1) of colors.

privately to opening with width and height and title (text), giving back text:
    let picture be "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 [width] [height]\" width=\"[width]\" height=\"[height]\" font-family=\"Helvetica, Arial, sans-serif\">\n".
    set picture to picture followed by "<rect width=\"[width]\" height=\"[height]\" fill=\"white\"/>\n".
    if title is not "":
        set picture to picture followed by "<text x=\"[place with (width divided by 2)]\" y=\"30\" text-anchor=\"middle\" font-size=\"20\" font-weight=\"bold\" fill=\"#222\">[escaped with title]</text>\n".
    give back picture.

note: A label under a bar or point is turned when it's too wide for its place.
privately to turned with words (text), giving back yes or no:
    give back (the number of characters in words) times 7 is above slot minus 6.

note: Room under the bars for their labels: more when they're turned.
privately to label room for labels (a list of text), giving back a number:
    let room be 40.
    for each words in labels:
        if turned with words:
            let needed be 30 plus (the number of characters in words) times 4.5.
            if needed is above room, set room to needed.
    give back room.

note: A label under a bar or point: turned when it's too wide for its place.
privately to label with x and bottom and words (text), giving back text:
    if turned with words:
        give back "<text x=\"[place with x]\" y=\"[bottom plus 16]\" text-anchor=\"end\" font-size=\"12\" fill=\"#333\" transform=\"rotate(-35 [place with x] [bottom plus 16])\">[escaped with words]</text>\n".
    give back "<text x=\"[place with x]\" y=\"[bottom plus 18]\" text-anchor=\"middle\" font-size=\"12\" fill=\"#333\">[escaped with words]</text>\n".

privately to bar chart with labels (a list of text) and values (a list of numbers) and title (text), giving back text:
    let bar count be the number of items in values.
    let width be 80 plus slot times bar count.
    if width is below 400, set width to 400.
    let left be (width minus slot times bar count) divided by 2.
    let top be 50.
    if title is "", set top to 20.
    let height be top plus plot height plus label room for labels.
    let bottom be top plus plot height.
    let biggest be 0.
    let smallest be 0.
    for each value in values:
        if value is above biggest, set biggest to value.
        if value is below smallest, set smallest to value.
    let range be biggest minus smallest.
    if range is 0, set range to 1.
    let scale be (plot height minus 20) divided by range.
    let zero be bottom minus (0 minus smallest) times scale.
    let picture be opening with width and height and title.
    count from 1 to bar count as position:
        let value be item position of values.
        let x be left plus (position minus 1) times slot plus 8.
        let bar width be slot minus 16.
        let y be zero minus value times scale.
        let bar height be value times scale.
        if value is below 0:
            set y to zero.
            set bar height to 0 minus value times scale.
        set picture to picture followed by "<rect x=\"[place with x]\" y=\"[place with y]\" width=\"[place with bar width]\" height=\"[place with bar height]\" fill=\"#0072B2\"/>\n".
        let middle be x plus bar width divided by 2.
        let above be y minus 6.
        if value is below 0, set above to y plus bar height plus 14.
        set picture to picture followed by "<text x=\"[place with middle]\" y=\"[place with above]\" text-anchor=\"middle\" font-size=\"12\" fill=\"#222\">[value with commas]</text>\n".
        set picture to picture followed by (label with middle and bottom and item position of labels).
    set picture to picture followed by "<line x1=\"[place with left]\" y1=\"[place with zero]\" x2=\"[place with (width minus left)]\" y2=\"[place with zero]\" stroke=\"#333\"/>\n".
    give back picture followed by "</svg>".

privately to line chart with labels (a list of text) and values (a list of numbers) and title (text), giving back text:
    let point count be the number of items in values.
    let width be 80 plus slot times point count.
    if width is below 400, set width to 400.
    let left be (width minus slot times point count) divided by 2.
    let top be 50.
    if title is "", set top to 20.
    let height be top plus plot height plus label room for labels.
    let bottom be top plus plot height.
    let biggest be 0.
    let smallest be 0.
    for each value in values:
        if value is above biggest, set biggest to value.
        if value is below smallest, set smallest to value.
    let range be biggest minus smallest.
    if range is 0, set range to 1.
    let scale be (plot height minus 20) divided by range.
    let zero be bottom minus (0 minus smallest) times scale.
    let picture be opening with width and height and title.
    set picture to picture followed by "<line x1=\"[place with left]\" y1=\"[place with zero]\" x2=\"[place with (width minus left)]\" y2=\"[place with zero]\" stroke=\"#333\"/>\n".
    let points be "".
    let dots be "".
    count from 1 to point count as position:
        let value be item position of values.
        let x be left plus (position minus 1) times slot plus slot divided by 2.
        let y be zero minus value times scale.
        if position is above 1, set points to points followed by " ".
        set points to points followed by "[place with x],[place with y]".
        set dots to dots followed by "<circle cx=\"[place with x]\" cy=\"[place with y]\" r=\"4\" fill=\"#0072B2\"/>\n".
        let above be y minus 10.
        if value is below 0, set above to y plus 20.
        set dots to dots followed by "<text x=\"[place with x]\" y=\"[place with above]\" text-anchor=\"middle\" font-size=\"12\" fill=\"#222\">[value with commas]</text>\n".
        set dots to dots followed by (label with x and bottom and item position of labels).
    set picture to picture followed by "<polyline points=\"[points]\" fill=\"none\" stroke=\"#0072B2\" stroke-width=\"3\"/>\n".
    give back picture followed by dots followed by "</svg>".

privately to pie chart with labels (a list of text) and values (a list of numbers) and title (text), giving back text:
    let total be 0.
    for each value in values:
        if value is above 0, add value to total.
    let width be 560.
    let top be 50.
    if title is "", set top to 20.
    let height be top plus plot height plus 10.
    let keys height be top plus 30 plus 22 times the number of items in values.
    if keys height is above height, set height to keys height.
    let radius be plot height divided by 2 minus 10.
    let cx be 30 plus radius.
    let cy be top plus 10 plus radius.
    let picture be opening with width and height and title.
    let angle be 0.
    let key y be top plus 20.
    count from 1 to the number of items in values as position:
        let value be item position of values.
        if value is above 0 and total is above 0:
            let share be value divided by total.
            let start be angle.
            set angle to angle plus share times 2 times pi value.
            let x1 be cx plus radius times sine with start.
            let y1 be cy minus radius times cosine with start.
            let x2 be cx plus radius times sine with angle.
            let y2 be cy minus radius times cosine with angle.
            let large be 0.
            if share is above 0.5, set large to 1.
            if share is at least 1:
                set picture to picture followed by "<circle cx=\"[place with cx]\" cy=\"[place with cy]\" r=\"[place with radius]\" fill=\"[color for position]\"/>\n".
            otherwise:
                set picture to picture followed by "<path d=\"M [place with cx] [place with cy] L [place with x1] [place with y1] A [place with radius] [place with radius] 0 [large] 1 [place with x2] [place with y2] Z\" fill=\"[color for position]\" stroke=\"white\" stroke-width=\"2\"/>\n".
        let percent be 0.
        if total is above 0 and value is above 0, set percent to value divided by total.
        let key x be cx plus radius plus 30.
        set picture to picture followed by "<rect x=\"[place with key x]\" y=\"[place with (key y minus 11)]\" width=\"14\" height=\"14\" fill=\"[color for position]\"/>\n".
        set picture to picture followed by "<text x=\"[place with (key x plus 22)]\" y=\"[place with key y]\" font-size=\"13\" fill=\"#222\">[escaped with item position of labels]: [value with commas] ([(percent rounded to 3 decimal places) as a percent])</text>\n".
        add 22 to key y.
    give back picture followed by "</svg>".

privately to chart with kind (text) and labels (a list of text) and values (a list of numbers) and title (text), giving back text:
    if kind is "line", give back line chart with labels and values and title.
    if kind is "pie", give back pie chart with labels and values and title.
    give back bar chart with labels and values and title.

note: The three ways in: a list of numbers (labelled 1, 2, 3, ...), or a map
note: whose keys are text or numbers.
to numbers chart with values (a list of numbers) and kind (text) and title (text), giving back text:
    let labels be an empty list of text.
    count from 1 to the number of items in values as position:
        add position as text to the end of labels.
    give back chart with kind and labels and values and title.

to text map chart with values (a map from text to numbers) and kind (text) and title (text), giving back text:
    give back chart with kind and keys of values and values of values and title.

to number map chart with values (a map from numbers to numbers) and kind (text) and title (text), giving back text:
    let labels be an empty list of text.
    for each key in keys of values:
        add key as text to the end of labels.
    give back chart with kind and labels and values of values and title.
