#include "ripemd-hash-4way.h"

#if defined(__AVX__)

#include <stddef.h>
#include <string.h>

/*
 * Round functions for RIPEMD-128 and RIPEMD-160.
 */
#define F1(x, y, z) \
   _mm_xor_si128( _mm_xor_si128( x, y ), z )

#define F2(x, y, z) \
   _mm_xor_si128( _mm_and_si128( _mm_xor_si128( y, z ), x ), z )

#define F3(x, y, z) \
   _mm_xor_si128( _mm_or_si128( x, mm_not( y ) ), z )

#define F4(x, y, z) \
   _mm_xor_si128( _mm_and_si128( _mm_xor_si128( x, y ), z ), y )

#define F5(x, y, z) \
   _mm_xor_si128( x, _mm_or_si128( y, mm_not( z ) ) )


static const uint32_t IV[5] =
{ 0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0 };

/*
 * Round constants for RIPEMD-160.
 */
#define K11  0x00000000
#define K12  0x5A827999
#define K13  0x6ED9EBA1
#define K14  0x8F1BBCDC
#define K15  0xA953FD4E

#define K21  0x50A28BE6
#define K22  0x5C4DD124
#define K23  0x6D703EF3
#define K24  0x7A6D76E9
#define K25  0x00000000

#define RR(a, b, c, d, e, f, s, r, k) \
do{ \
   a = _mm_add_epi32( mm_rotl_32( _mm_add_epi32( _mm_add_epi32( \
                _mm_add_epi32( a, f( b ,c, d ) ), r ), \
                                 _mm_set1_epi32( k ) ), s ), e ); \
   c = mm_rotl_32( c, 10 );\
} while (0)

#define ROUND1(a, b, c, d, e, f, s, r, k)  \
	RR(a ## 1, b ## 1, c ## 1, d ## 1, e ## 1, f, s, r, K1 ## k)

#define ROUND2(a, b, c, d, e, f, s, r, k)  \
	RR(a ## 2, b ## 2, c ## 2, d ## 2, e ## 2, f, s, r, K2 ## k)

static void ripemd160_4way_round( ripemd160_4way_context *sc )
{
   const __m128i *in = (__m128i*)sc->buf;
   __m128i *h  = (__m128i*)sc->val;
   register __m128i A1, B1, C1, D1, E1;
   register __m128i A2, B2, C2, D2, E2;
   __m128i tmp;

   A1 = A2 = h[0];
   B1 = B2 = h[1];
   C1 = C2 = h[2];
   D1 = D2 = h[3];
   E1 = E2 = h[4];

   ROUND1( A, B, C, D, E, F1, 11, in[ 0], 1 );
   ROUND1( E, A, B, C, D, F1, 14, in[ 1], 1 );
   ROUND1( D, E, A, B, C, F1, 15, in[ 2], 1 );
   ROUND1( C, D, E, A, B, F1, 12, in[ 3], 1 );
   ROUND1( B, C, D, E, A, F1,  5, in[ 4], 1 );
   ROUND1( A, B, C, D, E, F1,  8, in[ 5], 1 );
   ROUND1( E, A, B, C, D, F1,  7, in[ 6], 1 );
   ROUND1( D, E, A, B, C, F1,  9, in[ 7], 1 );
   ROUND1( C, D, E, A, B, F1, 11, in[ 8], 1 );
   ROUND1( B, C, D, E, A, F1, 13, in[ 9], 1 );
   ROUND1( A, B, C, D, E, F1, 14, in[10], 1 );
   ROUND1( E, A, B, C, D, F1, 15, in[11], 1 );
   ROUND1( D, E, A, B, C, F1,  6, in[12], 1 );
   ROUND1( C, D, E, A, B, F1,  7, in[13], 1 );
   ROUND1( B, C, D, E, A, F1,  9, in[14], 1 );
   ROUND1( A, B, C, D, E, F1,  8, in[15], 1 );

   ROUND1( E, A, B, C, D, F2,  7, in[ 7], 2 );
   ROUND1( D, E, A, B, C, F2,  6, in[ 4], 2 );
   ROUND1( C, D, E, A, B, F2,  8, in[13], 2 );
   ROUND1( B, C, D, E, A, F2, 13, in[ 1], 2 );
   ROUND1( A, B, C, D, E, F2, 11, in[10], 2 );
   ROUND1( E, A, B, C, D, F2,  9, in[ 6], 2 );
   ROUND1( D, E, A, B, C, F2,  7, in[15], 2 );
   ROUND1( C, D, E, A, B, F2, 15, in[ 3], 2 );
   ROUND1( B, C, D, E, A, F2,  7, in[12], 2 );
   ROUND1( A, B, C, D, E, F2, 12, in[ 0], 2 );
   ROUND1( E, A, B, C, D, F2, 15, in[ 9], 2 );
   ROUND1( D, E, A, B, C, F2,  9, in[ 5], 2 );
   ROUND1( C, D, E, A, B, F2, 11, in[ 2], 2 );
   ROUND1( B, C, D, E, A, F2,  7, in[14], 2 );
   ROUND1( A, B, C, D, E, F2, 13, in[11], 2 );
   ROUND1( E, A, B, C, D, F2, 12, in[ 8], 2 );

   ROUND1( D, E, A, B, C, F3, 11, in[ 3], 3 );
   ROUND1( C, D, E, A, B, F3, 13, in[10], 3 );
   ROUND1( B, C, D, E, A, F3,  6, in[14], 3 );
   ROUND1( A, B, C, D, E, F3,  7, in[ 4], 3 );
   ROUND1( E, A, B, C, D, F3, 14, in[ 9], 3 );
   ROUND1( D, E, A, B, C, F3,  9, in[15], 3 );
   ROUND1( C, D, E, A, B, F3, 13, in[ 8], 3 );
   ROUND1( B, C, D, E, A, F3, 15, in[ 1], 3 );
   ROUND1( A, B, C, D, E, F3, 14, in[ 2], 3 );
   ROUND1( E, A, B, C, D, F3,  8, in[ 7], 3 );
   ROUND1( D, E, A, B, C, F3, 13, in[ 0], 3 );
   ROUND1( C, D, E, A, B, F3,  6, in[ 6], 3 );
   ROUND1( B, C, D, E, A, F3,  5, in[13], 3 );
   ROUND1( A, B, C, D, E, F3, 12, in[11], 3 );
   ROUND1( E, A, B, C, D, F3,  7, in[ 5], 3 );
   ROUND1( D, E, A, B, C, F3,  5, in[12], 3 );

   ROUND1( C, D, E, A, B, F4, 11, in[ 1], 4 );
   ROUND1( B, C, D, E, A, F4, 12, in[ 9], 4 );
   ROUND1( A, B, C, D, E, F4, 14, in[11], 4 );
   ROUND1( E, A, B, C, D, F4, 15, in[10], 4 );
   ROUND1( D, E, A, B, C, F4, 14, in[ 0], 4 );
   ROUND1( C, D, E, A, B, F4, 15, in[ 8], 4 );
   ROUND1( B, C, D, E, A, F4,  9, in[12], 4 );
   ROUND1( A, B, C, D, E, F4,  8, in[ 4], 4 );
   ROUND1( E, A, B, C, D, F4,  9, in[13], 4 );
   ROUND1( D, E, A, B, C, F4, 14, in[ 3], 4 );
   ROUND1( C, D, E, A, B, F4,  5, in[ 7], 4 );
   ROUND1( B, C, D, E, A, F4,  6, in[15], 4 );
   ROUND1( A, B, C, D, E, F4,  8, in[14], 4 );
   ROUND1( E, A, B, C, D, F4,  6, in[ 5], 4 );
   ROUND1( D, E, A, B, C, F4,  5, in[ 6], 4 );
   ROUND1( C, D, E, A, B, F4, 12, in[ 2], 4 );

   ROUND1( B, C, D, E, A, F5,  9, in[ 4], 5 );
   ROUND1( A, B, C, D, E, F5, 15, in[ 0], 5 );
   ROUND1( E, A, B, C, D, F5,  5, in[ 5], 5 );
   ROUND1( D, E, A, B, C, F5, 11, in[ 9], 5 );
   ROUND1( C, D, E, A, B, F5,  6, in[ 7], 5 );
   ROUND1( B, C, D, E, A, F5,  8, in[12], 5 );
   ROUND1( A, B, C, D, E, F5, 13, in[ 2], 5 );
   ROUND1( E, A, B, C, D, F5, 12, in[10], 5 );
   ROUND1( D, E, A, B, C, F5,  5, in[14], 5 );
   ROUND1( C, D, E, A, B, F5, 12, in[ 1], 5 );
   ROUND1( B, C, D, E, A, F5, 13, in[ 3], 5 );
   ROUND1( A, B, C, D, E, F5, 14, in[ 8], 5 );
   ROUND1( E, A, B, C, D, F5, 11, in[11], 5 );
   ROUND1( D, E, A, B, C, F5,  8, in[ 6], 5 );
   ROUND1( C, D, E, A, B, F5,  5, in[15], 5 );
   ROUND1( B, C, D, E, A, F5,  6, in[13], 5 );

   ROUND2( A, B, C, D, E, F5,  8, in[ 5], 1 );
   ROUND2( E, A, B, C, D, F5,  9, in[14], 1 );
   ROUND2( D, E, A, B, C, F5,  9, in[ 7], 1 );
   ROUND2( C, D, E, A, B, F5, 11, in[ 0], 1 );
   ROUND2( B, C, D, E, A, F5, 13, in[ 9], 1 );
   ROUND2( A, B, C, D, E, F5, 15, in[ 2], 1 );
   ROUND2( E, A, B, C, D, F5, 15, in[11], 1 );
   ROUND2( D, E, A, B, C, F5,  5, in[ 4], 1 );
   ROUND2( C, D, E, A, B, F5,  7, in[13], 1 );
   ROUND2( B, C, D, E, A, F5,  7, in[ 6], 1 );
   ROUND2( A, B, C, D, E, F5,  8, in[15], 1 );
   ROUND2( E, A, B, C, D, F5, 11, in[ 8], 1 );
   ROUND2( D, E, A, B, C, F5, 14, in[ 1], 1 );
   ROUND2( C, D, E, A, B, F5, 14, in[10], 1 );
   ROUND2( B, C, D, E, A, F5, 12, in[ 3], 1 );
   ROUND2( A, B, C, D, E, F5,  6, in[12], 1 );

   ROUND2( E, A, B, C, D, F4,  9, in[ 6], 2 );
   ROUND2( D, E, A, B, C, F4, 13, in[11], 2 );
   ROUND2( C, D, E, A, B, F4, 15, in[ 3], 2 );
   ROUND2( B, C, D, E, A, F4,  7, in[ 7], 2 );
   ROUND2( A, B, C, D, E, F4, 12, in[ 0], 2 );
   ROUND2( E, A, B, C, D, F4,  8, in[13], 2 );
   ROUND2( D, E, A, B, C, F4,  9, in[ 5], 2 );
   ROUND2( C, D, E, A, B, F4, 11, in[10], 2 );
   ROUND2( B, C, D, E, A, F4,  7, in[14], 2 );
   ROUND2( A, B, C, D, E, F4,  7, in[15], 2 );
   ROUND2( E, A, B, C, D, F4, 12, in[ 8], 2 );
   ROUND2( D, E, A, B, C, F4,  7, in[12], 2 );
   ROUND2( C, D, E, A, B, F4,  6, in[ 4], 2 );
   ROUND2( B, C, D, E, A, F4, 15, in[ 9], 2 );
   ROUND2( A, B, C, D, E, F4, 13, in[ 1], 2 );
   ROUND2( E, A, B, C, D, F4, 11, in[ 2], 2 );

   ROUND2( D, E, A, B, C, F3,  9, in[15], 3 );
   ROUND2( C, D, E, A, B, F3,  7, in[ 5], 3 );
   ROUND2( B, C, D, E, A, F3, 15, in[ 1], 3 );
   ROUND2( A, B, C, D, E, F3, 11, in[ 3], 3 );
   ROUND2( E, A, B, C, D, F3,  8, in[ 7], 3 );
   ROUND2( D, E, A, B, C, F3,  6, in[14], 3 );
   ROUND2( C, D, E, A, B, F3,  6, in[ 6], 3 );
   ROUND2( B, C, D, E, A, F3, 14, in[ 9], 3 );
   ROUND2( A, B, C, D, E, F3, 12, in[11], 3 );
   ROUND2( E, A, B, C, D, F3, 13, in[ 8], 3 );
   ROUND2( D, E, A, B, C, F3,  5, in[12], 3 );
   ROUND2( C, D, E, A, B, F3, 14, in[ 2], 3 );
   ROUND2( B, C, D, E, A, F3, 13, in[10], 3 );
   ROUND2( A, B, C, D, E, F3, 13, in[ 0], 3 );
   ROUND2( E, A, B, C, D, F3,  7, in[ 4], 3 );
   ROUND2( D, E, A, B, C, F3,  5, in[13], 3 );

   ROUND2( C, D, E, A, B, F2, 15, in[ 8], 4 );
   ROUND2( B, C, D, E, A, F2,  5, in[ 6], 4 );
   ROUND2( A, B, C, D, E, F2,  8, in[ 4], 4 );
   ROUND2( E, A, B, C, D, F2, 11, in[ 1], 4 );
   ROUND2( D, E, A, B, C, F2, 14, in[ 3], 4 );
   ROUND2( C, D, E, A, B, F2, 14, in[11], 4 );
   ROUND2( B, C, D, E, A, F2,  6, in[15], 4 );
   ROUND2( A, B, C, D, E, F2, 14, in[ 0], 4 );
   ROUND2( E, A, B, C, D, F2,  6, in[ 5], 4 );
   ROUND2( D, E, A, B, C, F2,  9, in[12], 4 );
   ROUND2( C, D, E, A, B, F2, 12, in[ 2], 4 );
   ROUND2( B, C, D, E, A, F2,  9, in[13], 4 );
   ROUND2( A, B, C, D, E, F2, 12, in[ 9], 4 );
   ROUND2( E, A, B, C, D, F2,  5, in[ 7], 4 );
   ROUND2( D, E, A, B, C, F2, 15, in[10], 4 );
   ROUND2( C, D, E, A, B, F2,  8, in[14], 4 );

   ROUND2( B, C, D, E, A, F1,  8, in[12], 5 );
   ROUND2( A, B, C, D, E, F1,  5, in[15], 5 );
   ROUND2( E, A, B, C, D, F1, 12, in[10], 5 );
   ROUND2( D, E, A, B, C, F1,  9, in[ 4], 5 );
   ROUND2( C, D, E, A, B, F1, 12, in[ 1], 5 );
   ROUND2( B, C, D, E, A, F1,  5, in[ 5], 5 );
   ROUND2( A, B, C, D, E, F1, 14, in[ 8], 5 );
   ROUND2( E, A, B, C, D, F1,  6, in[ 7], 5 );
   ROUND2( D, E, A, B, C, F1,  8, in[ 6], 5 );
   ROUND2( C, D, E, A, B, F1, 13, in[ 2], 5 );
   ROUND2( B, C, D, E, A, F1,  6, in[13], 5 );
   ROUND2( A, B, C, D, E, F1,  5, in[14], 5 );
   ROUND2( E, A, B, C, D, F1, 15, in[ 0], 5 );
   ROUND2( D, E, A, B, C, F1, 13, in[ 3], 5 );
   ROUND2( C, D, E, A, B, F1, 11, in[ 9], 5 );
   ROUND2( B, C, D, E, A, F1, 11, in[11], 5 );

   tmp =  _mm_add_epi32( _mm_add_epi32( h[1], C1 ), D2 );
   h[1] = _mm_add_epi32( _mm_add_epi32( h[2], D1 ), E2 );
   h[2] = _mm_add_epi32( _mm_add_epi32( h[3], E1 ), A2 );
   h[3] = _mm_add_epi32( _mm_add_epi32( h[4], A1 ), B2 );
   h[4] = _mm_add_epi32( _mm_add_epi32( h[0], B1 ), C2 );
   h[0] = tmp;
}

void ripemd160_4way_init( ripemd160_4way_context *sc )
{
   sc->val[0] = _mm_set1_epi32( IV[0] );
   sc->val[1] = _mm_set1_epi32( IV[1] );
   sc->val[2] = _mm_set1_epi32( IV[2] );
   sc->val[3] = _mm_set1_epi32( IV[3] );
   sc->val[4] = _mm_set1_epi32( IV[4] );
   sc->count_high = sc->count_low = 0;
}

void ripemd160_4way( ripemd160_4way_context *sc, const void *data, size_t len )
{
   __m128i *vdata = (__m128i*)data;
   size_t ptr;
   const int block_size = 64;

   ptr = (unsigned)sc->count_low & (block_size - 1U);
   while ( len > 0 )
   {
      size_t clen;
      uint32_t clow, clow2;

      clen = block_size - ptr;
      if ( clen > len )
         clen = len;
      memcpy_128( sc->buf + (ptr>>2), vdata, clen>>2 );
      vdata = vdata + (clen>>2);
      ptr += clen;
      len -= clen;
      if ( ptr == block_size )
      {
         ripemd160_4way_round( sc );
         ptr = 0;
      }
      clow = sc->count_low;
      clow2 = clow + clen;
      sc->count_low = clow2;
      if ( clow2 < clow )
         sc->count_high++;
   }
}

void ripemd160_4way_close( ripemd160_4way_context  *sc, void *dst )
{
   unsigned ptr, u;
   uint32_t low, high;
   const int block_size = 64;
   const int pad = block_size - 8;

   ptr = (unsigned)sc->count_low & ( block_size - 1U);
   sc->buf[ ptr>>2 ] = _mm_set1_epi32( 0x80 );
   ptr += 4;

   if ( ptr > pad )
   {
       memset_zero_128( sc->buf + (ptr>>2), (block_size - ptr) >> 2 );
       ripemd160_4way_round( sc );
       memset_zero_128( sc->buf, pad>>2 );
   }
   else
       memset_zero_128( sc->buf + (ptr>>2), (pad - ptr) >> 2 );
        
    low = sc->count_low;
    high = (sc->count_high << 3) | (low >> 29);
    low = low << 3;
    sc->buf[  pad>>2      ] = _mm_set1_epi32( low  );
    sc->buf[ (pad>>2) + 1 ] = _mm_set1_epi32( high );
    ripemd160_4way_round( sc );
    for (u = 0; u < 5; u ++)
        casti_m128i( dst, u ) = sc->val[u];
}

#endif
