/**
 * @file        mmu.h
 * @author      Carlos Fernandes
 * @version     3.0
 * @date        25 May, 2020
 * @brief       Kernel MMU BSP Interface
*/

#ifndef _MMU_H_
#define _MMU_H_

#ifdef __cplusplus
    extern "C" {
#endif


/* Includes ----------------------------------------------- */
#include <types.h>
//#include <mmu_arch.h> ?
// -> PAGE_SIZE
// -> USER_PGT_SIZE


/* Exported types ----------------------------------------- */
typedef void* pgt_t;

typedef struct
{
    uint8_t     cpolicy;
    uint8_t     apolicy;
    uint8_t     shared;
    uint8_t     executable;
    uint8_t     global;
}memCfg_t;

// Page Buffer Vector
typedef struct
{
    ptr_t  data;
    size_t size;
}pbv_t;

/* Exported constants ------------------------------------- */
#define PAGE_SIZE           (4096)

enum
{
    CPOLICY_STRONGLY_ORDERED = 0,
    CPOLICY_UNCACHED,
    CPOLICY_WRITETHROUGH,
    CPOLICY_WRITEBACK,
    CPOLICY_WRITEALLOC,
    CPOLICY_DEVICE_SHARED,
    CPOLICY_DEVICE_PRIVATE,
};

enum
{
    APOLICY_NANA = 0,       // No access
    APOLICY_RWNA,           // Kernel read an write, User no access
    APOLICY_RWRO,           // Kernel read an write, User read only
    APOLICY_RWRW,           // Kernel read an write, User read an write
    APOLICY_RONA,           // Kernel read only, User no access
    APOLICY_RORO,           // Kernel read only, User read only
};

/* Exported macros ---------------------------------------- */



/* Exported functions ------------------------------------- */

/*
 * @brief   Get Kernel page table
 * @param   None
 * @retval  Kernel page table (physicall address)
 */
pgt_t MMU_KernelPGT(void);

/*
 * @brief   Get User mode page table
 * @param   None
 * @retval  User mode table (physicall address)
 */
pgt_t MMU_UserPGT(void);

/*
 * @brief   Maps the specified virtual address space with the specified memory configuration
 * @param   pgt - page table
 *          vaddr - virtual address
 *          pages - physical pages to be mapped
 *          count - number of pages present in the page buffer vector "pages"
 *          memCfg - Memory configuration
 * @retval  No Return
 */
void MMU_MapPages(pgt_t pgt, vaddr_t vaddr, pbv_t* pages, size_t count, memCfg_t* memCfg);

/*
 * @brief   Unmaps the specified virtual address space
 * @param   pgt - page table
 *          vaddr - virtual address
 *          size - size of the address space
 * @retval  No Return
 */
void MMU_UnmapPages(pgt_t pgt, vaddr_t vaddr, size_t size);

/*
 * @brief   Allocates a new page table
 *          Only User level page tables will be allocated
 * @param   None
 * @retval  Returns a new page table
 */
pgt_t MMU_AllocPGT(void);

/*
 * @brief   Deallocates the specified page table.
 *          This function should only be called after MMU_InvalidatePGT()
 * @param   pgt - page table
 * @retval  No return
 */
void MMU_FreePGT(pgt_t pgt);

/*
 * @brief   Set all page tables entries to invalide
 *          All lower level page tables will be deallocated
 * @param   pgt - page table
 * @retval  No return
 */
void MMU_InvalidatePGT(pgt_t pgt);

/*
 * @brief   Translate a virtual address to a physical address using the given page table
 * @param   pgt - page table
 *          vaddr - virtual address
 * @retval  Returns the physical address
 */
paddr_t MMU_V2P(pgt_t pgt, vaddr_t vaddr);

/*
 * @brief   Translate a logical address to a physical address
 * @param   vaddr - logical address (virtual address inside kernel virtual space)
 * @retval  Returns the physical address
 */
paddr_t MMU_L2P(vaddr_t vaddr);

/*
 * @brief   Translate a physical address to a logical address
 * @param   paddr - physical address
 * @retval  Returns the logical address (virtual address inside kernel virtual space)
 */
vaddr_t MMU_P2L(paddr_t paddr);

#ifdef __cplusplus
    }
#endif

#endif
