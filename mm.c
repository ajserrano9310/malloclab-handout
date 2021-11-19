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
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

/* rounds up to the nearest multiple of mem_pagesize() */
#define PAGE_ALIGN(size) (((size) + (mem_pagesize()-1)) & ~(mem_pagesize()-1))

// This assumes you have a struct or typedef called "block_header" and "block_footer"
#define OVERHEAD (sizeof(block_header)+sizeof(block_footer))

// Given a payload pointer, get the header or footer pointer
#define HDRP(bp) ((char *)(bp) - sizeof(block_header))
#define FTRP(bp) ((char *)(bp)+GET_SIZE(HDRP(bp))-OVERHEAD)

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
static void set_allocated(void *b, size_t size);
static void extend(size_t s);

// Struct that will hold the list of pages
typedef struct free_list
{
  struct free_list *next;
  void *ptr;
};

void *current_avail = NULL; 
struct free_list *head; // Head of the explicit free list 
void *first_bp; // head of the list 
int current_avail_size = 0;


 typedef struct 
{
  size_t size;
  char allocated;
}block_header;

 typedef struct 
{
  size_t size;
  int filler;
}block_footer;

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
  // init should just restore 
  // restore the pointers 
  // reset the allocator 
  head->next = NULL;
  head->ptr = NULL;
  current_avail = NULL;
  current_avail_size = 0;
  return 0;

}

void *mm_malloc(size_t size)
{

  if(size == 0)
  {
    return NULL;
  }
  /**
   * If head is null, then go ahead and set up 
   * stuff. 
   * Also, this is the extend method. 
  */
  int newsize;
  void *p;

  if(head->ptr == NULL)
  {
    /**
     * Initialize everything if the current head of the free
     * list is null. 
     * 
     * Set prolog header and footer and terminator. 
     * 
     * Initialize head of the explicit free list
    */
    newsize = ALIGN(size);
    current_avail_size = PAGE_ALIGN(newsize);
    current_avail = mem_map(current_avail_size);
    current_avail +=  16;
    GET_ALLOC(HDRP(current_avail)) = 1; // prolog header block
    GET_SIZE(HDRP(current_avail)) = 32;

    GET_ALLOC(FTRP(current_avail)) = 1; // prolog footer
    GET_SIZE(FTRP(current_avail)) = 32;

    current_avail += 32; // getting to the start of the free space

    head->ptr = current_avail; /* Set head of free list to the beginning */ 
    head->next = NULL;

    GET_SIZE(HDRP(current_avail)) = current_avail_size - 48; // sets the free space
    GET_ALLOC(HDRP(current_avail)) = 0; 

    /**
     * Set prolog footer 
    */
    GET_ALLOC(HDRP(NEXT_BLKP(current_avail))) = 1; 
    GET_SIZE(HDRP(NEXT_BLKP(current_avail))) = 0; // this is just to see if Github is working
    
  }
  else{
    /**
     * We need to check if we have enough space available for new block 
     * inside the free list
     * 
     * Traverse the Explicit Free List until it either finds
     * there's no free blocks and will call extedn, or finds one
     * and gets allocated
    */
    newsize = ALIGN(size);
    struct free_list *current_pointer = head;
    while(current_pointer != NULL)
    {
      void *free_block = current_pointer->ptr;

      if(!GET_ALLOC(HDRP(free_block)) && (GET_SIZE(HDRP(free_block)) >= newsize))
      {
        set_allocated(free_block, newsize);
        return free_block;

      }
      current_pointer = current_pointer->next;
    }
  }

  extend(size); // no free blocks found, then we extend

/*
  if (current_avail_size < newsize) {
    
    if (current_avail == NULL)
      return NULL;
  }

  p = current_avail; // to the next available size
  current_avail += newsize;
  current_avail_size -= newsize;
  */
  
  return p;
}

static void set_allocated(void *b, size_t size)
{

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

  int newsize = ALIGN(s);
  current_avail_size = PAGE_ALIGN(newsize);
  void* new_page = mem_map(PAGE_ALIGN(newsize));
  new_page +=  16;
  GET_ALLOC(HDRP(new_page)) = 1; // prolog header block
  GET_SIZE(HDRP(new_page)) = 32;
  GET_ALLOC(FTRP(new_page)) = 1; // prolog footer
  GET_SIZE(FTRP(new_page)) = 32;
  new_page += 32;

  GET_SIZE(HDRP(new_page)) = PAGE_ALIGN(newsize) - 48; // sets the free space
  GET_ALLOC(HDRP(new_page)) = 0; 

  GET_ALLOC(HDRP(NEXT_BLKP(new_page))) = 1; 
  GET_SIZE(HDRP(NEXT_BLKP(new_page))) = 0;

  struct free_list *next_page;
  next_page->next = NULL;
  next_page->ptr = new_page;

  struct free_list *b = head;
  while(b->next != NULL)
  {
    b = b->next;
  }
  b->next = next_page;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
}
