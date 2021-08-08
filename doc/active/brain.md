# Brain

Main coordination unit for all objects within a star. Controlled with Legion's
Intelligent System Program (LISP).


## Hardware

A brain is effectively a single core which executes LISP modules that have been
compiled down to a simple assembly language. The specification of a core are as
follows:

- **word**: Signed 64 bit integer which is the only supported data type.

- **registers**: 4 registers containing one word are availble. Note that
  variables in LISP modules are stored in registers which means that functions
  are limited to only 4 active variables.

- **speed**: Number of instruction executed by time tick which varies by the
  brain class. It can range from 8 to 128.

- **stack**: The stack size varies by the brain class. It ranges from 10 words
  to 34 words.

- **flags**: One of the following state flags:
  - **FLAG_IO**: Execution interupted by an io command to be processed.
  - **FLAG_SUSPENDED**: Execution suspended indifinetly.
  - **FLAG_FAULT_USER**: A user raised fault (ie. `assert` function).
  - **FLAG_FAULT_REG**: Reference to an invalid register
  - **FLAG_FAULT_STACK**: stack underflow or overflow.
  - **FLAG_FAULT_CODE**: Invalid opcode executed
  - **FLAG_FAULT_MATH**: Mathematical impossibility (eg. division by 0).
  - **FLAG_FAULT_IO**: Invalid io operation


## IO

IO commands are used to give commands to other objects or communicate with other
brain within a star system. The LISP syntax for executing an IO command looks as
follows:

```lisp
(io <command> <id> [args...])
```

- **command**: Word indicating which IO command to execute. An atom is provided
  for each IO command where the symbol of the atom correspond to the io commands
  detailed below (eg. the ping io command uses the atom `!io_ping`).

- **id**: The target of the IO command which consists of the type of item and
  it's sequential id. These ids can be constructed using the `id` LISP statement
  (eg. `(id !item_brain_1 1)`). To refer to yourself, the id `0` can be used.

- **args**: The list of arguments for the IO command and the details are command
  dependent.

Here's a few typical IO invocations examples:

```lisp
; Ping a component to test whether it exists
(io !io_ping (id !io_extract_1 1))

; Configure an extractor to run the tape of element a.
(io !io_item (id !io_extract_1 1) !item_elem_a)
```

The return values of IO commands are pushed on top of the stack in this order
(`>` indicates the top of the stack):

```
> <result> | [values] | ...
```

- **result**: Either the atom `!io_ok` or `!io_fail` which indicates whether the
  command was executed or not.

- **values**: The values returned by the command if any. This is entirely
  command and item dependent.

Given that the result is at the top of the stack, it will always be treated as
the return value of the IO command. Any additional values must be read using the
special `head` LISP function. As an example:

```lisp
(let ((coord (progn

               ; Get the coordinate of the current star and check the result
               (assert (= (io !io_coord 0) !io_ok))

               ; !io_coord return the coordinate on the stack which we access here.
               (head))))

	; Use the coordinates from the previous io command to execute another io command
	(io !io_scan (id !item_scanner_1 1) coord !item_elem_a))
```

The rest of this section will detail the IO commands supported by the Brain item.

### io_ping

```
(io !io_ping <id>) -> (result)
```

Purpose: Tests whether an item exists.

Example:

```lisp
(for (i 1) (i < 10) (+ i 1)
  (when (= (io !io_ping (id !item_extract_1 i)) !io_ok)
	(io !io_item (id !item_extract_1 i) !item_elem_a)))
```

### io_status


```
(io !io_status <id>) -> (result mod)
```

Purpose: Tests whtether the current brain is active by returning which module is
currently running.

Example:

```lisp
(io !io_mod (id !item_brain_1 2)
  (progn (io !io_status (id !item_brain_1 1)) (head)))
```

### io_mod

```
(io !io_mod <id> <mod>) -> (result)
```

Purpose: Reset the state of the brain and execute the provided module

Example:

```lisp
(io !io_mod (id !item_brain_1 2)
  (progn (io !io_status (id !item_brain_1 1)) (head)))
```

### io_reset

```
(io !io_reset <id>) -> (result)
```

Purpose: Resets the state of the brain including the core, the inbox and debug
state.

Example:

```lisp
(io !io_reset (id !item_brain_1 1))
```

### io_coord

```
(io !io_coord <id>) -> (result coord)
```

Purpose: Returns coordinate of the star current star system.

Example:

```lisp
(let ((coord (progn (io !io_coord 0) (head))))
	(io !io_scan (id !item_scanner_1 1) coord !item_elem_a))
```

### io_send

TBD

### io_recv

TBD

### io_dbg_attach

```
(io !io_dbg_attach <id>) -> (result)
```


Purpose: Pauses the core for manual inspection and enables the use of
`io_dbg_step`. Note that this is generally used via the game interface and
rarely through the LISP modules.

Example:

```lisp
(let ((target (id !item_brain_1 2)))
  (io !io_dbg_attach target)
  (for (i 0) (< i 20) (+ i 1) (io !io_dbg_step target))
  (io !io_dbg_detach target))
```

### io_dbg_detach

```
(io !io_dbg_attach <id>) -> (result)
```

Purpose: Resume execution of the core.

Example:

```lisp
(let ((target (id !item_brain_1 2)))
  (io !io_dbg_attach target)
  (for (i 0) (< i 20) (+ i 1) (io !io_dbg_step target))
  (io !io_dbg_detach target))
```

### io_dbg_break

```
(io !io_dbg_break <id> <0 | ip>) -> (result)
```

Purpose: Sets a breakpoint on a given instruction pointer (IP) and reduces the
core speed to 1. When the IP of the core reaches the the breakpoint then the
execution is paused for manual inspection. If a breakpoint of 0 is provided then
the breakpoint is cleared and the speed of the core is reset to it's default
value.

### io_dbg_step

```
(io !io_dbg_break <id>) -> (result)
```

Purpose: Executes a single instruction of the core. Only usable when the core
has been paused via the `io_dbg_attach` or `io_dbg_break` command.

Example:

```lisp
(let ((target (id !item_brain_1 2)))
  (io !io_dbg_attach target)
  (for (i 0) (< i 20) (+ i 1) (io !io_dbg_step target))
  (io !io_dbg_detach target))
```
