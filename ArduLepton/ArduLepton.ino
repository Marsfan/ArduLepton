#include "src/LeptonSDK/LEPTON_SDK.h"
#include "src/LeptonSDK/LEPTON_Types.h"
#include "src/LeptonSDK/LEPTON_SYS.h"
#include "src/LeptonSDK/LEPTON_Types.h"

LEP_CAMERA_PORT_DESC_T leptonCamera;
LEP_RESULT leptonCmdStat;

void errorHandler(char msg[]);
void errorHandler(char msg[], LEP_RESULT errorCode);

void setup()
{
    Serial.begin(115200);
    Serial.println("Initializing Lepton Module");
    leptonCmdStat = LEP_OpenPort(1, LEP_CCI_TWI, 400, &leptonCamera);
    if (leptonCmdStat != LEP_OK)
    {
        errorHandler("Could not open port to connect to Lepton. ");
    }

    leptonCmdStat = LEP_RunSysPing(&leptonCamera);
}
void loop()
{
}

void errorHandler(char msg[])
{
    Serial.print("ERROR: ");
    Serial.println(msg);
}

void errorHandler(char msg[], LEP_RESULT errorCode)
{
    Serial.print("ERROR: ");
    Serial.println(msg);
    Serial.print("Error Code: ");
    Serial.println(errorCode);
}
