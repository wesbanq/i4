# i4

**Instantaneously Interpreted Instruction Interface** *(i4, for short)* is a small and simple concatenative, stack-based esolang, that uses the code of the program as a second stack and the user's filesystem for storing state.

## Language Overview

Most interpreted languages include some sort of compilation step, be that bytecode or some sort of IR. The goal of i4 is to create a language that is *truly* interpreted — no intermediate representation, no hidden compilation. The source file is the runtime state, and the runtime state is the source file. At any point during execution, you can `cat` the code stack and see exactly what the interpreter is about to do next.

This comes with an unconventional tradeoff: instead of polluting your RAM, i4 fully unlocks the untapped potential of your hard drive. Memory safety is achieved by not using memory.

---

## How It Works

i4 maintains two stacks, both stored as plain text files on disk. The **code stack** contains the program currently being executed. The **data stack** holds values the program is working with.

Both stacks share the same mechanic — pushing is appending to the file, and popping is reading and truncating from the end. This is a deliberate design choice: reading the last token and truncating the file is an O(1) operation on any modern filesystem. Reading from the beginning and rewriting the file on every pop would be O(n) in the size of the stack — a significant difference for large programs.

Programs are written *backwards*. The interpreter reads tokens from the end of the code file, executing each one and truncating it away. When a user-defined word is encountered, its definition is appended to the code file — becoming the next thing to execute. This means function calls are literally just text being written to a file you can inspect at any moment.

Tokens are separated by whitespace. String literals are wrapped in quotes and pushed to the data stack. Anything unrecognised is also pushed as a string, making unknown tokens data rather than errors.

For a program `prog.i4`, the data stack lives in `prog.i4s`, each definition in `prog.i4d_<name>` and each label in `prog.i4l_<name>`, all next to the program. Running a program consumes it: the code file is eaten as it runs, and the data stack, definitions and labels stay on disk afterwards, where the next run will find them. Use `-box` to run a copy instead, or `-memory` to keep everything in memory.

### Everything reads backwards

Because the stacks are read from the end, text gets reversed whenever it makes a trip through the data stack:

- Text you write in the code comes out as written: `. "Hello"` prints `Hello`.
- Text an instruction puts back on the data stack comes out reversed: copies (`~~`), joined strings (`+`), slices (`[:]`), split parts (`spl`), input (`?`), program arguments and file contents (`open`). Two trips cancel out, which is also how `><` reverses: it's a trip that does nothing else.
- Numbers from arithmetic, comparisons, logic and `#`, and the single characters from `@`, always read normally.
- `<<` and `def` turn whatever text they're given into code, so give them text that reads normally.

Words are matched backwards too: `def` defines the word whose name is its argument reversed. So to define `foo`, write `def >< foo ...` (`><` reverses the name back); palindromes like `e` work either way. To redefine a word, quote its name (`def >< "foo" ...`), otherwise the old definition runs instead of being named.

### Escapes

Escapes are written `\X\`, so they read the same in both directions:

|Escape|Meaning|
|---|---|
|`\n\`|Newline|
|`\t\`|Tab|
|`\r\`|Carriage return|
|`\0\`|Null character|
|`\\\`|Backslash|
|`\"\`|Quote|

Any other `\X\` is kept as is, and a lone `\` is just a backslash. A quote with nothing to close it is an ordinary character. Escapes work outside quotes too (`foo\n\bar`), and text from input, program arguments and files can safely contain quotes and backslashes.

Escapes stay as they are on the stack and are only decoded when the text is used: printed (`.`), defined (`def`), run (`<<`), used as a file name or written to a file (`open`, `write`), or returned as the program's result. So `#`, `@` and `[:]` see the raw characters: `# "a\n\b"` is `5`.

---

## Examples

**Hello, World!**

```python
. "Hello, World!"
```

**An if statement definition**

```python
def >< if "<< + \"\fi\"\ cond def >< \"\0if\"\ def >< \"\1if\"\ def >< \"\cond\"\"
```

`if <cond> <then> <else>` takes three pieces of code. Reading the definition from the right, it saves `<cond>` as the word `cond`, `<then>` as `1if` and `<else>` as `0if`. Then it runs `cond`, joins the result onto `"fi"` with `+` (which reverses, giving `1if` or `0if`), and runs that word with `<<`.

**Truth machine**

```python
if "~= 0" "^: l . 1 : l" ". 0" ?
```

Reads a number: `0` prints `0` and stops, anything else prints `1` forever. `: l` saves the rest of the code as the label `l`, and `^: l` jumps back to it. Put the `if` definition below this line: the file runs from the end, so definitions have to come after the code that uses them.

---

## Primitive Instructions

When an instruction takes values from the data stack, it's written as `OP <arg0> <arg1> ...`. The order of arguments is the same as in the code stack's file: `- 5 3` is `2`.

Popping an empty stack gives an empty string, and an instruction that can't produce a result (a division by zero, a non-number where a number is needed, ...) pushes an empty string.

|Instruction|Explanation|
|---|---|
|`def <name> <meaning>`|Define the word `<name>` (reversed, see above) as the code `<meaning>`, replacing any earlier definition. When the word is reached, its code is put on the code stack|
|`: <name>`|Save the rest of the code stack as the label `<name>`, replacing any earlier label with that name|
|`^: <name>`|Replace the code stack with the code saved in the label `<name>`. Does nothing if there's no such label|
|`. <str>`|Pop `<str>` and print it (no newline)|
|`?`|Print `? `, read a line of input and push it|
|`~~ <str>`|Duplicate `<str>`|
|`p <str>`|Pop `<str>`|
|`# <str>`|Push the length of `<str>`|
|`@ <idx> <str>`|Push the character at `<idx>`. The index wraps around in both directions, so `-1` is the last character|
|`>< <str>`|Reverse `<str>`|
|`spl <sep> <str>`|Split `<str>` by `<sep>`, leaving the first part on top. An empty `<sep>` splits into characters|
|`[:] <start> <end> <str>`|Push the part of `<str>` from `<start>` up to `<end>`. Indices wrap like `@`, except that an `<end>` equal to the length means the end of the string, and a `<start>` past `<end>` wraps around the end. A non-number `<start>` or `<end>` means the start or end of the string|
|`<< <code>`|Put `<code>` on top of the code stack, so it runs next|
|`>>`|Move the next token from the code stack to the data stack|
|`<> <str>`|Swap the top of the code stack with `<str>`|
|`open <path>`|Push the contents of the file at `<path>`, relative to the program. Pushes an empty string if there's no such file|
|`write <path> <text>`|Write `<text>` to the file at `<path>`, relative to the program, replacing what's there|
|`= ~= < > <= >= <lhs> <rhs>`|Compare, pushing `1` or `0`. Numbers compare as numbers; if either side isn't one, both compare as text|
|`& \| ^ <lhs> <rhs>`|And, or, xor, pushing `1` or `0`. Non-zero numbers and non-empty strings are true|
|`~ <str>`|Not|
|`+ - * / % ** <lhs> <rhs>`|Take a guess. Integers stay integers; anything involving a float, and every `/` and `**`, gives a float with 6 decimals. `+` joins text when either side isn't a number. Apart from `+`, an empty value counts as `0`|
|`halt`|Halt (delete the whole code)|

---

## Input and Output

Program arguments go after `--` and are pushed in order, so the last one ends up on top: `i4 prog.i4 -- first second`.

When the code stack runs out, the top of the data stack is the program's result. A number becomes the exit code, `OK` or an empty stack exits with `0`, and any other text exits with `1`. The interpreter prints the result after the program's output as `Program finished with code: <result>`.

---

## Interpreter Flags

```
i4 [options] <file> [-- <program args>]
```

|Flag|Explanation|
|---|---|
|`-box`, `-b`|Copy the program into a fresh `<name>.i4box` directory next to it and run it there, so the original survives. Only the program file is copied|
|`-memory`, `-m`|Run inside a virtual, in-memory filesystem: the i4 files in the current directory are read at startup, any other file the first time it's used, and nothing is written to disk|
|`-step`, `-S`|Wait for Enter before executing each instruction|
|`-nofs`, `-F`|Disable `open` and `write` (they become ordinary words)|
|`-safe`, `-s`|`-nofs`, plus abort if the `.i4` files under the program's directory grow past 16 MiB|
|`-verbose`, `-v`|Print every token as it runs, in its internal reversed form|
|`-debug`, `-d`|`-memory`, `-step` and `-safe` combined|
|`-help`, `-h`|Print help message|
|`-version`, `-V`|Print version info|

---

## Building

You need [Ninja](https://ninja-build.org), `clang++`, `llvm-ar` and a POSIX `sh` (Git Bash works on Windows). Git is optional, for the version string.

```sh
ninja                   # builds build/i4 (build/i4.exe on Windows)
scripts/run_tests.sh    # runs the programs in tests/ and checks their output
```

---

## License

MIT - see [`LICENSE`](LICENSE).