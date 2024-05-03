# The DIY Dash Gauges

This is a custom built prop; there is no complete kit available. 

CircuitSetup at this point only offers the [bezel](https://circuitsetup.us/product/delorean-time-machine-dash-plutonium-gauge-bezel/); a Control Board which allows using a wide range of meters will be available soon. This prop was made to be compatible with the CircuitSetup line of movie props ([Time Circuits Display](https://tcd.out-a-ti.me), [Flux Capacitor](https://fc.out-a-ti.me), [SID](https://sid.out-a-ti.me)).

This manual is in transition; the new Control Board is near the end of its design stage but below document is not fully updated yet.

![Dash Gauges](img/thedg_n.jpg)

The Panel consists of several parts:
- Aluminum (Aluminium for non-Americans) enclosure; the measurements are in the ["enclosure"](/DIY/enclosure) folder of this repository. Can be bought at [CircuitSetup](https://circuitsetup.us/product/delorean-time-machine-dash-plutonium-gauge-bezel/) (does not fit model 142 gauge, see [here](#roentgens)).
- The gauges: The smaller gauges depicted are H&P 631-14672 (built by Phaostron) and the "Plutonium chamber" gauge is a Simpson 49L VU Meter. Movie-accurate faces for these gauges are in the ["faces-labels"](/DIY/faces-labels) folder of this repository.
- A Control Board, mounted on the smaller gauges.

### Control board

The Control board is mounted on the smaller gauges. Its features include
- 5V or 12V power supply (12V needed for digital gauges)
- audio, with speaker connector
- SD card slot
- a DAC for driving "analog" gauges with arbitrary voltages between 0 and 5V, and support for "digital" gauges (0/12V); room for user-mountable resistors to adjust voltage to gauge type used
- "Legacy" connector with pins for 12V digital Roentgens gauge, 12V Roentgens backlight, 12V "Empty" LED,
- Time Travel button, plus an additional multi-purpose button ("Button 1"); Time Travel connector for external button
- Connector for two Door Switches, for Door-Sound play back.

#### "Analog" vs. "Digital" gauges

The terms "analog" and "digital" have the following meaning in this document:

"Analog" gauges are ones that can show arbitrary values, ie move their pointers to arbitrary positions by using variable voltages. Best suited are voltmeters or VU-meters. Voltmeters can be usually driven with minimal voltages, even if their scale is far beyond that. It is mostly a matter of removing the meter's internal resistor(s), and putting suitable resistors on the Control Board. The Control Board can provide up to 5V and has room for two through-the-hole resistors per gauge. The firmware can easily be extended to define custom analog gauge types as regards their voltage range.

"Digital" gauges are ones that can only be controlled by power-on and power-off for "full" and "empty" pointer positions, respectively; this is useful if the gauge needs voltages beyond what the Control Board can provide (which is, as said, 5V), and is driven using external power and through a relay. One might also come up with the idea to create a gauge replica using a stepper motor and some logic to move the pointer to a fixed position on power-on, and reset it to the "Empty" position on power-loss. 

When using Digital gauges, the Control Board must be powered through the three-pin "12V" power connector, with the "DG+" and "+" pins shorted.

#### Control Board Hardware Configuration

In order to make the Control Board as versatile as possible, there are some solder jumpers (pads which need to be connected using solder), and easy-to-solder through-the-hole resistors which need to be added by the end user:

- "Light Power": Solder jumpers for internal or external gauge backlight power: Connect either INT or EXT. For 5V lighting, close INT. To use external power, connect the power supply to "Ext. Light Power", and close EXT. If all gauges are lit through LEDs, INT is preferred.
- LED1, LED2: Backlight LEDs for left and center gauge. These are soldered to the back of the Control Board so they directly stick into the gauge's enclosure (which naturally requires a hole in that enclosure, see below).
- R7, R8: Resistors for backlight LEDs of left and center gauge. The supply voltage is 5V (INT) or whatever you connect to "Ext. Light Power" (EXT). The resistor value depends on LED type and desired brightness. Example: 150R for a yellow LEDs at 5V (INT).
- R11: Resistor for Roentgens backlight on "Roentgens Light" connector [2]. If using lamps, just bridge. The supply voltage is 5V (INT) or whatever you connect to "Ext. Light Power" (EXT).

Hardware configuration for "analog" gauges:
- Left gauge:
  - R3, R4: Populate depending on gauge and supply voltage; see below
  - Close ANA4 solder jumper
- Center Gauge:
  - R1, R2: Populate depending on gauge and supply voltage; see below
  - Close ANA2 solder jumper
- "Roentgens" gauge (connected to "Analog Roentgens" connector [3]):
  - R5, R6: Populate depending on gauge and supply voltage; 

Configuration for digital gauges (requires 12V power):
- Left gauge:
  - R3/R4: Leave unpopulated
  - Close DIG4 solder jumper
  - Bridge DIG3 by wire (or resistor, depending on gauge type)
- Center gauge:    
  - R1/R2: Leave unpopulated
  - Close DIG2 solder jumper
  - Bridge DIG1 by wire (or resistor, depending on gauge type)
- Roentgens gauge (connected to "Digital Roentgens" connector, pins 1 (+) and 2 (-)):
  - R5/R6: Leave unpopulated
  - Bridge LEG5 by wire (or resistor, depending on gauge type)

You can mix analog and digital gauges; the firmware provides a type selection for each single gauge.

### Gauges

*Important*: When using analog gauges in your project, do not connect the actual gauges until you selected the right hardware type in the Config Portal and have powered-down and powered-up the device once afterwards. This power-cycle is needed to reset the hardware to the correct settings and to avoid a power output too high at boot.

#### "Primary", "Percent Power"

For the smaller gauges I used two H&P 631-14672 (built by Phaostron). The pointers on these are red, but an Edding permanent marker was quickly able to change them to black. These gauges need 8k2 + 470R resistors (R1=470R, R2=8k2; R3=470R, R3=8k2) and the "H&P 631-14672" gauge type setting.

Other proven-to-work options are 
- the Phaostron 0-5KV DC voltmeter (300-07970). These need no internal modifications. They work fine with 4k7 + 470R resistors (R1=470R, R2=4k7; R3=470R, R3=4k7) and the "Generic (0-5V)" gauge type setting.
- the Paostron "Cyclic Trim" meters (631-15099) with a minor modification: The two resistors and the pot inside the meter need to be removed, and the wire (which lead to the pot) needs to attached to the input terminal. With that modification, the meters work fine with 4k7 + 470R resistors (R1=470R, R2=4k7; R3=470R, R3=4k7) and the "Generic (0-5V)" gauge type setting.

Unusable:
- Phaostron 0-50/100/250/500A ammeter (639-16341).

Avoid Ammeters (Ampere meters) for currents >1A, and voltmeters for high voltages (>50V); those have stronger coils that cannot be used with low voltages. Otherwise, Ammeters (especially if the scale is in the milliampere range) can most likely be used after removing shunts, resistors or anything else that is between the two input terminals (apart from the coil, of course). 

To find out a suitable resistors value, use a common 5V power supply (eg one for Arduino), and start out with a 8k2 resistor between the + output of the power supply and the + of the gauge (usually the left terminal when looking at the back), and work your way from there, until the 5V plus the resistor make the pointer move to the right end of the scale (but not beyond!). 

Movie-accurate dials for those gauges are available in the [DIY/faces-labels](/DIY/faces-labels) folder in this repository.

For the backlight, I drilled a hole in the rear of the metal enclosure, center bottom, and put a 5mm yellow LED (590nm) on the Control Board. Most replicas use white LEDs, but I think on the A-Car as shown in the Petersen Museum, there are yellow ones used, and I found white ones too bright. The LEDs are mounted on the board and they stick out approx 12mm from PCB to the LED's top.

#### "Roentgens"

The big "Roentgens" gauge is more of a problem. The original in the movie was a real Roentgens meter from a CP95 radiac. Such devices are hard to find, let alone one with the correct Simpson meter. The CP95 was built over a long period of time and they used meters from different manufacturers.

Since I could not find a properly equipped CP95, I searched for an alternative ... and came across a lot of Simpson meters that looked good (while not identical). However: One - quite important - issue is that most Simpson meters are not illuminated. Because of this, their front is thinner, the glass is closer to the dial, and that is a problem because the movie-accurate "Empty" light won't fit.

A word on Simpson model numbers: Their main model number means "case style", not "type of meter". "Model 49" therefore only means "4.5 inch case", but not whether this is a VU meter, a voltmeter, or what not:

<img width="985" alt="Simpson meters" src="img/simpson_catalog.png">

The only Simpson meters that came with illumination - apart from the Roentgens meters - were apparently their VU meters, models 49L ("L" for "light"; not listed above) and 142 (10470, 10540). Model 49L has the correct front dimensions; depending on their build date, they have either the three bands of "stripes" (like in the movie), or one thicker band of "stripes" in the center. (Later models, unfortunately using the same model number, look entirely different.)

I was lucky to score a **Simpson model 49L VU-meter** with the movie-accurate front. It is illuminated through two 6V incandescent light bulbs. The additional red pointer was added by drilling a hole into the front and putting a pointer from another meter inside. This red pointer is not moving, so it is reasonably easy to add.

The **model 142 VU-meters**, while perfectly usable electronically, are a bit smaller (4.25x3.9" vs 4.66x4.2") and look a bit different on the back; their barrel is thicker (3.25" vs 2.78" in diameter), and the screws are not at the outer corners but closer to the barrel. Unfortunately, the barrel is so big that it does not allow for a hole for the "Empty" light; this must be done another way. There are special files in the DIY/enclosure folder for model 142 dimensions.

If you can't find a VU-meter or consider the 142 too far off, you could try going with a Simpson voltmeter or ammeter (models 29, 39, 49, 59, or 79 fit size-wise). These meters mostly are for voltages/currents beyond what we have available directly, but: Most of those meters have built-in resistors that reduce the voltage for the coil to something far lower: For instance, the **Simpson model 49 voltmeter 0-50V DC (SK 525-447)** has a 50K resistor inside; if this resistor is bridged, the meter shows full scale at 0.0375V. With a 5K6 or so resistor is shows full scale at approx 5V, which is perfectly usable for our purposes. But again: You also need to compromise on the "Empty" light, since non-illuminated Simpson meters are too thin.

Tested meter options and configuration:
- Standard VU meter (Simpson 49L, 142): Internally unmodified. R5=330R, R6=3k3; R11 bridged as the 6V incandescent light bulbs need to resistor. Gauge Type setting "Standard VU-Meter".
- Simpson model 49 voltmeter 0-50V DC: Internal resistor needs to be bridged. R5=bridged, R6=5k6; R11 depends on user's design of illumination. Gauge Type setting "Generic Analog (0-5V)".
- Simpson model 49 ammeter 0-250mA DC: Internal coil in the rear needs to be removed (no need to take the meter apart; cut the two wires leading from the terminals towards the center, the coil will fall out then; be sure to bend down the remaining stubs so that they don't touch anything), the resistor can remain. R6=1k0. Gauge type setting "Generic Analog (0-5V)".

Unusable:
- Simpson model 49 voltmeter 0-250V AC.

Most Simpson meters have a drop-shaped pointer top which I was not able to remove (in fact, I didn't even try; I don't think the pointer top would have ended up properly straight); although I mounted the "Empty" light as high on the scale as possible, the pointer was still too long and collided with the light. My solution was to change the bends of the pointer where it leaves the driving mechanism more into an "S" shape, and I could thereby make it ever so short enough to pass the light. Another way would be to cut off the drop part, but that would make the pointer a tad too short in my opinion.

Regarding the "Empty" light: I used a light like this one (12V version), available from aliexpress:

![emptylight](img/emptylight.png)

There are also buttons that look identical and can be used instead:

![emptybutton](img/emptybutton.png)

The LED in those lights/buttons requires 12V. The Control Board has two connectors for the Empty LED:
- When using the "Empty Light" [4] connector, the light/button's resistor needs to be removed: Pull up the red cover and pull out the LED; then desolder the LED and bridge the resistor.
- When using the LED pins of the Digital Roetgens connector [6]: No modification of light/button needed, but the Control Board must be run with a 12V power supply. When using 5V, the LED will stay dark.

About the hole for the "Empty" light: Above light/button requires a 16mm hole. I started off drilling that hole into the dial. Then I mounted the dial into the meter and marked the hole on the plastic enclosure with a pencil. The next step was to remove the dial again, wrap the mechanism in plastic to keep dust out, and drill a smaller hole (7mm) into the enclosure. Then I milled that hole into a bigger one, until the button fit. The plastic of the enclosure is quite brittle and breaks easily. Drilling a 16mm hole appeared too risky. Hence the drill/mill method.

### Enclosure

The enclosure consists of three parts: The front and two side pieces. Measurements are in the [DIY/enclosure](/DIY/enclosure) folder of this repository.

You can buy an enclosure at [CircuitSetup](https://circuitsetup.us/product/delorean-time-machine-dash-plutonium-gauge-bezel/); note that it is for a model 49 panel meter ('Roentgens'); a model 142 won't fit.

### Connecting a Time Travel button

The Control Board has a Time Travel button (marked "TT"). If you want to connect an external Time Travel button, connect it to the "TT" and "3V3" pins of the "Time Travel" connector.

### Connecting a TCD to the Dash Gauges by wire

If you want to connect a TCD to the Dash Gauges by wire (such as mentioned [here](https://github.com/realA10001986/Dash-Gauges/tree/main#connecting-a-tcd-by-wire)), wire as follows:

<table>
    <tr>
     <td align="center">Dash Gauges</td>
     <td align="center">TCD with control board 1.2</td>
     <td align="center">TCD with control board 1.3</td>
    </tr>
   <tr>
     <td align="center">GND of "Time Travel" connector</td>
     <td align="center">GND of "IO14" connector</td>
     <td align="center">GND on "Time Travel" connector</td>
    </tr>
    <tr>
     <td align="center">TT of "Time Travel" connector</td>
     <td align="center">IO14 of "IO14" connector</td>
     <td align="center">TT OUT on "Time Travel" connector</td>
    </tr>
</table>



