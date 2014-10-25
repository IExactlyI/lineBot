#include "UserProtocol.h"
#include "string.h"

#include <iostream>

using namespace std;

static const uint8_t UserProtocol_header[] = {0x7e, 0x81};
const uint8_t* UserProtocol::header = UserProtocol_header;

/**
 * @brief           This method is used for encode into UserProtocol format.
 *                  Output from this method is able to decode by method "processIncome" directly.
 * @param data_in   : It is a input pointer of data you would like to encode.
 * @param data_out  : It is a output pointer that this function will write the output to.
 *                    It's length is equal to length of data_in + header's
 *                    that can get by static method getHeaderLength().
 * @param length    : It is a length of data_in
 * @return          : return lenth of output
 */
uint16_t UserProtocol::createDataPack(const uint8_t    *data_in, \
                                  uint8_t          *data_out, \
                                  const uint16_t   length)
{
    uint8_t checksum = 0;

    memcpy(data_out, header, 2);
    memcpy(data_out + 2, (uint8_t*)(&length), sizeof(length));
    memcpy(data_out + 2 + sizeof(length), data_in, length);

    for (uint16_t i = 0 ; i < length ; i++) {
        checksum += data_in[i];
    }

    data_out[header_length + sizeof(length) + length] = checksum;

    return header_length + sizeof(length) + length + sizeof(checksum);
}

/**
 * @brief           This constructor will locate heap memory following input length
 * @param length    : To define data_tank size.
 */
UserProtocol::UserProtocol(const uint16_t length)
{
    //data_tank = (uint8_t*)malloc(length + protocol_length);
    //tank_length = length + protocol_length;
    tank_length = 50;
}

/**
 * @brief           This method is used for encode into UserProtocol format.
 *                  Output from this method is able to decode by method "processIncome" directly.
 * @param data_in   : It is a input pointer of data you would like to encode.
 * @param data_out  : It is a output pointer that this function will write the output to.
 *                    It's length is equal to length of data_in + header's
 *                    that can get by static method incomeLength().
 * @param length    : It is a length of data_in
 * @return          : return lenth of output
 */
int8_t UserProtocol::processIncome(const uint8_t raw_byte)
{
    static uint8_t getlength_state;
    static uint16_t retrieve_index;
    static uint16_t payload_length;
    static uint8_t current_state;
    static uint8_t last_byte;

    switch (current_state) {
    case STATE_IDLE :
        if ((last_byte == header[0]) && (raw_byte == header[1])) {
            current_state = STATE_GETLENGTH;
            getlength_state = 0;
            payload_length = 0;
        }
        break;
    case STATE_GETLENGTH :
        if (getlength_state == 0) {
            payload_length |= raw_byte;
            getlength_state = 1;
        } else if (getlength_state == 1) {
            payload_length |= ((uint16_t)raw_byte << 8);
            if (payload_length <= tank_length) {
              current_state = STATE_RETRIEVE;
              getlength_state = 0;
              retrieve_index = 0;
            } else {
              current_state = STATE_IDLE;
              return -2;
            }
        }
        break;
    case STATE_RETRIEVE :
        data_tank[retrieve_index] = raw_byte;
        retrieve_index++;
        if (retrieve_index >= payload_length) {
            current_state = STATE_CHECKSUM;
            retrieve_index = 0;
        }
        break;
    case STATE_CHECKSUM :
        uint8_t checksum = 0;
        for (uint16_t i = 0 ; i < payload_length ; i++) {
            checksum += data_tank[i];
        }
        
        current_state = STATE_IDLE;

        if (checksum == raw_byte) {
            p_length = payload_length;
            return 1;
        } else {
            return -1;
        }
        
        checksum = 0;
        break;

    }
    
    last_byte = raw_byte;

    return 0;
}

uint16_t UserProtocol::incomeLength()
{
    return p_length;
}

uint8_t* UserProtocol::incomePtr()
{
    return data_tank;
}
