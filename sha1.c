
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "sha1.h"

# define SHA1_BLOCK_SIZE  64
# define SHA1_DIGEST_SIZE 20
# define SHA1_HASH_SIZE   SHA1_DIGEST_SIZE
# define SHA2_GOOD        0
# define SHA2_BAD         1

# define rotl32(x,n) (((x) << n) | ((x) >> (32 - n)))

# define SHA1_MASK   (SHA1_BLOCK_SIZE - 1)

/* reverse byte order in 32-bit words   */
#define ch(x,y,z)       ((z) ^ ((x) & ((y) ^ (z))))
#define parity(x,y,z)   ((x) ^ (y) ^ (z))
#define maj(x,y,z)      (((x) & (y)) | ((z) & ((x) | (y))))

/* A normal version as set out in the FIPS. This version uses   */
/* partial loop unrolling and is optimised for the Pentium 4    */
# define rnd(f,k)    \
    t = a; a = rotl32(a,5) + f(b,c,d) + e + k + w[i]; \
    e = d; d = c; c = rotl32(b, 30); b = t


static void sha1_compile(sha1_ctx_t *ctx)
{
	uint32_t w[80], i, a, b, c, d, e, t;

	/* note that words are compiled from the buffer into 32-bit */
	/* words in big-endian order so an order reversal is needed */
	/* here on little endian machines                           */
	for (i = 0; i < SHA1_BLOCK_SIZE / 4; ++i)
		w[i] = htonl(ctx->wbuf[i]);

	for (i = SHA1_BLOCK_SIZE / 4; i < 80; ++i)
		w[i] = rotl32(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);

	a = ctx->hash[0];
	b = ctx->hash[1];
	c = ctx->hash[2];
	d = ctx->hash[3];
	e = ctx->hash[4];

	for (i = 0; i < 20; ++i) {
		rnd(ch, 0x5a827999);
	}

	for (i = 20; i < 40; ++i) {
		rnd(parity, 0x6ed9eba1);
	}

	for (i = 40; i < 60; ++i) {
		rnd(maj, 0x8f1bbcdc);
	}

	for (i = 60; i < 80; ++i) {
		rnd(parity, 0xca62c1d6);
	}

	ctx->hash[0] += a;
	ctx->hash[1] += b;
	ctx->hash[2] += c;
	ctx->hash[3] += d;
	ctx->hash[4] += e;
}

void sha1_begin(sha1_ctx_t *ctx)
{
	ctx->count[0] = ctx->count[1] = 0;
	ctx->hash[0] = 0x67452301;
	ctx->hash[1] = 0xefcdab89;
	ctx->hash[2] = 0x98badcfe;
	ctx->hash[3] = 0x10325476;
	ctx->hash[4] = 0xc3d2e1f0;
}

/* SHA1 hash data in an array of bytes into hash buffer and call the        */
/* hash_compile function as required.                                       */
void sha1_hash(const void *data, size_t length, sha1_ctx_t *ctx)
{
	uint32_t pos = (uint32_t) (ctx->count[0] & SHA1_MASK);
	uint32_t freeb = (uint32_t)(SHA1_BLOCK_SIZE - pos);
	const unsigned char *sp = (unsigned char *)data;

	if ((ctx->count[0] += length) < length)
		++(ctx->count[1]);

	while (length >= freeb) {	/* tranfer whole blocks while possible  */
		memcpy(((unsigned char *) ctx->wbuf) + pos, sp, freeb);
		sp += freeb;
		length -= freeb;
		freeb = SHA1_BLOCK_SIZE;
		pos = 0;
		sha1_compile(ctx);
	}

	memcpy(((unsigned char *) ctx->wbuf) + pos, sp, length);
}

void *sha1_end(void *resbuf, sha1_ctx_t *ctx)
{
	/* SHA1 Final padding and digest calculation  */
	#if BB_BIG_ENDIAN
		static uint32_t mask[4] = { 0x00000000, 0xff000000, 0xffff0000, 0xffffff00 };
		static uint32_t bits[4] = { 0x80000000, 0x00800000, 0x00008000, 0x00000080 };
	#else
		static uint32_t mask[4] = { 0x00000000, 0x000000ff, 0x0000ffff, 0x00ffffff };
		static uint32_t bits[4] = { 0x00000080, 0x00008000, 0x00800000, 0x80000000 };
	#endif /* __BYTE_ORDER */

	uint8_t *hval = (uint8_t *)resbuf;
	uint32_t i, cnt = (uint32_t) (ctx->count[0] & SHA1_MASK);

	/* mask out the rest of any partial 32-bit word and then set    */
	/* the next byte to 0x80. On big-endian machines any bytes in   */
	/* the buffer will be at the top end of 32 bit words, on little */
	/* endian machines they will be at the bottom. Hence the AND    */
	/* and OR masks above are reversed for little endian systems    */
	ctx->wbuf[cnt >> 2] =
		(ctx->wbuf[cnt >> 2] & mask[cnt & 3]) | bits[cnt & 3];

	/* we need 9 or more empty positions, one for the padding byte  */
	/* (above) and eight for the length count.  If there is not     */
	/* enough space pad and empty the buffer                        */
	if (cnt > SHA1_BLOCK_SIZE - 9) {
		if (cnt < 60)
			ctx->wbuf[15] = 0;
		sha1_compile(ctx);
		cnt = 0;
	} else				/* compute a word index for the empty buffer positions  */
		cnt = (cnt >> 2) + 1;

	while (cnt < 14)	/* and zero pad all but last two positions      */
		ctx->wbuf[cnt++] = 0;

	/* assemble the eight byte counter in the buffer in big-endian  */
	/* format					               */

	ctx->wbuf[14] = htonl((ctx->count[1] << 3) | (ctx->count[0] >> 29));
	ctx->wbuf[15] = htonl(ctx->count[0] << 3);

	sha1_compile(ctx);

	/* extract the hash value as bytes in case the hash buffer is   */
	/* misaligned for 32-bit words                                  */

	for (i = 0; i < SHA1_DIGEST_SIZE; ++i)
		hval[i] = (unsigned char) (ctx->hash[i >> 2] >> 8 * (~i & 3));
	
	return resbuf;
}

int zSHA1(char*inBuf,int inLen, char*outBuf,int*outLen)
{
	sha1_ctx_t tx;
	sha1_begin(&tx);
	sha1_hash(inBuf,inLen,&tx);
	sha1_end(outBuf,&tx);
	*outLen = SHA1_DIGEST_SIZE;
	return 0;	
}


