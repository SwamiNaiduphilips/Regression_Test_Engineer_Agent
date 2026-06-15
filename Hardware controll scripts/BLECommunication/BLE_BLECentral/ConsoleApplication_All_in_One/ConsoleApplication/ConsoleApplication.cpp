#define WIN32_LEAN_AND_MEAN

#include "BLELib.hpp"
#include "TuyaDatapoint.h"
#include <vector>
#include <stdio.h>
#include <iostream>
#include <exception>
#include <thread>
#include <queue>
#include <time.h>

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <fstream>
#include <sstream>
#include <iomanip>

#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 200
#define DEFAULT_PORT "51236"

static application::BleLib* bleLib = nullptr;

//Comport of BLE central
static std::string comPort = "";

// Shaver information *********************************************************
static std::array<uint8_t, 6> address = {};
static std::string tuyaPid = "";
static std::string tuyaDid = "";
static std::string tuyaAuthKey = "";
// Shaver information *********************************************************

// Tuya or Condor device ******************************************************
static bool isTuyaDevice = false;
// Tuya or Condor device ******************************************************

// LESC BLE connection ******************************************************
static bool isLESCconnection = false;
static bool NumericComparisonRequestReceived = false;


static bool keepTuyaLibProcesRunning = true;
static bool keepSocketProcesRunning = true;

static bool isTuyaLibProcesFinished = false;
static bool isSocketProcesFinished = false;

static bool readUuidComplete = false;
static bool serviceDiscoveryComplete = false;
static bool canBind = false;

static std::vector<std::string> socketData;
static std::vector<std::string> DataPointData;

static application::BleLib::Uuid UUIDToTransmit;
static std::string receivedDataFromRead = "";
static std::vector<uint8_t> DataToWriteToUUID;
static bool isWriteSuccesfull = false;
static bool enablingNotification = false;
static std::string receivedCondorNotifications = "";

static application::BleLibResponse::State deviceState = application::BleLibResponse::State::Disconnected;
static application::BleLibResponse::AuthenticationStatus authentication_status = application::BleLibResponse::AuthenticationStatus::Unknown;
static application::BleLib::PairingMode authenticationType = application::BleLib::PairingMode::Bond;

static std::string BLE_uuids = "";

static std::string notificationDataPointID = "";
static bool newNotification = false;
static bool otauUploadDone = false;
static bool restartOtauUpload = true;
static bool isBound = false;

// for logging purposes
typedef enum { BLE = 0, TST = 1, INTAPP = 2} LogType;
static bool isPopping = false;
std::queue<std::string> loggingOutput;

std::string getDataFromParameter(std::string allData, std::string parameter);
int sendDataTroughSocket(SOCKET* ClientSocket, std::string dataToSend);
void printLogging(std::string text, LogType type);
std::string hexify(uint8_t n, bool separator);


/**************************************************************************************************************/
// DeviceDiscovered
/**************************************************************************************************************/
class BLELibResponseImpl : public application::BleLibResponse
{
public:

    /**************************************************************************************************************/
    // NumericComparisonRequest
    /**************************************************************************************************************/
    virtual void NumericComparisonRequest(uint32_t passkey) override
    {
        NumericComparisonRequestReceived = true;
        printLogging("NumericComparisonRequest received", LogType::BLE);
    }

    /**************************************************************************************************************/
    // PasskeyDisplayRequest
    /**************************************************************************************************************/
    virtual void PasskeyDisplayRequest(uint32_t passkey) override
    {
        printLogging("PasskeyDisplayRequest received: " + passkey, LogType::BLE);
    }

    /**************************************************************************************************************/
    // PasskeyEntryRequest
    /**************************************************************************************************************/
    virtual void PasskeyEntryRequest( void ) override
    {
        printLogging("PasskeyEntryRequest received", LogType::BLE);
    }

    /**************************************************************************************************************/
    // Version
    /**************************************************************************************************************/
    virtual void Version(const std::string& version) override
    {
        // TODO
    }

    /**************************************************************************************************************/
    // ProtocolVersion
    /**************************************************************************************************************/
    virtual void ProtocolVersion(uint8_t version) override
    {
        // TODO
    }

    /**************************************************************************************************************/
    // IndicationReceived
    /**************************************************************************************************************/
    virtual void IndicationReceived(const Uuid& uuid, const std::vector<uint8_t>& data) override
    {
        // TODO
    }

    /**************************************************************************************************************/
    // AuthenticationComplete
    /**************************************************************************************************************/
    virtual void AuthenticationComplete(AuthenticationStatus status) override
    {
        std::string rec_status = "-";
        authentication_status = status;

        switch (status)
        {
            case AuthenticationStatus::Success:
                rec_status = "Success";
                break;
            case AuthenticationStatus::PasskeyEntryFailed:
                rec_status = "PasskeyEntryFailed";
                break;
            case AuthenticationStatus::AuthenticationRequirementsNotMet:
                rec_status = "AuthenticationRequirementsNotMet";
                break;
            case AuthenticationStatus::PairingNotSupported:
                rec_status = "PairingNotSupported";
                break;
            case AuthenticationStatus::InsufficientEncryptionKeySize:
                rec_status = "InsufficientEncryptionKeySize";
                break;
            case AuthenticationStatus::NumericComparisonFailed:
                rec_status = "NumericComparisonFailed";
                break;
            case AuthenticationStatus::Timeout:
                rec_status = "Timeout";
                break;
            case AuthenticationStatus::Unknown:
                rec_status = "Unknown";
                break;
            case AuthenticationStatus::Max:
                rec_status = "Max";
                break;
            default:
                rec_status = "error";
                break;
        }

        printLogging("AuthenticationComplete received: "+ rec_status, LogType::BLE);
    }

    /**************************************************************************************************************/
    // NotificationReceived
    /**************************************************************************************************************/
    virtual void NotificationReceived(const Uuid& uuid, const std::vector<uint8_t>& data) override
    {
        std::stringstream ss;
        ss << std::hex;

        for (int i(0); i < data.size(); ++i)
            ss << std::setw(2) << std::setfill('0') << (int)data[i];

        printLogging("Notification: " + ss.str(), LogType::BLE);
        receivedCondorNotifications += ss.str() + "|";
    }

    /**************************************************************************************************************/
    // ReadCompleted
    /**************************************************************************************************************/
    virtual void ReadCompleted(ReadWriteResult result, const std::vector<uint8_t>& data) override
    {
        receivedDataFromRead = "";
        std::stringstream ss;
        ss << std::hex;

        for (int i(0); i < data.size(); ++i)
            ss << std::setw(2) << std::setfill('0') << (int)data[i];

        receivedDataFromRead = ss.str();
        readUuidComplete = true;
    }
    

    /**************************************************************************************************************/
    // WriteCompleted
    /**************************************************************************************************************/
    virtual void WriteCompleted(ReadWriteResult result) override
    {
        if (application::BleLibResponse::ReadWriteResult::Success == result)
        {
            isWriteSuccesfull = true;
        }
        else
        {
            isWriteSuccesfull = false;
        }
    }

    /**************************************************************************************************************/
    // CondorOtauProgress
    /**************************************************************************************************************/
    virtual void CondorOtauProgress(bool success, uint32_t progress, uint32_t totalNrBytes) override
    {
        printLogging("Uploaded-> " + std::to_string(progress) + " of " + std::to_string(totalNrBytes) + " bytes", LogType::INTAPP);

        if (progress == totalNrBytes)
        {
            otauUploadDone = true;
        }
    }

    /**************************************************************************************************************/
    // ServiceDiscoveryComplete
    /**************************************************************************************************************/
    virtual void ServiceDiscoveryComplete(const std::vector<Service>& services) override
    {
        // clear the uuid list
        BLE_uuids = "";

        if ( false == services.empty())
        {
            for (int serv=0; serv < services.size(); serv++)
            {
                std::string uuid = "";
                std::string props = "";

                for (int i = 0; i < services.at(serv).uuid.size(); i++)
                {
                    uuid += hexify((services.at(serv).uuid.at(i)), false);
                }

                BLE_uuids += uuid + "|";

                std::vector < application::BleLibResponse::Characteristic> characteristics = services.at(serv).characteristics;
                for (int chars = 0; chars < characteristics.size(); chars++)
                {   
                    uuid = "";
                    props = "";

                    for (int i = 0; i < characteristics.at(chars).uuid.size(); i++)
                    {
                        uuid += hexify((characteristics.at(chars).uuid.at(i)), false);
                    }

                    props += hexify((uint8_t)characteristics.at(chars).properties, false);
                                      
                    BLE_uuids += uuid + ";" + props + "|";
                }
            }
            serviceDiscoveryComplete = true;
        }
    }

    /**************************************************************************************************************/
    // TuyaOtauProgress
    /**************************************************************************************************************/
    virtual void TuyaOtauProgress(bool success, uint32_t progress, uint32_t totalNrBytes) override
    {
        printLogging("Uploaded-> " + std::to_string(progress) + " of " + std::to_string(totalNrBytes) + " bytes", LogType::INTAPP);
        
        if (progress == totalNrBytes)
        {
            otauUploadDone = true;
        }
    }


    /**************************************************************************************************************/
    // DeviceDiscovered
    /**************************************************************************************************************/
    virtual void DeviceDiscovered(const std::array<uint8_t, 6>& discoveredAddress) override
    {
        printLogging("DeviceDiscovered:", LogType::BLE);

        // print the adress of the device
        std::cout << "BLE central\t: Mac of device-> ";
        for (int i = 0; i < sizeof(discoveredAddress); ++i)
            std::cout << std::hex << (uint16_t)discoveredAddress[i] << ":";
        std::cout << std::endl;

        if (address == discoveredAddress)
        {
            printLogging("Found device!", LogType::BLE);
            bleLib->StopDeviceDiscovery();
            return;
        }
    };


    /**************************************************************************************************************/
    // DeviceDiscoveryComplete
    /**************************************************************************************************************/
    virtual void DeviceDiscoveryComplete(void) override
    {
        printLogging("Device Discovery Complete", LogType::BLE);
    };


    /**************************************************************************************************************/
    // CentralStateChanged
    /**************************************************************************************************************/
    std::array<const char*, 5>  centralStates = { "Stopped", "Connecting", "Connected", "Disconnected", "Scanning" };
    virtual void StateChanged(application::BleLibResponse::State state) override
    {
        printLogging("Central State changed: " + (std::string)centralStates.at((int)state), LogType::BLE);
        deviceState = state;

        if (application::BleLibResponse::State::Disconnected == state)
        {
            isBound = false;
            canBind = false;
        }
    }

    /**************************************************************************************************************/
    // TuyaStateChanged
    /**************************************************************************************************************/
    std::array<const char*, 2>  tuyaStates = { "Bound", "NotBound" };
    virtual void TuyaStateChanged(TuyaState state) override
    {
        printLogging("Tuya State changed: " + (std::string)tuyaStates.at((int)state), LogType::BLE);

        if ("Bound" == (std::string)tuyaStates.at((int)state))
        {
            isBound = true;
        }
        else
        {
            isBound = false;
            canBind = true;
        }

        std::vector<uint8_t> indices;
        bleLib->QueryDataPoints(indices);
    }


    /**************************************************************************************************************/
    // addDataPoint
    /**************************************************************************************************************/
    void addDataPoint(TUYA_DataPoint* dp)
    {
        bool dPointPresent = false;
        int dpIndex = 0;

        // check if the datpoint is already there
        for (dpIndex = 0; dpIndex < DataPointData.size(); dpIndex++)
        {
            if (hexify(dp->id, false) == getDataFromParameter(DataPointData[dpIndex], "id"))
            {
                dPointPresent = true;
                break;
            }
        }

        std::string data = "id";
        data += hexify(dp->id, true);
        data += hexify(dp->type, true);
        data += hexify(dp->length, true);

        data += "|";
        if (dp->length == 1 && TUYA_DataPointType::TUYA_DT_STRING != dp->type)
        {
            data += hexify(dp->value8, false);
        }
        else if (dp->length == 2 && TUYA_DataPointType::TUYA_DT_STRING != dp->type)
        {
            data += hexify((dp->value16 >> 8), false);
            data += hexify(dp->value16, false);
        }
        else if (dp->length == 4 && TUYA_DataPointType::TUYA_DT_STRING != dp->type)
        {
            data += hexify((dp->value32 >> 24), false);
            data += hexify((dp->value32 >> 16), false);
            data += hexify((dp->value32 >> 8), false);
            data += hexify(dp->value32, false);
        }
        else
        {
            if (NULL != dp->raw)
            {
                for (int i = 0; i < dp->length; i++)
                {
                    data += hexify((uint8_t) * (dp->raw + i), false);
                }
            }
            else
            {
                data += "-";
            }
        }

        data += "*";

        // add the datapoint to the list
        if (false == dPointPresent)
        {
            DataPointData.push_back(data);
        }
        // update the existing datapoint
        else
        {
            DataPointData[dpIndex] = data;
        }

        if (getDataFromParameter(data, "id") == notificationDataPointID)
        {
            newNotification = true;
        }

    }


    /**************************************************************************************************************/
    // DataPointsReceived
    /**************************************************************************************************************/
    virtual void TuyaDataPointsReceived(const std::vector<uint8_t>& data) override
    {
        //printLogging("Received datapoint entry", LogType::TUYA);
        TUYA_DataPoint* dp = TUYA_GetFirstDataPoint(const_cast<uint8_t*>(&data[0]), static_cast<uint16_t>(data.size()));
        
        while (dp != NULL)
        {
            if ( std::to_string(static_cast<int32_t>(dp->id)) != "120" && //DP120 is MotorCurrent
                 std::to_string(static_cast<int32_t>(dp->id)) != "138" )  //DP138 is MotionType
            {
                printLogging("Received datapoint with ID : " + std::to_string(static_cast<int32_t>(dp->id)), LogType::BLE);
            }
            addDataPoint(dp);
            dp = TUYA_GetNextDataPoint();
        }
        //printLogging("Received datapoint exit", LogType::TUYA);
    }
};

/**************************************************************************************************************/
// otauProgress
/**************************************************************************************************************/
std::string currentTime() {
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%X", &tstruct);

    return buf;
}


/**************************************************************************************************************/
// hexify
/**************************************************************************************************************/
std::string hexify(uint8_t n, bool separator)
{
    std::string res;
    std::string leadingZero = "";

    if (n < 16)
    {
        leadingZero = "0";
    }

    do
    {
        res += "0123456789ABCDEF"[n % 16];
        n >>= 4;
    } while (n);

    if (separator)
    {
        return "|" + leadingZero + std::string(res.rbegin(), res.rend());
    }
    else
    {
        return leadingZero + std::string(res.rbegin(), res.rend());
    }
}


/**************************************************************************************************************/
// processTuyaLib
/**************************************************************************************************************/
long long milliseconds_now() 
{
    static LARGE_INTEGER s_frequency;
    static BOOL s_use_qpc = QueryPerformanceFrequency(&s_frequency);
    if (s_use_qpc) {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        return (1000LL * now.QuadPart) / s_frequency.QuadPart;
    }
    else {
        return GetTickCount();
    }
}


/**************************************************************************************************************/
// waitXseconds
/**************************************************************************************************************/
void waitXseconds(int seconds)
{
    long long start = milliseconds_now();
    long long elapsed = 0;
    do
    {
        elapsed = milliseconds_now() - start;
    } while (elapsed < (seconds * 1000));
}


/**************************************************************************************************************/
// hex_char_to_int
/**************************************************************************************************************/
unsigned hex_char_to_int(char c) {
    unsigned result = -1;
    if (('0' <= c) && (c <= '9')) {
        result = c - '0';
    }
    else if (('A' <= c) && (c <= 'F')) {
        result = 10 + c - 'A';
    }
    else if (('a' <= c) && (c <= 'f')) {
        result = 10 + c - 'a';
    }
    else {
        //assert(0);
    }
    return result;
}

/**************************************************************************************************************/
// ConvertUUIDtoBytes
/**************************************************************************************************************/
application::BleLib::Uuid ConvertUUIDtoByte(const char* str, size_t n) {
    application::BleLib::Uuid result;
    for (size_t i = 0; i < n; i += 2) {
        unsigned number = hex_char_to_int(str[i]); // most signifcnt nibble
        if ((i + 1) < n) {
            unsigned lsn = hex_char_to_int(str[i + 1]); // least signt nibble
            number = (number << 4) + lsn;
        }
        result.push_back(number);
    }
    return result;
}


/**************************************************************************************************************/
// processTuyaLib
/**************************************************************************************************************/
static bool receivedDeviceParameters = false;

enum class actionType {
    IDLE = 0,
    act_StartDeviceDiscovery,
    act_StopDeviceDiscovery,
    act_Connect,
    act_ServiceDiscovery,
    act_Bind,
    act_Disconnect,
    act_RemoveBonds,
    act_Unbind,
    act_QueryDataPoints,
    act_SendDataPoints,
    act_StartOtauUpload,
    act_StartOtauUpgrade,
    act_StopOtauUpload,
    act_CheckForTuyaDevice,
    act_ReadFromUUID,
    act_WriteToUUID,
    act_EnableNotificationForUUID,
    act_SendNumericComparisonRequestConformation
};

std::queue<actionType> tuyaLibActions;
static bool b_actionDone = false;
static std::vector<uint8_t> dpsToRequest;
static std::vector<TUYA_DataPoint> dpsToTransmit;
static std::vector<uint8_t> otauData;

void processTuyaLib(void)
{
    while (false == receivedDeviceParameters)
    {
        Sleep(10);
    }

    BLELibResponseImpl response;
    application::BleLibFactory::Create(comPort, response, tuyaPid, tuyaDid, tuyaAuthKey, isLESCconnection, false);
    bleLib = application::BleLibFactory::Get();

    while ( true == keepTuyaLibProcesRunning)
    {
        // perform MainloopRun as much as possible
        bleLib->MainloopRun();

        // handle all tuyaLib actions in queue
        if ( true != tuyaLibActions.empty())
        {
            switch (tuyaLibActions.front())
            {
            case actionType::act_Disconnect:
                bleLib->Disconnect();
                printLogging("Action: act_Disconnect", LogType::INTAPP);
                break;

            case actionType::act_Connect:
                application::BleLibResponse::Address device = address;

                //random adressing, no bonding
                bleLib->Connect(device, application::BleLibResponse::AddressType::Random, authenticationType);
                printLogging("Action: act_Connect", LogType::INTAPP);
                break;

            case actionType::act_ServiceDiscovery:
                bleLib->ServiceDiscovery();
                printLogging("Action: act_ServiceDiscovery", LogType::INTAPP);
                break;

            case actionType::act_Bind:
                bleLib->Bind();
                printLogging("Action: act_Bind", LogType::INTAPP);
                break;

            case actionType::act_SendNumericComparisonRequestConformation:
                bleLib->NumericComparisonConfirm(true);
                printLogging("Action: act_SendNumericComparisonRequestConformation", LogType::INTAPP);
                break;

            case actionType::act_QueryDataPoints:
                bleLib->QueryDataPoints(dpsToRequest);
                printLogging("Action: act_QueryDataPoints", LogType::INTAPP);
                break;

            case actionType::act_SendDataPoints:
                bleLib->SendDataPoints(dpsToTransmit);
                printLogging("Action: act_SendDataPoints", LogType::INTAPP);
                break;

            case actionType::act_Unbind:
                bleLib->Unbind();
                printLogging("Action: act_Unbind", LogType::INTAPP);
                break;

            case actionType::act_RemoveBonds:
                bleLib->RemoveBonds();
                printLogging("Action: act_RemoveBonds", LogType::INTAPP);
                break;

            case actionType::act_StartOtauUpload:
                // check if device is Tuya or Condor
                if (true == isTuyaDevice)
                {
                    bleLib->StartTuyaOtauUpload(otauData);
                }
                else
                {
                    // It is a Condor product
                    bleLib->StartCondorOtauUpload(otauData, restartOtauUpload);
                }
                printLogging("Action: act_StartOtauUpload", LogType::INTAPP);
                break;

            case actionType::act_StartOtauUpgrade:
                // check if device is Tuya or Condor
                if (true == isTuyaDevice)
                {
                    bleLib->StartTuyaOtauUpgrade();
                }
                else
                {
                    // It is a Condor product
                    bleLib->StartCondorOtauUpgrade();
                }
                printLogging("Action: act_StartOtauUpgrade", LogType::INTAPP);
                break;

            case actionType::act_CheckForTuyaDevice:
                isTuyaDevice = bleLib->HasTuya();
                printLogging("Action: act_CheckForTuyaDevice", LogType::INTAPP);
                break;

            case actionType::act_ReadFromUUID:
                readUuidComplete = false;
                bleLib->Read(UUIDToTransmit);
                printLogging("Action: act_ReadFromUUID", LogType::INTAPP);
                break;

            case actionType::act_WriteToUUID:
                bleLib->Write(UUIDToTransmit, DataToWriteToUUID);
                printLogging("Action: act_WriteToUUID", LogType::INTAPP);
                break;

            case actionType::act_EnableNotificationForUUID:
                bleLib->EnableNotifications(UUIDToTransmit, enablingNotification);
                printLogging("Action: act_EnableNotificationForUUID", LogType::INTAPP);
                break;

            default:
                printLogging("Action: Something went verry wrong!!", LogType::INTAPP);
                break;
            }

            tuyaLibActions.pop();
        }
    }

    isTuyaLibProcesFinished = true;
}

/**************************************************************************************************************/
// performTuyaAction
/**************************************************************************************************************/
void performTuyaAction(actionType tuyaLibAction)
{
    // add the action to the tuyaLibActions
    tuyaLibActions.push(tuyaLibAction);
}

/**************************************************************************************************************/
// getDataFromParameter
/**************************************************************************************************************/
std::string getDataFromParameter(std::string allData, std::string parameter)
{
    std::string parameterData = "";
    std::vector<std::string> seglist;
    bool continuProces = true;

    while (true == continuProces)
    {
        std::string item = "";

        for (int i = 0; i < allData.length(); i++)
        {
            if (allData[i] == '|')
            {
                seglist.push_back(item);
                item = "";
            }
            else if (allData[i] == '*')
            {
                seglist.push_back(item);
                continuProces = false;
            }
            else
            {
                item += allData[i];
            }
        }
    }

    // get the item from the list corresponding with parameter
    for (int i = 0; i < seglist.size(); i++)
    {
        if (0 == seglist[i].compare(parameter))
        {
            parameterData = seglist[i + 1];
            break;
        }
    }

    return parameterData;
}


std::array<std::string, 9>  connect_error = {
    "Success",
    "PasskeyEntryFailed",
    "AuthenticationRequirementsNotMet",
    "PairingNotSupported",
    "InsufficientEncryptionKeySize",
    "NumericComparisonFailed",
    "Timeout",
    "Unknown",
    "Max"
};
/**************************************************************************************************************/
// proces_incomming_data
/**************************************************************************************************************/
void proces_incomming_data(SOCKET* ClientSocket, std::string receivedData)
{
    std::string command = getDataFromParameter(receivedData, "command");
    printLogging("Received command: " + command, LogType::TST);

    socketData.push_back(receivedData);

    /**************************************************************************************/
    if ("init" == command)
    {
        /* A Tuya device will be searched and connected */
        comPort = getDataFromParameter(receivedData, "comPort");
        tuyaPid = getDataFromParameter(receivedData, "tuyaPid");
        tuyaDid = getDataFromParameter(receivedData, "tuyaDid");
        tuyaAuthKey = getDataFromParameter(receivedData, "tuyaAuthKey");
        std::string shaverMacAddr = getDataFromParameter(receivedData, "shaverMacAddr");
        
        std::string LESC  = getDataFromParameter(receivedData, "isLESC");

        printLogging("Device has LESC: " + LESC, LogType::TST);
        printLogging("Device MAC: " + shaverMacAddr, LogType::TST);

        if ("true" == LESC)
        {
            isLESCconnection = true;
        }
        else
        {
            isLESCconnection = false;
        }

        std::string authType = getDataFromParameter(receivedData, "authType");
        if ("bond" == authType)
        {
            authenticationType = application::BleLib::PairingMode::Bond;
        }
        else
        {
            authenticationType = application::BleLib::PairingMode::Pair;
        }

        // conver the macadress to hex values
        int macIndex = 0;
        for (unsigned int i = 0; i < shaverMacAddr.length(); i += 2)
        {
            std::string byteString = shaverMacAddr.substr(i, 2);
            address[macIndex] = (char)strtol(byteString.c_str(), NULL, 16);
            macIndex++;
        }

        // all parameters are received
        receivedDeviceParameters = true;
        
        sendDataTroughSocket(ClientSocket, "OK");
     }

    /**************************************************************************************/
    else if ("connect" == command)
    {
        /* A Tuya device will be connected */
        std::string connectData = getDataFromParameter(receivedData, "data");
        std::string hasMITM = getDataFromParameter(receivedData, "mitm");
        printLogging("Data: " + connectData, LogType::TST);
        isBound = false;
        canBind = false;
        serviceDiscoveryComplete = false;
        authentication_status = application::BleLibResponse::AuthenticationStatus::Unknown;

        if (application::BleLib::State::Disconnected == deviceState)
        {
            // perform tuyaLib action
            performTuyaAction(actionType::act_Connect);

            // wait until found and connected;
            int maxTime_ms = (1 * 60 * 1000);

            long long start = milliseconds_now();
            long long elapsed = 0;
            // wait for connect to complete
            do
            {
                elapsed = milliseconds_now() - start;
            } while (application::BleLib::State::Connected != deviceState && elapsed <= maxTime_ms);

            // Wait for 2 seconds to check if a disconnect does not happen
            waitXseconds(2);

            if (application::BleLib::State::Connected == deviceState)
            {
                if (true == isLESCconnection)
                {
                    // do service discovery
                    start = milliseconds_now();
                    elapsed = 0;
                    // wait for connect to complete

                    std::string b_True = "true";
                    // Yes "Man In The Middle"
                    if (b_True == hasMITM)
                    {
                        do
                        {
                            elapsed = milliseconds_now() - start;
                            //} while (application::BleLibResponse::AuthenticationStatus::Success != authentication_status && elapsed <= maxTime_ms);
                        } while ((false == NumericComparisonRequestReceived) && (elapsed <= maxTime_ms));
                    }
                    // Nope no "Man In The Middle"
                    else
                    {
                        do
                        {
                            elapsed = milliseconds_now() - start;
                        } while (application::BleLibResponse::AuthenticationStatus::Success != authentication_status && elapsed <= maxTime_ms);
                        //} while ((false == NumericComparisonRequestReceived) && (elapsed <= maxTime_ms));
                    }
                }
                sendDataTroughSocket(ClientSocket, "OK");
            }
            else
            {
                sendDataTroughSocket(ClientSocket, "NOK");
            }
        }
        else
        {
            sendDataTroughSocket(ClientSocket, "OK");
        }
    }

    /**************************************************************************************/
    else if ("startServiceDiscovery" == command)
    {
        // wait until found and connected;
        int maxTime_ms = (1 * 60 * 1000);

        long long start = milliseconds_now();
        long long elapsed = 0;
        // wait for connect to complete

        if (true == NumericComparisonRequestReceived &&
            application::BleLib::State::Connected == deviceState &&
            true == isLESCconnection)
        {
            performTuyaAction(actionType::act_SendNumericComparisonRequestConformation);
        }        

        do
        {
            elapsed = milliseconds_now() - start;
        } while (application::BleLibResponse::AuthenticationStatus::Success != authentication_status && elapsed <= maxTime_ms);

        if (application::BleLibResponse::AuthenticationStatus::Success == authentication_status)
        {
            performTuyaAction(actionType::act_ServiceDiscovery);

            // Max wait time
            int maxTime_ms = (1 * 60 * 1000);

            long long start = milliseconds_now();
            long long elapsed = 0;

            // wait for servicediscovery to complete
            do
            {
                elapsed = milliseconds_now() - start;
            } while ((false == serviceDiscoveryComplete) && (elapsed <= maxTime_ms));

            // check if device is Tuya device
            performTuyaAction(actionType::act_CheckForTuyaDevice);
            waitXseconds(2);

            if (true == isTuyaDevice)
            {
                // perform tuyaLib action
                performTuyaAction(actionType::act_Bind);

                start = milliseconds_now();
                elapsed = 0;
                // wait for bind to complete
                do
                {
                    elapsed = milliseconds_now() - start;
                } while (false == (true == isBound && application::BleLib::State::Connected == deviceState) && elapsed <= maxTime_ms);

                // respond if connection was succesfull or not for Tuya device
                if (application::BleLib::State::Connected == deviceState && true == isBound && true == serviceDiscoveryComplete)
                {
                    sendDataTroughSocket(ClientSocket, "OK");
                }
                else
                {
                    sendDataTroughSocket(ClientSocket, "NOK");
                }
            }
            else
            {
                // respond if connection was succesfull or not for Condor device
                if ((application::BleLib::State::Connected == deviceState) &&
                    (true == serviceDiscoveryComplete) &&
                    (application::BleLibResponse::AuthenticationStatus::Success == authentication_status))
                {
                    sendDataTroughSocket(ClientSocket, "OK");
                }
                else
                {
                    sendDataTroughSocket(ClientSocket, "NOK, " + connect_error[(uint8_t)authentication_status]);
                }
            }
        }
        else
        {
            sendDataTroughSocket(ClientSocket, "NOK");
        }
    }

    /**************************************************************************************/
    else if ("connectNoBind" == command)
    {
        /* A Tuya device will be connected */
        std::string connectData = getDataFromParameter(receivedData, "data");
        std::string deviceType = getDataFromParameter(receivedData, "deviceType");
        printLogging("Data: " + connectData, LogType::TST);
        isBound = false;
        canBind = false;
        serviceDiscoveryComplete = false;

        // Back-up original authenticationType
        application::BleLib::PairingMode backup_authenticationType = authenticationType;
        if ("tuya" != deviceType)
        {
            authenticationType = application::BleLib::PairingMode::None;
        }

        if ( application::BleLib::State::Disconnected == deviceState)
        {
            // perform tuyaLib action
            performTuyaAction(actionType::act_Connect);
        }

        // wait until found and connected;
        int maxTime_ms = (2 * 60 * 1000);

        long long start = milliseconds_now();
        long long elapsed = 0;
        // wait for connect to complete
        do
        {
            elapsed = milliseconds_now() - start;
        } while ( application::BleLib::State::Connected != deviceState && elapsed <= maxTime_ms);

        if (application::BleLib::State::Connected == deviceState)
        {
            // do service discovery
            waitXseconds(2);
            performTuyaAction(actionType::act_ServiceDiscovery);

            // wait for servicediscovery to complete
            start = milliseconds_now();
            elapsed = 0;
            do
            {
                elapsed = milliseconds_now() - start;
            } while ( (false == serviceDiscoveryComplete) && (elapsed <= maxTime_ms) );

            // check if device is Tuya device
            performTuyaAction(actionType::act_CheckForTuyaDevice);
            waitXseconds(2);
        }


        if (application::BleLib::State::Connected == deviceState && true == serviceDiscoveryComplete)
        {
            sendDataTroughSocket(ClientSocket, "OK");
        }
        else
        {
            sendDataTroughSocket(ClientSocket, "NOK");
        }

        // restore the original authenticationType
        if ("tuya" != deviceType)
        {
            authenticationType = backup_authenticationType;
        }
    
    }

    /**************************************************************************************/
    else if ("bind" == command)
    {
        // check if device is Tuya or Condor
        if (true == isTuyaDevice)
        {
            /* A Tuya device will be Bind */
            std::string bindData = getDataFromParameter(receivedData, "data");
            printLogging("Data: " + bindData, LogType::TST);
            isBound = false;

            if (application::BleLib::State::Connected == deviceState && true == canBind)
            {

                performTuyaAction(actionType::act_Bind);
            }

            // wait until Bound;
            int maxTime_ms = (2 * 60 * 1000);

            long long start = milliseconds_now();
            long long elapsed = 0;
            // wait for Bind to complete
            do
            {
                elapsed = milliseconds_now() - start;
            } while (false == (true == isBound && application::BleLib::State::Connected == deviceState) && elapsed <= maxTime_ms);

            if (true == isBound)
            {
                sendDataTroughSocket(ClientSocket, "OK");
            }
            else
            {
                sendDataTroughSocket(ClientSocket, "NOK");
            }
        }
        else
        {
            sendDataTroughSocket(ClientSocket, "NOK, no Tuya device");
        }
    }

    /**************************************************************************************/
    else if ("disconnect" == command)
    {
        /* A BLE device will be disconnected */
        std::string disconnectData = getDataFromParameter(receivedData, "data");
        printLogging("Data: " + disconnectData, LogType::TST);

        if (application::BleLib::State::Connected == deviceState)
        {
            // perform tuyaLib action
            performTuyaAction(actionType::act_Disconnect);
        }

        sendDataTroughSocket(ClientSocket, "OK");

        DataPointData.clear();
    }

    /**************************************************************************************/
    else if ("getdatapoint" == command)
    {
        // check if device is Tuya or Condor
        if (true == isTuyaDevice)
        {
            /* A datapoint will be retrieved from a Tuya device */
            std::string datapointData = getDataFromParameter(receivedData, "datapoint");
            std::string retrieveType = getDataFromParameter(receivedData, "typeRWN");
            bool dataPointFound = false;

            printLogging("Data: " + datapointData, LogType::TST);

            if (application::BleLib::State::Connected == deviceState)
            {
                for (int dpIndex = 0; dpIndex < DataPointData.size(); dpIndex++)
                {
                    if (datapointData == getDataFromParameter(DataPointData[dpIndex], "id"))
                    {
                        dataPointFound = true;
                        sendDataTroughSocket(ClientSocket, DataPointData[dpIndex]);
                    }
                }

                if (false == dataPointFound)
                {
                    //if ("R" != retrieveType)
                    //{
                    uint8_t dp_id = std::stoi(datapointData, nullptr, 16);
                    dpsToRequest.clear();
                    dpsToRequest.push_back(dp_id);

                    // perform tuyaLib action
                    performTuyaAction(actionType::act_QueryDataPoints);
                    //}
                    //else
                    //{
                    //    // THIS is only when the datapoint is readonly
                    //    DataPoint dPoint;
                    //    dPoint.id = std::stoi(datapointData, nullptr, 16);
                    //    dPoint.length = 1;
                    //    dPoint.type = DT_VALUE;
                    //    dPoint.value8 = 0;                                      

                    //    // add the data point to the array
                    //    dpsToTransmit.clear();
                    //    dpsToTransmit.push_back(dPoint);

                    //    // perform tuyaLib action
                    //   performTuyaAction(actionType::act_SendDataPoints, 10);

                    //}
                    sendDataTroughSocket(ClientSocket, "retry");

                }
            }
            else
            {
                sendDataTroughSocket(ClientSocket, "not connected");
            }
        }
        else
        {
            sendDataTroughSocket(ClientSocket, "NOK, no Tuya device");
        }
    }

    /**************************************************************************************/
    else if ("readUUID" == command)
    {
        // check if device is Tuya or Condor
        if (false == isTuyaDevice)
        {
            /* A UUID will be retrieved from a Condor device */
            std::string uuid = getDataFromParameter(receivedData, "uuid");

            printLogging("Requested UUID: " + uuid, LogType::TST);

            if ( ( application::BleLib::State::Connected == deviceState ) ) //&&
                 //( application::BleLibResponse::AuthenticationStatus::Success == authentication_status) )
            {
                if ("" == receivedDataFromRead)
                {
                    UUIDToTransmit.clear();
                    UUIDToTransmit = ConvertUUIDtoByte(uuid.c_str(), uuid.length());
                    // perform tuyaLib action
                    performTuyaAction(actionType::act_ReadFromUUID);

                    sendDataTroughSocket(ClientSocket, "retry");
                }
                else
                {
                    sendDataTroughSocket(ClientSocket, receivedDataFromRead);
                    receivedDataFromRead = "";
                }
            }
            else
            {
                if ((application::BleLib::State::Connected == deviceState) &&
                    (application::BleLibResponse::AuthenticationStatus::Success != authentication_status))
                {
                    sendDataTroughSocket(ClientSocket, "not connected, "+connect_error[(uint8_t)authentication_status]);
                }
                else
                {
                    sendDataTroughSocket(ClientSocket, "not connected");
                }
            }
        }
        else
        {
            sendDataTroughSocket(ClientSocket, "NOK, no Condor device");
        }

    }

    /**************************************************************************************/
    else if ("writeUUID" == command)
    {
        // check if device is Tuya or Condor
        if (false == isTuyaDevice)
        {
            /* A UUID will be written from a Condor device */
            std::string uuid = getDataFromParameter(receivedData, "uuid");
            std::string dataToSend = getDataFromParameter(receivedData, "data");

            printLogging("To be written UUID: " + uuid, LogType::TST);

            if ((application::BleLib::State::Connected == deviceState) &&
                (application::BleLibResponse::AuthenticationStatus::Success == authentication_status))
            {
                UUIDToTransmit.clear();
                UUIDToTransmit = ConvertUUIDtoByte(uuid.c_str(), uuid.length());

                DataToWriteToUUID.clear();
                DataToWriteToUUID = ConvertUUIDtoByte(dataToSend.c_str(), dataToSend.length());

                // perform tuyaLib action
                performTuyaAction(actionType::act_WriteToUUID);

                waitXseconds(1);

                if (true == isWriteSuccesfull)
                {
                    sendDataTroughSocket(ClientSocket, "OK");
                }
                else
                {
                    sendDataTroughSocket(ClientSocket, "NOK");
                }
            }
            else
            {
                if ((application::BleLib::State::Connected == deviceState) &&
                    (application::BleLibResponse::AuthenticationStatus::Success != authentication_status))
                {
                    sendDataTroughSocket(ClientSocket, "not connected, " + connect_error[(uint8_t)authentication_status]);
                }
                else
                {
                    sendDataTroughSocket(ClientSocket, "not connected");
                }
            }
        }
        else
        {
            sendDataTroughSocket(ClientSocket, "NOK, no Condor device");
        }

    }


    /**************************************************************************************/
    else if ("enablenotifications" == command)
    {
        // check if device is Tuya or Condor
        if (false == isTuyaDevice)
        {
            /* Notifications for a UUID will be en or disabled */
            std::string uuid = getDataFromParameter(receivedData, "uuid");
            std::string dataToSend = getDataFromParameter(receivedData, "data");

            printLogging("To be en- or dis-abled: " + uuid + " -> " + dataToSend, LogType::TST);

            if (application::BleLib::State::Connected == deviceState)
            {
                UUIDToTransmit.clear();
                UUIDToTransmit = ConvertUUIDtoByte(uuid.c_str(), uuid.length());
                enablingNotification = ("True" == dataToSend) ? true : false;

                // perform tuyaLib action
                performTuyaAction(actionType::act_EnableNotificationForUUID);

                waitXseconds(1);
                if (true == isWriteSuccesfull)
                {
                    sendDataTroughSocket(ClientSocket, "OK");
                }
                else
                {
                    sendDataTroughSocket(ClientSocket, "NOK");
                }
            }
            else
            {
                sendDataTroughSocket(ClientSocket, "not connected");
            }
        }
        else
        {
            sendDataTroughSocket(ClientSocket, "NOK, no Condor device");
        }
    }

    /**************************************************************************************/
    else if ("setdatapoint" == command)
    {
        // check if device is Tuya or Condor
        if (true == isTuyaDevice)
        {
            /* A datapoint will be set in a Tuya device */
            std::string datapoint = getDataFromParameter(receivedData, "datapoint");
            std::string datapointData = getDataFromParameter(receivedData, "data");

            printLogging("Datapoint: " + datapoint + ", datapointData: " + datapointData, LogType::TST);

            if (application::BleLib::State::Connected == deviceState)
            {
                TUYA_DataPoint dPoint;
                dPoint.id = std::stoi(datapoint, nullptr, 16);
                dPoint.length = datapointData.length() / 2;
                dPoint.type = (TUYA_DataPointType)std::stoi(getDataFromParameter(receivedData, "type"), nullptr, 10);
                dPoint.value32 = 0;

                if (dPoint.length == 1 && TUYA_DataPointType::TUYA_DT_STRING != dPoint.type)
                {
                    dPoint.value8 = std::stoi(datapointData, nullptr, 16);
                }
                else if (dPoint.length == 2 && TUYA_DataPointType::TUYA_DT_STRING != dPoint.type)
                {
                    dPoint.value16 = std::stoi(datapointData, nullptr, 16);
                }
                else if (dPoint.length == 4 && TUYA_DataPointType::TUYA_DT_STRING != dPoint.type)
                {
                    dPoint.value32 = std::stoi(datapointData, nullptr, 16);
                }
                else
                {
                    dPoint.raw = (uint8_t*)datapointData.c_str();
                }

                // add the data point to the array6
                dpsToTransmit.clear();
                dpsToTransmit.push_back(dPoint);

                // perform tuyaLib action
                performTuyaAction(actionType::act_SendDataPoints);

                sendDataTroughSocket(ClientSocket, "OK");
            }
            else
            {
                sendDataTroughSocket(ClientSocket, "Device is not connected");
            }
        }
        else
        {
            sendDataTroughSocket(ClientSocket, "NOK, no Tuya device");
        }
    }

    /**************************************************************************************/
    else if ("listenfornotifications" == command)
    {
        /* A tuya DataPoint will be reomved from the stored list */
        notificationDataPointID = getDataFromParameter(receivedData, "datapoint");
        long listentime = std::stoi(getDataFromParameter(receivedData, "listentime"));
        int runtime = 0;
        int sleepTime_ms = 500;
        std::string notifications = "";

        long keepAliveTimer = 0;
        long msec_prev = milliseconds_now();

        // check if device is Tuya or Condor
        if (true == isTuyaDevice)
        {
            // log notification for the given time
            do
            {
                if (true == newNotification)
                {
                    newNotification = false;

                    for (int dpIndex = 0; dpIndex < DataPointData.size(); dpIndex++)
                    {
                        if (notificationDataPointID == getDataFromParameter(DataPointData[dpIndex], "id"))
                        {
                            notifications += DataPointData[dpIndex];
                        }
                    }
                }

                if (keepAliveTimer >= (50 * 60 * 1000))
                {
                    uint8_t dp_id = 0xA3;
                    dpsToRequest.clear();
                    dpsToRequest.push_back(dp_id);

                    // perform tuyaLib action
                    performTuyaAction(actionType::act_QueryDataPoints);

                    // wait until Bound;
                    int maxTime_ms = (2 * 60 * 1000);

                    long long start = milliseconds_now();
                    long long elapsed = 0;

                    // wait for dp to be received
                    bool dataPointFound = false;
                    do
                    {
                        for (int dpIndex = 0; dpIndex < DataPointData.size(); dpIndex++)
                        {
                            if ("A3" == getDataFromParameter(DataPointData[dpIndex], "id"))
                            {
                                dataPointFound = true;
                            }
                        }

                        elapsed = milliseconds_now() - start;
                    } while (false == dataPointFound && elapsed <= maxTime_ms);


                    TUYA_DataPoint dPoint;
                    dPoint.id = 0xA3;
                    dPoint.length = 4;
                    dPoint.type = TUYA_DT_VALUE;
                    dPoint.value32 = 0;

                    for (int dpIndex = 0; dpIndex < DataPointData.size(); dpIndex++)
                    {
                        if ("A3" == getDataFromParameter(DataPointData[dpIndex], "id"))
                        {
                            size_t pos = DataPointData[dpIndex].find_last_of("|");
                            std::string token = DataPointData[dpIndex].substr(pos + 1, dPoint.length * 2);

                            dPoint.value32 = std::stoi(token, nullptr, 16);
                        }
                    }

                    // add the data point to the array
                    dpsToTransmit.clear();
                    dpsToTransmit.push_back(dPoint);

                    // perform tuyaLib action
                    performTuyaAction(actionType::act_SendDataPoints);
                    printLogging("Datapoint was sent to reset MaxConnectivity counter", LogType::INTAPP);
                    keepAliveTimer = 0;
                }
                else
                {
                    keepAliveTimer += milliseconds_now() - msec_prev;
                    msec_prev = milliseconds_now();
                }

                Sleep(sleepTime_ms);
                runtime += sleepTime_ms;
            } while (runtime <= (listentime * 1000));

            notificationDataPointID = "";
        }
        else
        {
            receivedCondorNotifications = "";
            

            // Apperently when it passes this point its a Condor device
            // log notification for the given time
            do
            {
                if ( keepAliveTimer >= (50 * 60 * 1000) )
                {
                    std::string uuidKeepALive = "8D5601163CB94387A7E8B79D826A7025";
                    UUIDToTransmit.clear();
                    UUIDToTransmit = ConvertUUIDtoByte(uuidKeepALive.c_str(), uuidKeepALive.length());

                    receivedDataFromRead = "";
                    performTuyaAction(actionType::act_ReadFromUUID);

                    int maxTime_ms = (10 * 60 * 1000);
                    long long start = milliseconds_now();
                    long long elapsed = 0;

                    do
                    {
                        elapsed = milliseconds_now() - start;
                    } while (false == readUuidComplete && elapsed <= maxTime_ms);


                    // wait some extra to continu
                    waitXseconds(2);

                    DataToWriteToUUID.clear();
                    DataToWriteToUUID = ConvertUUIDtoByte( receivedDataFromRead.c_str() , receivedDataFromRead.length());

                    printLogging("KeepAlive data : "+ receivedDataFromRead, LogType::INTAPP);

                    // perform tuyaLib action
                    performTuyaAction(actionType::act_WriteToUUID);
                    printLogging("KeepAlive was sent to reset MaxConnectivity counter", LogType::INTAPP);
                    receivedDataFromRead = "";
                    keepAliveTimer = 0;
                }
                else
                {
                    keepAliveTimer += milliseconds_now() - msec_prev;
                    msec_prev = milliseconds_now();
                }

                Sleep(sleepTime_ms);
                runtime += sleepTime_ms;
            } while (runtime <= (listentime * 1000));

            notifications = "C"+receivedCondorNotifications;
        }

        // send the notifications back to teststand
        sendDataTroughSocket(ClientSocket, notifications);
    }

    /**************************************************************************************/
    else if ("clearDP" == command)
    {
        // check if device is Tuya or Condor
        if (true == isTuyaDevice)
        {
            /* A tuya DataPoint will be reomved from the stored list */
            std::string clearDPData = getDataFromParameter(receivedData, "datapoint");
            printLogging("Datapoint: " + clearDPData, LogType::TST);

            if ("00" == clearDPData)
            {
                DataPointData.clear();
            }
            else
            {
                // remove datapoint if present
                for (int dpIndex = 0; dpIndex < DataPointData.size(); dpIndex++)
                {
                    if (clearDPData == getDataFromParameter(DataPointData[dpIndex], "id"))
                    {
                        printLogging("Remove dataPoint: " + DataPointData[dpIndex], LogType::INTAPP);
                        DataPointData.erase(DataPointData.begin() + dpIndex);
                        break;
                    }
                }
            }

            sendDataTroughSocket(ClientSocket, "OK");
        }
        else
        {
            sendDataTroughSocket(ClientSocket, "NOK, no Tuya device");
        }
    }

    /**************************************************************************************/
    else if ("getDPList" == command)
    {
        // check if device is Tuya or Condor
        if (true == isTuyaDevice)
        {
            std::string dpList = "";

            if (application::BleLib::State::Connected == deviceState)
            {
                for (int dpIndex = 0; dpIndex < DataPointData.size(); dpIndex++)
                {
                    dpList += getDataFromParameter(DataPointData[dpIndex], "id")+"|";
                }
            }
            else
            {
                dpList = "not connected";
            }
    
            sendDataTroughSocket(ClientSocket, dpList);
        }
        else
        {
            sendDataTroughSocket(ClientSocket, "NOK, no Tuya device");
        }
    }


    /**************************************************************************************/
    else if ("getuuidlist" == command)
    {
        if (application::BleLib::State::Connected == deviceState && "" != BLE_uuids)
        {
            sendDataTroughSocket(ClientSocket, BLE_uuids);
        }
        else
        {
            sendDataTroughSocket(ClientSocket, "not connected");
        }
    }

    /**************************************************************************************/
    else if ("unbind" == command)
    {
        // check if device is Tuya or Condor
        if (true == isTuyaDevice)
        {
            /* A tuya device will be unbind */
            std::string unbindData = getDataFromParameter(receivedData, "data");
            printLogging("Data: " + unbindData, LogType::TST);

            // perform tuyaLib action
            performTuyaAction(actionType::act_Unbind);

            sendDataTroughSocket(ClientSocket, "OK");
        }
        else
        {
            sendDataTroughSocket(ClientSocket, "NOK, no Tuya device");
        }
    }

    /**************************************************************************************/
    else if ("removebonds" == command)
    {
        /* A tuya device will be unbind */
        std::string unbondData = getDataFromParameter(receivedData, "data");
        printLogging("Data: " + unbondData, LogType::TST);

        // perform tuyaLib action
        performTuyaAction(actionType::act_RemoveBonds);

        sendDataTroughSocket(ClientSocket, "OK");
    }

    /**************************************************************************************/
    else if ("otauupload" == command)
    {
        /* An Over The Air Update will be performed */
        std::string product = getDataFromParameter(receivedData, "product");
        std::string otauFile = getDataFromParameter(receivedData, "file");
        std::string uploadType = getDataFromParameter(receivedData, "type");
        int waitTime_min = atoi(getDataFromParameter(receivedData, "waittime_min").c_str());
        std::string otauFilename = "C:\\Projects\\ETP\\trunk\\Software\\LabETP\\02-TestStand\\01-Default\\config\\DUTSoftware\\" + product + "\\" + otauFile;
        printLogging("Data: " + otauFilename, LogType::TST);

        otauUploadDone = false;

        // open the file:
        std::streampos fileSize;
        std::ifstream file(otauFilename, std::ios::binary);

        // get its size:
        file.seekg(0, std::ios::end);
        fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        // read the data:
        otauData.clear();
        otauData.resize(fileSize);
        file.read((char*)&otauData[0], fileSize);

        // try to start the data transfer of the upg file
        performTuyaAction(actionType::act_StartOtauUpload);

        //*********************************************************
        long long maxTime_ms = (( long long )waitTime_min * 60 * 1000);

        long long start = milliseconds_now();
        long long elapsed = 0;
        do
        {
            if ("chunck" == uploadType && elapsed >= 30*1000)
            {
                // perform tuyaLib action
                performTuyaAction(actionType::act_Disconnect);
                break;
            }
            elapsed = milliseconds_now() - start;
        } while (true != otauUploadDone && elapsed <= maxTime_ms);
        //*********************************************************
        
        if ("chunck" == uploadType)
        {
            restartOtauUpload = false;
        }
        else
        {
            restartOtauUpload = true;
        }

        if (true == otauUploadDone || "chunck" == uploadType)
        {
            sendDataTroughSocket(ClientSocket, "OK");
        }
        else
        {
            sendDataTroughSocket(ClientSocket, "NOK");
        }
    }

    /**************************************************************************************/
    else if ("otauupgrade" == command)
    {
        /* An Over The Air Update will be performed */

        if (true == otauUploadDone)
        {
            // perform tuyaLib action
            performTuyaAction(actionType::act_StartOtauUpgrade);
        }

        otauUploadDone = false;

        sendDataTroughSocket(ClientSocket, "OK");
    }

    /**************************************************************************************/
    else if ("DIE" == command)
    {
        printLogging("It's getting cold and dark.....", LogType::INTAPP);
        keepTuyaLibProcesRunning = false;
        keepSocketProcesRunning = false;

        if (application::BleLib::State::Disconnected != deviceState)
        {
            // perform tuyaLib action
            performTuyaAction(actionType::act_Disconnect);
        }
    }
    else
    {
        printLogging("Shit hit the fan!!", LogType::INTAPP);
        sendDataTroughSocket(ClientSocket, "UNKNOWN_COMMAND");
    }
}


/**************************************************************************************************************/
// sendDataTroughSocket
/**************************************************************************************************************/
int sendDataTroughSocket(SOCKET* ClientSocket, std::string dataToSend)
{
    int iSendResult = send(*ClientSocket, dataToSend.c_str(), dataToSend.length(), 0);
    if (iSendResult == SOCKET_ERROR) {
        printLogging(("send failed with error: " + std::to_string(WSAGetLastError())), LogType::INTAPP);
        closesocket(*ClientSocket);
        WSACleanup();
        return 1;
    }

    if ( iSendResult < 300)
    {
        printLogging(("Bytes sent: " + std::to_string(iSendResult) + " -> " + dataToSend), LogType::INTAPP);
    }
    else
    {
        printLogging(("Bytes sent: " + std::to_string(iSendResult) + " -> ..... a shitload of data to much to print here!!"), LogType::INTAPP);
    }
}

/**************************************************************************************************************/
// processSocketInput
/**************************************************************************************************************/
int processSocketInput(void)
{
    WSADATA wsaData;
    int iResult;
    bool continuReceving = true;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

    struct addrinfo* result = NULL;
    struct addrinfo hints;

    int iSendResult;
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for connecting to server
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // Accept a client socket
    printLogging("Awaiting connection from client.", LogType::INTAPP);
    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) {
        printf("accept failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }
    else
    {
        printLogging("Connection accepted from client.", LogType::INTAPP);
        waitXseconds(2);
    }

    // No longer need server socket
    closesocket(ListenSocket);

    /**************************************************************************************************/
    // Proces all the incoming data
    /**************************************************************************************************/
    while (true == keepSocketProcesRunning)
    {

        // Receive until the peer shuts down the connection
        do
        {
            iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
        } while (recvbuf[iResult - 1] != '*');

        /* Respond back to the client*/
        if (iResult > 0)
        {
            //sendDataTroughSocket(&ClientSocket, "OK");
        }
        else
        {
            printf("recv failed with error: %d\n", WSAGetLastError());
            closesocket(ClientSocket);
            WSACleanup();
            continuReceving = false;
        }

        // data is reveived now proces it
        std::string recievedData = recvbuf;
        proces_incomming_data(&ClientSocket, recievedData.substr(0, iResult));
    }
    /**************************************************************************************************/

    // shutdown the connection since we're done
    iResult = shutdown(ClientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }

    // cleanup
    closesocket(ClientSocket);
    WSACleanup();

    isSocketProcesFinished = true;
}


/**************************************************************************************************************/
// main
/**************************************************************************************************************/
std::array<std::string, 3>  logtype = { "BLE_Central", "Teststand", "InterfaceApp" };
int main(int argc, const char* argv[])
{
    printLogging("===|| Hello world! 2.0.3 ||===", LogType::INTAPP);

    std::thread processSocketInput_thr(processSocketInput);
    std::thread processTuyaLibt_thr(processTuyaLib);

    do
    {
        if ( false == loggingOutput.empty())
        {
            std::cout.flush();
            std::cout << loggingOutput.front()+"\n";
            isPopping = true;
            loggingOutput.pop();
            isPopping = false;
        }

    } while (true == keepTuyaLibProcesRunning || true == keepSocketProcesRunning || false == loggingOutput.empty());

    while ( true != isTuyaLibProcesFinished || true != isSocketProcesFinished )
    {
        Sleep(10);
    }

    std::cout << logtype[LogType::INTAPP] << "\t: " << "Processing is stopped." << std::endl;
    std::cout << logtype[LogType::INTAPP] << "\t: " << "Goodbye forever, this is really the end.... I mean it!!" << std::endl;

    return 0;
}

/**************************************************************************************************/
// Print the provide text
/**************************************************************************************************/
void printLogging(std::string text, LogType type)
{
    while (true == isPopping)
    {
        Sleep(1);
    }

    //loggingOutput.push( logtype[(int)type] + "\t: " + text );
    isPopping = true;
    std::cout << "[" << currentTime() << "] " << logtype[(int)type] << "\t: " << text << std::endl;
    isPopping = false;
}
