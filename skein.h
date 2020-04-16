/* Skein256 hash as described in the paper "skein 1.3" */

#include <stdint.h>

#define SKEIN_BLK_BYTES 32

/* types */
#define SKEIN_TYPE_KEY  0
#define SKEIN_TYPE_CFG  4
#define SKEIN_TYPE_PRS  8
#define SKEIN_TYPE_PK  12
#define SKEIN_TYPE_KDF 16
#define SKEIN_TYPE_NON 20
#define SKEIN_TYPE_MSG 48
#define SKEIN_TYPE_OUT 63

typedef union {
	uint64_t word[4];
	uint8_t byte[SKEIN_BLK_BYTES];
} skein_word_t;

/* don't use - bit order may be wrong */
typedef struct {
	uint64_t final:1;
	uint64_t first:1;
	uint64_t type:6;
	uint64_t bit_pad:1;
	uint64_t tree_level:7;
	uint64_t reserved:16;
	uint64_t pos_hi: 32;
	uint64_t pos:64;
} skein_tweak_bits_t;

typedef struct {
	uint8_t schema_id[4];
	uint8_t version[2];
	uint8_t reserved1[2];
	uint8_t outlen[8];
	uint8_t tree_leaf_sz_enc;
	uint8_t tree_fanout_enc;
	uint8_t max_tree_hght;
	uint8_t reserved2[13];
} skein_conf_str_t;

typedef union {
	uint8_t byte[16];
	uint64_t word[2];
} skein_tweak_t;

typedef struct {
	skein_word_t G;
	skein_tweak_t T;
	skein_word_t buff;
	uint8_t buff_bytes;
} skein_context_t;

void mk_skein_block_b(skein_word_t *w, uint8_t *s);
void mk_skein_tweak_b(skein_tweak_t *w, uint8_t *s);
void skein_set_final(skein_tweak_t *w, uint8_t val);
void skein_set_first(skein_tweak_t *w, uint8_t val);
void skein_set_type(skein_tweak_t *w, uint8_t val);
void skein_set_pos(skein_tweak_t *w, uint64_t val);
uint64_t skein_get_pos(skein_tweak_t *w);

void skein_hash(uint8_t *hash, uint8_t *M, uint32_t mlen);
void skein_init(skein_context_t *ctx);
void skein_update(skein_context_t *ctx, uint8_t *msg, uint32_t mlen);
void skein_final(skein_context_t *ctx, uint8_t *hash);

/* for testing and debugging */
void threefish(skein_word_t *K, skein_tweak_t *T, skein_word_t *P, skein_word_t *E);
void UBI(skein_word_t *G, skein_tweak_t *T, uint8_t *M, uint32_t mlen);
void skein_parts_test(void);
