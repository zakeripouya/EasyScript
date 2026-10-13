# Installing EasyScript

EasyScript is one program, `easyscript`, built from source with a C compiler. It turns your `.es` files into C and builds them with the C compiler too, so the compiler has to stay installed.

## What you need

- **macOS:** the Xcode command line tools (`xcode-select --install`), which give `cc` and `make`.
- **Linux:** a C compiler and `make`, for example `sudo apt install build-essential` on Debian and Ubuntu, or `sudo dnf install gcc make` on Fedora.
- **Windows:** use WSL (below). Running natively on Windows comes later.

## Build and install

From the EasyScript folder:

```
make
sudo make install
```

`make` builds `./easyscript`. `make install` copies it to `/usr/local/bin/easyscript`. To put it somewhere else, set `PREFIX` (the program goes in `PREFIX/bin`):

```
make install PREFIX="$HOME/.local"
```

No `sudo` is needed when you can write to `PREFIX`. Make sure `PREFIX/bin` is on your `PATH` (`$HOME/.local/bin` usually is).

## Check that it works

```
easyscript --version
```

shows `EasyScript 0.2.0 (the 2026 edition)`. Then make a first program and run it:

```
easyscript new hello
cd hello
easyscript run main.es
```

It says `Hello!`. `easyscript new NAME --template report` starts from a report instead (see `easyscript new --list`), and `easyscript` on its own opens the interactive shell.

## Uninstall

```
sudo make uninstall
```

(or `make uninstall PREFIX=...` with the same `PREFIX` you installed with) removes `PREFIX/bin/easyscript`. Your programs are untouched.

## Windows, through WSL

EasyScript runs on Windows inside WSL (the Windows Subsystem for Linux):

1. In PowerShell as administrator, run `wsl --install`, then restart.
2. Open "Ubuntu" from the Start menu and run `sudo apt install build-essential git`.
3. Get EasyScript (`git clone ...`) and follow "Build and install" above.

Your Windows files are under `/mnt/c/Users/YOUR NAME/` inside WSL. A native Windows version, without WSL, comes later.

## Homebrew

A Homebrew formula is being prepared (`packaging/homebrew/easyscript.rb`); until it's published, build from source as above.
