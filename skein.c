/* Skein256 hash as described in the paper "skein 1.3" */

#include "skein.h"

#if defined(SKEIN_DEBUG) || defined(SKEIN_VERBOSE_DEBUG)
#define SKEIN_PRINT
#endif

#ifdef SKEIN_PRINT
#include <stdio.h>
#endif

#define SKEIN_ROUNDS 72

uint8_t skein_rot_tbl[2][8] =
{
	{14, 52, 23, 5, 25, 46, 58, 32},
	{16, 57, 40, 37, 33, 12, 22, 32}
};

skein_conf_str_t skein_conf_str;

#ifdef SKEIN_PRINT
void skein_print_words(uint64_t *w, uint8_t cnt)
{
	uint8_t i;
	
	for (i = 0; i < cnt; i++)
	{
		printf("0x%016lX\n", *(w++));
	}
}
#endif

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
		w->word[1] |= (1L) << (127 - 64);	
	}
	else
	{
		w->word[1] &= ~((1L) << (127 - 64));
	}
}

void skein_set_first(skein_tweak_t *w, uint8_t val)
{
	if (val)
	{
		w->word[1] |= (1L) << (126 - 64);	
	}
	else
	{
		w->word[1] &= ~((1L) << (126 - 64));
	}
}

void skein_set_type(skein_tweak_t *w, uint8_t val)
{
	w->word[1] &= (0xC0L) << (120 - 64);
	w->word[1] |= ((uint64_t)(val & 0x3F)) << (120 - 64);
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
	uint32_t i;
	skein_word_t P;

	i = mlen / SKEIN_BLK_BYTES; /* full blocks */
	
	if (i * SKEIN_BLK_BYTES < mlen) i++;
	while (i > 0) /* while full blocks of data */
	{
		/* set handled bytes - current block included */
		skein_set_pos(T, skein_get_pos(T) + mlen);
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
}

void skein_hash(uint8_t *hash, uint8_t *M, uint32_t mlen)
{
	uint8_t i;
	uint32_t f, b;
	skein_word_t G;
	skein_tweak_t T;
	skein_word_t tmp_msg;
	
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
	f = mlen / SKEIN_BLK_BYTES; /* full blocks */
	b = mlen % SKEIN_BLK_BYTES; /* last partial block bytes */
	if (b == 0) /* only full blocks */
	{
		/* save last full block for final */
		f -=1;
		b = SKEIN_BLK_BYTES;
	}
	if (f > 0)
	{
		/* all non-last blocks */
		UBI(&G, &T, M, f * SKEIN_BLK_BYTES);
		M += f * SKEIN_BLK_BYTES;
	}
	/* handle final block */
	for (i = 0; i < SKEIN_BLK_BYTES; i++)
	{
		/* zero-fill last block if needed */
		if (i < b)
		{
			tmp_msg.byte[i] = *(M++);
		}
		else
		{
			tmp_msg.byte[i] = 0;
		}
	}
	skein_set_final(&T, 1);
	UBI(&G, &T, tmp_msg.byte, b);

	/* last block */
	
#ifdef SKEIN_DEBUG
	printf("Message UBI result\n");
	skein_print_words(G.word, 4);
#endif	
	/* get output */
	skein_set_first(&T, 1);
	skein_set_final(&T, 1);
	skein_set_type(&T, SKEIN_TYPE_OUT);
	skein_set_pos(&T, 0L);
	tmp_msg.word[0] = 0L;
	tmp_msg.word[1] = 0L;
	tmp_msg.word[2] = 0L;
	tmp_msg.word[3] = 0L;
	UBI(&G, &T, tmp_msg.byte, sizeof(uint64_t));
#ifdef SKEIN_DEBUG
	printf("Output UBI result\n");
	skein_print_words(G.word, 4);
#endif
	for (i = 0; i < SKEIN_BLK_BYTES; i++)
	{
		*(hash++) =G.byte[i];
	}
}

void skein_init(skein_context_t *ctx)
{
	/* replace config UBI with IV */
	ctx->G.word[0] = 0xFC9DA860D048B449L;
	ctx->G.word[1] = 0x2FCA66479FA7D833L;
	ctx->G.word[2] = 0xB33BC3896656840FL;
	ctx->G.word[3] = 0x6A54E920FDE8DA69L;
	/* set up for first message UBI */
	ctx->T.word[0] = 0L;
	ctx->T.word[1] = 0L;
	skein_set_first(&(ctx->T), 1);
	skein_set_final(&(ctx->T), 0);
	skein_set_type(&(ctx->T), SKEIN_TYPE_MSG);
	skein_set_pos(&(ctx->T), 0L);
	ctx->buff_bytes = 0;
}

void skein_update(skein_context_t *ctx, uint8_t *msg, uint32_t mlen)
{
	uint8_t i, j;
	uint32_t total_bytes, f, b;
	
	if (mlen == 0) return;

	total_bytes = mlen + (uint32_t)(ctx->buff_bytes);
	
	f = total_bytes / SKEIN_BLK_BYTES; /* full blocks */
	b = total_bytes % SKEIN_BLK_BYTES; /* last partial block bytes */
	if (b == 0) /* only full blocks */
	{
		/* save last full block for final */
		f -=1;
		b = SKEIN_BLK_BYTES;
	}
	if (f > 0)
	{
		/* handle buffered data first */
		for (i = ctx->buff_bytes; i < SKEIN_BLK_BYTES; i++)
		{
			ctx->buff.byte[i] = *(msg++);
		}
		UBI(&(ctx->G), &(ctx->T), ctx->buff.byte, SKEIN_BLK_BYTES);
		ctx->buff_bytes = 0; /* buffer used */
		f--;
		/* rest of full blocks */
		if (f)
		{
			UBI(&(ctx->G), &(ctx->T), msg, f * SKEIN_BLK_BYTES);
		}
		msg += f * SKEIN_BLK_BYTES;	
	}
	/* buffer last, possibly non-full block */
	for (i = 0; i < b; i++)
	{
		ctx->buff.byte[i] = *(msg++);
	}
	ctx->buff_bytes = b;
}

void skein_final(skein_context_t *ctx, uint8_t *hash)
{
	uint8_t i;
	skein_word_t tmp_msg;
	
	/* handle buffered bytes - there must be some */
	for (i = ctx->buff_bytes; i < SKEIN_BLK_BYTES; i++)
	{
		/* zero-fill */
		ctx->buff.byte[i] = 0;
	}
	skein_set_final(&(ctx->T), 1);
	UBI(&(ctx->G), &(ctx->T), ctx->buff.byte, ctx->buff_bytes);
	/* get output */
	skein_set_first(&(ctx->T), 1);
	skein_set_final(&(ctx->T), 1);
	skein_set_type(&(ctx->T), SKEIN_TYPE_OUT);
	skein_set_pos(&(ctx->T), 0L);
	tmp_msg.word[0] = 0L;
	tmp_msg.word[1] = 0L;
	tmp_msg.word[2] = 0L;
	tmp_msg.word[3] = 0L;
	UBI(&(ctx->G), &(ctx->T), tmp_msg.byte, sizeof(uint64_t));
	for (i = 0; i < SKEIN_BLK_BYTES; i++)
	{
		hash[i] = ctx->G.byte[i];
	}	
}

#ifdef SKEIN_PRINT
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
#endif
