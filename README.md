# Arduino Implementation of FLIR Lepton using the FLIR Lepton SDK

This is a simple project to implement the FLIR Lepton SDK on board of an Arduino, in a form that is highly platform agnostic, allowing for it to be rapidly ported to other frameworks.

## Table of Contents

- [Table of Contents](#table-of-contents)
- [Repository Structure](#repository-Structure)
- [Getting It Working](#Getting-It-Working)
  - [The Oddities of the Lepton SDK](#The-Oddities-of-the-Lepton-SDK)
  - [How I Got the SDK Working](#how-i-got-the-sdk-working)
  - [Adapting for Other Platforms](#adapting-for-other-platforms)

## Repository Structure

| **Directory**  | **Purpose**  |
|----------------|--------------|
| [ArduLepton](./ArduLepton) | Arduino Program for the FLIR Lepton | 
| [ArduLepton/src/leptonSDK](./ArduLepton/src/sdk) | FLIR Lepton SDK Modified for use with the Arduino IDE (placed in a src folder because that is what the Arduino Compiler requires to make it work, [though this might change in the future](https://github.com/arduino/Arduino/pull/11110/files)) | 
| [Docs](./Docs) | Schematics, datasheets, and other documentation that I found useful when working on this project. | 
| [LeptonSDKPure](./LeptonSDKPure) | A clean, unmodified version of the FLIR Lepton SDK, for comparasion with modified version in [ArduLepton/src/leptonSDK](./ArduLepton/src/sdk) | 
| [pythonReader](./PythonReader) | A simple python utility that reads incoming data from a Serial Port and dumps it into a pgm file. Used to get images that are sent by the ArduLepton script. |


## Getting It Working

### The oddities of the Lepton SDK

The Lepton SDK Embedded SDK is really weird. It has code in it written specifically for the [FT2232H](https://www.ftdichip.com/Products/ICs/FT2232H.html), as well as stuff for a USB to I2C/SPI adapter by Total Phase called the [Aardvark](https://www.totalphase.com/products/aardvark-i2cspi/). This prevents the SDK for being able to be compiled for other stuff, like an Arduino compatible board.

To fix this, I stripped out the unneeded files that handle performing I2C tasks with the FT2232H or the Aardvark, and wrote a custom I2C implementation for the SDK that leverages the Arduino Wire Library. Based on the fact that the [GroupGets Lepton Library](https://github.com/groupgets/LeptonModule) seems to have done the same thing, I am guessing that this is the correct way to use the SDK on other boards.

To anyone from FLIR who might be looking at this repository, PLEASE DOCUMENT THIS SOMEWHERE. There is almost no documentation at all for the LEPTON SDK beside a PDF that lists what all of the general SDK functions do, and nothing about how the SDK is not actually written as a general-purpose set of files for implementation on any embedded system. THIS IS BAD!

### How I Got the SDK Working

_Note: At the time of writing this readme, it is actually **not** working, and I am using this
section to document what I am doing to fix it. Once I have it fully working I will remove this message, and provide a link to the commit that shows exactly what I changed to make it work with the arduino_

This is a list of things that I changed to get the SDK to compile and work with an Arduino Board (specifically the SparkFun RedBoard Artemis ATP).

- Strip out the files that are specific for the Aardvark and the FT2232
- Remove the now useless makefile
- Re-write `FLIR_I2C.c` and `FLIR_I2C.h` to use the Arduino Libraries (`Wire.h` and `SPI.h`) instead of the FT2232H and Aardvark libraries.

The first two of these items are not actually necessary, but I don't like leaving useless files lying around. However, I placed a pure version of the SDK into a folder in this repo called _[LeptonSDKPure](./LeptonSDKPure)_ so that the original contents of the SDK is available, and can be compared to the modified version of the SDK that I have written.

### Adapting for Other Platforms

If you want to adapt the Lepton SDK for whatever system you want (Raspberry Pi, Nvidia Jetson, Intel Edison, Acorn Archimedes, UNIVAC II, etc.), all you should have to do is modify the `FLIR_I2C.c` and `FLIR_I2C.h` files to use the I2C API on the platform that you are using.
