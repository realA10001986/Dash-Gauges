
## The Control Board

To have [JCLPCB](https://jlcpcb.com) make your board:
1) Create an account at jlcpcb.com
2) Click "Upload Gerber file" or "order now"
3) Upload the Gerber file (.zip, do not decompress!); leave all options at their defaults. You can choose a PCB color though...
4) Activate "PCB assembly", click "NEXT"
5) Enjoy a view of the PCB, click "NEXT"
6) Upload the BOM and "PickAndPlace" (CPL) files, click "Process BOM & CPL"
7) After uploading BOM and CPL, JLCPCB will complain about missing data for the axial resistors R1-R8, R11. These are intentionally missing in the BOM since they depend on your hardware. Click "Continue".
8) Read the remarks regarding the BOM below and uncheck superfluous parts in the list, click "NEXT"
9) Click "Do not place" when JLCPCB complains about "unselected parts"
10) Enjoy a nice 2D or 3D view of your future board, click "NEXT". (If the display stalls at "Processing files", click "NEXT" regardless).
11) Select a "product description" (eg. Movie prop) and click "Save to cart". Then finalize your order.


### About the BOM:

#### Things requiring manual selection:
1) L1, L2, L3: Only ONE of those three can be placed on the board since they share the same space. Choose one, deselect the others.
2) CN5SW-A and CN5SW-B are mutually exclusive. -A is an [S2B-XH-A-1](https://www.lcsc.com/product-detail/Wire-To-Board-Connector_JST-S2B-XH-A-1-LF-SN_C163035.html) header, -B a [2510 KK](https://www.lcsc.com/product-detail/Wire-To-Board-Connector_HCTL-HC-2510-2AW_C2982041.html) header. Choose one. To use this board to replace a "legacy" board, use the -B version.

#### Unmatched or "shortfall" parts:
1) HDR1 and HDR2 are [19-pin 2,54mm pitch female headers](https://www.lcsc.com/product-detail/Female-Headers_CONNFLY-Elec-DS1023-1x19SF11_C7509529.html) for the NodeMCU ESP32S dev board. You have to source and solder them on yourself.
2) CN12V, CNDSW, CNTT are 3-pin 3.5mm pitch screw terminals which you will have to source and solder yourself. I use [these](https://www.mouser.com/ProductDetail/TE-Connectivity/284514-3?qs=woBvfblj%2FzwGS50caoQlYA%3D%3D) with [these](https://www.mouser.com/ProductDetail/TE-Connectivity/284506-3?qs=pW%2FyRk%2FT1EErkHTioRHy7Q%3D%3D).
3) CN5V, CNELP are 2-pin 3.5mm pitch screw terminals which you will have to source and solder yourself. I use [these](https://www.mouser.com/ProductDetail/TE-Connectivity/284514-2?qs=woBvfblj%2FzwP8grZOAh0Gg%3D%3D) with [these](https://www.mouser.com/ProductDetail/TE-Connectivity/284506-2?qs=pW%2FyRk%2FT1EEEaP6r3xD3uw%3D%3D).
4) RELAY1, RELAY2 are either FRT5-5V, [Panasonic TQ2-5V](https://www.mouser.com/ProductDetail/Panasonic-Industrial-Devices/TQ2-5V?qs=HLLy2pIPwutHaTSpVfb1kw%3D%3D) or [Kemet EA2-5Nx](https://www.mouser.com/ProductDetail/KEMET/EA2-5NU?qs=UeqeubEbzTX2QGWq8LyCiw%3D%3D). You will probably have to source and solder those yourself.

#### You additionally need
1) NodeMCU ESP32 devboard, preferably with CP2102 USB-to-UART converter. 19pin version. For example: [This one](https://www.waveshare.com/nodemcu-32s.htm)
2) 2x LED. For example: [5mm yellow (595nm, >=3000mcd) LED](https://www.mouser.com/ProductDetail/Kingbright/WP7113SYT?qs=58z0TXQGVSR5GO%2FDcefd%2FA%3D%3D).
