#include "BleLib.hpp"

#include <iomanip>
#include <iostream>
#include <stdio.h>

static char* comPort = "COM66";
static std::array<uint8_t, 6> address = { 0x33, 0x33, 0x33, 0xe6, 0xe3, 0x65 };

static application::BleLib* bleLib = nullptr;

namespace
{
    void PrintHex(const char* msg, const std::vector<uint8_t>& data)
    {
        std::cout << msg << ": ";
        for (uint8_t i = 0; i < data.size(); i++)
            std::cout << std::hex << std::setfill('0') << std::setw(2) << static_cast<uint32_t>(data[i]);
        std::cout << std::dec << std::endl;
    }
}
class BleLibResponseImpl 
    : public application::BleLibResponse
{
public:
    std::array<const char*, 5> states = { "Stopped", "connecting", "Connected", "Disconnected", "Scanning" };
    virtual void StateChanged(State state) override
    {
        std::cout << "State changed: " << std::dec << states[(int)state] << std::endl;

        if (state == State::Connected)
            bleLib->ServiceDiscovery();

        static bool getVersionInfo = true;
        if (state == State::Disconnected)
        {
            if (getVersionInfo)
            {
                bleLib->GetVersion();
                getVersionInfo = false;
            }
            else if (deviceFound)
                bleLib->Connect(address, application::BleLib::AddressType::Public, application::BleLib::PairingMode::Pair);
        }
    }

    virtual void Version(const std::string& version) override
    {
        std::cout << "Version: " << version << std::endl;
        bleLib->GetProtocolVersion();
    }

    virtual void ProtocolVersion(uint8_t version)
    {
        std::cout << "Protocol Version: " << static_cast<uint32_t>(version) << std::endl;
        bleLib->StartDeviceDiscovery();
    };

    bool deviceFound = false;

    virtual void DeviceDiscovered(const std::array<uint8_t, 6>& discoveredAddress) override
    {
        if (deviceFound)
            return;

        std::cout << "DeviceDiscovered: ";

        for (int i = 0; i<sizeof(discoveredAddress); ++i)
            std::cout << std::hex << (uint16_t)discoveredAddress[i];

        std::cout << std::endl;

        if (address == discoveredAddress)
        {
            std::cout << "Found device!" << std::endl;
            deviceFound = true;

            bleLib->StopDeviceDiscovery();
        }
    }

    virtual void DeviceDiscoveryComplete(void) override
    {
        std::cout << "Device Discovery Complete" << std::endl;
    }


    std::array<const char*, 2>  tuyaStates = { "Bound", "NotBound" };
    virtual void TuyaStateChanged(TuyaState state) override
    {
        std::cout << "Tuya State changed: " << std::dec << tuyaStates[(int)state] << std::endl;

        if (state == TuyaState::Bound)
        {
            std::vector<uint8_t> indices;
            bleLib->QueryDataPoints(indices);
        }
    }

    virtual void TuyaDataPointsReceived(const std::vector<uint8_t>& data) override
    {
        TUYA_DataPoint* dp = TUYA_GetFirstDataPoint(const_cast<uint8_t*>(&data[0]), static_cast<uint16_t>(data.size()));

        while (dp != NULL)
        {
            std::cout << "Received datapoint with ID " << static_cast<int32_t>(dp->id) << std::endl;
         
            dp = TUYA_GetNextDataPoint();
        }

        static std::vector<uint8_t> otadata(1000, 1);
        if (bleLib->HasCondor())
            bleLib->StartCondorOtauUpload(otadata);
        else bleLib->StartTuyaOtauUpload(otadata);
    }

    virtual void ServiceDiscoveryComplete(const std::vector<Service>& services)
    {
        for (auto const& service : services)
        {
            PrintHex("Service UUID: ", service.uuid);
            for (auto const& characteristic : service.characteristics)
            {
                PrintHex("  Characteristic UUID: ", characteristic.uuid);
                for (auto const& descriptor : characteristic.descriptors)
                    PrintHex("    Descriptor UUID: ", descriptor.uuid);
            }
        }
        std::cout << "Has Tuya   : " << bleLib->HasTuya() << std::endl;
        std::cout << "Has Condor : " << bleLib->HasCondor() << std::endl;

        bleLib->Read(Uuid{ 0x2a, 0x00 });
    }

    virtual void ReadCompleted(ReadWriteResult result, const std::vector<uint8_t>& data)
    {   
        PrintHex("Read result: ", data);
        bleLib->Write(Uuid{ 0x2a, 0x00 }, std::vector<uint8_t>{ 0 });   //Writing to this will result in an error response.
    }

    virtual void IndicationReceived(const Uuid& uuid, const std::vector<uint8_t>& data)
    {
        PrintHex("Indication received on UUID:", uuid);
        PrintHex("  data: ", data);
    }

    virtual void NotificationReceived(const Uuid& uuid, const std::vector<uint8_t>& data)
    {
        PrintHex("Notification received on UUID:", uuid);
        PrintHex("  data: ", data);
    }

    virtual void WriteCompleted(ReadWriteResult result)
    {
        std::cout << "Write Result : " << static_cast<uint32_t>(result) << std::endl;

        if (result != ReadWriteResult::Success)
        {
            // This is the result of writing to 0x2a (which is not allowed).
            bleLib->EnableIndications(Uuid{ 0x2a, 0x05 }, true);
        }
        else
        {
            std::cout << "Start Tuya Bind" << std::endl;
            bleLib->Bind();
        }
    }

    virtual void CondorOtauProgress(bool success, uint32_t progress)
    {
        std::cout << "Condor OTAU result: " << success << "; upload progress: " << progress << std::endl;

        if (progress == 1000)
            bleLib->StartCondorOtauUpgrade();
    }

    virtual void TuyaOtauProgress(bool success, uint32_t progress)
    {
        std::cout << "Tuya OTAU result: " << success << "; upload progress: " << progress << std::endl;

        if (progress == 1000)
            bleLib->StartTuyaOtauUpgrade();
    }

};

int main(int argc, const char* argv[])
{
    printf("==Ble Library Console Test Application==");

    BleLibResponseImpl response;
    application::BleLibFactory::Create(comPort, response, "mbf1fqc0", "tuyaf2aba80dc6f8", "grTAqxgAwLkCAGSLTYXfXnEOnyBsRHgc", false);
    bleLib = application::BleLibFactory::Get();

    while (true)
        bleLib->MainloopRun();

    return 0;
}
