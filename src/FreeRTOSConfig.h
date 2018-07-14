
#define configUSE_PREEMPTION 0

#define configMINIMAL_STACK_SIZE 128

#define configTOTAL_HEAP_SIZE		( ( size_t ) ( 32 * 1024 ) )

/* Use the defaults for everything else */
#include_next<FreeRTOSConfig.h>
