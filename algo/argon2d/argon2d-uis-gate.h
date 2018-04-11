#ifndef ARGON2D_UIS_GATE_H__
#define ARGON2D_UIS_GATE_H__

#include "algo-gate-api.h"
#include <stdint.h>

bool register_argon2d_uis_algo( algo_gate_t* gate );

void argon2d_uis_hash( void *state, const void *input );

int scanhash_argon2d_uis( int thr_id, struct work *work, uint32_t max_nonce,
                    uint64_t *hashes_done );


#endif
