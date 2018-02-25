#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "argon2.h"
#include "cores.h"

void initial_kat(const uint8_t *blockhash, const argon2_context *context,
                 argon2_type type)
{
    unsigned i;

    if (blockhash != NULL && context != NULL) {
        printf("=======================================");

        switch (type) {
        case Argon2_d:
            printf("Argon2d\n");
            break;

        case Argon2_i:
            printf("Argon2i\n");
            break;

        default:
            break;
        }

        printf("Memory: %u KiB, Iterations: %u, Parallelism: %u lanes, Tag "
               "length: %u bytes\n",
               context->m_cost, context->t_cost, context->lanes,
               context->outlen);

        printf("Password[%u]: ", context->pwdlen);

        if (context->flags & ARGON2_FLAG_CLEAR_PASSWORD) {
            printf("CLEARED\n");
        } else {
            for (i = 0; i < context->pwdlen; ++i) {
                printf("%2.2x ", ((unsigned char *)context->pwd)[i]);
            }

            printf("\n");
        }

        printf("Salt[%u]: ", context->saltlen);

        for (i = 0; i < context->saltlen; ++i) {
            printf("%2.2x ", ((unsigned char *)context->salt)[i]);
        }

        printf("\n");

        printf("Secret[%u]: ", context->secretlen);

        if (context->flags & ARGON2_FLAG_CLEAR_SECRET) {
            printf("CLEARED\n");
        } else {
            for (i = 0; i < context->secretlen; ++i) {
                printf("%2.2x ", ((unsigned char *)context->secret)[i]);
            }

            printf("\n");
        }

        printf("Associated data[%u]: ", context->adlen);

        for (i = 0; i < context->adlen; ++i) {
            printf("%2.2x ", ((unsigned char *)context->ad)[i]);
        }

        printf("\n");

        printf("Pre-hashing digest: ");

        for (i = 0; i < ARGON2_PREHASH_DIGEST_LENGTH; ++i) {
            printf("%2.2x ", ((unsigned char *)blockhash)[i]);
        }

        printf("\n");
    }
}

void print_tag(const void *out, uint32_t outlen)
{
    unsigned i;
    if (out != NULL) {
        printf("Tag: ");

        for (i = 0; i < outlen; ++i) {
            printf("%2.2x ", ((uint8_t *)out)[i]);
        }

        printf("\n");
    }
}

void internal_kat(const argon2_instance_t *instance, uint32_t pass)
{
    if (instance != NULL) {
        uint32_t i, j;
        printf("\n After pass %u:\n", pass);

        for (i = 0; i < instance->memory_blocks; ++i) {
            uint32_t how_many_words =
                (instance->memory_blocks > ARGON2_WORDS_IN_BLOCK)
                    ? 1
                    : ARGON2_WORDS_IN_BLOCK;

            for (j = 0; j < how_many_words; ++j)
                printf("Block %.4u [%3u]: %016" PRIx64 "\n", i, j,
                       instance->memory[i].v[j]);
        }
    }
}

static void fatal(const char *error) {
    fprintf(stderr, "Error: %s\n", error);
    exit(1);
}

static void generate_testvectors(const char *type)
{
#define TEST_OUTLEN 32
#define TEST_PWDLEN 32
#define TEST_SALTLEN 16
#define TEST_SECRETLEN 8
#define TEST_ADLEN 12
    argon2_context context;

    unsigned char out[TEST_OUTLEN];
    unsigned char pwd[TEST_PWDLEN];
    unsigned char salt[TEST_SALTLEN];
    unsigned char secret[TEST_SECRETLEN];
    unsigned char ad[TEST_ADLEN];
    const allocate_fptr myown_allocator = NULL;
    const deallocate_fptr myown_deallocator = NULL;

    unsigned t_cost = 3;
    unsigned m_cost = 16;
    unsigned lanes = 4;

    memset(pwd, 1, TEST_OUTLEN);
    memset(salt, 2, TEST_SALTLEN);
    memset(secret, 3, TEST_SECRETLEN);
    memset(ad, 4, TEST_ADLEN);

    context.out = out;
    context.outlen = TEST_OUTLEN;
    context.pwd = pwd;
    context.pwdlen = TEST_PWDLEN;
    context.salt = salt;
    context.saltlen = TEST_SALTLEN;
    context.secret = secret;
    context.secretlen = TEST_SECRETLEN;
    context.ad = ad;
    context.adlen = TEST_ADLEN;
    context.t_cost = t_cost;
    context.m_cost = m_cost;
    context.lanes = lanes;
    context.threads = lanes;
    context.allocate_cbk = myown_allocator;
    context.free_cbk = myown_deallocator;
    context.flags = 0;

#undef TEST_OUTLEN
#undef TEST_PWDLEN
#undef TEST_SALTLEN
#undef TEST_SECRETLEN
#undef TEST_ADLEN

    if (!strcmp(type, "d")) {
        argon2d(&context);
    } else if (!strcmp(type, "i")) {
        argon2i(&context);
    } else
        fatal("wrong Argon2 type");
}

int main(int argc, char *argv[])
{
    const char *type = (argc > 1) ? argv[1] : "i";
    generate_testvectors(type);
    return ARGON2_OK;
}
