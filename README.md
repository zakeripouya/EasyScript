# EasyScript

EasyScript is a pseudo-code-like programming language that compiles to C. It features readable English statements, object-oriented and functional programming capabilities, and advanced error handling.

## Features

- **PRINT ##**: Prints a value.
- **MAKE A FOR LOOP**: Creates a for loop.
- **MAKE A FUNCTION**: Defines a function.
- **IF**: Conditional statement.
- **ELSE**: Else statement.
- **VARIABLE DECLARATION**: Declares a variable.
- **ASSIGNMENT**: Assigns a value to a variable.
- **FILE OPEN filename**: Opens a file.
- **FILE READ filename**: Reads from a file.
- **FILE WRITE filename content**: Writes to a file.
- **FILE CLOSE filename**: Closes a file.
- **CALL function_name**: Calls a function.

## Installation

1. Clone the repository:

    ```bash
    git clone https://github.com/zakeripouya/easyscript.git
    cd easyscript
    ```

2. Build the project:

    ```bash
    make
    ```

## Usage

```bash
./easyscript run script.es            # compile and run
./easyscript build script.es -o app   # compile to an executable
./easyscript emit script.es           # print the generated C
./easyscript tokens script.es         # print the tokens (new lexer)
./easyscript                          # interactive shell
make test                             # run the test suite
```

Example `script.es` (old syntax, being replaced):

```plaintext
MAKE A VARIABLE x ASSIGN 10
MAKE A VARIABLE y ASSIGN 20
PRINT # x
PRINT # y
FILE OPEN myfile
FILE WRITE myfile HelloWorld
FILE READ myfile
FILE CLOSE myfile
```
