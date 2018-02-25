/*
 * Argon2 source code package
 *
 * Written by Daniel Dinu and Dmitry Khovratovich, 2015
 *
 * This work is licensed under a Creative Commons CC0 1.0 License/Waiver.
 *
 * You should have received a copy of the CC0 Public Domain Dedication along
 * with
 * this software. If not, see
 * <http://creativecommons.org/publicdomain/zero/1.0/>.
 */
#ifndef ARGON2_H
#define ARGON2_H

#include <stdint.h>
#include <stddef.h>
#include <limits.h>

#if defined(__cplusplus)
extern "C" {
#endif

/*************************Argon2 input parameter
 * restrictions**************************************************/

/* Minimum and maximum number of lanes (degree of parallelism) */
#define ARGON2_MIN_LANES UINT32_C(1)
#define ARGON2_MAX_LANES UINT32_C(0xFFFFFF)

/* Minimum and maximum number of threads */
#define ARGON2_MIN_THREADS UINT32_C(1)
#define ARGON2_MAX_THREADS UINT32_C(0xFFFFFF)

/* Number of synchronization points between lanes per pass */
#define ARGON2_SYNC_POINTS UINT32_C(4)

/* Minimum and maximum digest size in bytes */
#define ARGON2_MIN_OUTLEN UINT32_C(4)
#define ARGON2_MAX_OUTLEN UINT32_C(0xFFFFFFFF)

/* Minimum and maximum number of memory blocks (each of BLOCK_SIZE bytes) */
#define ARGON2_MIN_MEMORY (2 * ARGON2_SYNC_POINTS) /* 2 blocks per slice */

#define ARGON2_MIN(a, b) ((a) < (b) ? (a) : (b))
/* Max memory size is half the addressing space, topping at 2^32 blocks (4 TB)
 */
#define ARGON2_MAX_MEMORY_BITS                                                 \
    ARGON2_MIN(UINT32_C(32), (sizeof(void *) * CHAR_BIT - 10 - 1))
#define ARGON2_MAX_MEMORY                                                      \
    ARGON2_MIN(UINT32_C(0xFFFFFFFF), UINT64_C(1) << ARGON2_MAX_MEMORY_BITS)

/* Minimum and maximum number of passes */
#define ARGON2_MIN_TIME UINT32_C(1)
#define ARGON2_MAX_TIME UINT32_C(0xFFFFFFFF)

/* Minimum and maximum password length in bytes */
#define ARGON2_MIN_PWD_LENGTH UINT32_C(0)
#define ARGON2_MAX_PWD_LENGTH UINT32_C(0xFFFFFFFF)

/* Minimum and maximum associated data length in bytes */
#define ARGON2_MIN_AD_LENGTH UINT32_C(0)
#define ARGON2_MAX_AD_LENGTH UINT32_C(0xFFFFFFFF)

/* Minimum and maximum salt length in bytes */
#define ARGON2_MIN_SALT_LENGTH UINT32_C(8)
#define ARGON2_MAX_SALT_LENGTH UINT32_C(0xFFFFFFFF)

/* Minimum and maximum key length in bytes */
#define ARGON2_MIN_SECRET UINT32_C(0)
#define ARGON2_MAX_SECRET UINT32_C(0xFFFFFFFF)

#define ARGON2_FLAG_CLEAR_PASSWORD (UINT32_C(1) << 0)
#define ARGON2_FLAG_CLEAR_SECRET (UINT32_C(1) << 1)
#define ARGON2_FLAG_CLEAR_MEMORY (UINT32_C(1) << 2)
#define ARGON2_DEFAULT_FLAGS                                                   \
    (ARGON2_FLAG_CLEAR_PASSWORD | ARGON2_FLAG_CLEAR_MEMORY)

/* Error codes */
typedef enum Argon2_ErrorCodes {
    ARGON2_OK = 0,

    ARGON2_OUTPUT_PTR_NULL = 1,

    ARGON2_OUTPUT_TOO_SHORT = 2,
    ARGON2_OUTPUT_TOO_LONG = 3,

    ARGON2_PWD_TOO_SHORT = 4,
    ARGON2_PWD_TOO_LONG = 5,

    ARGON2_SALT_TOO_SHORT = 6,
    ARGON2_SALT_TOO_LONG = 7,

    ARGON2_AD_TOO_SHORT = 8,
    ARGON2_AD_TOO_LONG = 9,

    ARGON2_SECRET_TOO_SHORT = 10,
    ARGON2_SECRET_TOO_LONG = 11,

    ARGON2_TIME_TOO_SMALL = 12,
    ARGON2_TIME_TOO_LARGE = 13,

    ARGON2_MEMORY_TOO_LITTLE = 14,
    ARGON2_MEMORY_TOO_MUCH = 15,

    ARGON2_LANES_TOO_FEW = 16,
    ARGON2_LANES_TOO_MANY = 17,

    ARGON2_PWD_PTR_MISMATCH = 18,    /* NULL ptr with non-zero length */
    ARGON2_SALT_PTR_MISMATCH = 19,   /* NULL ptr with non-zero length */
    ARGON2_SECRET_PTR_MISMATCH = 20, /* NULL ptr with non-zero length */
    ARGON2_AD_PTR_MISMATCH = 21,     /* NULL ptr with non-zero length */

    ARGON2_MEMORY_ALLOCATION_ERROR = 22,

    ARGON2_FREE_MEMORY_CBK_NULL = 23,
    ARGON2_ALLOCATE_MEMORY_CBK_NULL = 24,

    ARGON2_INCORRECT_PARAMETER = 25,
    ARGON2_INCORRECT_TYPE = 26,

    ARGON2_OUT_PTR_MISMATCH = 27,

    ARGON2_THREADS_TOO_FEW = 28,
    ARGON2_THREADS_TOO_MANY = 29,

    ARGON2_MISSING_ARGS = 30,

    ARGON2_ERROR_CODES_LENGTH /* Do NOT remove; Do NOT add error codes after
                                 this
                                 error code */
} argon2_error_codes;

/* Memory allocator types --- for external allocation */
typedef int (*allocate_fptr)(uint8_t **memory, size_t bytes_to_allocate);
typedef void (*deallocate_fptr)(uint8_t *memory, size_t bytes_to_allocate);

/* Argon2 external data structures */

/*
 *****Context: structure to hold Argon2 inputs:
 * output array and its length,
 * password and its length,
 * salt and its length,
 * secret and its length,
 * associated data and its length,
 * number of passes, amount of used memory (in KBytes, can be rounded up a bit)
 * number of parallel threads that will be run.
 * All the parameters above affect the output hash value.
 * Additionally, two function pointers can be provided to allocate and
 deallocate the memory (if NULL, memory will be allocated internally).
 * Also, three flags indicate whether to erase password, secret as soon as they
 are pre-hashed (and thus not needed anymore), and the entire memory
 ****************************
 Simplest situation: you have output array out[8], password is stored in
 pwd[32], salt is stored in salt[16], you do not have keys nor associated data.
 You need to spend 1 GB of RAM and you run 5 passes of Argon2d with 4 parallel
 lanes.
 You want to erase the password, but you're OK with last pass not being erased.
 You want to use the default memory allocator.
 */
typedef struct Argon2_Context {
    uint8_t *out;    /* output array */
    uint8_t *pwd;    /* password array */
    uint8_t *salt;    /* salt array */
    /*uint8_t *secret;*/    /* key array */
    /*uint8_t *ad;*/   /* associated data array */

    allocate_fptr allocate_cbk; /* pointer to memory allocator */
    deallocate_fptr free_cbk;   /* pointer to memory deallocator */

    /*uint32_t outlen;*/ /* digest length */
    uint32_t pwdlen;  /* password length */
    /*uint32_t saltlen;*/ /* salt length */
    /*uint32_t secretlen;*/ /* key length */
    /*uint32_t adlen;*/ /* associated data length */
    /*uint32_t t_cost;*/  /* number of passes */
    /*uint32_t m_cost;*/  /* amount of memory requested (KB) */
    /*uint32_t lanes;*/   /* number of lanes */
    /*uint32_t threads;*/ /* maximum number of threads */
    /*uint32_t flags;*/ /* array of bool options */

} argon2_context;

/**
 * Function to hash the inputs in the memory-hard fashion (uses Argon2i)
 * @param  out  Pointer to the memory where the hash digest will be written
 * @param  outlen Digest length in bytes
 * @param  in Pointer to the input (password)
 * @param  inlen Input length in bytes
 * @param  salt Pointer to the salt
 * @param  saltlen Salt length in bytes
 * @pre    @a out must have at least @a outlen bytes allocated
 * @pre    @a in must be at least @inlen bytes long
 * @pre    @a saltlen must be at least @saltlen bytes long
 * @return Zero if successful, 1 otherwise.
 */
/*int hash_argon2i(void *out, size_t outlen, const void *in, size_t inlen,
                 const void *salt, size_t saltlen, unsigned int t_cost,
                 unsigned int m_cost);*/

/* same for argon2d */
/*int hash_argon2d(void *out, size_t outlen, const void *in, size_t inlen,
                 const void *salt, size_t saltlen, unsigned int t_cost,
                 unsigned int m_cost);*/

/*
 * **************Argon2d: Version of Argon2 that picks memory blocks depending
 * on the password and salt. Only for side-channel-free
 * environment!!***************
 * @param  context  Pointer to current Argon2 context
 * @return  Zero if successful, a non zero error code otherwise
 */
int argon2d(argon2_context *context);

/*
 *  * **************Argon2i: Version of Argon2 that picks memory blocks
 *independent on the password and salt. Good for side-channels,
 ******************* but worse w.r.t. tradeoff attacks if
 *******************only one pass is used***************
 * @param  context  Pointer to current Argon2 context
 * @return  Zero if successful, a non zero error code otherwise
 */
int argon2i(argon2_context *context);

/*
 *   * **************Argon2di: Reserved name***************
 * @param  context  Pointer to current Argon2 context
 * @return  Zero if successful, a non zero error code otherwise
 */
int argon2di(argon2_context *context);

/*
 *   * **************Argon2ds: Argon2d hardened against GPU attacks, 20%
 * slower***************
 * @param  context  Pointer to current Argon2 context
 * @return  Zero if successful, a non zero error code otherwise
 */
int argon2ds(argon2_context *context);

/*
 *   * **************Argon2id: First half-pass over memory is
 *password-independent, the rest are password-dependent
 ********************OK against side channels: they reduce to 1/2-pass
 *Argon2i***************
 * @param  context  Pointer to current Argon2 context
 * @return  Zero if successful, a non zero error code otherwise
 */
int argon2id(argon2_context *context);

/*
 * Verify if a given password is correct for Argon2d hashing
 * @param  context  Pointer to current Argon2 context
 * @param  hash  The password hash to verify. The length of the hash is
 * specified by the context outlen member
 * @return  Zero if successful, a non zero error code otherwise
 */
int verify_d(argon2_context *context, const char *hash);

/*
 * Get the associated error message for given error code
 * @return  The error message associated with the given error code
 */
const char *error_message(int error_code);

/* ==================================================================== */
/*
 * Code specific to Argon2i.
 *
 * The code below applies the following format:
 *
 *  $argon2i$m=<num>,t=<num>,p=<num>[,keyid=<bin>][,data=<bin>][$<bin>[$<bin>]]
 *
 * where <num> is a decimal integer (positive, fits in an 'unsigned long')
 * and <bin> is Base64-encoded data (no '=' padding characters, no newline
 * or whitespace). The "keyid" is a binary identifier for a key (up to 8
 * bytes); "data" is associated data (up to 32 bytes). When the 'keyid'
 * (resp. the 'data') is empty, then it is ommitted from the output.
 *
 * The last two binary chunks (encoded in Base64) are, in that order,
 * the salt and the output. Both are optional, but you cannot have an
 * output without a salt. The binary salt length is between 8 and 48 bytes.
 * The output length is always exactly 32 bytes.
 */

int encode_string(char *dst, size_t dst_len, argon2_context *ctx);

#if defined(__cplusplus)
}
#endif

#endif
