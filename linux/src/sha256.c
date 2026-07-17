#include "ue4ss/sha256.h"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#define ROTRIGHT(value, bits) (((value) >> (bits)) | ((value) << (32u - (bits))))
#define CHOICE(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJORITY(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTRIGHT((x), 2u) ^ ROTRIGHT((x), 13u) ^ ROTRIGHT((x), 22u))
#define EP1(x) (ROTRIGHT((x), 6u) ^ ROTRIGHT((x), 11u) ^ ROTRIGHT((x), 25u))
#define SIG0(x) (ROTRIGHT((x), 7u) ^ ROTRIGHT((x), 18u) ^ ((x) >> 3u))
#define SIG1(x) (ROTRIGHT((x), 17u) ^ ROTRIGHT((x), 19u) ^ ((x) >> 10u))

static const uint32_t k_round_constants[64] = {
        0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u, 0xd807aa98u, 0x12835b01u, 0x243185beu,
        0x550c7dc3u, 0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u, 0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu, 0x4a7484aau,
        0x5cb0a9dcu, 0x76f988dau, 0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u, 0x27b70a85u,
        0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u, 0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u,
        0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u, 0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu,
        0x682e6ff3u, 0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u, 0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u,
};

static void transform(ue4ss_sha256_context* context, const uint8_t data[64])
{
    uint32_t words[64];
    for (uint32_t index = 0; index < 16u; ++index)
    {
        const uint32_t offset = index * 4u;
        words[index] = ((uint32_t)data[offset] << 24u) | ((uint32_t)data[offset + 1u] << 16u) | ((uint32_t)data[offset + 2u] << 8u) | (uint32_t)data[offset + 3u];
    }
    for (uint32_t index = 16u; index < 64u; ++index)
    {
        words[index] = SIG1(words[index - 2u]) + words[index - 7u] + SIG0(words[index - 15u]) + words[index - 16u];
    }

    uint32_t a = context->state[0];
    uint32_t b = context->state[1];
    uint32_t c = context->state[2];
    uint32_t d = context->state[3];
    uint32_t e = context->state[4];
    uint32_t f = context->state[5];
    uint32_t g = context->state[6];
    uint32_t h = context->state[7];

    for (uint32_t index = 0; index < 64u; ++index)
    {
        const uint32_t first = h + EP1(e) + CHOICE(e, f, g) + k_round_constants[index] + words[index];
        const uint32_t second = EP0(a) + MAJORITY(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + first;
        d = c;
        c = b;
        b = a;
        a = first + second;
    }

    context->state[0] += a;
    context->state[1] += b;
    context->state[2] += c;
    context->state[3] += d;
    context->state[4] += e;
    context->state[5] += f;
    context->state[6] += g;
    context->state[7] += h;
}

void ue4ss_sha256_init(ue4ss_sha256_context* context)
{
    context->data_length = 0u;
    context->bit_length = 0u;
    context->state[0] = 0x6a09e667u;
    context->state[1] = 0xbb67ae85u;
    context->state[2] = 0x3c6ef372u;
    context->state[3] = 0xa54ff53au;
    context->state[4] = 0x510e527fu;
    context->state[5] = 0x9b05688cu;
    context->state[6] = 0x1f83d9abu;
    context->state[7] = 0x5be0cd19u;
}

void ue4ss_sha256_update(ue4ss_sha256_context* context, const uint8_t* data, size_t length)
{
    for (size_t index = 0; index < length; ++index)
    {
        context->data[context->data_length++] = data[index];
        if (context->data_length == 64u)
        {
            transform(context, context->data);
            context->bit_length += 512u;
            context->data_length = 0u;
        }
    }
}

void ue4ss_sha256_final(ue4ss_sha256_context* context, uint8_t hash[32])
{
    uint32_t index = context->data_length;
    context->data[index++] = 0x80u;

    if (index > 56u)
    {
        while (index < 64u)
        {
            context->data[index++] = 0u;
        }
        transform(context, context->data);
        index = 0u;
    }
    while (index < 56u)
    {
        context->data[index++] = 0u;
    }

    context->bit_length += (uint64_t)context->data_length * 8u;
    for (uint32_t byte = 0; byte < 8u; ++byte)
    {
        context->data[63u - byte] = (uint8_t)(context->bit_length >> (byte * 8u));
    }
    transform(context, context->data);

    for (uint32_t word = 0; word < 8u; ++word)
    {
        hash[word * 4u] = (uint8_t)(context->state[word] >> 24u);
        hash[word * 4u + 1u] = (uint8_t)(context->state[word] >> 16u);
        hash[word * 4u + 2u] = (uint8_t)(context->state[word] >> 8u);
        hash[word * 4u + 3u] = (uint8_t)context->state[word];
    }
}

int ue4ss_sha256_file(const char* path, char output_hex[65])
{
    static const char k_hex[] = "0123456789abcdef";
    const int descriptor = open(path, O_RDONLY | O_CLOEXEC);
    if (descriptor < 0)
    {
        return errno != 0 ? errno : EIO;
    }

    ue4ss_sha256_context context;
    ue4ss_sha256_init(&context);
    uint8_t buffer[32768];
    for (;;)
    {
        const ssize_t bytes_read = read(descriptor, buffer, sizeof(buffer));
        if (bytes_read > 0)
        {
            ue4ss_sha256_update(&context, buffer, (size_t)bytes_read);
            continue;
        }
        if (bytes_read == 0)
        {
            break;
        }
        if (errno == EINTR)
        {
            continue;
        }
        const int read_error = errno != 0 ? errno : EIO;
        close(descriptor);
        return read_error;
    }
    if (close(descriptor) != 0)
    {
        return errno != 0 ? errno : EIO;
    }

    uint8_t digest[32];
    ue4ss_sha256_final(&context, digest);
    for (size_t index = 0; index < sizeof(digest); ++index)
    {
        output_hex[index * 2u] = k_hex[digest[index] >> 4u];
        output_hex[index * 2u + 1u] = k_hex[digest[index] & 0x0fu];
    }
    output_hex[64] = '\0';
    return 0;
}
