/**
 * @file        mmu.c
 * @author      Carlos Fernandes
 * @version     3.0
 * @date        25 May, 2020
 * @brief       Kernel MMU BSP Implementation
*/


/* Includes ----------------------------------------------- */
#include <mmu.h>
#include <misc.h>


/* Private types ------------------------------------------ */


/* Private constants -------------------------------------- */
// L1 Page Table Entries Identifiers
#define FAULT           (0x0)
#define L2_PGT          (0x1)
#define SECTION         (0x2)
#define SUPERSECTION    ((1 << 18)|(0x2))
// L2 Page Table Entries Identifiers
#define LARGEPAGE       (0x1)
#define SMALLPAGE       (0x2)

// MMU map sizes
#define LARGE_SECTION_SIZE  0x1000000
#define SECTION_SIZE        0x100000
#define LARGE_PAGE_SIZE     0x10000
#define SMALL_PAGE_SIZE     0x1000

// MMU L1 Entry Flags
#define MMU_L1_B        (1 << 2)    // pte[2]     -> B   - Write Buffer
#define MMU_L1_C        (1 << 3)    // pte[3]     -> C   - Cache
#define MMU_L1_XN       (1 << 4)    // pte[4]     -> XN  - Execute Never
#define MMU_L1_AP0      (1 << 10)   // pte[11:10] -> AP  - Access permission
#define MMU_L1_AP1      (1 << 11)
#define MMU_L1_TEX0     (1 << 12)   // pte[14:12] -> TEX - Type permission
#define MMU_L1_TEX1     (1 << 13)
#define MMU_L1_TEX2     (1 << 14)
#define MMU_L1_APX      (1 << 15)   // pte[15]    -> APX - Access permission
#define MMU_L1_S        (1 << 16)   // pte[16]    -> S   - Sharable
#define MMU_L1_nG       (1 << 17)   // pte[17]    -> nG  - Not Global

// MMU L2 Large Page Entry Flags
#define MMU_L2_L_B      (1 << 2)    // pte[2]     -> B   - Write Buffer
#define MMU_L2_L_C      (1 << 3)    // pte[3]     -> C   - Cache
#define MMU_L2_L_AP0    (1 << 4)    // pte[5:4]   -> AP  - Access permission
#define MMU_L2_L_AP1    (1 << 5)
#define MMU_L2_L_APX    (1 << 9)    // pte[9]     -> APX - Access permission
#define MMU_L2_L_S      (1 << 10)   // pte[10]    -> S   - Sharable
#define MMU_L2_L_nG     (1 << 11)   // pte[11]    -> nG  - Not Global
#define MMU_L2_L_TEX0   (1 << 12)   // pte[14:12] -> TEX - Type permission
#define MMU_L2_L_TEX1   (1 << 13)
#define MMU_L2_L_TEX2   (1 << 14)
#define MMU_L2_L_XN     (1 << 15)   // pte[15]    -> XN  - Execute Never

// MMU L2 Small Page Entry Flags
#define MMU_L2_S_XN     (1 << 0)    // pte[0]     -> XN  - Execute Never
#define MMU_L2_S_B      (1 << 2)    // pte[2]     -> B   - Write Buffer
#define MMU_L2_S_C      (1 << 3)    // pte[3]     -> C   - Cache
#define MMU_L2_S_AP0    (1 << 4)    // pte[5:4]   -> AP  - Access permission
#define MMU_L2_S_AP1    (1 << 5)
#define MMU_L2_S_TEX0   (1 << 6)    // pte[8:6]   -> TEX - Type permission
#define MMU_L2_S_TEX1   (1 << 7)
#define MMU_L2_S_TEX2   (1 << 8)
#define MMU_L2_S_APX    (1 << 9)    // pte[9]     -> APX - Access permission
#define MMU_L2_S_S      (1 << 10)   // pte[10]    -> S   - Sharable
#define MMU_L2_S_nG     (1 << 11)   // pte[11]    -> nG  - Not Global

/* Private macros ----------------------------------------- */


/* Private variables -------------------------------------- */
static ulong_t L1CacheCfgs[] =
{                                       // TEX[2:0] C B
    (0x0),                              //  0 0 0   0 0 -> Strongly-ordered
    (MMU_L1_TEX0),                      //  0 0 1   0 0 -> Outer and Inner Non-cacheable
    (MMU_L1_C),                         //  0 0 0   1 0 -> Outer and Inner Write-Through, no Write-Allocate
    (MMU_L1_C | MMU_L1_B),              //  0 0 0   1 1 -> Outer and Inner Write-Back, no Write-Allocate
    (MMU_L1_TEX0 | MMU_L1_C | MMU_L1_B),//  0 0 1   1 1 -> Outer and Inner Write-Back, Write-Allocate
    (MMU_L1_TEX0),                      //  0 0 1   0 0 -> Shareable Device
    (MMU_L1_TEX1)                       //  0 1 0   0 0 -> Non-shareable Device
};

static ulong_t L1AccessCfgs[] =
{
    (0x0),                              // No access
    (MMU_L1_AP0),                       // Kernel read an write, User no access
    (MMU_L1_AP1),                       // Kernel read an write, User read only
    (MMU_L1_AP0 | MMU_L1_AP1),          // Kernel read an write, User read an write
    (MMU_L1_APX),                       // Kernel read only, User no access
    (MMU_L1_APX | MMU_L1_AP1)           // Kernel read only, User read only
};

static ulong_t L2_L_CacheCfgs[] =
{                                             // TEX[2:0] C B
    (0x0),                                    //  0 0 0   0 0   -> Strongly-ordered
    (MMU_L2_L_TEX0),                          //  0 0 1	  0 0   -> Outer and Inner Non-cacheable
    (MMU_L2_L_C),                             //  0 0 0	  1 0   -> Outer and Inner Write-Through, no Write-Allocate
    (MMU_L2_L_C | MMU_L2_L_B),                //  0 0 0	  1 1   -> Outer and Inner Write-Back, no Write-Allocate
    (MMU_L2_L_TEX0 | MMU_L2_L_C | MMU_L2_L_B),//  0 0 1	  1 1   -> Outer and Inner Write-Back, Write-Allocate
    (MMU_L2_L_TEX0),                          //  0 0 1	  0 0   -> Shareable Device
    (MMU_L2_L_TEX1)                           //  0 1 0	  0 0   -> Non-shareable Device
};

static ulong_t L2_L_AccessCfgs[] =
{
    (0x0),                                  // No access
    (MMU_L2_L_AP0),                         // Kernel read an write, User no access
    (MMU_L2_L_AP1),                         // Kernel read an write, User read only
    (MMU_L2_L_AP0 | MMU_L2_L_AP1),          // Kernel read an write, User read an write
    (MMU_L2_L_APX),                         // Kernel read only, User no access
    (MMU_L2_L_APX | MMU_L2_L_AP1)           // Kernel read only, User read only
};

static ulong_t L2_S_CacheCfgs[] =
{                                             // TEX[2:0] C B
    (0x0),                                    //  0 0 0   0 0   -> Strongly-ordered
    (MMU_L2_S_TEX0),                          //  0 0 1	  0 0   -> Outer and Inner Non-cacheable
    (MMU_L2_S_C),                             //  0 0 0	  1 0   -> Outer and Inner Write-Through, no Write-Allocate
    (MMU_L2_S_C | MMU_L2_S_B),                //  0 0 0	  1 1   -> Outer and Inner Write-Back, no Write-Allocate
    (MMU_L2_S_TEX0 | MMU_L2_S_C | MMU_L2_S_B),//  0 0 1	  1 1   -> Outer and Inner Write-Back, Write-Allocate
    (MMU_L2_S_TEX0),                          //  0 0 1	  0 0   -> Shareable Device
    (MMU_L2_S_TEX1)                           //  0 1 0	  0 0   -> Non-shareable Device
};

static ulong_t L2_S_AccessCfgs[] =
{
    (0x0),                                  // No access
    (MMU_L2_S_AP0),                         // Kernel read an write, User no access
    (MMU_L2_S_AP1),                         // Kernel read an write, User read only
    (MMU_L2_S_AP0 | MMU_L2_S_AP1),          // Kernel read an write, User read an write
    (MMU_L2_S_APX),                         // Kernel read only, User no access
    (MMU_L2_S_APX | MMU_L2_S_AP1)           // Kernel read only, User read only
};

/* Private function prototypes ---------------------------- */

inline static ulong_t GetL1PteFlags(memCfg_t* memCfg)
{
    ulong_t flags = L1CacheCfgs[memCfg->cpolicy] | L1AccessCfgs[memCfg->apolicy];

    if(memCfg->shared) flags |= MMU_L1_S;
    if(!memCfg->global) flags |= MMU_L1_nG; 
    if(!memCfg->executable) flags |= MMU_L1_XN;

    return flags;
}

inline static ulong_t GetL2_L_PteFlags(memCfg_t* memCfg)
{
    ulong_t flags = L2_L_CacheCfgs[memCfg->cpolicy] | L2_L_AccessCfgs[memCfg->apolicy];

    if(memCfg->shared) flags |= MMU_L2_L_S;
    if(!memCfg->global) flags |= MMU_L2_L_nG; 
    if(!memCfg->executable) flags |= MMU_L2_L_XN;

    return flags;
}

inline static ulong_t GetL2_S_PteFlags(memCfg_t* memCfg)
{
    ulong_t flags = L2_S_CacheCfgs[memCfg->cpolicy] | L2_S_AccessCfgs[memCfg->apolicy];

    if(memCfg->shared) flags |= MMU_L2_S_S;
    if(!memCfg->global) flags |= MMU_L2_S_nG; 
    if(!memCfg->executable) flags |= MMU_L2_S_XN;

    return flags;
}

void MMU_Map16MbPages(ulong_t* pgt, ulong_t paddr, ulong_t vaddr, size_t pages, memCfg_t* memCfg)
{
    // Get Page Table Entry flags
    ulong_t flags = GetL1PteFlags(memCfg);
    // Get first 16Mb entry
    pgt = (ulong_t*)((ulong_t)pgt + (vaddr >> 18));
    // Populate page table (16Mb entries)
    uint32_t i, j;
    for(i = 0; i < pages; ++i)
    {
        // Set page table entry
        ulong_t pte = (flags | (paddr & 0xFF000000) | SUPERSECTION) + (i << 24);
        for(j = 0; j < 16; j += 4)
        {
            pgt[0] = pgt[1] = pgt[2] = pgt[3] = pte;
            pgt += 4; 
        }
    }
}

void MMU_Map1MbPages(ulong_t* pgt, ulong_t paddr, ulong_t vaddr, size_t pages, memCfg_t* memCfg)
{
    // Get first 1Mb entry
    pgt = (ulong_t*)((ulong_t)pgt + (vaddr >> 18));
    // Set page table entry
    ulong_t pte = (GetL1PteFlags(memCfg) | (paddr & 0xFFF00000) | SECTION);
    // Populate page table (1Mb entries)
    uint32_t i;
    for(i = 0; i < pages; ++i)
    {
        *pgt++ = pte + (i << 20);
    }
}

void MMU_Map64KbPages(ulong_t* pgt, ulong_t paddr, ulong_t vaddr, size_t pages, memCfg_t* memCfg)
{
    // Get Page Table Entry flags
    ulong_t flags = GetL2_L_PteFlags(memCfg);
    // Get first 64Kb entry for L2 Page Tables
    pgt = (ulong_t *)((ulong_t)pgt + ((vaddr & 0xFF000) >> 10));
    // Populate page table (64Kb entries)
    uint32_t i, j;
    for(i = 0; i < pages; ++i)
    {
        // Set page table entry
        ulong_t pte = (flags | (paddr & 0xFFFF0000) | LARGEPAGE) + (i << 16);
        for(j = 0;j < 16; j += 4)
        {
            pgt[0] = pgt[1] = pgt[2] = pgt[3] = pte;
            pgt += 4;
        }
    }
}

void MMU_Map4KbPages(ulong_t* pgt, ulong_t paddr, ulong_t vaddr, size_t pages, memCfg_t* memCfg)
{
    // Get first 4Kb entry
    pgt = (ulong_t*)((ulong_t)pgt + ((vaddr & 0xFF000) >> 10));
    // Set page table entry
    ulong_t pte = (GetL2_S_PteFlags(memCfg) | (paddr & 0xFFFFF000) | SMALLPAGE);
    // Populate page table (1Mb entries)
    uint32_t i;
    for(i = 0; i < pages; ++i)
    {
        *pgt++ = pte + (i << 12);
    }
}

void MMU_AttachL2PGT(ulong_t* pgt, ulong_t* l2pgt, ulong_t vaddr)
{
    // Get page table entry
    pgt += ((vaddr & 0xFFF00000) >> 18);

    // Set entry
    // Note: l2pgt is virtual address so we need to convert it to physical address
    *pgt = (((ulong_t)(MMU_L2P((vaddr_t)l2pgt)) & 0xfffffc00) | L2_PGT);
}

/* Private functions -------------------------------------- */

/**
 * MMU_KernelPGT Implementation (See arch/include/mmu.h for description)
*/
pgt_t MMU_KernelPGT(void)
{
    pgt_t pgt;
    // TTBR1 is the Kernel PGT
    asm volatile("mrc   p15, 0, %[_pgt], c2, c0, 1" : [_pgt] "=r" (pgt));
    // Remove control bits
    return (pgt_t)((ulong_t)pgt & 0xFFFFC000);
}

/**
 * MMU_UserPGT Implementation (See arch/include/mmu.h for description)
*/
pgt_t MMU_UserPGT(void)
{
    pgt_t pgt;
    // TTBR0 always has an User PGT after kernel is started
    asm volatile("mrc   p15, 0, %[_pgt], c2, c0, 0" : [_pgt] "=r" (pgt));
    // Remove control bits
    return (pgt_t)((ulong_t)pgt & 0xFFFFC000);
}

/**
 * MMU_MapPages Implementation (See arch/include/mmu.h for description)
*/
void MMU_MapPages(pgt_t pgt, vaddr_t vaddr, pbv_t* pages, size_t count, memCfg_t* memCfg)
{
    // All pars of addresses and sizes are assumed to have the same alignment
    // No virtual addresses colision will be check since it is assumed that this was done by a Virtual Space Manager

    ulong_t v_addr = (ulong_t)vaddr;
    uint32_t i;
    for(i = 0; i < count; v_addr += pages[i++].size)
    {
        if((pages[i].size >= LARGE_SECTION_SIZE) && !(v_addr & (LARGE_SECTION_SIZE - 1)))
        {
            MMU_Map16MbPages((ulong_t*)pgt, (ulong_t)pages[i].data, v_addr, pages[i].size / LARGE_SECTION_SIZE, memCfg);
        }
        else if((pages[i].size >= SECTION_SIZE) && !(v_addr & (SECTION_SIZE - 1)))
        {
            MMU_Map1MbPages((ulong_t*)pgt, (ulong_t)pages[i].data, v_addr, pages[i].size / SECTION_SIZE, memCfg);
        }
        else
        {
            // Check if we already have a L2 Page Table
            pgt_t l2pgt = NULL;

            // Check if we don't exceed the 1MB boundary
            size_t map_size = pages[i].size;

            if(((v_addr & (SECTION_SIZE - 1)) + pages[i].size) > SECTION_SIZE)
            {
                map_size = SECTION_SIZE - (v_addr & (SECTION_SIZE - 1));
            }

            if((map_size >= LARGE_PAGE_SIZE) && !(v_addr & (LARGE_PAGE_SIZE - 1)))
            {
                MMU_Map64KbPages((ulong_t*)l2pgt, (ulong_t)pages[i].data, v_addr, map_size / LARGE_PAGE_SIZE, memCfg);
            }
            else
            {
                MMU_Map4KbPages((ulong_t*)l2pgt, (ulong_t)pages[i].data, v_addr, map_size / SMALL_PAGE_SIZE, memCfg);
            }

            // Is there is leftovers let MMU_MapPages decide how to map it
            if(map_size < pages[i].size)
            {
                pbv_t page = {(ptr_t)((ulong_t)pages[i].data + map_size), pages[i].size - map_size};
                MMU_MapPages(pgt, (vaddr_t)(v_addr + map_size), &page, 1, memCfg);
            }
        }
    }
}

/**
 * MMU_L2P Implementation (See arch/include/mmu.h for description)
*/
paddr_t MMU_L2P(vaddr_t vaddr)
{
    extern ulong_t KernelVirtualBase;

    return (paddr_t)((ulong_t)vaddr - ((ulong_t)&KernelVirtualBase - (ulong_t)MMU_KernelPGT()));
}

/**
 * MMU_P2L Implementation (See arch/include/mmu.h for description)
*/
vaddr_t MMU_P2L(paddr_t paddr)
{
    extern ulong_t KernelVirtualBase;

    return (vaddr_t)((ulong_t)paddr + ((ulong_t)&KernelVirtualBase - (ulong_t)MMU_KernelPGT()));
}