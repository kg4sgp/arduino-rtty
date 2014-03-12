High Altitude Balloon Telemetry
===============================

This code is being used on a high altitude balloon flight. We are trying to take as many precautions as we can to make sure this code absolutely works with out fail.

In this code there is an abundant use of casting in places that might feel rather silly. 

For example

unsigned char i = (unsigned char)0;

This make it painfully obvious what we want to do: assign a value of zero represented by a unsigned char to the unsigned char i. In doing this the splint static analyser also knows exactly what we want to do and doesn't complain.

