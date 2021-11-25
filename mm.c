/*
 * mm-naive.c - The least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by allocating a
 * new page as needed.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused.
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

/* always use 16-byte alignment */
#define ALIGNMENT 16

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1))

/* rounds up to the nearest multiple of mem_pagesize() */
#define PAGE_ALIGN(size) (((size) + (mem_pagesize() - 1)) & ~(mem_pagesize() - 1))

// This assumes you have a struct or typedef called "block_header" and "block_footer"
#define OVERHEAD (sizeof(block_header) + sizeof(block_footer))

// Given a payload pointer, get the header or footer pointer
#define HDRP(bp) ((char *)(bp) - sizeof(block_header))
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - OVERHEAD)

// ******These macros assume you are using a struct for headers and footers*******

// Given a header pointer, get the alloc or size
#define GET_ALLOC(p) ((block_header *)(p))->allocated
#define GET_SIZE(p) ((block_header *)(p))->size

// Given a pointer to a header, get or set its value
#define GET(p) (*(size_t *)(p))
#define PUT(p, val) (*(size_t *)(p) = (val))

// Combine a size and alloc bit
#define PACK(size, alloc) ((size) | (alloc))

// Given a payload pointer, get the next or previous payload pointer
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE((char *)(bp)-OVERHEAD))

// Helper functions
static void *set_allocated(void *b, size_t size);
static void extend(size_t s);
static void remove_block_from_list(void* allocated, void *free_block);
static void set_new_free_block(void *free_block);
static void *find_block(void *free_block, size_t);
static void check_free_list();

// Struct that will hold the list of pages
typedef struct free_list
{
  struct free_list *next;
  struct free_list *prev;

} free_list;

free_list *head = NULL;

void *head_free_list; // head of the list
int current_avail_size = 0;
void *current_avail;

typedef struct
{
  size_t size;
  char allocated;
} block_header;

typedef struct
{
  size_t size;
  int filler;
} block_footer;

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
  // init should just restore
  // restore the pointers
  // reset the allocator
  head = NULL; 
  current_avail_size = 0;
  return 0;
}

void *mm_malloc(size_t size)
{

  check_free_list();
  if (size == 0)
  {
    return NULL;
  }
  if (head == NULL)
  {
    extend(size);
  }

  free_list *current = head;
  int newsize;
  void *p = NULL;
  newsize = ALIGN(size + OVERHEAD);
  while (current != NULL)
  {
    int alloc = GET_ALLOC(HDRP(current));
    int size = GET_SIZE(HDRP(current));
    if (!GET_ALLOC(HDRP(current)) && (GET_SIZE(HDRP(current)) >= newsize))
    {
      int allocation = GET_ALLOC(HDRP(current));
      int ss = GET_SIZE(HDRP(current));

      current = set_allocated(current, newsize);
      return current;
    }
    current = current->next;
  }
  extend(newsize);
  // if we reach here current is NULL
  p = set_allocated(p, newsize);

  return p;
}

static void *set_allocated(void *b, size_t size)
{
  if(b == NULL)
  {
    b = find_block(b, size); 
  }
  size_t extra_size = GET_SIZE(HDRP(b)) - size;
  if (extra_size > ALIGN(1 + OVERHEAD))
  {
    GET_SIZE(HDRP(b)) = size;
    GET_SIZE(HDRP(NEXT_BLKP(b))) = extra_size;
    GET_ALLOC(HDRP(NEXT_BLKP(b))) = 0;
    GET_SIZE(FTRP(NEXT_BLKP(b))) = extra_size;
  }
  GET_ALLOC(HDRP(b)) = 1;
  GET_SIZE(FTRP(b)) = size; 
  void *new_free_block = NEXT_BLKP(b);
  int alloc = GET_ALLOC(HDRP(new_free_block));
  int ss = GET_SIZE(HDRP(new_free_block));
  check_free_list();
  remove_block_from_list(b, new_free_block);

  return b;
}

static void extend(size_t s)
{
  /**
   * If non of the free blocks can hold the incoming payload
   * then we need to create a new page. 
   * 
   * Page will be free block and will need to be connected to the next
   * of the head. 
   */

  //int newsize = ALIGN(s + OVERHEAD); Use this inside malloc
  current_avail_size = PAGE_ALIGN(s*4 + OVERHEAD + 16);
  void *new_page = mem_map(current_avail_size);
  new_page += 16;
  GET_ALLOC(HDRP(new_page)) = 1; // prolog header block
  GET_SIZE(HDRP(new_page)) = 32;
  GET_ALLOC(FTRP(new_page)) = 1; // prolog footer
  GET_SIZE(FTRP(new_page)) = 32;

  new_page += OVERHEAD;

  GET_SIZE(HDRP(new_page)) = current_avail_size - 48; // sets the free space
  GET_ALLOC(HDRP(new_page)) = 0;
  GET_SIZE(FTRP(new_page)) = current_avail_size - 48;
  int size_of_block = GET_SIZE(HDRP(new_page));

  GET_ALLOC(HDRP(NEXT_BLKP(new_page))) = 1; // terminator
  GET_SIZE(HDRP(NEXT_BLKP(new_page))) = 0;

  if (head == NULL)
  {

    head = new_page;
    head->next = NULL;
    head->prev = NULL;
    check_free_list();
  }

  else
  {
    set_new_free_block(new_page);
    check_free_list();
  }
}

static void remove_block_from_list(void* allocated, void *free_block)
{

  int allocation = GET_ALLOC(HDRP(free_block));
  int ss = GET_SIZE(HDRP(free_block));
  int allocation = GET_ALLOC(HDRP(allocated));
  int ss2 = GET_SIZE(HDRP(allocated));
  free_list *current = head;
  if(allocated == head)
  {
    free_list *head_next = head->next; 
    free_list *f = free_block; 
    head = f;
    head->next = head_next;
    head->prev = NULL; 
    check_free_list();
    return; 
  }

  free_list *allocated_next = ((free_list*)allocated)->next;
  free_list *allocated_prev = ((free_list*)allocated)->prev;
  free_list *free = free_block;
  int alloc = GET_ALLOC(HDRP(free));
  int size = GET_SIZE(HDRP(free));
  allocated_prev->next = free; 
  free->prev = allocated_prev;
  free->next = allocated_next; 
  check_free_list();
  return; 

}

static void* find_block(void *free_block, size_t size)
{
  free_list* current = head;
  if(head == NULL)
  {
    return NULL;
  }

  while (current != NULL)
  {
    if (!GET_ALLOC(HDRP(current)) && (GET_SIZE(HDRP(current)) >= size))
    {
      int allocation = GET_ALLOC(HDRP(current));
      int ss = GET_SIZE(HDRP(current));
      return current;
    }
    current = current->next;
  }
  return current;
}

static void set_new_free_block(void *free_block)
{
  free_list *current = head;
  while(current->next != NULL)
  {
    current = current->next;
  }
  free_list *block = free_block; 
  current->next = block;
  block->prev = current;
  block->next = NULL;
  int alloc = GET_ALLOC(HDRP(block));
  int size = GET_SIZE(HDRP(block));
  check_free_list(); 
  return;
}



/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
  return;
}

static void check_free_list()
{
  int index = 0;
  if(head == NULL)
  {
    printf("List is empty\n");
    return; 
  }
  free_list* current = head;
  while(current != NULL)
  {
    int ss = GET_SIZE(HDRP(current));
    printf("----%d----\n", index);
    printf("allocation: %d\n ", GET_ALLOC(HDRP(current)));
    printf("size: %d \n", ss);
    printf("Pointer: %p \n", current);
    
    current = current->next; 
    index++;
  }
}
