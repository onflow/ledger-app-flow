#include "picosha2.h"

#define CX_SHA256_SIZE 32
extern "C" void sha256(const uint8_t *message, uint16_t messageLen, uint8_t message_digest[CX_SHA256_SIZE]) {
    picosha2::hash256(message, message+messageLen, message_digest, message_digest+CX_SHA256_SIZE);
}
