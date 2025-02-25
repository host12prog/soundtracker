# soundchip dump format

the format is very similar to the one used in family BASIC for the play command.

every property may take a value from a Lisp expression if it contains parentheses (e.g. `f(* 256 4)`.

## proper format

+ $: select channel.
+ f: set frequency. 4 hexadecimal digits as argument.
+ V: volume (0-127).
- P: panning (char).
+ Y: duty (0-127).
+ S: shape (0-7).
+ c: cutoff (short).
+ r: resonance (char).
+ I: filter flag (0-7).
- O: base octave (0-9).
- e: reset channel.
- g: reset filter.
- h: set ring/sync flags.
+ R: finish tick, then rest for a number of ticks.
+ ;: finish tick.

- [#]CDEFGAB: notes. appending a number will add that number of octaves to the base one.

- M: enable sweep units (0-7).
- v: configure volume sweep.
- l: configure filter sweep.
- k: configure freq sweep.

- p: enable/configure PCM.

- T: set sync period.

- /: go to line.

- =: execute Lisp expression/ignore value.
- ?: go to line if expression is true.

## sweep config format

- frequency and filter:
  - SSSSaabb
    - S: speed. a: amount/dir. b: bound.
- volume:
  - SSSSaabb
    - S: speed. a: amount/dir/loop/loopi. b: bound.
    
## Lisp symbols

the SSLisp interpreter is minimalistic, and therefore only supports one or two-letter symbol names.

the following symbols are set by ssinter:

- `xf`: frequency
- `xv`: volume
- `xc`: cutoff

- `xl`: current frame

## Lisp functions

- `(note x)`: calculate note frequency for `x`
- `(trans x)`: calculate transpose for `x` semitones
