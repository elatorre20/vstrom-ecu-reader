#  2006 Vstrom Gear Indicator and ECU interface

This is a device that uses the diagnostic interface on a 2006 Suzuki Vstrom DL650 to provide a gear indicator. Although the exact setup is specific to my bike (it would probably also work on other year Vstroms and SV650s), it could be fairly easily modified to work with most motorcycles of a similar vintage, or any vehicle that supports the K-Line/KWP2000 interface. 

## Inspiration/Motivation

I came across [this project](https://hackaday.com/2018/09/11/adding-upgrades-to-a-stock-motorcycle/) on google and wanted to install it on my motorcycle. However, after looking into the technical aspects, I figured that the $50 sparkfun ELM327 board the author used was not necessary and could be eliminated with a few transistors a bit more code. I also did not like the display setup, so I wanted to change that as well.

## Important links

All of these were very useful and informed my design:

*[An explanation of the K-Line/KWP2000 physical and data layers and how to interface it with TTL logic](https://m0agx.eu/reading-obd2-data-without-elm327-part-2-k-line.html)
*[The original project that I started from](https://hackaday.com/2018/09/11/adding-upgrades-to-a-stock-motorcycle/)
*[Many resources about the diagnostic interface on Suzuki motorcycles](https://github.com/synfinatic/sv650sds/wiki)
*[The display setup that I ended up copying](https://www.youtube.com/watch?v=Gqyt9sBvENI)

## Technical Outline

The main control module is an Arduino pro micro. You will have to provide 5v regulated power. There is some code in the repo from an earlier version in which I was going to use a Waveshare RP2040 1.28" LCD module, but it is unfinished and does not work. The timer interrupt portion used for scheduling is specific to ATMega32u4-based Arduinos and clones, as it uses the register names and interrupt handler directly. This saves 2kb of program memory compared to using the generic Arduino timer interrupt library, but may require some tweaking for other Arduino compatible boards. As K-Line communication is just a half-duplex UART, an ELM327 is not necessary and the Arduino can communicate with it directly through the Serial1 peripheral. The Arduino RX/TX are connected to the K-Line through a level shifter circuit. You can use whatever you like to do this, I used a circuit very similar to that described in the links, however be careful that you must not invert the output on the K-Line relative to the Arduino serial. My final circuit diagram is as shown. The display is an SSD1306 OLED module mounted inside the RPM dial. Like in the second video, I also added a temperature display. I used a DHT11 sensor because I had one lying around, but you could easily remove this and just used the gear display function by commenting out the associated lines.

![the circuit diagram](/circuit_diagram.png)