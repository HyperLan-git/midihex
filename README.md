# MidiHex
A raw midi data editor that can be used to edit the events and headers in a midi file.
You can also get some insight on how the midi standard is designed (badly).
My original motivation for making this was precisely controlling the tempo data in my files because my DAW and other editors kept deleting it.

## Compile
Just use `make`. You will need imgui cloned in the parent folder on branch *docking* and opengl + glfw installed.

The binary will be found as *bin/midihex*.

## Example image
![Image](https://github.com/HyperLan-git/midihex/blob/main/screenshot.png)
