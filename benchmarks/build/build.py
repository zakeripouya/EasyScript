# Inside a function, CPython grows a local string in place when nothing else
# holds it, as EasyScript does (at the top level, `text = text + ...` copies
# every time and takes over two minutes).
def main():
    text = ""
    for _ in range(1_000_000):
        text += "word "
    print(len(text))


main()
