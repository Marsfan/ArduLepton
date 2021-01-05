#include "src/LeptonSDK/LEPTON_SDK.hpp"
#include "src/LeptonSDK/LEPTON_Types.hpp"
#include "src/LeptonSDK/LEPTON_SYS.hpp"
#include "src/LeptonSDK/LEPTON_Types.hpp"
#include "src/leptonSDK/LEPTON_OEM.hpp"
#include "src/leptonSDK/LEPTON_AGC.hpp"
#include <stdlib.h>
#include <SPI.h>

#define VOSPI_FRAME_SIZE (164)
#define CSPIN 28
#define VSYNCPIN A32

void errorHandler(char msg[]);
void errorHandler(char msg[], LEP_RESULT errorCode);
bool responseCheck(LEP_RESULT result);
bool responseCheck(LEP_RESULT result, char command[]);
void read_lepton_frame(void);
void lepton_sync(void);
void resync(void);
int transfer();
void vsyncit();

SPISettings spiFLIR(10000000, MSBFIRST, SPI_MODE3);

LEP_CAMERA_PORT_DESC_T leptonCamera;
LEP_CAMERA_PORT_DESC_T_PTR lepCamPtr = &leptonCamera;
LEP_OEM_PART_NUMBER_T partNumber;
LEP_OEM_SW_VERSION_TAG softwareVersion;
LEP_SYS_FLIR_SERIAL_NUMBER_T serialNumber;
LEP_AGC_ENABLE_E agcState = LEP_AGC_ENABLE;
LEP_AGC_ROI_T agcRegion;
LEP_OEM_VIDEO_OUTPUT_FORMAT_E vidFormat = LEP_VIDEO_OUTPUT_FORMAT_RAW14;
LEP_SYS_TELEMETRY_ENABLE_STATE_E telemetryMode = LEP_TELEMETRY_DISABLED;
LEP_SYS_TELEMETRY_LOCATION_E telemetryLoc = LEP_TELEMETRY_LOCATION_HEADER;
LEP_OEM_VIDEO_OUTPUT_ENABLE_E vidState = LEP_VIDEO_OUTPUT_ENABLE;
// FLIR Lepton breakout board data matrix is just the model number of the breakout
// FLIR Lepton module data matrix reads same as printed numbers (A0381303). This is the serial number

#define mySDI D6
#define mySDO D7
#define myCLK D5

// MbedSPI mySPI(mySDI, mySDO, myCLK);

uint8_t lepton_frame_packet[VOSPI_FRAME_SIZE];
bool doneCapturing = false;
uint16_t image[80][60];
static unsigned int lepton_image[80][80];

void setup()
{
    pinMode(CSPIN, OUTPUT);
    pinMode(VSYNCPIN, INPUT);
    digitalWrite(CSPIN, HIGH);
    char softwareVersionMessage[100];
    char roiMessage[75];
    Serial.begin(115200);
    Serial.println("Initializing Lepton Module");
    responseCheck(LEP_OpenPort(1, LEP_CCI_TWI, 400, lepCamPtr), "LEP_OpenPort");
    responseCheck(LEP_RunSysPing(lepCamPtr), "LEP_RunSysPing");
    responseCheck(LEP_GetOemFlirPartNumber(lepCamPtr, &partNumber), "LEP_GetOemFlirPartNumber");
    Serial.print("Lepton Part Number: ");
    Serial.println(partNumber.value);
    responseCheck(LEP_GetOemSoftwareVersion(lepCamPtr, &softwareVersion), "LEP_GetOemSoftwareVersion");
    sprintf(softwareVersionMessage,
            "Lepton Software Version:\n\tGeneral Purpose Processor: %u.%u.%u\n\tDSP: %u.%u.%u",
            softwareVersion.gpp_major, softwareVersion.gpp_minor, softwareVersion.gpp_build,
            softwareVersion.dsp_major, softwareVersion.dsp_minor, softwareVersion.dsp_build);
    Serial.println(softwareVersionMessage);
    responseCheck(LEP_GetSysFlirSerialNumber(lepCamPtr, &serialNumber), "LEP_GetSysFlirSerialNumber");
    Serial.print("Lepton Serial Number: ");
    Serial.println((unsigned long long)serialNumber);
    responseCheck(LEP_SetAgcEnableState(lepCamPtr, agcState), "SetAgcState");
    responseCheck(LEP_GetAgcROI(lepCamPtr, &agcRegion), "GetAgcROI");
    sprintf(roiMessage,
            "AGC ROI:\n\tStartCol: %u\n\tStartRow: %u\n\tEndCol: %u\n\tEndRow: %u",
            agcRegion.startCol, agcRegion.startRow, agcRegion.endCol, agcRegion.endRow);
    Serial.println(roiMessage);
    responseCheck(LEP_SetOemVideoOutputFormat(lepCamPtr, vidFormat), "SetVidFmt");
    responseCheck(LEP_SetSysTelemetryEnableState(lepCamPtr, telemetryMode), "SetTelemState");
    responseCheck(LEP_SetSysTelemetryLocation(lepCamPtr, telemetryLoc), "SettelemetryLoc");
    responseCheck(LEP_SetOemVideoOutputEnable(lepCamPtr, vidState), "vidEnable");
    responseCheck(LEP_SetOemGpioMode(lepCamPtr, LEP_OEM_GPIO_MODE_VSYNC), "gpioMode");
    responseCheck(LEP_SetOemGpioVsyncPhaseDelay(lepCamPtr, LEP_OEM_VSYNC_DELAY_MINUS_3), "setvsync");
    SPI.begin();
    delay(1000);

}

bool imageReady = false;
void vsyncit()
{
    imageReady = true;
}

void loop()
{
    
    resync();

        transfer();

        save_pgm_file();
        imageReady = false;
}

bool responseCheck(LEP_RESULT result)
{
    if (result != LEP_OK)
    {
        return false;
    }
    else
    {
        return true;
    }
}
bool responseCheck(LEP_RESULT result, char command[])
{
    char message[100];

    if (!responseCheck(result))
    {
        sprintf(message, "Occurred during command %s", command);
        errorHandler(message, result);
        return false;
    }
    return true;
}

void resync()
{
    SPI.endTransaction();
    SPI.end();
    digitalWrite(CSPIN, HIGH);
    delay(200);
    SPI.begin();
}

void errorHandler(char msg[])
{
    Serial.print("ERROR: ");
    Serial.println(msg);
    while (true)
        ;
}

void errorHandler(char msg[], LEP_RESULT errorCode)
{
    Serial.print("ERROR: ");
    Serial.println(msg);
    Serial.print("Error Code: ");
    Serial.println(errorCode);
    while (true)
        ;
}

// These functions are from https://github.com/groupgets/LeptonModule/blob/master/software/raspberrypi_capture/raspberrypi_capture.c
// Modified slightly to work with arduino.

int transfer()
{
    int i;
    int frame_number = 0;

    SPI.beginTransaction(spiFLIR);
    digitalWrite(CSPIN, LOW);

    while (frame_number < 60)
    {
        SPI.transfer(lepton_frame_packet, VOSPI_FRAME_SIZE);

        {
            frame_number = lepton_frame_packet[1];

            if (frame_number < 60)
            {
                for (i = 0; i < 80; i++)
                {
                    lepton_image[frame_number][i] = (lepton_frame_packet[2 * i + 4] << 8 | lepton_frame_packet[2 * i + 5]);
                }
            }
        }
    }
    digitalWrite(CSPIN, LOW);
    SPI.endTransaction();
    return frame_number;
}

static void save_pgm_file(void)
{
    int i;
    int j;
    unsigned int maxval = 0;
    unsigned int minval = UINT_MAX;

    printf("Calculating min/max values for proper scaling...\n");
    for (i = 0; i < 60; i++)
    {
        for (j = 0; j < 80; j++)
        {
            if (lepton_image[i][j] > maxval)
            {
                maxval = lepton_image[i][j];
            }
            if (lepton_image[i][j] < minval)
            {
                minval = lepton_image[i][j];
            }
        }
    }
    Serial.print("maxval = ");
    Serial.println(maxval);
    Serial.print("minval = ");
    Serial.println(minval);

    Serial.print("P2\n80 60\n");
    Serial.println(maxval - minval);
    for (i = 0; i < 60; i++)
    {
        for (j = 0; j < 80; j++)
        {
            Serial.print(lepton_image[i][j] - minval, HEX);
            Serial.print(" ");
        }
        Serial.print("\n");
    }
    Serial.print("\n\n");
}
