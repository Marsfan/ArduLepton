import serial

ser = serial.Serial("COM3", 1000000)

with open("image.pgm", "wb") as fileObj:
    while True:
        newByte = ser.read()
        if newByte == b'\r':
            break
        fileObj.write(newByte)
ser.close()