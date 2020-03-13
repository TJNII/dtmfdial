DTMF Dial
=========

Dial generates the dtmf tone signals used for telephone
dialing and by default sends the signals to stdout.
The output can be piped directly into ```aplay -``` to play the DTMF.

Usage:
Usually you just run need to give the phone number
that you wish to dial as an argument.

```
# Local usage:
$ dial 555-3456 | aplay -

# Docker usage
$ docker build -t dial .
$ docker run --rm dial 555-3456 | aplay -
```

By default the tone generated for each dialed digit is
100 milliseconds. Silence between digits is 50 milliseconds.
A comma or a blank space between digits is replaces with
a 500 milliseconds delay.

The duration values can be set by these arguments:
  --tone-time   milliseconds 	(default 100)
  --silent-time milliseconds	(default 50)
  --sleep-time  milliseconds	(default 500)

By default the output is sent to stdout. Generated samples
are 8 bits unsigned values (AFMT_U8). 8000 samples/second.

To send the output to another device one can use the option:
  --output-dev device		(default -)
This option can be used also to store raw audio samples to
a file, or to stdout. Use the file name with the --output-dev
option, or a simple "-" to sent to stdout.
If the output device is not the sound special file then another
option should be used to eliminate attempts to set the sound
driver parameters:
  --use-audio  onoff	(default 0)
Use "--use-audio 1" to enable sound ioctls.

It is possible to generate the samples in AFMT_S16_LE format,
To do that use the option:
  --bits  bits-per-sample  (default 8)
8 will generate AFMT_U8 format, 16 will generate AFMT_S16_LE format.

It is possible to generate samples in a different sampling rate.
To do that use the option:
  --speed sampling-rate    (default 8000)

Sound samples are generated by sub-sampling values from a cosine
table. By default the cosine table contains 256 samples.
Smaller cosine table sizes still produce the correct frequencies
but with some distortion.
These options can be used:
  --table-size size		(default 256)
The table used is of a different size
  --volume volume		(default 100)
Values stored in the table are scaled down volume/100 times
the maximum value which does not cause overflow on the 8 or
16 bit samples.

To generate a stereo sound use the options --right or
--left. By default they are 0 and a mono sample is generated.
If any of these option are 1, a stereo sample will be generated
and the corresponding channel will be turned on.

Copyright and License
=====================

(C) 1998 Itai Nahshon, nahshon@actcom.co.il
Use and redistribution are subject to the GNU GENERAL PUBLIC LICENSE.