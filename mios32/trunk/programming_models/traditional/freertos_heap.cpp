// $Id: freertos_heap.cpp 989 2010-04-07 18:48:20Z philetaylor $
//////////////////////////////////////////////////////////////////////////////
// as proposed by http://www.state-machine.com/arm/AN_QP_and_ARM7_ARM9-GNU.pdf
//
// but instead of disabling malloc and free, we overload this with FreeRTOS
// based versions.
// In addition, we support calloc
// TODO: check if additional functions have to be overloaded
//////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>                   // for prototypes of malloc() and free()

#include <FreeRTOS.h>
#include <portmacro.h>

//............................................................................
#include <stdlib.h>	// for prototypes of malloc(), calloc() and free()

//............................................................................
extern "C" void *malloc(size_t size)
{
  return pvPortMalloc(size);
}

//............................................................................
extern "C" void *calloc(size_t count, size_t size)
{
  return pvPortMalloc(count*size);
}

//............................................................................
#if 0 // Commented out as umm_malloc has a dedicated realloc function.
extern "C" void *realloc(void *p, size_t size)
{
  vPortFree(p);
  p = pvPortMalloc(size);
  return p;
}
#endif
//............................................................................
extern "C" void free(void *p)
{
  vPortFree(p);
}
