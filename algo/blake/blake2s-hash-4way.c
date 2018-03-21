/**
 * BLAKE2 reference source code package - reference C implementations
 *
 * Written in 2012 by Samuel Neves <sneves@dei.uc.pt>
 *
 * To the extent possible under law, the author(s) have dedicated all copyright
 * and related and neighboring rights to this software to the public domain
 * worldwide. This software is distributed without any warranty.
 *
 * You should have received a copy of the CC0 Public Domain Dedication along with
 * this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
 */

#include "blake2s-hash-4way.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#if defined(__AVX__)

static const uint32_t blake2s_IV[8] =
{
	0x6A09E667UL, 0xBB67AE85UL, 0x3C6EF372UL, 0xA54FF53AUL,
	0x510E527FUL, 0x9B05688CUL, 0x1F83D9ABUL, 0x5BE0CD19UL
};

static const uint8_t blake2s_sigma[10][16] =
{
	{  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 } ,
	{ 14, 10,  4,  8,  9, 15, 13,  6,  1, 12,  0,  2, 11,  7,  5,  3 } ,
	{ 11,  8, 12,  0,  5,  2, 15, 13, 10, 14,  3,  6,  7,  1,  9,  4 } ,
	{  7,  9,  3,  1, 13, 12, 11, 14,  2,  6,  5, 10,  4,  0, 15,  8 } ,
	{  9,  0,  5,  7,  2,  4, 10, 15, 14,  1, 11, 12,  6,  8,  3, 13 } ,
	{  2, 12,  6, 10,  0, 11,  8,  3,  4, 13,  7,  5, 15, 14,  1,  9 } ,
	{ 12,  5,  1, 15, 14, 13,  4, 10,  0,  7,  6,  3,  9,  2,  8, 11 } ,
	{ 13, 11,  7, 14, 12,  1,  3,  9,  5,  0, 15,  4,  8,  6,  2, 10 } ,
	{  6, 15, 14,  9, 11,  3,  0,  8, 12,  2, 13,  7,  1,  4, 10,  5 } ,
	{ 10,  2,  8,  4,  7,  6,  1,  5, 15, 11,  9, 14,  3, 12, 13 , 0 } ,
};

// define a constant for initial param.

int blake2s_4way_init( blake2s_4way_state *S, const uint8_t outlen )
{
   blake2s_nway_param P[1];

   P->digest_length = outlen;
   P->key_length    = 0;
   P->fanout        = 1;
   P->depth         = 1;
   P->leaf_length   = 0;    
   *((uint64_t*)(P->node_offset)) = 0;
   P->node_depth    = 0;
   P->inner_length  = 0;
   memset( P->salt,     0, sizeof( P->salt ) );
   memset( P->personal, 0, sizeof( P->personal ) );

   memset( S, 0, sizeof( blake2s_4way_state ) );
   for( int i = 0; i < 8; ++i )
      S->h[i] = _mm_set1_epi32( blake2s_IV[i] );

   uint32_t *p = ( uint32_t * )( P );

   /* IV XOR ParamBlock */
   for ( size_t i = 0; i < 8; ++i )
      S->h[i] = _mm_xor_si128( S->h[i], _mm_set1_epi32( p[i] ) );
   return 0;
}

int blake2s_4way_compress( blake2s_4way_state *S, const __m128i* block )
{
   __m128i m[16];
   __m128i v[16];

   memcpy_128( m, block, 16 );
   memcpy_128( v, S->h, 8 );

   v[ 8] = _mm_set1_epi32( blake2s_IV[0] );
   v[ 9] = _mm_set1_epi32( blake2s_IV[1] );
   v[10] = _mm_set1_epi32( blake2s_IV[2] );
   v[11] = _mm_set1_epi32( blake2s_IV[3] );
   v[12] = _mm_xor_si128( _mm_set1_epi32( S->t[0] ),
                          _mm_set1_epi32( blake2s_IV[4] ) );
   v[13] = _mm_xor_si128( _mm_set1_epi32( S->t[1] ),
                          _mm_set1_epi32( blake2s_IV[5] ) );
   v[14] = _mm_xor_si128( _mm_set1_epi32( S->f[0] ),
                          _mm_set1_epi32( blake2s_IV[6] ) );
   v[15] = _mm_xor_si128( _mm_set1_epi32( S->f[1] ),
                          _mm_set1_epi32( blake2s_IV[7] ) );

#define G4W(r,i,a,b,c,d) \
do { \
   a = _mm_add_epi32( _mm_add_epi32( a, b ), m[ blake2s_sigma[r][2*i+0] ] ); \
   d = mm_rotr_32( _mm_xor_si128( d, a ), 16 ); \
   c = _mm_add_epi32( c, d ); \
   b = mm_rotr_32( _mm_xor_si128( b, c ), 12 ); \
   a = _mm_add_epi32( _mm_add_epi32( a, b ), m[ blake2s_sigma[r][2*i+1] ] ); \
   d = mm_rotr_32( _mm_xor_si128( d, a ),  8 ); \
   c = _mm_add_epi32( c, d ); \
   b = mm_rotr_32( _mm_xor_si128( b, c ),  7 ); \
} while(0)

#define ROUND4W(r)  \
do { \
   G4W( r, 0, v[ 0], v[ 4], v[ 8], v[12] ); \
   G4W( r, 1, v[ 1], v[ 5], v[ 9], v[13] ); \
   G4W( r, 2, v[ 2], v[ 6], v[10], v[14] ); \
   G4W( r, 3, v[ 3], v[ 7], v[11], v[15] ); \
   G4W( r, 4, v[ 0], v[ 5], v[10], v[15] ); \
   G4W( r, 5, v[ 1], v[ 6], v[11], v[12] ); \
   G4W( r, 6, v[ 2], v[ 7], v[ 8], v[13] ); \
   G4W( r, 7, v[ 3], v[ 4], v[ 9], v[14] ); \
} while(0)

   ROUND4W( 0 );
   ROUND4W( 1 );
   ROUND4W( 2 );
   ROUND4W( 3 );
   ROUND4W( 4 );
   ROUND4W( 5 );
   ROUND4W( 6 );
   ROUND4W( 7 );
   ROUND4W( 8 );
   ROUND4W( 9 );

   for( size_t i = 0; i < 8; ++i )
      S->h[i] = _mm_xor_si128( _mm_xor_si128( S->h[i], v[i] ), v[i + 8] );

#undef G4W
#undef ROUND4W
   return 0;
}

int blake2s_4way_update( blake2s_4way_state *S, const void *in,
                         uint64_t inlen )
{
  __m128i *input = (__m128i*)in;
  __m128i *buf = (__m128i*)S->buf;
  const int bsize = BLAKE2S_BLOCKBYTES;

   while( inlen > 0 )
   {
      size_t left = S->buflen;
      if( inlen >= bsize - left )
      {
         memcpy_128( buf + (left>>2), input, (bsize - left) >> 2 );
         S->buflen += bsize - left;
         S->t[0] += BLAKE2S_BLOCKBYTES;
         S->t[1] += ( S->t[0] < BLAKE2S_BLOCKBYTES );
         blake2s_4way_compress( S, buf ); 
         S->buflen = 0;
         input += ( bsize >> 2 );
         inlen -= bsize;
      }
      else
      {
          memcpy_128( buf + ( left>>2 ), input, inlen>>2 );
          S->buflen += (size_t) inlen; 
          input += ( inlen>>2 );
          inlen -= inlen;
      }
   }
   return 0;
}

int blake2s_4way_final( blake2s_4way_state *S, void *out, uint8_t outlen )
{
   __m128i *buf = (__m128i*)S->buf;

   S->t[0] += S->buflen;
   S->t[1] += ( S->t[0] < S->buflen );
   if ( S->last_node ) 
      S->f[1] = ~0U;
   S->f[0] = ~0U;

   memset_zero_128( buf + ( S->buflen>>2 ),
                    ( BLAKE2S_BLOCKBYTES - S->buflen ) >> 2 );      
   blake2s_4way_compress( S, buf );

   for ( int i = 0; i < 8; ++i )
      casti_m128i( out, i ) = S->h[ i ];
   return 0;
}

#if defined(__AVX2__)

int blake2s_8way_compress( blake2s_8way_state *S, const __m256i *block )
{
   __m256i m[16];
   __m256i v[16];

   memcpy_256( m, block, 16 );
   memcpy_256( v, S->h, 8 );

   v[ 8] = _mm256_set1_epi32( blake2s_IV[0] );
   v[ 9] = _mm256_set1_epi32( blake2s_IV[1] );
   v[10] = _mm256_set1_epi32( blake2s_IV[2] );
   v[11] = _mm256_set1_epi32( blake2s_IV[3] );
   v[12] = _mm256_xor_si256( _mm256_set1_epi32( S->t[0] ),
                             _mm256_set1_epi32( blake2s_IV[4] ) );
   v[13] = _mm256_xor_si256( _mm256_set1_epi32( S->t[1] ),
                             _mm256_set1_epi32( blake2s_IV[5] ) );
   v[14] = _mm256_xor_si256( _mm256_set1_epi32( S->f[0] ),
                             _mm256_set1_epi32( blake2s_IV[6] ) );
   v[15] = _mm256_xor_si256( _mm256_set1_epi32( S->f[1] ),
                             _mm256_set1_epi32( blake2s_IV[7] ) );

#define G8W(r,i,a,b,c,d) \
do { \
   a = _mm256_add_epi32( _mm256_add_epi32( a, b ), \
                          m[ blake2s_sigma[r][2*i+0] ] ); \
   d = mm256_rotr_32( _mm256_xor_si256( d, a ), 16 ); \
   c = _mm256_add_epi32( c, d ); \
   b = mm256_rotr_32( _mm256_xor_si256( b, c ), 12 ); \
   a = _mm256_add_epi32( _mm256_add_epi32( a, b ), \
                         m[ blake2s_sigma[r][2*i+1] ] ); \
   d = mm256_rotr_32( _mm256_xor_si256( d, a ),  8 ); \
   c = _mm256_add_epi32( c, d ); \
   b = mm256_rotr_32( _mm256_xor_si256( b, c ),  7 ); \
} while(0)

#define ROUND8W(r)  \
do { \
   G8W( r, 0, v[ 0], v[ 4], v[ 8], v[12] ); \
   G8W( r, 1, v[ 1], v[ 5], v[ 9], v[13] ); \
   G8W( r, 2, v[ 2], v[ 6], v[10], v[14] ); \
   G8W( r, 3, v[ 3], v[ 7], v[11], v[15] ); \
   G8W( r, 4, v[ 0], v[ 5], v[10], v[15] ); \
   G8W( r, 5, v[ 1], v[ 6], v[11], v[12] ); \
   G8W( r, 6, v[ 2], v[ 7], v[ 8], v[13] ); \
   G8W( r, 7, v[ 3], v[ 4], v[ 9], v[14] ); \
} while(0)

   ROUND8W( 0 );
   ROUND8W( 1 );
   ROUND8W( 2 );
   ROUND8W( 3 );
   ROUND8W( 4 );
   ROUND8W( 5 );
   ROUND8W( 6 );
   ROUND8W( 7 );
   ROUND8W( 8 );
   ROUND8W( 9 );

   for( size_t i = 0; i < 8; ++i )
      S->h[i] = _mm256_xor_si256( _mm256_xor_si256( S->h[i], v[i] ), v[i + 8] );

#undef G8W
#undef ROUND8W
   return 0;
}

int blake2s_8way_init( blake2s_8way_state *S, const uint8_t outlen )
{
   blake2s_nway_param P[1];

   P->digest_length = outlen;
   P->key_length    = 0;
   P->fanout        = 1;
   P->depth         = 1;
   P->leaf_length   = 0;
   *((uint64_t*)(P->node_offset)) = 0;
   P->node_depth    = 0;
   P->inner_length  = 0;
   memset( P->salt,     0, sizeof( P->salt ) );
   memset( P->personal, 0, sizeof( P->personal ) );

   memset( S, 0, sizeof( blake2s_8way_state ) );
   for( int i = 0; i < 8; ++i )
      S->h[i] = _mm256_set1_epi32( blake2s_IV[i] );

   uint32_t *p = ( uint32_t * )( P );

   /* IV XOR ParamBlock */
   for ( size_t i = 0; i < 8; ++i )
      S->h[i] = _mm256_xor_si256( S->h[i], _mm256_set1_epi32( p[i] ) );
   return 0;
}

int blake2s_8way_update( blake2s_8way_state *S, const void *in,
                         uint64_t inlen )
{
  __m256i *input = (__m256i*)in;
  __m256i *buf = (__m256i*)S->buf;
  const int bsize = BLAKE2S_BLOCKBYTES;

   while( inlen > 0 )
   {
      size_t left = S->buflen;
      if( inlen >= bsize - left )
      {
         memcpy_256( buf + (left>>2), input, (bsize - left) >> 2 );
         S->buflen += bsize - left;
         S->t[0] += BLAKE2S_BLOCKBYTES;
         S->t[1] += ( S->t[0] < BLAKE2S_BLOCKBYTES );
         blake2s_8way_compress( S, buf );
         S->buflen = 0;
         input += ( bsize >> 2 );
         inlen -= bsize;
      }
      else
      {
          memcpy_256( buf + ( left>>2 ), input, inlen>>2 );
          S->buflen += (size_t) inlen;
          input += ( inlen>>2 );
          inlen -= inlen;
      }
   }
   return 0;
}

int blake2s_8way_final( blake2s_8way_state *S, void *out, uint8_t outlen )
{
   __m256i *buf = (__m256i*)S->buf;

   S->t[0] += S->buflen;
   S->t[1] += ( S->t[0] < S->buflen );
   if ( S->last_node )
      S->f[1] = ~0U;
   S->f[0] = ~0U;

   memset_zero_256( buf + ( S->buflen>>2 ),
                    ( BLAKE2S_BLOCKBYTES - S->buflen ) >> 2 );
   blake2s_8way_compress( S, buf );

   for ( int i = 0; i < 8; ++i )
      casti_m256i( out, i ) = S->h[ i ];
   return 0;
}


#endif // __AVX2__

#if 0
int blake2s( uint8_t *out, const void *in, const void *key, const uint8_t outlen, const uint64_t inlen, uint8_t keylen )
{
	blake2s_state S[1];

	/* Verify parameters */
	if ( NULL == in ) return -1;

	if ( NULL == out ) return -1;

	if ( NULL == key ) keylen = 0; /* Fail here instead if keylen != 0 and key == NULL? */

	if( keylen > 0 )
	{
		if( blake2s_init_key( S, outlen, key, keylen ) < 0 ) return -1;
	}
	else
	{
		if( blake2s_init( S, outlen ) < 0 ) return -1;
	}

	blake2s_update( S, ( uint8_t * )in, inlen );
	blake2s_final( S, out, outlen );
	return 0;
}
#endif

#endif // __AVX__
