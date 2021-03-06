/**************************************************************************/ /*!
@File
@Title		Implementation Callbacks for Physmem (PMR) abstraction
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description	Part of the memory management.  This file is for definitions that
                are private to the world of PMRs, but that needs to be shared between
                pmr.c itself and the modules that implement the callbacks for the
                PMR.
@License        Dual MIT/GPLv2

The contents of this file are subject to the MIT license as set out below.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

Alternatively, the contents of this file may be used under the terms of
the GNU General Public License Version 2 ("GPL") in which case the provisions
of GPL are applicable instead of those above.

If you wish to allow use of your version of this file only under the terms of
GPL, and not to allow others to use your version of this file under the terms
of the MIT license, indicate your decision by deleting the provisions above
and replace them with the notice and other provisions required by GPL as set
out in the file called "GPL-COPYING" included in this distribution. If you do
not delete the provisions above, a recipient may use your version of this file
under the terms of either the MIT license or GPL.

This License is also included in this distribution in the file called
"MIT-COPYING".

EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/ /***************************************************************************/

#ifndef _SRVSRV_PMR_IMPL_H_
#define _SRVSRV_PMR_IMPL_H_

/* include/ */
#include "img_types.h"
#include "pvrsrv_error.h"

typedef struct _PMR_ PMR;
/* stuff that per-flavour callbacks need to share with pmr.c */
typedef void *PMR_IMPL_PRIVDATA;

typedef PVRSRV_MEMALLOCFLAGS_T PMR_FLAGS_T;
typedef struct _PMR_MAPPING_TABLE_ PMR_MAPPING_TABLE;
typedef void *PMR_MMAP_DATA;

typedef struct {
    /*
     * LockPhysAddresses() and UnlockPhysAddresses()
     *
     * locks down the physical addresses for the whole PMR.  If memory
     * is "prefaulted", i.e. exists phsycally at PMR creation time,
     * then there is no need to override this callback.  The default
     * implementation is to simply increment a lock-count for
     * debugging purposes.
     *
     * If overridden, this function will be called when someone first
     * requires a physical addresses, and the UnlockPhysAddresses()
     * counterpart will be called when the last such reference is
     * released.
     *
     * The PMR implementation may assume that physical addresses will
     * have been "locked" in this manner before any call is made to
     * the pfnDevPhysAddr() callback
     */
    PVRSRV_ERROR (*pfnLockPhysAddresses)(PMR_IMPL_PRIVDATA pvPriv,
                                         IMG_UINT32 uiLog2DevPageSize);
    PVRSRV_ERROR (*pfnUnlockPhysAddresses)(PMR_IMPL_PRIVDATA pvPriv);
    /*
     * called iteratively or once to obtain page(s) physical address
     * ("page" might be device mmu page, or host cpu mmu page, or
     * something else entirely... the PMR implementation should
     * make no assumption, and honour the request for a physical
     * address of any byte in the PMR)
     *
     * [ it's the callers responsibility to ensure that no addresses
     * are missed, by calling this at least as often as once per
     * "1<<contiguityguarantee" ]
     *
     * the LockPhysAddresses callback (if overridden) is guaranteed to
     * have been called prior to calling this one, and the caller
     * promises not to rely on the physical address thus obtained
     * after the UnlockPhysAddresses callback is called.
     *
     * Overriding this callback is mandatory in all PMR
     * implementations.
     */
    PVRSRV_ERROR (*pfnDevPhysAddr)(PMR_IMPL_PRIVDATA pvPriv,
                                   IMG_UINT32 ui32NumOfAddr,
                                   IMG_DEVMEM_OFFSET_T *puiOffset,
								   IMG_BOOL *pbValid,
                                   IMG_DEV_PHYADDR *psDevAddrPtr);
    /*
     * Called iteratively to obtain PDump symbolic addresses.  Behaves
     * just like pfnDevPhysAddr callback, except for returning Symbolic
     * Addresses.
     *
     * It is optional to override this callback.  The default
     * implementation will construct an address from the PMR type and
     * serial number.
     *
     * If the PMR factory supports unpinning of memory the symbolic
     * address must not be calculated depending on the physical
     * addresses of the PMR because it can change!
     */
    PVRSRV_ERROR (*pfnPDumpSymbolicAddr)(PMR_IMPL_PRIVDATA pvPriv,
                                         IMG_DEVMEM_OFFSET_T uiOffset,
                                         IMG_CHAR *pszMemspaceName,
                                         IMG_UINT32 ui32MemspaceNameLen,
                                         IMG_CHAR *pszSymbolicAddr,
                                         IMG_UINT32 ui32SymbolicAddrLen,
                                         IMG_DEVMEM_OFFSET_T *puiNewOffset,
                                         IMG_DEVMEM_OFFSET_T *puiNextSymName);
    /*
     * AcquireKernelMappingData()/ReleaseKernelMappingData()
     *
     * called to obtain a kernel virtual address for the PMR for use
     * internally in services.
     *
     * It is not necessary to override this callback, but failure to
     * do so will mean that kernel mappings will not be possible
     */
    PVRSRV_ERROR (*pfnAcquireKernelMappingData)(PMR_IMPL_PRIVDATA pvPriv,
                                                size_t uiOffset,
                                                size_t uiSize,
                                                void **ppvKernelAddressOut,
                                                IMG_HANDLE *phHandleOut,
                                                PMR_FLAGS_T ulFlags);
    void (*pfnReleaseKernelMappingData)(PMR_IMPL_PRIVDATA pvPriv,
                                            IMG_HANDLE hHandle);
    /*
     * Read up to uiBufSz bytes from the PMR.
     * The pmr will be already locked.
     *
     * Overriding this is optional.  The default implementation will
     * acquire a kernel virtual address with
     * pfnAcquireKernelMappingData and OSMemCopy the data directly
     */
    PVRSRV_ERROR (*pfnReadBytes)(PMR_IMPL_PRIVDATA pvPriv,
                                 IMG_DEVMEM_OFFSET_T uiOffset,
                                 IMG_UINT8 *pcBuffer,
                                 size_t uiBufSz,
                                 size_t *puiNumBytes);

    /*
     * Write up to uiBufSz bytes into the PMR.
     * The pmr will be already locked.
     *
     * Overriding this is optional.  The default implementation will
     * acquire a kernel virtual address with
     * pfnAcquireKernelMappingData and OSMemCopy the data directly
     *
     * Note:
     * This function callback is optional and unlike pfnReadBytes
     * isn't required if pfnAcquireKernelMappingData isn't provided
     */
    PVRSRV_ERROR (*pfnWriteBytes)(PMR_IMPL_PRIVDATA pvPriv,
                                  IMG_DEVMEM_OFFSET_T uiOffset,
                                  IMG_UINT8 *pcBuffer,
                                  size_t uiBufSz,
                                  size_t *puiNumBytes);

    /* Mark the PMR as unpinned so that the OS or whatever is
     * responsible for the PMR factory can reclaim the physical
     * memory of it and use it for something else.
     *
     * Expected return values:
     *     - PVRSRV_OK if unpin was successful.
     *     - PVRSRV_ERROR_NOT_SUPPORTED if unpinning is not supported
     *       in this PMR factory.
     *     - ... a different error for all other problems.
     */
    PVRSRV_ERROR (*pfnUnpinMem)(PMR_IMPL_PRIVDATA pPriv);

    /* Repins the PMR and checks if the physical memory
     * was lost, in that case it allocates new pages and returns
     * an error to indicate what happened.
     *
     * Expected return values:
     *     - PVRSRV_OK if pin was successful.
     *     - PVRSRV_ERROR_PMR_NEW_MEMORY if pin was successful but
     *       the content of the physical memory is lost.
     *     - PVRSRV_ERROR_NOT_SUPPORTED if pinning is not supported
     *       in this PMR factory.
     *     - ... a different error for all other problems.
     * */
    PVRSRV_ERROR (*pfnPinMem)(PMR_IMPL_PRIVDATA pPriv,
    							PMR_MAPPING_TABLE *psMappingTable);

    PVRSRV_ERROR (*pfnChangeSparseMem)(PMR_IMPL_PRIVDATA pPriv,
    								const PMR *psPMR,
    								IMG_UINT32 ui32AllocPageCount,
    								IMG_UINT32 *pai32AllocIndices,
    								IMG_UINT32 ui32FreePageCount,
    								IMG_UINT32 *pai32FreeIndices,
    								IMG_UINT32	uiFlags,
    								IMG_UINT32	*pui32Status);

    PVRSRV_ERROR (*pfnChangeSparseMemCPUMap)(PMR_IMPL_PRIVDATA pPriv,
    											const PMR *psPMR,
    											IMG_UINT64 sCpuVAddrBase,
    											IMG_UINT32	ui32AllocPageCount,
    											IMG_UINT32	*pai32AllocIndices,
    											IMG_UINT32	ui32FreePageCount,
    											IMG_UINT32	*pai32FreeIndices,
    											IMG_UINT32	*pui32Status);

	/*
	 * Maps the PMR into a user process address space.
	 *
	 * This callback is optional; However, it is strongly recommended
	 * that an implementation is provided and that it performs any
	 * necessary reference counting on the PMR. When this is not the
	 * case a generic implementation is used instead.
	 */
	PVRSRV_ERROR (*pfnMMap)(PMR_IMPL_PRIVDATA pPriv,
				const PMR *psPMR,
				PMR_MMAP_DATA pMMapData);
    /*
     * Finalize()
     *
     * This callback will be called once when the last reference to
     * the PMR has disappeared.
     */
    PVRSRV_ERROR (*pfnFinalize)(PMR_IMPL_PRIVDATA pvPriv);
} PMR_IMPL_FUNCTAB;

#endif /* of #ifndef _SRVSRV_PHYSMEM_PRIV_H_ */
