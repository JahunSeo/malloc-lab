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

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t))) // ALIGN(8)


/* 
 * Basic constants and macros
 */

#define WSIZE 4  // word, header, footer의 크기 (단위 bytes)
#define DSIZE 8  // double word size (단위 bytes)
#define CHUNKSIZE (1<<12)  // 힙을 1회 확장할 때의 기본 크기 (단위 bytes)

#define MAX(x, y) ((x) > (y)? (x) : (y))

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
// - FTRP: 블록의 주소값(단위 bytes)에서 블록의 크기(N바이트)만큼 앞으로 이동한 뒤, 2개 워드 크기만큼 다시 뒤로 이동
#define HDRP(bp) ((char *)(bp) - WSIZE) 
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

// 블록의 주소값으로 이전 블록과 다음 블록의 주소값 찾기
// - NEXT_BLKP: 현재 블록의 헤더로 가서 현재 블록의 사이즈를 구한 다음, 현재 블록 주소값에 더해 다음 블록 위치로 이동
// - PREV_BLKP: 이전 블록의 풋터로 가서 이전 블록의 사이즈를 구한 다음, 현재 블록 주소값에서 빼 이전 블록 위치로 이동
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

// heap에서 포인터의 위치를 정적 변수로 정의
static char *heap_listp; 
// heap의 크기 기록: 크기가 증가할 때마다 업데이트
static size_t heap_size = 0; 

// 추가 함수 형상 정의
static void *extend_heap(size_t words);
static void *coalesce(void *bp);

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    heap_size = 0;
    // printf("***************************\n");
    // printf("[mm_init] %u\n", heap_size);
    // 비어 있는 가용 리스트(HEAP) 생성: 길이 4개 워드
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1) {
        return -1;
    }
    // HEAP에 패딩, 프롤로그, 에필로그 삽입
    // - 이 때, heap_listp의 자료형은 (char *)로 byte 단위  
    PUT(heap_listp, 0); // Alignment 패딩에 0 삽입: (*(unsigned int *)(heap_listp) = (0))
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1)); // 프롤로그 헤더에 8/1 삽입
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1)); // 프롤로그 풋터에 8/1 삽입 
    PUT(heap_listp + (3*WSIZE), PACK(0, 1)); // 에필로그 헤더에 0/1 삽입
    heap_listp += (2*WSIZE); // heap 주소값을 프롤로그 푸터 위치로 이동

    heap_size = 4;
    // 비어 있는 HEAP을 CHUNKSIZE(단위 bytes)만큼 확장
    // - 이 때, extend_heap은 입력값으로 필요한 워드의 개수를 받음
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL) {
        return -1;
    }
    // 미리 공간을 분할해두기
    // - 1차로 일단 1/2을 만들어보자
    char *bp = heap_listp;
    size_t size;



    return 0;
}

static void *divide_block(void* bp) {
    // block이 free이고 분리 가능한 크기인지 확인
    
    // 본래 가용공간(a) + 8 이므로, 새로운 가용공간은 (a-8)//2 이며, 새로운 사이즈는 a//2 + 4
}

/*
 * extend_heap: 필요한 워드의 개수를 입력 받아 HEAP을 확장
 */
static void *extend_heap(size_t words) {
    // printf("[extend_heap] %u\n", words * WSIZE);
    // 블록 주소값 초기화, 확장할 워드의 개수 초기화
    char *bp;
    size_t size; 
    // alignment를 유지하기 위해 워드 개수를 2의 배수로 반올림
    size = (words%2) ? (words+1) * WSIZE : words * WSIZE;
    // HEAP의 크기를 size(단위 bytes)만큼 확장
    // - mem_sbrk을 실행하고 그 결과값을 bp에 저장, bp를 long에 캐스팅, 그 값을 -1과 비교
    if ((long)(bp = mem_sbrk(size)) == -1) { 
        return NULL;
    }
    // 새로운 가용 블록의 헤더와 풋터
    // - 현재는 새로 추가되는 메모리 전체가 하나의 블록으로 처리됨
    // - 이런 상황에서는 새로운 블록을 할당할 때마다 남은 영역을 나눠주어야 함 (그렇지 않으면 블록을 1개 추가할 때마다 힙이 확장될 테니까)
    // - 과연 이게 효율적일까?
    // - TODO: 처음 힙을 확장할 때, 블록들을 어느 정도 분할해 두는 방안 고민해보기
    PUT(HDRP(bp), PACK(size, 0)); 
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); // 새로운 에필로그의 헤더
    // 이전 블록이 free 상태였다면 이전 블록과 결합
    bp = coalesce(bp);
    // heap 크기 업데이트
    heap_size += size;
    return bp;
}


void mm_free(void *bp) {
    // printf("\n[free] %u\n", GET_SIZE(HDRP(bp)));
    // 반환할 블록의 헤더에서 블록 사이즈 가져오기
    size_t size = GET_SIZE(HDRP(bp));
    // 반환할 블록의 헤더와 풋터를 업데이트: size/0
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    // 인접한 블록과 결합
    // TODO: free할 때마다 인접한 블록을 결합하는 것이 과연 효율적일까?
    coalesce(bp);
}

// 왜 static 이지??
static void *coalesce(void *bp) {
    // 이전 블록과 현재 블록의 할당 여부 체크 (자료형 char로 하면 안 되나?)
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    // 현재 블록의 사이즈
    size_t size = GET_SIZE(HDRP(bp));
    // CASE 1: 이전 블록 할당, 다음 블록 미할당
    if (prev_alloc && next_alloc) {
        return bp;
    }
    // CASE 2: 이전 블록 할당, 다음 블록 미할당
    else if (prev_alloc && !next_alloc) {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0)); // 주의! FTRP는 블록의 헤더에 담긴 사이즈를 활용해 계산되므로, 앞 라인에서 업데이트된 헤더를 활용함
    }
    // CASE 3: 이전 블록 미할당, 다음 블록 할당
    else if (!prev_alloc && next_alloc) {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp); // 주의! 새로운 bp로 업데이트 해주어야 함 (이 때, 현재 블록의 헤더는 아직 그대로 남아 있기 때문에 활용 가능)
    }
    // CASE 4: 이전 블록 미할당, 다음 블록 미할당
    else {
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    // printf("[coalesce] result: %u\n", GET_SIZE(HDRP(bp)));
    return bp;
}


char *find_fit(size_t size) {
    // 탐색 시작점 초기 설정
    // BestFit: 모든 가용 블록을 검사하며 크기가 맞는 가장 작은 블록을 선택
    char *bp = heap_listp; // 프롤로그의 포인트
    char *best_bp = NULL;
    // - GET_ALLOC(~) == 0 && GET_SIZE(~) >= size 인 블록
    while (GET_SIZE(HDRP(bp)) != 0) {
        bp = NEXT_BLKP(bp);
        // 현재 블록이 할당되어 있지 않고, 현재 블록의 사이즈가 찾는 크기 이상인 경우
        if (!GET_ALLOC(HDRP(bp)) && GET_SIZE(HDRP(bp)) >= size) {
            // 현재 블록의 사이즈가 지금까지의 최적 크기보다 작은 경우
            if (best_bp == NULL || GET_SIZE(HDRP(bp)) < GET_SIZE(HDRP(best_bp))) {
                best_bp = bp;
            } 
        }
    }
    
    if (best_bp == NULL) {
        // printf("[find_fit] fail to find best fit for %u\n", size);
    } else {
        // printf("[find_fit] found %u for %u: %p\n", GET_SIZE(HDRP(best_bp)), size, best_bp);
    }

    return best_bp;
}


void place(char *bp, size_t size) {
    // 기존 블록 크기 계산
    size_t orig_size = GET_SIZE(HDRP(bp));
    // printf("[place] before: %u %u\n", orig_size, size);
    // 남는 영역이 분할하기 어려울 경우
    if (orig_size - size < 2 * DSIZE) {
        // printf("  - case 1: %u\n", orig_size);
        // 기존에 남은 사이즈에 맞게 배치하기
        PUT(HDRP(bp), PACK(orig_size, 1));
        PUT(FTRP(bp), PACK(orig_size, 1));
    }
    // 남는 영역이 분할 가능할 경우 
    else {
        // printf("  - case 2: %u %u\n", size, orig_size - size);
        // 요청된 사이즈에 맞게 배치하기
        PUT(HDRP(bp), PACK(size, 1));
        PUT(FTRP(bp), PACK(size, 1));
        // 남은 영역 분할하기
        // - 현재 방식에서는 extend_heap 할 때 큰 덩어리를 하나의 블록으로 두기 때문에, 매번 분할해주어야 함
        // - TODO: 만약 미리 블록 크기를 분할해둔다면, 남은 영역 분할도 선택적으로 해볼 수 있음
        bp = NEXT_BLKP(bp);
        size = orig_size - size;
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    orig_size = GET_SIZE(HDRP(bp));
    // printf("[place] after: %u\n", orig_size);

} 


void *mm_malloc(size_t size) { // 바이트 단위
    // printf("\n[malloc] %u // %u\n", size, heap_size);

    size_t adj_size;  // alignment를 위해 조정된 블록 사이즈
    size_t ext_size;  // HEAP에 fit한 블록이 없을 때 HEAP을 확장할 사이즈
    char *bp;

    // 잘못된 요청 처리
    if (size <= 0) {
        return NULL;
    }
    // 블록 사이즈 조정하기
    // - 요청된 사이즈가 2개 워드 이하인 경우, 최소 4개의 워드(즉, 16 바이트)로 조정
    if (size <= DSIZE) {
        adj_size = 2*DSIZE;       
    } 
    // - 요청된 사이즈를 8의 배수가 될 수 있도록 조정
    else {
        adj_size = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);
    }
    // HEAP에서 가용한 블록 탐색
    if ((bp = find_fit(adj_size)) != NULL) {
        // printf(" - use existing block!\n");
        place(bp, adj_size);
        return bp;
    }
    // 가용한 블록이 없는 경우
    // - 이 때, 필요한 추가 메모리가 기본 확장 단위보다 커질 수 있으므로 ext_size로 조정
    // - adj_size와 CHUNKSIZE 모두 8의 배수이며, extend_heap이 정상 작동 시 요청된 사이즈를 넣을 공간이 마련됨
    ext_size = MAX(adj_size, CHUNKSIZE);
    if ((bp = extend_heap(ext_size/WSIZE)) == NULL) {
        return NULL;
    } 
    // printf(" - heap is extended! %u %u\n", adj_size, ext_size);
    place(bp, adj_size);
    return bp;
}



// /* 
//  * mm_malloc - Allocate a block by incrementing the brk pointer.
//  *     Always allocate a block whose size is a multiple of the alignment.
//  */
// void *mm_malloc(size_t size)
// {
//     int newsize = ALIGN(size + SIZE_T_SIZE);
//     void *p = mem_sbrk(newsize);
//     if (p == (void *)-1)
// 	       return NULL;
//     else {
//         *(size_t *)p = size;
//         return (void *)((char *)p + SIZE_T_SIZE);
//     }
// }


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
    copySize = GET_SIZE(HDRP(oldptr));
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}














