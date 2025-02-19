# simpleQRcode6
Console-based QR code generator.

Implements a subset of the features of QR Code, or QR Code Model 2, as defined in ISO/IEC 18004:2015.

The code is not optimized for performance or memory efficiency, but for ease of debugging and manual conversion to assembly.



Dependencies: None.

Build: Compile "myqr.cpp" with a C++ compiler. 

Usage: myqr  -1  18  "Content of QR code"
  
    argument 3: "Content...", is the string that will be encoded in the QR code
  
    argument 2: 18, must be less than or equal to the length (in bytes) of argument 3
  
    argument 1: -1, signifies no Extended Channel Interpretation
  
              any number greater than -1 is used as the ECI mode for the entire QR code
              
              ECI numbers greater than 127 are not currently supported

    argument 0: "myqr", is the name of the executable
              
Output: a QR code is rendered in the console using # signs as light modules and spaces as dark modules. Best viewed with a monospace font with square aspect ratio.
