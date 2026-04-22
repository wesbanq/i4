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

String literals are wrapped in quotes and pushed to the data stack as-is. Anything unrecognised is also pushed as a string, making unknown tokens data rather than errors.  



---

## Examples

**Hello, World!**

```python
. "Hello, World!"
```

**An if statement definition**

```python
def >< if "udef \"\cond\"\ udef true udef false << != 0 cond def >< cond asstr def true asstr def false asstr" 
def >< udef "def ~~ asstr" def >< asstr "<< [:] ee" def ee "e e" def e "\"\\"\"
def >< true "1" def >< false "0"
```

**Truth machine**

```python
if "~= false" "loop \"\. true\"\" ". false" ? def >< loop "^: l inner : l def \"\inner\"\ asstr"
```

---

## Primitive Instructions

For most instructions that operate on the stack, if the stack is empty they will also return an empty string.  
When an instruction expects a value on the stack, it will be written as `OP <arg0> <arg1> ...`. The order of arguments is
the same as in the code stack's file.  

|Instruction|Explanation|
|---|---|
|`def <name> <meaning>`					|When encountering a non-literal string <name>, put <meaning> on the code stack|
|`: <name>`						|Create a new|
|`^: <name>`						|Replace the contents of the code stack, with the code in the label|
|`. <str>`						|Pop <str> and print|
|`?`							|Put user input on top of the data|
|`~~ <s>`						|Duplicates \<s> and puts it on top of the data|
|`# <str>`						|Put the length of <str> on top of the data|
|`@ <idx>`						|Put the character at <idx> of the string on top of the data stack. <id> wraps around the string if the index is out of bounds. Puts an empty string if stack is empty.|
|`>< <std>`						|Pops <str> and puts reversed on the data|
|`spl <sep> <str>`					|Split the <str> by <sep>, if <sep> is empty - split by character|
|`p <s>`						|Pops \<s> off the data|
|`[:] <start> <end> <str>`				|Puts a copy of <str> on top of the data stack, starting at <start> and ending at <end>. Has the same index wrapping behavior as `@`. If <start> or <end> is NaN, put a copy of the string as a literal on the stack|
|`<< <str>`						|Puts <str> on top of the code|
|`>>`							|Takes the string off the top of the code stack, and puts it on top of the data|
|`<> <str>`						|Swaps the top of the code and data stacks|
|`open <filename>`              |Pastes the entire content of <filename> into the data stack backwards. Puts empty string if file was not found, or an error happened|
|`get <addr>`						|Does an HTTP GET request, puts response on the data stack|
|`post/put/patch/delete/options <addr> <payload>`	|Does the corresponding HTTP request, puts the response on the data stack|
|`=/~=/>/</>=/<= <lhs> <rhs>`				|Compares|
|`&/\|/^ <lhs> <rhs>`					|Logic instructions. Every non-zero value or non-empty string is treated as true|
|`~ <str>`					            |Logic not|
|`+/-/*///%/** <lhs> <rhs>`				|Take a guess|
|`halt`						|Halt (delete the whole code)|

---

## Interpreter Flags

|Flag|Explanation|
|---------------|-----------------------------------------------------------------------------------------------|
|`-box`		|Create a new directory, copy the source code into it, and run the program from that directory	|
|`-mem`	 	|Run the program inside a virtual, in-memory filesystem	 	 				|
|`-step`	|Await user input before executing each instruction from the code stack	  			|
|`-limit`	|Abort if the sizes of all i4 files combined is over 10MB					|
|`-no-fs`	|Disable all filesystem instructions		 						|
|`-no-web`	|Disable all web instructions		 							|
|`-no-ext`	|-no-web and -no-fs combined									|
|`-safe`	|-no-ext and -limit combined	 								|
|`-verbose`	|Output names of all executed instructions to stdout		 				|
|`-debug`	|-mem, -step and -safe combined		 							|
|`-help`	|Print help message		 								|
|`-version`	|Print version info|

---

## License

MIT - see [`LICENSE`](LICENSE).