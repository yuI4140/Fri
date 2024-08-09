#pragma once
#include <stdbool.h>
typedef unsigned long size_t;
typedef unsigned int uint;
#define  BYTE_SIZE sizeof(char)
typedef struct ByteRange {
   int n; 
   size_t ranges[4];
   size_t count;
}ByteRange;
extern ByteRange br_init();
extern bool br_push(ByteRange *br,uint v);
extern uint br_extract(const ByteRange *br,uint v);
extern uint br_len_byte(const ByteRange *br,uint v);
