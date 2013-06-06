arduino-rtty
============

A interrupt driven, pwm audio output , RTTY modulator for the arduino. All you need is a speaker. This program produces 45 baud baudot RTTY with 170Hz spacing with the "standard" shift (mark is high tone) by default but *is* configurable!

What this offers over other modulators:

* The ability to do other processes (so long as you can handle them being interrupted regularly).
* Audio output. I get good decodes by placing a small speaker from pin 3 to ground.
* Configurability. Want to change the output frequency, or the baud rate? Go ahead!

Sorry for the lack of comments right now. I'll tidy this repo up so that others can readily understand it.

To Do List:
* Figure out why the sample rate needed to be modified to get accurate frequencies and baud rates and if they're consistent across arduinos!
* Break the code out into a library for a better interface to the code.
* Comments
* Configurable start and stop bits.

Hope this helps you in your communication endeavors!

KG4SGP - Jim
