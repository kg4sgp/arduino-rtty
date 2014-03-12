High Altitude Balloon Telemetry
===============================

This code is being used on a high altitude balloon flight, the University of Akron's UA-HABP2 (habp.w8upd.org and wiki.w8upd.org). We are trying to take as many precautions as we can to make sure this code absolutely works with out fail.

In this code there is an abundant use of casting in places that might feel rather silly. 

For example

unsigned char i = (unsigned char)0;

This makes it painfully obvious what we want to do: assign a value of zero represented by a unsigned char to the unsigned char i. In doing this the splint static analyser also knows exactly what we want to do, namely that the types match, and doesn't complain.

