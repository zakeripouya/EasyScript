# Words that a sentence about email or web pages would use keep their meaning
# as the program's own names and definitions (proposal 0021).
let website be "shop.example.com".
show website.
to send with message:
    say "Sending:", message.
send with "hi".
let page be "https://example.com".
say "Visit", page.
every 1 seconds:
    send with "from an event".
    stop the program.
