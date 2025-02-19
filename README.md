# simpleQRcode6
Console-based QR code generator.

Implements a subset of the features of QR Code, or QR Code Model 2, as defined in ISO/IEC 18004:2015.

Dependencies: None.

Build: Compile "myqr.cpp" with a C++ compiler. 

Usage: myqr "Content of QR code" 18 -1
  argument 0: "myqr", is the name of the executable
  argument 1: "Content...", is the string that will be encoded in the QR code
  argument 2: 18, must be less than or equal to the length (in bytes) of argument 1
  argument 3: -1, signifies no Extended Channel Interpretation
              any number different from -1 is used as the ECI mode for the entire QR code
              ECI numbers greater than 127 are not currently supported
              
Output: a QR code is rendered in the console using # signs as light modules and spaces as dark modules.
