#ifndef UE4SS_SHA256_H
#define UE4SS_SHA256_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct ue4ss_sha256_context
    {
        uint8_t data[64];
        uint32_t data_length;
        uint64_t bit_length;
        uint32_t state[8];
    } ue4ss_sha256_context;

    void ue4ss_sha256_init(ue4ss_sha256_context* context);
    void ue4ss_sha256_update(ue4ss_sha256_context* context, const uint8_t* data, size_t length);
    void ue4ss_sha256_final(ue4ss_sha256_context* context, uint8_t hash[32]);
    int ue4ss_sha256_file(const char* path, char output_hex[65]);

#ifdef __cplusplus
}
#endif

#endif
