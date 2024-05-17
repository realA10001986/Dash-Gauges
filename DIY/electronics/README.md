
## The Control Board

To have JCLPCB make your board:
1) Create an account at jlcpcb.com
2) Click "Upload Gerber file" or "order now"
3) Upload the Gerber file (.zip, do not decompress!); leave all options at their defaults. You can choose a PCB color though...
4) Activate "PCB assembly", click "NEXT"
5) Upload the BOM and "PickAndPlace" files, click NEXT
6) After uploading BOM and CPL, JLCPCB will complain about missing data for the axial resistors R1-R8, R11. These are intentionally missing in the BOM since they depend on your hardware. Click "Continue".
7) Read the remarks regarding the BOM below and uncheck superfluous parts in the list, click Next
8) Click "Do not place" when JLCPCB complains about "unselected parts"
9) Enjoy a nice 2D or 3D views of your future board, click "Next"
10) Select a "product description" and click "Save to cart". Then finalize your order.


### About the BOM:

#### Things requiring manual selection:
1) L1, L2, L3: Only ONE of those three can be placed on the board since they share the same space. Choose one, deselect the others.
2) CN5SW-A and CN5SW-B are mutually exclusive. -A is an S2B-XH-A-1 header, -B a 2510 KK header. Choose one. To use this board to replace a "legacy" board, use the -B version.

#### Unmatched or "shortfall" parts:
1) HDR1 and HDR2 are 19-pin 2,54mm pitch female headers for the NodeMCU ESP32S dev board. You have to source and solder them on yourself.
2) CN12V,CNDSW, CNTT are 3-pin 3.5mm pitch screw terminals which you will have to source and solder yourself.
3) CN5V, CNELP are 2-pin 3.5mm pitch screw terminals which you will have to source and solder yourself.
4) RELAY1, RELAY2 are either FRT5-5V, Panasonic TQ2-5V or Kema EA2-5Nx. You will probably have to source and solder those yourself.
