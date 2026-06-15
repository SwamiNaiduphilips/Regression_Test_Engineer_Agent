#ifndef BLE_REFERENCE_BLE_LIB_HPP
#define BLE_REFERENCE_BLE_LIB_HPP

#include "TuyaDatapoint.h"
#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace application
{
    // tag::doc_BleLibBase[]
    class BleLibBase
    {
    public:
        /**
         * 6-byte BLE address
         */
        typedef std::array<uint8_t, 6> Address;

        /**
         * UUID of service, characteristic or descriptor. Can either be a 16-bit value,
         * or a 128-bit value.
         */
        typedef std::vector<uint8_t> Uuid;

        /**
         * State of the Central
         */
        enum class State
        {
            Stopped,      /**< Central is not started.                    */
            Connecting,   /**< Central is connecting to peripheral.       */
            Connected,    /**< Central is connected to peripheral.        */
            Disconnected, /**< Central is disconnected from peripheral.   */
            Scanning      /**< Central is scanning for peripherals.       */
        };

        /**
         * Tuya State (only for peripherals supporting Tuya)
         */
        enum class TuyaState
        {
            Bound,   /**< Tuya binding is established.               */
            NotBound /**< No Tuya binding is established.            */
        };

        /**
         * BLE Address type.
         */
        enum class AddressType : uint8_t
        {
            Public,
            Random
        };

        /**
         * BLE Pairing Mode
         */
        enum class PairingMode : uint8_t
        {
            Pair,
            Bond,
            None
        };

        /**
         * BLE Authentication Status
         */
        enum class AuthenticationStatus : uint8_t
        {
            Success,
            PasskeyEntryFailed,
            AuthenticationRequirementsNotMet,
            PairingNotSupported,
            InsufficientEncryptionKeySize,
            NumericComparisonFailed,
            Timeout,
            Unknown,
            Max
        };

        /**
         * Mask indicating the properties of a characteristic or descriptor.
         */
        enum class Properties : uint8_t
        {
            Broadcast = 0x01,
            Read = 0x02,
            WriteWithoutResponse = 0x04,
            Write = 0x08,
            Notify = 0x10,
            Indicate = 0x20
        };

        /**
         * Result of a read/write operation
         */
        enum class ReadWriteResult : uint8_t
        {
            Success,
            InsufficientAuthentication,
            UnknownError
        };

        /**
         * Structure of a BLE Descriptor.
         */
        struct Descriptor
        {
            Uuid uuid;
            Properties properties;
        };

        /**
         * Structure of a BLE Characteristic.
         * Contains of a UUID, properties and optionaly a list of BLE Descriptors
         */
        struct Characteristic
        {
            Uuid uuid;
            Properties properties;

            std::vector<Descriptor> descriptors;
        };

        /**
         * Structure of a BLE Service.
         * Contains of a UUID and a list of its BLE Characteristics
         */
        struct Service
        {
            Uuid uuid;
            std::vector<Characteristic> characteristics;
        };
    };
    // end::doc_BleLibBase[]

    // tag::doc_BleLibResponse[]
    class BleLibResponse : public BleLibBase
    {
    public:
        /**
         * Event generated when the Central state changes.
         *
         * @param 'state' Central State.
         */
        virtual void StateChanged(State state) = 0;

        /**
         * Event generated when a device is discovered.
         *
         * @param 'deviceAddress' BLE Address of the discovered device.
         */
        virtual void DeviceDiscovered(const Address& deviceAddress) = 0;

        /**
         * Event generated when device discovery is completed.
         */
        virtual void DeviceDiscoveryComplete(void) = 0;

        /**
         * Event generated when authentication is completed.
         */
        virtual void AuthenticationComplete(AuthenticationStatus status) = 0;

        /**
         * Event generated when numeric comparison is required during authenticated pairing.
         * Application needs to display the passkey to user and confirm that the shown key is the same as
         * shown by the peripheral by calling NumericComparisonConfirm. If the confirmation is not provided
         * in time or it is denied then authentication fails and connection terminates.
         *
         * @param 'passkey' Key value that needs to be compared to the value displayed by the peripheral
         */
        virtual void NumericComparisonRequest(uint32_t passkey) = 0;

        /**
         * Event generated when passkey display is required during authenticated pairing.
         * Application needs to provide the passkey to the user which needs to enter it on
         * the peripheral device. If the passkey is not provided in time or an incorrect
         * passkey is provided, authentication fails and connection terminates.
         *
         * @param 'passkey' Key value that needs to be displayed and entered on the peripheral
         */
        virtual void PasskeyDisplayRequest(uint32_t passkey) = 0;

        /**
         * Event generated when passkey entry is required during authenticated pairing.
         * Application needs to provide the passkey shown by the peripheral by calling SetPassKey
         * If the passkey is not provided in time or an incorrect passkey is provided, authentication
         * fails and connection terminates.
         */
        virtual void PasskeyEntryRequest() = 0;

        /**
         * Event generated when an indication is received. This event will only be generated
         * for indications which are explicitly enabled by calling 'EnableIndications'.
         *
         * @param 'uuid' UUID of the characteristic which raised the indication.
         * @param 'data' Data corresponding to the send indication.
         */
        virtual void IndicationReceived(const Uuid& uuid, const std::vector<uint8_t>& data) = 0;

        /**
         * Event generated when a notification is received. This event will only be generated
         * for notifications which are explicitly enabled by calling 'EnableNotifications'.
         *
         * @param 'uuid' UUID of the characteristic which raised the notification.
         * @param 'data' Data corresponding to the send notification.
         */
        virtual void NotificationReceived(const Uuid& uuid, const std::vector<uint8_t>& data) = 0;

        /**
         * Event generated when device discovery is completed.
         *
         * @param 'services' Discovered services (including characteristics and descriptors).
         */
        virtual void ServiceDiscoveryComplete(const std::vector<Service>& services) = 0;

        /**
         * Event generated when a GATT read operation is completed.
         *
         * @param 'result' Result of the read operation.
         * @param 'data' Data the read operation.
         */
        virtual void ReadCompleted(ReadWriteResult result, const std::vector<uint8_t>& data) = 0;

        /**
         * Event generated when a GATT write operation is completed.
         *
         * @param 'result' Result of the write operation.
         * @note This event is also raised as a result of 'EnableNotifications' or 'EnableIndications'
         * which are essence a 'Write' operation.
         */
        virtual void WriteCompleted(ReadWriteResult result) = 0;

        /**
         * Event generated during OTAU using the Condor protocol.
         *
         * @param 'success' Result of the OTAU. Once the result is FALSE the download failed.
         * @param 'actualNrBytes' Current progress in bytes.
         * @param 'totalNrBytes' Total number of bytes which will be uploaded. When the progress is
         * equal to the total number of bytes, the download is finished and can be deployed by
         * using 'StartCondorOtauUpgrade'.
         */
        virtual void CondorOtauProgress(bool success, uint32_t actualNrBytes, uint32_t totalNrBytes) = 0;

        /**
         * Event generated when one or more Tuya DataPoints are received.
         *
         * @param 'data' Raw data containing Tuya DataPoints. DataPoints can be decoded using the
         * Tuya interface.
         */
        virtual void TuyaDataPointsReceived(const std::vector<uint8_t>& data) = 0;

        /**
         * Event generated during OTAU using the Tuya protocol.
         *
         * @param 'success' Result of the OTAU. Once the result is FALSE the download failed.
         * @param 'actualNrBytes' Current progress in bytes.
         * @param 'totalNrBytes' Total number of bytes which will be uploaded. When the progress is
         * equal to the total number of bytes, the download is finished and can be deployed by
         * using 'StartTuyaOtauUpgrade'.
         */
        virtual void TuyaOtauProgress(bool success, uint32_t actualNrBytes, uint32_t totalNrBytes) = 0;

        /**
         * Event generated when the Tuya State Changed. In case a peripheral is considered a Tuya
         * device, this event will be generated DURING the service discovery. Binding to a Tuya
         * peripheral is only allowed AFTER the service discovery.
         *
         * @param 'state' Tuya State.
         */
        virtual void TuyaStateChanged(TuyaState state) = 0;

        /**
         * Event generated when the version information of the BLE Central is requested.
         *
         * @param 'version' Version (GIT hash) of the BLE Central.
         */
        virtual void Version(const std::string& version) = 0;

        /**
         * Event generated when the protocol version information of the BLE Central is requested.
         *
         * @param 'version' Protocol version of the BLE Central.
         */
        virtual void ProtocolVersion(uint8_t version) = 0;
    };
    // end::doc_BleLibResponse[]

    // tag::doc_BleLib[]
    class BleLib : public BleLibBase
    {
    public:
        /**
         * Removes the bonds from the BLE Central.
         */
        virtual void RemoveBonds() = 0;

        /**
         * Starts BLE Device (peripheral) Discovery.
         *
         * @note: Results in the following events:
         *   StateChanged            --> State changes to Scanning
         *   DeviceDiscovered        --> For each discovered event
         *   DeviceDiscoveryComplete --> In case the device discovery has not been stopped within timeout (30 seconds).
         */
        virtual void StartDeviceDiscovery() = 0;

        /**
         * Stops BLE Device (peripheral) Discovery.
         *
         * @note: Results in the following events:
         *   StateChanged            --> State changes to 'Disconnected'
         *   DeviceDiscoveryComplete --> When device discovery is stopped.
         */
        virtual void StopDeviceDiscovery() = 0;

        /**
         * Connects to the specified BLE periperal
         *
         * @param [in] 'address'. BLE address of the peripheral to connect.
         * @param [in] 'addressType'. Type of the address (public/random).
         * @param [in] 'pairingMode'. Pairing mode (pair/bond/none). In case the pairing mode is set
         *  to 'None'; manual pairing/bonding can be established after the state becomes 'Connected'
         * @note: Results in the following events:
         *   StateChanged           --> State changes to 'Connecting' and eventually 'Connected'
         *   AuthenticationComplete --> Status of authentication; only in case pairing/bonding is used.
         */
        virtual void Connect(const Address& address, AddressType addressType, PairingMode pairingMode) = 0;

        /**
         * Disconnects from the BLE periperal
         *
         * @note: Results in the following event:
         *   StateChanged --> State changes to 'Disconnected'.
         */
        virtual void Disconnect() = 0;

        /**
         * Pairs with the BLE periperal
         *
         * @note: Results in the following event:
         *   AuthenticationComplete --> Status of the authentication.
         */
        virtual void Pair() = 0;

        /**
         * Bonds with the BLE periperal
         *
         * @note: Results in the following event:
         *   AuthenticationComplete --> Status of the authentication.
         */
        virtual void Bond() = 0;

        /**
         * Sets the passkey during authenticated pairing
         *
         * @param [in]: Key value
         * @note: See PasskeyEntryRequest
         */
        virtual void SetPasskey(uint32_t passkey) = 0;

        /**
         * Confirms or denies the passkey equivalence during authenticated pairing
         *
         * @param [in]: Confirmation
         * @note: See NumericComparisonRequest
         */
        virtual void NumericComparisonConfirm(bool confirm) = 0;

        /**
         * Enables or disables indications for the specified UUID
         *
         * @param [in] 'uuid'. UUID of the characteristic.
         * @param [in] 'enable'. TRUE to enable; FALSE otherwise.
         * @note: Results in the following events:
         *   WriteCompleted     --> When the indication is enabled/disabled.
         *   IndicationReceived --> When an indication is received.
         */
        virtual void EnableIndications(const Uuid& uuid, bool enable) = 0;

        /**
         * Enables or disables notifications for the specified UUID
         *
         * @param [in] 'uuid'. UUID of the characteristic.
         * @param [in] 'enable'. TRUE to enable; FALSE otherwise.
         * @note: Results in the following events:
         *   WriteCompleted       --> When the notification is enabled/disabled.
         *   NotificationReceived --> When an notification is received.
         */
        virtual void EnableNotifications(const Uuid& uuid, bool enable) = 0;

        /**
         * Reads the specified UUID.
         * There can be only one read operation in progress.
         *
         * @param [in] 'uuid'. UUID of the characteristic.
         * @note: Results in the following event:
         *   ReadCompleted --> When the read operation is completed.
         */
        virtual void Read(const Uuid& uuid) = 0;

        /**
         * Executes a Service Discovery.

         * @note: Results in the following event:
         *   ServiceDiscoveryComplete --> When the Service Discovery is completed.
         */
        virtual void ServiceDiscovery() = 0;

        /**
         * Writes the specified UUID.
         * There can be only one write operation in progress.
         *
         * @param [in] 'uuid'. UUID of the characteristic.
         * @param [in] 'data'. Data which needs to be written.
         * @note: Results in the following event:
         *   WriteCompleted --> When the write operation is completed.
         */
        virtual void Write(const Uuid& uuid, const std::vector<uint8_t>& data) = 0;

        /**
         * Writes the specified UUID (requires characteristic which supports
         * 'Write Without Response').
         *
         * @param [in] 'uuid'. UUID of the characteristic.
         * @param [in] 'data'. Data which needs to be written.
         */
        virtual void WriteWithoutResponse(const Uuid& uuid, const std::vector<uint8_t>& data) = 0;

        /**
         * Returns whether or not the connected peripheral supports the Condor protocol.
         * In order to return a valid status, this function requires the Service Discovery
         * to be completed.
         *
         * @return TRUE in case supported, FALSE otherwise.
         */
        virtual bool HasCondor() = 0;

        /**
         * Start an Over The Air Update (OTAU) using the Condor protocol.
         *
         * @param [in] 'data'. Data - Content of the upgrade pack
         * @param [in] 'restart'. Indicates whether or not the upload should be restarted
         *  from the beginning, or continued where it stopped previous time (in case of a
         *  disconnect).
         * @note: Results in the following event:
         *    CondorOtauProgress --> Multiple events when progress is updated or an error
         *                           has occured.
         */
        virtual void StartCondorOtauUpload(std::vector<uint8_t>& data, bool restart) = 0;

        /**
         * Stops (aborts) an OTAU using the Condor protocol.
         */
        virtual void StopCondorOtauUpload() = 0;

        /**
         * Deploys an upgrade pack using the Condor protocol.
         */
        virtual void StartCondorOtauUpgrade() = 0;

        /**
         * Returns whether or not the connected peripheral supports the Tuya protocol.
         * In order to return a valid status, this function requires the Service Discovery
         * to be completed.
         *
         * @return TRUE in case supported, FALSE otherwise.
         */
        virtual bool HasTuya() = 0;

        /**
         * Binds to a Tuya device (note that this is different than BLE binding).
         *
         * @note: Results in the following event:
         *    TuyaStateChanged --> Changes to 'Bound' when binding operation is completed.
         */
        virtual void Bind() = 0;

        /**
         * Unbinds from a Tuya device.
         *
         * @note: Results in the following event:
         *    TuyaStateChanged --> Changes to 'NotBound' when unbinding operation is completed.
         */
        virtual void Unbind() = 0;

        /**
         * Queries Tuya DataPoints
         *
         * @param [in] 'dataPointIndices'. Indices of the DataPoints to be queried.
         * @note: Results in the following event:
         *    TuyaDataPointsReceived.
         */
        virtual void QueryDataPoints(std::vector<uint8_t>& dataPointIndices) = 0;

        /**
         * Sends one or more Tuya DataPoints
         *
         * @param [in] 'dataPoints'. DataPoins to be send.
         */
        virtual void SendDataPoints(std::vector<TUYA_DataPoint>& dataPoints) = 0;

        /**
         * Start an Over The Air Update (OTAU) using the Tuya protocol.
         *
         * @param [in] 'data'. Data - Content of the upgrade pack.
         * @note: Results in the following event:
         *    TuyaOtauProgress --> Multiple events when progress is updated or an error
         *                           has occured.
         */
        virtual void StartTuyaOtauUpload(std::vector<uint8_t>& data) = 0;

        /**
         * Stops (aborts) an OTAU using the Tuya protocol.
         */
        virtual void StopTuyaOtauUpload() = 0;

        /**
         * Deploys an upgrade pack using the Tuya protocol.
         */
        virtual void StartTuyaOtauUpgrade() = 0;

        /**
         * Request the BLE Central Protocol Version information.
         *
         * @note: Results in the following event:
         *    ProtocolVersion.
         */
        virtual void GetProtocolVersion() = 0;

        /**
         * Request the BLE Central Version information.
         *
         * @note: Results in the following event:
         *    Version.
         */
        virtual void GetVersion() = 0;

        /**
         * Main loop of the BleLib. Must be called by the application periodically
         * to trigger execution of the library processes.
         */
        virtual void MainloopRun() = 0;

        /**
         * Restarts the BLE stack of the BLE Central.
         * @note: Results in the following events:
         *    StateChanged --> State changes to 'Stopped' and eventually 'Disconnected'.
         */
        virtual void Restart() = 0;
    };
    // end::doc_BleLib[]

    // tag::doc_BleLibFactory[]
    class BleLibFactory
    {
    public:
        /**
         * Creates a (singleton) instance of the BleLib.
         *
         * @param [in] 'comPort'. COM port of the BLE Central hardware.
         * @param [in] 'response'. Response instance; must be implemented by the application.
         * @param [in] 'tuyaPid'. Tuya Product ID. In case Tuya support is not required, this field can be left empty.
         * @param [in] 'tuyaDid'. Tuya Device ID. In case Tuya support is not required, this field can be left empty.
         * @param [in] 'tuyaAuthKey'. Tuya Authentication key. In case Tuya support is not required, this field can be left empty.
         * @param [in] 'secureConnections'. TRUE in case LE Secure Connections is supported; FALSE in case only LE Legacy Pairing is supported.
         * @param [in] 'log'. TRUE in case console logging must be enabled; FALSE otherwise.
         */
        static void Create(std::string comPort, BleLibResponse& response, std::string tuyaPid, std::string tuyaDid, std::string tuyaAuthKey, bool secureConnections, bool log = false);

        /**
         * Returns the created instance which is created by using 'Create'
         */
        static BleLib* Get();

    private:
        static BleLib* instance;
    };
    // end::doc_BleLibFactory[]
}

#endif
