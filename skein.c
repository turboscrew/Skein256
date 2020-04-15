/* Skein256 hash as described in the paper "skein 1.3" */

#include <stdio.h>
#include "skein.h"

//#define SKEIN_DEBUG
//#define SKEIN_VERBOSE_DEBUG
#define SKEIN_ROUNDS 72

uint8_t skein_rot_tbl[2][8] =
{
	{14, 52, 23, 5, 25, 46, 58, 32},
	{16, 57, 40, 37, 33, 12, 22, 32}
};

skein_conf_str_t skein_conf_str;

void skein_reverse_bytes(uint8_t *s, uint8_t len)
{
	uint8_t tmp;
	uint8_t i;
	for (i = 0; i < len / 2; i ++)
	{
		tmp = s[i];
		s[i] = s[(len - 1) - i];
		s[(len - 1) - i] = tmp;
	}
}

void skein_print_words(uint64_t *w, uint8_t cnt)
{
	uint8_t i;
	
	for (i = 0; i < cnt; i++)
	{
		printf("0x%016lX\n", *(w++));
	}
}

void mk_skein_block_b(skein_word_t *w, uint8_t *s)
{
	/* skein is little-endian */
	uint8_t i, j;
	uint64_t tmp;
	
	for (i = 0; i < 4; i++)
	{
		tmp = 0L;
		for (j = 0; j < 8; j++)
		{
			 tmp |= ((uint64_t)(*(s++))) << (j * 8) ;
		}
		w->word[i] = tmp;
	}
}

void mk_skein_tweak_b(skein_tweak_t *w, uint8_t *s)
{
	/* skein is little-endian */
	uint8_t i;
	uint64_t tmp = 0L;
	
	for (i = 0; i < 8; i ++)
	{
		tmp |= ((uint64_t)(*(s++))) << (i * 8) ;
	}
	w->word[0] = tmp;
	tmp = 0L;
	for (i = 0; i < 8; i ++)
	{
		tmp |= ((uint64_t)(*(s++))) << (i * 8) ;
	}
	w->word[1] = tmp;
}

void skein_set_final(skein_tweak_t *w, uint8_t val)
{
	if (val)
	{
		w->byte[15] |= 1 << 7;
	}
	else
	{
		w->byte[15] &= ~(1 << 7);
	}
}

void skein_set_first(skein_tweak_t *w, uint8_t val)
{
	if (val)
	{
		w->byte[15] |= 1 << 6;
	}
	else
	{
		w->byte[15] &= ~(1 << 6);
	}
}

void skein_set_type(skein_tweak_t *w, uint8_t val)
{
	w->byte[15] &= 0xC0;
	w->byte[15] |= (val & 0x3F);	
}

void skein_set_pos(skein_tweak_t *w, uint64_t val)
{
	w->word[0] = val;
}

uint64_t skein_get_pos(skein_tweak_t *w)
{
	return w->word[0];
}

uint64_t skein_rotl(uint64_t word, uint8_t round, uint8_t half)
{
	uint64_t res;
	uint8_t bits, r;

	r = round % 8;
	bits = skein_rot_tbl[half][r];
	res = word >> (64 - bits);
	res |= word << bits;
	return res;
}

void skein_mix(uint64_t *E, uint8_t round)
{
	uint64_t tmp0, tmp1;
	
	/* first half */
	tmp0 = E[0] + E[1];
	tmp1 = skein_rotl(E[1], round, 0) ^ tmp0;
	E[0] = tmp0;
	E[1] = tmp1;
	
	/* second half */
	tmp0 = E[2] + E[3];
	tmp1 = skein_rotl(E[3], round, 1) ^ tmp0;
	E[2] = tmp0;
	E[3] = tmp1;
}

void skein_permute(uint64_t *E)
{
	uint64_t tmp;
	
	tmp = E[1];
	E[1] = E[3];
	E[3] = tmp;
}

void threefish(skein_word_t *K, skein_tweak_t *T, skein_word_t *P, skein_word_t *E)
{
	/* all additions are mod 2**64 */
	uint8_t r, s;
	uint64_t k[5];
	uint64_t t[3];
	uint64_t e[4];
	uint64_t ks[4];
	
	/* tweak */
	t[0] = T->word[0];
	t[1] = T->word[1];
	t[2] = t[0] ^ t[1];
	
#ifdef SKEIN_DEBUG
	printf("\nthreefish: T\n");
	skein_print_words(T->word, 2);
	printf("threefish: t\n");
	skein_print_words(t, 3);
#endif
	/* key schedule prep */
	k[0] = K->word[0];
	k[1] = K->word[1];
	k[2] = K->word[2];
	k[3] = K->word[3];
	k[4] = 0;

	for (r=0; r < 4; r++)
	{
		k[4] ^= k[r];
	}
	k[4] ^= 0x1BD11BDAA9FC1A22L; /* C240 */
	
#ifdef SKEIN_DEBUG
	printf("\nthreefish: k\n");
	skein_print_words(k, 5);
	printf("\n");
#endif
	e[0] = P->word[0];
	e[1] = P->word[1];
	e[2] = P->word[2];
	e[3] = P->word[3];

	for (r = 0; r < SKEIN_ROUNDS; r++)
	{
		/* key schedule */
		if (r % 4 == 0)
		{
			s = r / 4;
			/* calculate subkey */
			ks[0] = k[(s+0) % 5];
			ks[1] = k[(s+1) % 5] + t[s % 3];
			ks[2] = k[(s+2) % 5] + t[(s+1) % 3];
			ks[3] = k[(s+3) % 5] + s;
			
#ifdef SKEIN_VERBOSE_DEBUG
			printf("subkeys (s=%d):\n", (int)s);
			skein_print_words(ks, 4);
#endif

			/* add subkey */
			e[0] += ks[0];
			e[1] += ks[1];
			e[2] += ks[2];
			e[3] += ks[3];
		}
		/* basic steps */
		skein_mix(e,r);
		skein_permute(e);
	}

	s = r / 4;
	/* calculate subkey */
	ks[0] = k[(s+0) % 5];
	ks[1] = k[(s+1) % 5] + t[s % 3];
	ks[2] = k[(s+2) % 5] + t[(s+1) % 3];
	ks[3] = k[(s+3) % 5] + s;
#ifdef SKEIN_VERBOSE_DEBUG
	printf("subkeys (s=%d):\n", (int)s);
	skein_print_words(ks, 4);
#endif

	/* add subkey */
	e[0] += ks[0];
	e[1] += ks[1];
	e[2] += ks[2];
	e[3] += ks[3];

	/* return result */
	E->word[0] = e[0];
	E->word[1] = e[1];
	E->word[2] = e[2];
	E->word[3] = e[3];
}

void mk_skein_conf(void)
{
	uint8_t i;
	uint8_t *pcf;
	uint8_t tmp;
	
	/* clear configuretion string */
	pcf = (uint8_t *)&skein_conf_str;
	for (i = 0; i < SKEIN_BLK_BYTES; i++) pcf[i] = 0;
	
	/* add relevant info */
	skein_conf_str.schema_id[0] = 'S';
	skein_conf_str.schema_id[1] = 'H';
	skein_conf_str.schema_id[2] = 'A';
	skein_conf_str.schema_id[3] = '3';
	skein_conf_str.version[0] = 1; /* 0x0001 */
	skein_conf_str.version[1] = 0;
	skein_conf_str.outlen[0] = 0;
	skein_conf_str.outlen[1] = 1; /* 256 bits */

//	skein_reverse_bytes(pcf, SKEIN_BLK_BYTES);
#ifdef SKEIN_DEBUG
	printf("\nconfig str:\n");
	skein_print_words((uint64_t *)pcf, 4);
#endif
}

void UBI(skein_word_t *G, skein_tweak_t *T, uint8_t *M, uint32_t mlen)
{
	uint32_t i, j;
	skein_word_t P;
	uint8_t buff[SKEIN_BLK_BYTES];

	i = mlen / SKEIN_BLK_BYTES; /* full blocks */
	j = mlen % SKEIN_BLK_BYTES; /* last partial block bytes */
	
	while (i > 0) /* while full blocks of data */
	{
		if ((i == 1) && (j == 0))
		{
			/* last block is full */
			skein_set_final(T, 1);
		}
		/* set handled bytes - current block included */
		skein_set_pos(T, skein_get_pos(T) + SKEIN_BLK_BYTES);
		mk_skein_block_b(&P, M);
#ifdef SKEIN_DEBUG
		printf("\nUbi data:\n");
		skein_print_words(P.word, 4);
#endif
		threefish(G, T, &P, G);

		/* "feedforward" */
		G->word[0] ^= P.word[0];
		G->word[1] ^= P.word[1];
		G->word[2] ^= P.word[2];
		G->word[3] ^= P.word[3];

		M += SKEIN_BLK_BYTES; /* next block */
		skein_set_first(T, 0);
		i--; /* one block less to handle */
	}
	if (j > 0) /* possible last non-full block */
	{
		/* set handled bytes - current block included */
		skein_set_pos(T, skein_get_pos(T) + (uint64_t)j);
		/* take the bytes and zero-pad the last block */
		for (i = 0; i < SKEIN_BLK_BYTES; i++)
		{
#if 1
			if (i < j) buff[i] = M[i];
			else buff[i] = 0; /* zero-pad */
			mk_skein_block_b(&P, buff);
#else
			if (i < j) P.byte[i] = M[i];
			else P.byte[i] = 0; /* zero-pad */
#endif
		}
#ifdef SKEIN_DEBUG
		printf("\nUbi data:\n");
		skein_print_words(P.word, 4);
#endif
		skein_set_final(T, 1);
		threefish(G, T, &P, G);
		
		/* "feedforward" */
		G->word[0] ^= P.word[0];
		G->word[1] ^= P.word[1];
		G->word[2] ^= P.word[2];
		G->word[3] ^= P.word[3];

		skein_set_first(T, 0);
	}
}

void skein_hash(uint8_t *hash, uint8_t *M, uint32_t mlen)
{
	uint8_t i;
	skein_word_t G;
	skein_tweak_t T;
	skein_word_t zero;
	
	/* configure */
#if 0
	{
		uint64_t iv[] = {
			0xFC9DA860D048B449L, 0x2FCA66479FA7D833L, 0xB33BC3896656840FL, 0x6A54E920FDE8DA69L
		};
		for (i = 0; i < 4; i++) G.word[i] = iv[i];
	}
#else
	mk_skein_conf();
	T.word[0] = 0L;
	T.word[1] = 0L;
	skein_set_first(&T, 1);
	skein_set_final(&T, 1);
	skein_set_type(&T, SKEIN_TYPE_CFG);
	G.word[0] = 0L;
	G.word[1] = 0L;
	G.word[2] = 0L;
	G.word[3] = 0L;
	UBI(&G, &T, (uint8_t *)&skein_conf_str, SKEIN_BLK_BYTES);
#ifdef SKEIN_DEBUG
	printf("Config UBI result\n");
	skein_print_words(G.word, 4);
#endif
#endif
	/* handle message */
	skein_set_first(&T, 1);
	skein_set_final(&T, 0);
	skein_set_type(&T, SKEIN_TYPE_MSG);
	skein_set_pos(&T, 0L);
	UBI(&G, &T, M, mlen);
#ifdef SKEIN_DEBUG
	printf("Message UBI result\n");
	skein_print_words(G.word, 4);
#endif	
	/* get output */
	skein_set_first(&T, 1);
	skein_set_final(&T, 1);
	skein_set_type(&T, SKEIN_TYPE_OUT);
	skein_set_pos(&T, 0L);
	zero.word[0] = 0L;
	zero.word[1] = 0L;
	zero.word[2] = 0L;
	zero.word[3] = 0L;
	UBI(&G, &T, zero.byte, sizeof(uint64_t));
#ifdef SKEIN_DEBUG
	printf("Output UBI result\n");
	skein_print_words(G.word, 4);
#endif
	for (i = 0; i < SKEIN_BLK_BYTES; i++)
	{
		*(hash++) =G.byte[i];
	}
}

void skein_parts_test(void)
{
	uint64_t tst1, tst2;
	skein_word_t E;
	skein_tweak_t T;
	
	/* test rotl (64 - 23 = 41)*/
	tst1 = (1L << (64 - 23)) - 1;
	tst2 = skein_rotl(tst1, 2, 0);
	printf("rotl\n");
	skein_print_words(&tst1, 1);
	skein_print_words(&tst2, 1);
	tst1 = 0x0101010101010101L;
	tst2 = skein_rotl(tst1, 7, 0);
	printf("rotl (for mix test)\n");
	skein_print_words(&tst1, 1);
	skein_print_words(&tst2, 1);
	
	E.word[0] = 0x1010101010101010L;
	E.word[1] = 0x0101010101010101L;
	E.word[2] = 0x1010101010101010L;
	E.word[3] = 0x0101010101010101L;
	/* test mix */
	skein_mix(E.word, 7);
	printf("mix\n");
	skein_print_words(E.word, 4);
	
	E.word[0] = 0xAAAAAAAAAAAAAAAAL;
	E.word[1] = 0xBBBBBBBBBBBBBBBBL;
	E.word[2] = 0xCCCCCCCCCCCCCCCCL;
	E.word[3] = 0xDDDDDDDDDDDDDDDDL;
	/* test permute */
	skein_permute(E.word);
	printf("permute\n");
	skein_print_words(E.word, 4);
	
	/* test UBI */
	E.word[0] = 0L;
	E.word[1] = 0L;
	E.word[2] = 0L;
	E.word[3] = 0L;
	T.word[0] = 0L;
	T.word[1] = 0L;
	skein_set_first(&T, 1);
	skein_set_final(&T, 1);
	skein_set_type(&T, SKEIN_TYPE_CFG);
	printf("\nUBI(config)\n");
	printf("config tweak\n");
	skein_print_words(T.word, 2);	
	mk_skein_conf();
	UBI(&E, &T, (uint8_t *)&skein_conf_str, SKEIN_BLK_BYTES);
	printf("\nconfig UBI result\n");
	skein_print_words(E.word, 4);

	printf("\nend of tests\n\n");
}
