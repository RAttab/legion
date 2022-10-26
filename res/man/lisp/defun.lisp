{@LISP/DEFUN}

{=SYNOPSIS}

{n} ({*defun} {_name} ({_parameters}) <statements>) -> 0


{=DESCRIPTION}

Creates user-defined functions to encapsulate a list of
statements. Typically used to avoid duplicating sequences of
operations that are frequently used.

Each function is associated to a symbolic {_name} by which it will be
referred to when calling the function. It must be unique within the
current mod and with respect to the predefined core functions provided
by the Legion system.

{_parameters} defines a list of zero to four parameter variables that
will be bound to each of the arguments provided by the caller. Each
parameter is specified by a symbol that must be unique within the
parameter list. Each arguments will be bound to a parameter variable
in the written order where the first argument will be bound to the
first parameter, the second argument to the second parameter, etc.

If {_parameters} is empty then no arguments will be bound. If too few
parameter variables are specified then the extra arguments will not be
bound to variables. If too many parameter variables are specified then
the content of the extra variables will be undefined.

Note that Legion imposes a limit of four variables within a given
execution scope and each of the parameter variables will be active for
the entire scope of the function. This limits the number of parameter
variables to four and implies that if four parameter variables are
specified then no additional variables can be defined within the body
of the function.

The body of the function is composed of a list of {_statements} that
will be executed in sequence. The return value of the last statement
will be returned to the caller of the function. It is not possible to
return multiple values to the caller.

All functions must be defined at the top-level of a mod and can't be
nested. Calling a user-defined functions can be done using the same
syntax as calling one of the core Legion functions and can be called
before or after they have been defined in a mod. Additionally, all
user-defined functions can be called from other mods by using the
following syntax:

{`
;; Calls my-fn in the most recent version of my-mod
(my-mod.my-fn 1 2 3)

;; Calls my-fn in version 2 of my-mod
(my-mod.2.my-fn 1 2 3)
}


{=RETURN}

{*defun} always returns the value 0. This is not to be confused by the
return value of the user-defined function which will be the return
value of the last statement in the body.


{=ASSEMBLY}

{`
(asm
  (JMP @end)
  <statements>
  (SWAP)
  (RET)
  (@ end))
  (PUSH 0)
}


{=EXAMPLE}

{`
;; Creates a function to count the number of instances of an item
;; within our current star.
(defun count (item)
  (io !io-probe (id !item-prober 1) item (ior !io-coord (self)))
  (let ((count -1))
    (while (< count 0) (set count (ior !io-value prober-id)))
    count))

(count !item-elem-a)
}


{=RELATED}
