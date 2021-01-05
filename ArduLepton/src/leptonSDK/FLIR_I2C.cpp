/*******************************************************************************
 * FLIR_I2C.c re-implemented to use Arduino Libraries instead
 * Work Performed by Gabriel Roper
 * Below is the initial header that was written by a developer for FLIR Systems
 * Included as required. 
 ******************************************************************************/

/*******************************************************************************
**
**    File NAME: FLIR_I2C.c
**
**      AUTHOR:  Hart Thomson
**
**      CREATED: 7/10/2015
**  
**      DESCRIPTION: Lepton Device-Specific Driver for various Master I2C Devices
**
**      HISTORY:  7/19/2015 HT - Initial Draft 
**
** Copyright 2010, 2011, 2012, 2013, 2014, 2015 FLIR Systems - Commercial Vision Systems
**
**  All rights reserved.
**
**  Redistribution and use in source and binary forms, with or without
**  modification, are permitted provided that the following conditions are met:
**
**  Redistributions of source code must retain the above copyright notice, this
**  list of conditions and the following disclaimer.
**
**  Redistributions in binary form must reproduce the above copyright notice,
**  this list of conditions and the following disclaimer in the documentation
**  and/or other materials provided with the distribution.
**
**  Neither the name of the Indigo Systems Corporation nor the names of its
**  contributors may be used to endorse or promote products derived from this
**  software without specific prior written permission.
**
**  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
**  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
**  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
**  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
**  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
**  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
**  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
**  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
**  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
**  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
**  THE POSSIBILITY OF SUCH DAMAGE.
**
*******************************************************************************/
/******************************************************************************/
/** INCLUDE FILES                                                            **/
/******************************************************************************/

#include "LEPTON_Types.hpp"
#include "LEPTON_ErrorCodes.hpp"
#include "LEPTON_Macros.hpp"
#include "FLIR_I2C.hpp"
#include "LEPTON_I2C_Reg.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <Arduino.h>
#include <Wire.h>

/******************************************************************************/
/** LOCAL DEFINES                                                            **/
/******************************************************************************/

#define ADDRESS_SIZE_BYTES 2
#define VALUE_SIZE_BYTES 2
#define I2C_BUFFER_SIZE (ADDRESS_SIZE_BYTES + LEP_I2C_DATA_BUFFER_0_LENGTH)
LEP_UINT8 tx[I2C_BUFFER_SIZE];
LEP_UINT8 rx[I2C_BUFFER_SIZE];

/******************************************************************************/
/** LOCAL TYPE DEFINITIONS                                                   **/
/******************************************************************************/

/******************************************************************************/
/** PRIVATE DATA DECLARATIONS                                                **/
/******************************************************************************/
LEP_PROTOCOL_DEVICE_E masterDevice;

LEP_CMD_PACKET_T cmdPacket;
LEP_RESPONSE_PACKET_T responsePacket;

// TODO: Allow user to set which i2c device they want to use. (such as Wire1)

/******************************************************************************/
/** EXPORTED PUBLIC FUNCTIONS                                                **/
/******************************************************************************/

// TODO: See What the heck needs to be changed on this function
// Looks like this is the function that would be used to select the correct I2C bus
// But of course we can't just give it a pointer to the correct device. That would be too easy
// I suppose this is better to minimize how many files need editing, but I still think it is stupid. 
// Instead we need some sort of switch to handle selecting the rig
LEP_RESULT DEV_I2C_MasterSelectDevice(LEP_PROTOCOL_DEVICE_E device)
{
    LEP_RESULT result = LEP_OK;

    masterDevice = device;

    return (result);
}

/******************************************************************************/
/**
 * Performs I2C Master Initialization
 * 
 * @param portID     LEP_UINT16  User specified port ID tag.  Can be used to
 *                   select between multiple cameras
 * 
 * @param BaudRate   Clock speed in kHz. Typically this is 400.
 *                   The Device Specific Driver will try to match the desired
 *                   speed.  This parameter is updated to the actual speed the
 *                   driver can use.
 * 
 * @return LEP_RESULT  0 if all goes well, errno otherwise
 */
LEP_RESULT DEV_I2C_MasterInit(LEP_UINT16 portID,
                              LEP_UINT16 *BaudRate)
{

    Wire.begin();
    Wire.setClock(400000);
    *BaudRate = 400;
    LEP_RESULT result = LEP_OK;
    return (result);
}

/**
 * Closes the I2C driver connection.
 * 
 * @return LEP_RESULT  0 if all goes well, errno otherwise.
 */
LEP_RESULT DEV_I2C_MasterClose()
{

    Wire.end();
    LEP_RESULT result = LEP_OK;

    return (result);
}

/**
 * Resets the I2C driver back to the READY state.
 * 
 * @return LEP_RESULT  0 if all goes well, errno otherwise.
 */
LEP_RESULT DEV_I2C_MasterReset(void)
{
    Wire.end();
    Wire.begin();
    LEP_RESULT result = LEP_OK;
    return (result);
}

LEP_RESULT DEV_I2C_MasterReadData(LEP_UINT16 portID,        // User-defined port ID
                                  LEP_UINT8 deviceAddress,  // Lepton Camera I2C Device Address
                                  LEP_UINT16 regAddress,    // Lepton Register Address
                                  LEP_UINT16 *readDataPtr,  // Read DATA buffer pointer
                                  LEP_UINT16 wordsToRead,   // Number of 16-bit words to Read
                                  LEP_UINT16 *numWordsRead, // Number of 16-bit words actually Read
                                  LEP_UINT16 *status        // Transaction Status
)
{
    LEP_RESULT result = LEP_OK;
    uint8_t i2cStatus;
    LEP_UINT32 bytesToWrite = ADDRESS_SIZE_BYTES;
    LEP_UINT32 bytesToRead = wordsToRead << 1; // Same as multiplying by 2. Need to do this as Arduino Wire works in 8 bit words
    LEP_UINT32 bytesActuallyWritten = 0;
    LEP_UINT32 bytesActuallyRead = 0;
    LEP_UINT32 wordsActuallyRead = 0;
    LEP_UINT8 *txdata = &tx[0];
    LEP_UINT8 *rxdata = &rx[0];
    LEP_UINT16 *dataPtr;
    LEP_UINT16 *writePtr;

    *(LEP_UINT16 *)txdata = REVERSE_ENDIENESS_UINT16(regAddress);

    /************************************* 
    /   Begin Arduino Transmission Section 
    ***************************************/
   // TODO: COnfigure this to use the write function below. 
    Wire.beginTransmission(deviceAddress);

    bytesActuallyWritten = Wire.write((const uint8_t*)txdata, (int)bytesToWrite);
    i2cStatus = Wire.endTransmission();
    if (i2cStatus || bytesActuallyWritten != bytesToWrite)
    {
        result = LEP_ERROR;
        return (result);
    }
    Wire.requestFrom(deviceAddress, bytesToRead);
    // TODO: Check if we need to do this wait
    // delayMicroseconds(10); // Wait a bit to make sure full transaction happens.
    bytesActuallyRead = Wire.available();
    for (LEP_UINT32 b = 0; b < bytesActuallyRead; b++)
    {   
        rxdata[b] = Wire.read();
    }
    /************************************** 
    *   End Arduino Transmission Section  * 
    ***************************************/

    wordsActuallyRead = (LEP_UINT16)(bytesActuallyRead >> 1);
    *numWordsRead = wordsActuallyRead;

    if (result == LEP_OK)
    {
        dataPtr = (LEP_UINT16 *)&rxdata[0];
        writePtr = readDataPtr;
        while (wordsActuallyRead--)
        {
            *writePtr++ = REVERSE_ENDIENESS_UINT16(*dataPtr);
            dataPtr++;
        }
    }

    return (result);
}

LEP_RESULT DEV_I2C_MasterWriteData(LEP_UINT16 portID,           // User-defined port ID
                                   LEP_UINT8 deviceAddress,     // Lepton Camera I2C Device Address
                                   LEP_UINT16 regAddress,       // Lepton Register Address
                                   LEP_UINT16 *writeDataPtr,    // Write DATA buffer pointer
                                   LEP_UINT16 wordsToWrite,     // Number of 16-bit words to Write
                                   LEP_UINT16 *numWordsWritten, // Number of 16-bit words actually written
                                   LEP_UINT16 *status)          // Transaction Status
{
    LEP_RESULT result = LEP_OK;
    uint8_t i2cStatus;
    LEP_INT32 bytesOfDataToWrite = (wordsToWrite << 1); // Mult by 2 since arduino using 8 bit words for i2c
    LEP_INT32 bytesToWrite = bytesOfDataToWrite + ADDRESS_SIZE_BYTES;
    LEP_INT32 bytesActuallyWritten = 0;
    LEP_UINT8 *txdata = &tx[0];
    LEP_UINT16 *dataPtr;
    LEP_UINT16 *txPtr;

    *(LEP_UINT16 *)txdata = REVERSE_ENDIENESS_UINT16(regAddress);
    dataPtr = (LEP_UINT16 *)&writeDataPtr[0];
    txPtr = (LEP_UINT16 *)&txdata[ADDRESS_SIZE_BYTES];
    while (wordsToWrite--)
    {
        *txPtr++ = (LEP_UINT16)REVERSE_ENDIENESS_UINT16(*dataPtr);
        dataPtr++;
    }

    /***************************************
    *   Begin Arduino Transmission Section * 
    ***************************************/
    Wire.beginTransmission(deviceAddress);
    bytesActuallyWritten = Wire.write((const uint8_t*)txdata, (int)bytesToWrite);
    i2cStatus = Wire.endTransmission();
    if (i2cStatus || bytesActuallyWritten != bytesToWrite)
    {
        result = LEP_ERROR;
        return result;
    }
    /**************************************
    *   End Arduino Transmission Section  *
    ***************************************/

    *numWordsWritten = (bytesActuallyWritten >> 1);

    return (result);
}

LEP_RESULT DEV_I2C_MasterReadRegister(LEP_UINT16 portID,
                                      LEP_UINT8 deviceAddress,
                                      LEP_UINT16 regAddress,
                                      LEP_UINT16 *regValue, // Number of 16-bit words actually written
                                      LEP_UINT16 *status)
{
    LEP_RESULT result = LEP_OK;

    LEP_UINT16 wordsActuallyRead;
    /* Place Device-Specific Interface here
    */
    result = DEV_I2C_MasterReadData(portID, deviceAddress, regAddress, regValue, 1 /*1 word*/, &wordsActuallyRead, status);

    return (result);
}

LEP_RESULT DEV_I2C_MasterWriteRegister(LEP_UINT16 portID,
                                       LEP_UINT8 deviceAddress,
                                       LEP_UINT16 regAddress,
                                       LEP_UINT16 regValue, // Number of 16-bit words actually written
                                       LEP_UINT16 *status)
{
    LEP_RESULT result = LEP_OK;
    LEP_UINT16 wordsActuallyWritten;
    /* Place Device-Specific Interface here
    */
    result = DEV_I2C_MasterWriteData(portID, deviceAddress, regAddress, &regValue, 1, &wordsActuallyWritten, status);

    return (result);
}

LEP_RESULT DEV_I2C_MasterStatus(void)
{
    LEP_RESULT result = LEP_OK;

    /* Place Device-Specific Interface here
    */

    return (result);
}

/******************************************************************************/
/** PRIVATE MODULE FUNCTIONS                                                 **/
/******************************************************************************/
