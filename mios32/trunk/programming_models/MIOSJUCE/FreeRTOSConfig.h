/*
	FreeRTOS Emu
*/

// Much of this is probably unneeded but it won't hurt


#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* Library includes. */
// unneeded for JUCE emulation
// #include <stm32f10x_lib.h>

// MIOS32 specific predefines - can be overruled in your local mios32_config.h file
#include "mios32_config.h"

#ifndef MIOS32_MINIMAL_STACK_SIZE
#define MIOS32_MINIMAL_STACK_SIZE 1024
#endif

#ifndef MIOS32_HEAP_SIZE
#define MIOS32_HEAP_SIZE 10*1024
#endif


/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
 * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE.
 *----------------------------------------------------------*/

#define configUSE_PREEMPTION		1
#define configUSE_IDLE_HOOK		1
#define configUSE_TICK_HOOK		1
#define configCPU_CLOCK_HZ		( ( unsigned portLONG ) 72000000 )	
#define configTICK_RATE_HZ		( ( portTickType ) 1000 )
#define configMAX_PRIORITIES		( ( unsigned portBASE_TYPE ) 5 )
#define configMINIMAL_STACK_SIZE	( ( unsigned portSHORT ) (MIOS32_MINIMAL_STACK_SIZE/4) )
#define configTOTAL_HEAP_SIZE		( ( size_t ) ( MIOS32_HEAP_SIZE ) )
#define configMAX_TASK_NAME_LEN		( 16 )
#define configUSE_TRACE_FACILITY	0
#define configUSE_16_BIT_TICKS		0
#define configIDLE_SHOULD_YIELD		1
#define configUSE_MUTEXES		1

/* Co-routine definitions. */
#define configUSE_CO_ROUTINES 		0
#define configMAX_CO_ROUTINE_PRIORITIES ( 2 )

/* Set the following definitions to 1 to include the API function, or zero
to exclude the API function. */

#define INCLUDE_vTaskPrioritySet	1
#define INCLUDE_uxTaskPriorityGet	1
#define INCLUDE_vTaskDelete		1
#define INCLUDE_vTaskCleanUpResources	0
#define INCLUDE_vTaskSuspend		1
#define INCLUDE_vTaskDelayUntil		1
#define INCLUDE_vTaskDelay		1

/* This is the raw value as per the Cortex-M3 NVIC.  Values can be 255
(lowest) to 0 (1?) (highest). */
#define configKERNEL_INTERRUPT_PRIORITY 		255
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 	191 /* equivalent to 0xa0, or priority 5. */


/* This is the value being used as per the ST library which permits 16
priority values, 0 to 15.  This must correspond to the
configKERNEL_INTERRUPT_PRIORITY setting.  Here 15 corresponds to the lowest
NVIC value of 255. */
#define configLIBRARY_KERNEL_INTERRUPT_PRIORITY	15

#endif /* FREERTOS_CONFIG_H */

