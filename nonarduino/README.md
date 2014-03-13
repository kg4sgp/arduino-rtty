High Altitude Balloon Telemetry
===============================

This code is being used on a high altitude balloon flight, the University of Akron's UA-HABP2 (habp.w8upd.org and wiki.w8upd.org). We are trying to take as many precautions as we can to make sure this code absolutely works with out fail.

Some of these precautions include:

* Enabling all of the warnings we can on the avr-gcc compiler taking any warnings as errors
* Statically analysing our code using splint

We make sure the avr-gcc compiler compiles without any warnings whatsoever. We try to reduce the amount of errors reported by splint to the maximum extent we can.

Subsequently in this code there is an abundant use of casting in places that might feel rather silly. We are not trying to obfuscate the code, or quite the compiler because we don't know what were doing. We are trying to make it painfully obvious what the compiler should do with our code and how the static analyser should interpret our code.

For example:

unsigned char i = (unsigned char)0;

This makes it painfully obvious what we want to do: assign a value of zero represented by a unsigned char to the unsigned char i. In doing this the compiler and splint static analyser knows exactly what we want to do, namely that the types match, and doesn't complain.

