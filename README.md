elecmidi is a C program to help write electribe 2 patterns. It connects with the electribe 2 and modifies the current pattern.

compilation:

  In the text of elecmidi.c, at the beginning are defined:

device: the midi device to connect.

channel: the midi channel.
  
  To compile, type make


usage:

echo "command" | ./electmidi

or

cat script | ./elecmidi


example1.txt, example2.txt are script examples.

Command line arguments:

-d device  (with -d 0 the program doesn't connect to any device)

-c channel  (1-16)
