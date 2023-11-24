# The DIY Dash Gauges

Note that this is a custom built prop; there is no kit available.

![Dash Gauges](https://github.com/realA10001986/Dash-Gauges/assets/76924199/e01aeb44-ea61-4685-95ee-d673b1e654ff)

The Panel consists of several parts:
- Aluminum (Aluminium for non-Americans) enclosure; the measurements are in the ["enclosure"](DIY/enclosure) folder of this repository.
- The gauges: The smaller gauges are H&P 631-14672 (built by Phaostron) and the "Plutonium chamber" gauge is a Simpson 49L VU Meter, all driven by a MCP4728 quad-DAC (on an Adafruit breakout board). Movie-accurate faces for these gauges are in the ["faces-labels"](DIY/faces-labels) folder of this repository.
- A Switch Board, which is mounted on the smaller gauges' back side; it carries the interface for the gauges, the lights and switches, as well as some connectors for external switches/buttons. This Switch Board is easy to assemble, it only carries some resistors and connectors. The gerbers as well as an EasyEDA file are in the ["electronics"](DIY/electronics) folder of this repository, JLCPCB can make this board for you. A BOM is available as well.
- A Control Board: In lack of a dedicated control board (which to design unfortunately is beyond my abilities), the heart of the device is a slightly modified TCD control board from [CircuitSetup](https://circuitsetup.us/product/time-circuits-display-control-board-with-keypad-trw-style-lenses/).

### Control board

The control board is a modified TCD control board from CircuitSetup:

- The ENTER button is removed from the PCB and its pins serve as the "Side Switch".
- The white LED serves as the "Empty" LED in the "Roentgens" meter; it needs to desoldered.
- The other LEDS are removed from the PCB as they stick out and are not needed.
- The volume pot can be removed; this is, of course, optional, but since this board is assumingly hidden somewhere, the pot would be inaccessible anyway.
- The i2c, audio and IO connectors were relocated to the front side of the PCB (they are normally on the back)

![ControlBoard](https://github.com/realA10001986/Dash-Gauges/assets/76924199/82f25c22-d60b-493c-a785-5c8b78f2ca32)

As regards the cabling, two remarks:
- I used each 2 wires for GND and 5V (black/white, green/blue), since the wires I used are quite thin.
- I put each a wire of GND (brown, orange) between and beside the SDA/SCL wires to avoid crosstalk.

### Switch board

For the very gauges hardware I used, I designed a simple "switch board" which mounts on the smaller gauges. It carries the MCP4728 breakout board, a single channel relay board, all resistors for the gauges and LEDs, the LEDs for the smaller gauges and connectors for cables to the control board and to the "Roentgens" gauge. Also, it has connectors for an external Time Travel button and the door switches.

![SwitchBoard1.7](https://github.com/realA10001986/Dash-Gauges/assets/76924199/1adf1639-6e81-44df-b56d-0eba04ba26b8)

It carries this relay module for the lights (desolder the screw terminals, put in pin socket, pitch 5mm, 3 positions, for example Samtec HPF-03-01-T-S):

![relay](https://github.com/realA10001986/Dash-Gauges/assets/76924199/eef6cc62-21e1-4700-99df-499739c4ea61)

This module is not the most elegant solution but the easiest one if you, like me, are using real lamps instead of LEDs: also, it allows for an external power source for the lights (eg 12V).

The Switch Board also carries the Adafruit MCP4728 break-out board; just solder pin sockets to both lines of pads on the PCB (pitch 2.54mm, 6 positions, for example Samtec SSW-106-01-F-S). 

Connectors on the Switch Board:
- Audio: This allows connecting the audio from the Control Board; the signal is directed to the top-left audio connector, where a speaker is to be attached.
- Data (from left to right):
  * 3_3V: 3.3V from the Control Board
  * SW: From the ENTER button of the Control Board
  * E-LED: From the white LED on the Control Board
  * IO14: From "IO14" on the Control Board
  * IO13: From "IO13" on the Control Board
  * IO27: From "IO27" on the Control Board
 
- 5V/GND: 5V and GND from the Control Board. There are two connectors for power on the Switch Board; you can choose which one suits you better.

- SCL/SDA: The i2c signal from he Control Board.

A connection diagram is [here](electronics/connection%20diagram.png).

### Gauges

The gauges are connected through an Adafruit MCP4728 breakout board, which is an i2c-driven quad-DAC and mounted on the switch board. This DAC allows arbitrary voltages up to 5V, but the gauges used here use much lower voltages. If you use different gauges, you need to add your configuration to the firmware. More on this below.

*Important*: When using the MCP4728 in your project, do not connect the actual gauges until you selected the right hardware type in the Config Portal and have powered-down and powered-up the device once afterwards. This power-cycle is needed to reset the MCP4728's EEPROM to the correct settings and to avoid a power output too high at boot.

#### "Primary", "Percent Power"

For the smaller gauges I used two H&P 631-14672 (built by Phaostron), which are (were?) oddly available in volumes on ebay. The pointers on these are red, but an Edding permanent marker was quickly able to change them to black. These gauges have a scale from 0 to 0.5V, but - again: oddly - can't cope with 0.5V, they need a resistor in series. I found 8k2 + 470R to be the best value (R1+R2, R3+R4 on the Switch Board).

Those two gauges are driven by channels A and B of the MCP4728. They show nearly identical readings at 10% and at 90%, in the middle they differ a bit,despite the voltage being identical. But since the idea here is not to actually measure voltage, that is still ok, the "full" percentage is configurable after all.

The movie-accurate faces of those gauges are available in this repository.

For the backlight, I drilled a hole in the rear of the metal enclosure, center bottom, and put a yellow LED (590nm) on the Switch Board (LED1, LED2, with suitable resistors (R8/R9; depending on LED, probably around 150R). Most replicas use white LEDs, but I think on the A-Car as shown in the Petersen Museum, there are yellow ones used, and I found white ones too bright.

#### "Roentgens"

The big "Roentgens" gauge is more of a problem. I was lucky to score a Simpson 49L VU-meter, which is operated with a 3k6 resistor in series (R5+R6 on the Switch Board). Its lighting is through two 6V lamps which need no resistor; R11 on the Switch Board therefore needs to be bridged with a wire. If you prefer LEDs, put a suitable resistor in place instead.

Properly looking Simpson meters are obtainable, even on ebay, but there are many different ones. In the movie, they used one with lights (BA9S, 6V), which means that the enclosure is much thicker than on those without. That thickness, however, is needed for the "Empty" light. The second problem is that the meters mostly are for voltages beyond what we have available. Since I am more of a software guy, I can't give any further advice here; maybe the voltmeters can be modified somehow (I assume they have big resistors which can be removed or bridged), or a level shifter can be used. In the worst case, the meter can only show two values - 0 and "full" via a level shifter or a relay, using external power. For the purpose of the Dash Gauges that is not perfect, but enough for re-creating the movie scenes.

The "Empty" light: That was the easy part; I used a button like this one:

![emptybutton](https://github.com/realA10001986/Dash-Gauges/assets/76924199/53187b70-9399-44a4-bd78-090f055a3423)

The LED in those buttons is driven with 12V using a resistor; this resistor needs to be removed. No need to take out the blue part for this modification; just remove the red cover and pull out the LED; then desolder the LED and bridge the resistor.

Most Simpson meters have a drop-shaped pointer top which I was not able to remove (in fact, I didn't even try; I don't think the pointer top would have ended up properly straight); although I mounted the "Empty" light as high on the scale as possible, the pointer was still too long and collided with the light. My solution was to change the bends of the pointer where it leaves the driving mechanism more into an "S" shape, and I could thereby make it ever so short enough to pass the light. Another way would be to cut off the drop part, but that would make the pointer a tad too short in my opinion.

About the hole for the "Empty" light: Above button requires a 16mm hole. I started off drilling that hole into the face plate. Then I mounted the face plate into the meter and marked the hole on the plastic enclosure with a pencil. The next step was to remove the scale, wrap the mechanism in plastic to keep dust out, and drill a smaller hole (7mm) into the enclosure. Then I milled that hole into a bigger one, until the button fit. The plastic of the enclosure is quite brittle and breaks easily. Drilling a 16mm hole appeared too risky. Hence the drill/mill method.

### Enclosure

The enclosure consists of three parts: The front and two side pieces. Measurements are in the "enclosure" folder of this repository. The gauges are not marked completely in the drawing, but the distance is. The distance is important if you want to fit the Switch Board.


### Connecting a TCD to the Dash Gauges by wire

If you want to connect a TCD to the Dash Gauges by wire (such as mentioned [here](https://github.com/realA10001986/Dash-Gauges/tree/main#connecting-a-tcd-by-wire)), you have two options on the Dash Gauges' side: Either the control board, or the switch board. 

<table>
    <tr>
     <td align="center">Dash Gauges:<br>"IO13" connector on DG's TCD control board</td>
     <td align="center">TCD control board 1.2</td>
     <td align="center">TCD control board 1.3</td>
    </tr>
   <tr>
     <td align="center">GND of "IO13" connector</td>
     <td align="center">GND of "IO13" connector</td>
     <td align="center">GND on "Time Travel" connector</td>
    </tr>
    <tr>
     <td align="center">IO13 of "IO13" connector</td>
     <td align="center">IO13 of "IO13" connector</td>
     <td align="center">TT OUT on "Time Travel" connector</td>
    </tr>
</table>

<table>
    <tr>
     <td align="center">Dash Gauges:<br>"TIME TRAVEL" connector on Switch Board</td>
     <td align="center">TCD control board 1.2</td>
     <td align="center">TCD control board 1.3</td>
    </tr>
   <tr>
     <td align="center">GND of "TIME TRAVEL" connector</td>
     <td align="center">GND of "IO13" connector</td>
     <td align="center">GND on "Time Travel" connector</td>
    </tr>
    <tr>
     <td align="center">IN of "TIME TRAVEL" connector</td>
     <td align="center">IO13 of "IO13" connector</td>
     <td align="center">TT OUT on "Time Travel" connector</td>
    </tr>
</table>

The same goes for the Time Travel button: It connects the same way, but with one distint difference: Instead of GND, it connects to 5V (on the DG' control board) or 3_3V on the DG's Switch Board.
