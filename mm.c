/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "1조",
    /* First member's full name */
    "김민우/이재윤/서자헌",
    /* First member's email address */
    "hello@mail.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

// /* single word (4) or double word (8) alignment */
// #define ALIGNMENT 8

// /* rounds up to the nearest multiple of ALIGNMENT */
// #define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

// #define SIZE_T_SIZE (ALIGN(sizeof(size_t)))


/* 
 * Basic constants and macros
 */
#define WSIZE = 4  // word, header, footer의 크기 (단위 bytes)
#define DSIZE = 8  // double word size (단위 bytes)
#define CHUNKSIZE = (1<<12)  // 힙을 1회 확장할 때의 기본 크기 (단위 bytes)

#define MAX(x, y) ((x) < (y)? (x) : (y))

// 헤더와 풋터를 저장할 정보를 워드 1개 크기로 묶기(pack)
// - 크기 8의 블록 free:       0b1000 | 0 == 0b1000 == 8 --> 8/0
// - 크기 8의 블록 allocated:  0b1000 | 1 == 0b1001 == 9 --> 8/1
#define PACK(size, alloc) ((size) | (alloc)) 

// 주소값 p의 워드에 데이터 읽고 쓰기
// - 워드 1개의 크기가 4바이트이기 때문에 주소값 p의 자료형을 4바이트인 int로 캐스팅한 뒤, 그 값을 읽음
// - GET은 헤더와 풋터를 워드 1개 크기(PACK된 상태)로 기록된 정보를 읽을 때 사용
// - PUT은 헤더와 풋터를 워드 1개 크기(PACK된 상태)로 정보를 기록할 때 사용 (val은 PACK된 상태여야 함)
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

// 주소값 p의 워드에 저장된 데이터에서 블록의 크기와 할당 여부 가져오기
// - 사이즈 가져오는 과정
//   - 8/0 일 때: 0b1000 & 0b1111111111110111 == 0b1000
//   - 8/1 일 때: 0b1001 & 0b1111111111110111 == 0b1000
// - 할당 여부 가져오는 과정
//   - 8/0 일 때: 0b1000 & 0b0000 == 0
//   - 8/1 일 때: 0b1000 & 0b0001 == 1
#define GET_SIZE(p) (GET(p) & ~0x7) // (단위 bytes)
#define GET_ALLOC(p) (GET(p) & 0x1)

// 블록의 주소값으로 해당 블록의 헤더 주소값과 푸터 주소값 찾기
// - HDRP: 블록의 주소값(단위 bytes)에서 1개 워드 크기(4바이트)만큼 뒤로 이동
// - FDRP: 블록의 주소값(단위 bytes)에서 블록의 크기(~바이트)만큼 앞으로 이동한 뒤, 2개 워드 크기만큼 다시 뒤로 이동
#define HDRP(bp) ((char *)(bp) - WSIZE) 
#define FDRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

// 블록의 주소값으로 이전 블록과 다음 블록의 주소값 찾기
// - NEXT_BLKP: 현재 블록의 헤더로 가서 현재 블록의 사이즈를 구한 다음, 현재 블록 주소값에 더해 다음 블록 위치로 이동
// - PREV_BLKP: 이전 블록의 풋터로 가서 이전 블록의 사이즈를 구한 다음, 현재 블록 주소값에서 빼 이전 블록 위치로 이동
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
	return NULL;
    else {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}














