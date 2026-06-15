#ifndef C_INSTANTIATIONS_TUYA_DATAPOINT_H
#define C_INSTANTIATIONS_TUYA_DATAPOINT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
//tag::doc[]

typedef enum
{
    TUYA_DT_RAW = 0,
    TUYA_DT_BOOL = 1,
    TUYA_DT_VALUE = 2,
    TUYA_DT_INT = TUYA_DT_VALUE,
    TUYA_DT_STRING = 3,
    TUYA_DT_ENUM = 4,
    TUYA_DT_BITMASK = 5,
    TUYA_DT_CHAR = 7,            /* Currently not supported */
    TUYA_DT_UCHAR = 8,           /* Currently not supported */
    TUYA_DT_SHORT = 9,           /* Currently not supported */
    TUYA_DT_USHORT = 10,         /* Currently not supported */
    TUYA_DT_LMT = TUYA_DT_USHORT
} TUYA_DataPointType;

#define TUYA_MAX_DATAPOINT_DATA_LENGTH 507
/**
 * Sends a raw DataPoint message.
 *
 * Data format      : Dp_id Dp_type Dp_len Dp_data ...  Dp_id Dp_type Dp_len Dp_data
 * Length in Bytes  :   1     1       2    Dp_len         1       1      2    Dp_len
 *
 * @param [in] 'data'. Pointer to the data.
 * @param [in] 'length'. Length of the data.
 * @note Maximum size is TUYA_MAX_DATAPOINT_DATA_LENGTH.
 */
bool TUYA_WriteDataPointsRaw(uint8_t* data, uint16_t length);


typedef struct
{
    uint8_t id;
    TUYA_DataPointType type;
    uint16_t length;

    union
    {
        uint8_t value8;
        uint16_t value16;
        uint32_t value32;
        const uint8_t* raw;
    };
} TUYA_DataPoint;

TUYA_DataPoint TUYA_DpBitmask(uint8_t id, uint32_t value, uint8_t length);
TUYA_DataPoint TUYA_DpBool(uint8_t id, bool value);
TUYA_DataPoint TUYA_DpEnum(uint8_t id, uint8_t value);
TUYA_DataPoint TUYA_DpInt(uint8_t id, int32_t value);
TUYA_DataPoint TUYA_DpStr(uint8_t id, const char* value);

TUYA_DataPoint TUYA_DpChar(uint8_t id, int8_t value);         /* Currently not supported */
TUYA_DataPoint TUYA_DpUChar(uint8_t id, uint8_t value);       /* Currently not supported */
TUYA_DataPoint TUYA_DpShort(uint8_t id, int16_t value);       /* Currently not supported */
TUYA_DataPoint TUYA_DpUShort(uint8_t id, uint16_t value);     /* Currently not supported */
TUYA_DataPoint TUYA_DpRaw(uint8_t id, const uint8_t* value, uint16_t length);


/**
 * Initializes a DataPoint buffer which is required when multiple DataPoints must
 * be sent in one single message. The buffer must remain allocated till the DataPoints
 * are sent by 'TUYA_WriteBufferedDataPoints'
 *
 * @param [in] 'buffer'. Pointer to the buffer storage.
 * @param [in] 'length'. Length of the buffer.
 * @note Maximum size is TUYA_MAX_DATAPOINT_DATA_LENGTH
 */
void TUYA_InitializeDataPointBuffer(uint8_t* buffer, uint16_t length);


/**
 * Adds a DataPoint to the buffer initialized by 'TUYA_InitializeDataPointBuffer',
 *
 * @param [in] 'dataPoint'. Pointer to DataPoint which will be added to the buffer. Cannot be NULL.
 * @return TRUE in case the DataPoint is successfully sent, FALSE otherwise.
 */
bool TUYA_BufferDataPoint(TUYA_DataPoint* dataPoint);


/**
* Returns whether or not there are buffered DataPoints
*
* @return TRUE in case there are buffered DataPoints, FALSE otherwise.
*/
bool TUYA_HasBufferedDataPoints();


/**
 * Sends all buffered DataPoints. In case the same buffer must be reused, it must
 * be reset by calling 'TUYA_InitializeDataPointBuffer'.
 *
 * @return TRUE in case the DataPoints are successfully sent, FALSE in case no
 *  buffer is allocated, or no DataPoints in buffer.
 */
bool TUYA_WriteBufferedDataPoints();


/**
 * Sends a single DataPoints.
 * @param [in] 'dataPoint'. Pointer to DataPoint which will be added to the buffer. Cannot be NULL.
 * @param [in] 'buffer'. Pointer to the buffer; which is used to construct the serialized DataPoint structure.
 * @param [in] 'length'. Length of the buffer, must at least be the size of the serialized DataPoint.
 * @return TRUE in case the DataPoint is successfully sent, FALSE otherwise.
 * @note Maximum size is TUYA_MAX_DATAPOINT_DATA_LENGTH
 */
bool TUYA_WriteDataPoint(TUYA_DataPoint* dataPoint, uint8_t* buffer, uint16_t length);


/**
 * Helper to iterate over a received DataPoint structure
 *
 * @param [in] 'buffer'. Pointer to the buffer.
 * @param [in] 'length'. Length of the buffer.
 * @return Pointer to first DataPoint; or NULL in case the buffer is empty or does not contain a valid DataPoint.
 */
TUYA_DataPoint* TUYA_GetFirstDataPoint(uint8_t* buffer, uint16_t length);


/**
 * Helper to iterate over a received DataPoint structure. Must be initialized by 'TUYA_GetFirstDataPoint'
 *
 * @return Pointer to next DataPoint; or NULL in case no more DataPoints are available.
 */
TUYA_DataPoint* TUYA_GetNextDataPoint();


//end::doc[]
#ifdef __cplusplus
}
#endif

#endif
