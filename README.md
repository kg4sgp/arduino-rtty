arduino-RTTY
============

To use this:

* Open arduino
* Open sketch, and navigate to rtty/rtty.ino
* Upload to Arduino.

A interrupt driven, PWM audio output , RTTY modulator for the arduino. All you need is a speaker. This program produces 45 baud baudot RTTY, 170Hz spacing, the "standard" shift (mark is high tone) and two stop bits by default but *is* configurable!

A low pass filter on the output of the modulator is desirable but not absolutely necessary (it will just be a marginally dirty audio signal if you don't add the filter). I'll update with a simple RC circuit in due time.

What this offers over other modulators that I've found on github:

* The ability to do other processes (so long as you can handle them being interrupted regularly).
* Audio output. I get good decodes by placing a small speaker from pin 3 to ground.
* Configurability. Want to change the output frequencies, or the baud rate? Go ahead!

Sorry for the lack of comments right now. I'll tidy this repo up so that others can readily understand it.

To Do List:
* Figure out why the sample rate needed to be modified to get accurate frequencies and baud rates and if they're consistent across arduinos!
* Break the code out into a library for a better interface to the code.
* Comments
* Configurable start and stop bits.

Hope this helps you in your communication endeavors!

KG4SGP - Jim
