#include "tools/bitmap.h"
#include "tools/klib.h"
void bitmap_init(bitmap_t* bitm, uint8_t* bit, int count, int default_value)
{
    bitm -> count = count;
    bitm -> pbit = bit;
    int bytes = bit_count(bitm->count);
    kernel_memset(bitm->pbit, default_value ? 0xFF: 0, bytes);
}

int bit_count(int count){
    return (count + 8 - 1)/8;
}

int bitmap_get_index(bitmap_t* bitmap, int index)
{
    return bitmap->pbit[index/8] & (1 << (index % 8));
}

void bitmap_set_nbit(bitmap_t* bitmap, int index, int count, int bit)
{
    for(int i = 0; i < count&& index < bitmap->count; ++i, ++index){
        if(bit == 1)
        {
            bitmap->pbit[index/8] |= (1 << (index % 8));
        }
        else{
            bitmap->pbit[index/8] &= ~(1 << (index % 8));
        }
    }
}

int bitmap_is_set(bitmap_t* pbit, int index){
    return bitmap_get_index(pbit, index) ? 1 : 0;
}

int bitmap_alloc_nbit(bitmap_t* pbit, int count, int bit)
{
    int search_idx = 0;
    int ok_index = -1;
    
    while(search_idx < pbit->count){
        if(bitmap_get_index(pbit, search_idx) != bit)
        {
            ++search_idx;
            continue;
        }
        ok_index  = search_idx;
        int i = 1;
        for(; i < count && search_idx < pbit->count; ++i){
            if(bitmap_get_index(pbit, search_idx++) != bit)
            {
                ok_index = -1;
                break;
            }      
        }
        if(i >= count){
            bitmap_set_nbit(pbit, ok_index, count, !bit);
            return ok_index;
        } 
    }     
    
    
    return -1;
}