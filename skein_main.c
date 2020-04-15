#include <stdio.h>
#include "skein.h"

/* test program for the skein256 hash */

//#define SKEIN_PARTS_TESTS
//#define SKEIN_TEST_MSG1
//#define SKEIN_TEST_MSG2
#define SKEIN_TEST_MSG3

uint8_t hash[SKEIN_BLK_BYTES];

#if defined(SKEIN_TEST_CONFIG) || defined(SKEIN_TEST_MSG1)
uint8_t msg[] = {0xFF};
#define SKEIN_MSG_LEN 1
#define TST_MSG "MSG1"
#endif

#ifdef SKEIN_TEST_MSG2
uint8_t msg[] = {
0xFF, 0xFE, 0xFD, 0xFC, 0xFB, 0xFA, 0xF9, 0xF8, 0xF7, 0xF6, 0xF5, 0xF4, 0xF3, 0xF2, 0xF1, 0xF0,
0xEF, 0xEE, 0xED, 0xEC, 0xEB, 0xEA, 0xE9, 0xE8, 0xE7, 0xE6, 0xE5, 0xE4, 0xE3, 0xE2, 0xE1, 0xE0
};
#define SKEIN_MSG_LEN 32
#define TST_MSG "MSG2"
#endif

#ifdef SKEIN_TEST_MSG3
uint8_t msg[] = {
0xFF, 0xFE, 0xFD, 0xFC, 0xFB, 0xFA, 0xF9, 0xF8, 0xF7, 0xF6, 0xF5, 0xF4, 0xF3, 0xF2, 0xF1, 0xF0,
0xEF, 0xEE, 0xED, 0xEC, 0xEB, 0xEA, 0xE9, 0xE8, 0xE7, 0xE6, 0xE5, 0xE4, 0xE3, 0xE2, 0xE1, 0xE0,
0xDF, 0xDE, 0xDD, 0xDC, 0xDB, 0xDA, 0xD9, 0xD8, 0xD7, 0xD6, 0xD5, 0xD4, 0xD3, 0xD2, 0xD1, 0xD0,
0xCF, 0xCE, 0xCD, 0xCC, 0xCB, 0xCA, 0xC9, 0xC8, 0xC7, 0xC6, 0xC5, 0xC4, 0xC3, 0xC2, 0xC1, 0xC0
};
#define SKEIN_MSG_LEN 64
#define TST_MSG "MSG3"
#endif

void skein_prthash(uint8_t *s)
{
	uint8_t i;

	for (i = 0; i < SKEIN_BLK_BYTES; i++)
	{
		if (i % (SKEIN_BLK_BYTES / 2) == 0) printf("\n");
		printf(" %02X", s[i]);
	}
	printf("\n");
}

int main()
{
#ifdef SKEIN_PARTS_TESTS	
	skein_parts_test();
#else  /*SKEIN_PARTS_TESTS */
	skein_hash(hash, msg, SKEIN_MSG_LEN);
    printf("\nTesting with %s", TST_MSG);
    printf("\nResult is:");
	skein_prthash(hash);
	printf("\nResult should be:\n");

#ifdef SKEIN_TEST_MSG1
	printf(" 0B 98 DC D1 98 EA 0E 50 A7 A2 44 C4 44 E2 5C 23\n");
	printf(" DA 30 C1 0F C9 A1 F2 70 A6 63 7F 1F 34 E6 7E D2\n");
#endif
#ifdef SKEIN_TEST_MSG2
	printf(" 8D 0F A4 EF 77 7F D7 59 DF D4 04 4E 6F 6A 5A C3\n");
	printf(" C7 74 AE C9 43 DC FC 07 92 7B 72 3B 5D BF 40 8B\n");
#endif
#ifdef SKEIN_TEST_MSG3
	printf(" DF 28 E9 16 63 0D 0B 44 C4 A8 49 DC 9A 02 F0 7A\n");
	printf(" 07 CB 30 F7 32 31 82 56 B1 5D 86 5A C4 AE 16 2F\n");
#endif
#endif /*SKEIN_PARTS_TESTS */
	return 0;
}
