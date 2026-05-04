// sysmem.c - STM32CubeIDE System Memory calls file

#include <errno.h>
#include <stdint.h>

static uint8_t *__sbrk_heap_end = NULL;

// _sbrk() allocates memory to the newlib heap and is used by malloc
void *_sbrk(ptrdiff_t incr)
{
  extern uint8_t _end; 
  extern uint8_t _estack; 
  extern uint32_t _Min_Stack_Size; 
  const uint32_t stack_limit = (uint32_t)&_estack - (uint32_t)&_Min_Stack_Size;
  const uint8_t *max_heap = (uint8_t *)stack_limit;
  uint8_t *prev_heap_end;

  if (NULL == __sbrk_heap_end)
  {
    __sbrk_heap_end = &_end;
  }

  if (__sbrk_heap_end + incr > max_heap)
  {
    errno = ENOMEM;
    return (void *)-1;
  }

  prev_heap_end = __sbrk_heap_end;
  __sbrk_heap_end += incr;

  return (void *)prev_heap_end;
}
