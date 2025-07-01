Dependencies: None.  

Build: Compile "qrwinconsole.asm" with FASM, while "qrcodes.inc" is in the same directory.  

Usage: qrwinconsole.exe contentfile.txt [ 0 ] 
- argument 2 (optional): 0, ECI number to indicate the encoding of the input file  
  - to know which number corresponds to which encoding, read comments in qrcodes.inc 
- argument 1: contentfile.txt, file whose contents will be rendered as a QR code
- argument 0: "qrwinconsole.exe", is the name of the executable

Output: a QR code is rendered in the console using # signs as light modules and spaces as dark modules.  
Best viewed with a monospace font with square aspect ratio.  
