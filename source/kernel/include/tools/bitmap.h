#ifndef BITMAP_H
#define BITMAP_H

#include "comm/types.h"
typedef struct _bitmap_t
{
    int count;
    uint8_t* pbit;
}bitmap_t;
int bit_count(int count);
void bitmap_init(bitmap_t* bitm, uint8_t* bit, int count, int default_value);
int bitmap_get_index(bitmap_t* pbit, int index);
void bitmap_set_nbit(bitmap_t* pbit, int index, int count, int bit);
int bitmap_is_set(bitmap_t* pbit, int index);
int bitmap_alloc_nbit(bitmap_t* pbit, int count, int bit);
#endif