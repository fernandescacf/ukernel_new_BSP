/**
 * @file        pmu.h
 * @author      Carlos Fernandes
 * @version     1.0
 * @date        14 February, 2021
 * @brief       ARMv7-A Performance Monitor Unit Header File
*/

#ifndef _PMU_H_
#define _PMU_H_

#ifdef __cplusplus
    extern "C" {
#endif


/* Includes ----------------------------------------------- */
#include <types.h>

/* Exported types ----------------------------------------- */


/* Exported constants ------------------------------------- */


/* Exported macros ---------------------------------------- */

#define PERFORMANCE_MONITORING_START(count)     \
    count = pmu_get_cyclecount()

#define PERFORMANCE_MONITORING_STOP(count)      \
    count = pmu_get_cyclecount() - count

/* Exported functions ------------------------------------- */

void pmu_int_perfcounters(uint32_t do_reset, uint32_t enable_divider);

uint32_t pmu_get_cyclecount(void);

#ifdef __cplusplus
    }
#endif

#endif /* _PMU_H_ */
