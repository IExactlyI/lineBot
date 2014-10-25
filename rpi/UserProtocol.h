#ifndef _USER_PROTOCOL_ARDU_H_
#define _USER_PROTOCOL_ARDU_H_ 0

#include <stdint.h>

class UserProtocol {

public:
    static uint16_t createDataPack(const uint8_t    *data_in, \
                               uint8_t          *data_out, \
                               const uint16_t   length);
    static uint16_t getHeaderLength();

public:
    UserProtocol(const uint16_t length);
    int8_t processIncome(uint8_t raw_byte);
    uint16_t incomeLength();
    uint8_t* incomePtr();

private:
    static const uint8_t STATE_IDLE = 0;
    static const uint8_t STATE_GETLENGTH = 1;
    static const uint8_t STATE_RETRIEVE = 2;
    static const uint8_t STATE_CHECKSUM = 3;

    //header = {0x7e, 0x81} define in .cpp
    static const uint8_t *header;
    static const uint8_t header_length = 2;

    //5 bytes is header(2bytes) + length(2bytes) + checksum(1byte)
    static const uint8_t protocol_length = 5;

private:
    uint8_t             data_tank[50];
    uint16_t            tank_length;
    uint16_t            p_length;

};

#endif
