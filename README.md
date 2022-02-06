elecmidi is a C program to help write electribe 2 patterns. It connects with the electribe 2 and modifies the current pattern.

compilation:

  In the text of elecmidi.c, at the beginning are defined:

device: the midi device to connect.

channel: the midi channel.
  
  To compile, type make


usage:

echo "command" | ./elecmidi

or

cat script | ./elecmidi


example1.txt, example2.txt are script examples.

if a command ends in ! then it is an immediate command. For example:

echo "name Pattern1 !" | ./elecmidi

is equivalent to the following 3 lines.

read

name Pattern1

write

Command line arguments:

-d device  

-c channel  (1-16)
