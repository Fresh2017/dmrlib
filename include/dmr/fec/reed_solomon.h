/**
 * @file
 * @brief  Reed-Solomon codes.
 * @author Phil Karn, KA9Q
 */
#ifndef _DMR_FEC_REED_SOLOMON_H
#define _DMR_FEC_REED_SOLOMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <stdbool.h>

/* Maimum number of errors that can be corrected. */
#define DMR_RS_MAX_TT 8

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int     mm;         /*!< Bits per symbol */
    int     nn;         /*!< Symbols per block (= (1<<mm)-1) */
    uint8_t *alpha_to;  /*!< Log lookup table */
    uint8_t *index_of;  /*!< Antilog lookup table */
    uint8_t gg[17];     /*!< Generator polynomial */
    uint8_t tt;         /*!< Number of errors that can be corrected */
    unsigned int n;     /*!< distance = nn-kk+1 = 2*tt+1 */
} dmr_rs_t;

extern dmr_rs_t *dmr_rs_new(unsigned int generator_polinomial, int mm, unsigned char tt);
extern void dmr_rs_encode(dmr_rs_t *rs, const unsigned char *data, unsigned char *bb);
extern int dmr_rs_decode(dmr_rs_t *rs, unsigned char *input, unsigned char *recd);

extern int dmr_rs_12_9_4_encode(uint8_t bytes[12], uint8_t crc_mask);
extern int dmr_rs_12_9_4_decode_and_repair(uint8_t bytes[12], uint8_t crc_mask);
extern int dmr_rs_12_9_4_decode_and_verify(uint8_t bytes[12], uint8_t crc_mask);

#ifdef __cplusplus
}
#endif

#endif // _DMR_FEC_REED_SOLOMON_H
