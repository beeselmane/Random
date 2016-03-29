/**=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=**/
/* CXAllocator.h - Allocator Object for CX Framework               */
/**=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=**/
/* beeselmane - 23.3.2016  - 8:30 PM PST                           */
/**=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=**/
/* beeselmane - 28.3.2016  - 10:00 PM EST                          */
/**=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=**/

#ifndef __CX_CXALLOCATOR__
#define __CX_CXALLOCATOR__ 1

#include <CX/CXBase.h>

OSStartCBlock

/*
 * @description: A pointer to a function which allocates a block of memory.
 *
 * @parameter size: The minimum size of the allocated memory region
 *     The actual region may be larger than the given size, but never smaller
 * @parameter userinfo: The user information specified when the allocater was created
 *
 * @return: A pointer to the memory region or kOSNullPointer of at least the specified
 *     size could not be allocated.
 */
typedef OSAddress (*CXAllocatorAllocateFunction)(CXSize size, void *userinfo);

/*
 * @description: A pointer to a function which reallocates a memory region with a new size.
 *     The address of the newly allocated region may be different from the original address.
 *
 * @parameter memory: The original address of the previously allocated memory
 * @parameter newSize: The minimum size of the new region
 *     The actual region may be larger than the given size, but never smaller
 * @parameter userInfo: The user information value specified when the allocator was created
 *
 * @return: A pointer to the new memory region or kOSNullPointer if a new region of at least
 *     the specified size could not be allocated. May or may not be the same as the original address
 */
typedef OSAddress (*CXAllocatorReallocateFunction)(OSAddress memory, CXSize newSize, OSPointer userinfo);

/*
 * @description: A pointer to a function which frees a region of allocated memory
 *
 * @parameter memory: The address of the previously allocated memory region
 * @parameter userinfo: The user information value specified when the allocator was created
 */
typedef void (*CXAllocatorDeallocateFunction)(OSAddress memory, OSPointer userinfo);

/*
 * @description: A pointer to a function which increments the reference count for the allocator's userinfo value
 *
 * @parameter userinfo: The userinfo pointer
 */
typedef OSPointer (*CXAllocatorRetainUserInfoFunction)(OSPointer userinfo);

/*
 * @description: A pointer to a function which decreases the reference count for the allocator's userinfo value
 *
 * @parameter userinfo: The userinfo pointer
 */
typedef void (*CXAllocatorReleaseUserInfoFunction)(OSPointer userinfo);

/*
 * @description: A structure which describes how an allocator allocates memory
 *
 * @field userinfo: A nullable pointer to an unknown value which is passed to the allocator's functions
 * @field allocate: A pointer to the function an allocator will to use to allocate memory
 * @field reallocate: A nullable pointer to the function an allocator will use to resize allocated memory
 * @field deallocate: A pointer to the funciton an allocator will use to free allocated memory
 * @field retainUserInfo: A nullable pointer to the function an allocator will use to retain its userinfo pointer
 * @field releaseUserInfo: A nullable pointer to the function an allocator will use to release its userinfo pointer
 */
typedef struct __CXAllocatorContext {
    OSPointer                           userinfo;
    CXAllocatorAllocateFunction         allocate;
    CXAllocatorReallocateFunction       reallocate;
    CXAllocatorDeallocateFunction       deallocate;
    CXAllocatorRetainUserInfoFunction   retainUserInfo;
    CXAllocatorReleaseUserInfoFunction  releaseUserInfo;
} CXAllocatorContext;

/*
 * @description: Retrieve the TypeID associated with the CXAllocator Class
 *
 * @return: The TypeID for the CXAllocator Class
 */
CXExport CXTypeID CXAllocatorGetTypeID(void);

#pragma mark - Creating Allocators

/*
 * @description: Create a new allocator with the given allocator and context.
 *
 * @parameter allocator: The allocator used to allocate the new allocator object
 * @parameter context:   The context
 */
CXExport CXAllocatorRef CXAllocatorCreate(CXAllocatorRef allocator, CXAllocatorContext context);

#pragma mark - Allocator Methods

/*
 * @description: The CXAllocator method to allocate memory of a given size.
 *
 * @parameter allocator: The allocator with which to allocate the memory
 * @parameter size: The minimum size for the allocated memory
 *
 * @return: The address of the allocated memory region, or kOSNullPointer if no memory could be allocated
 */
CXExport OSAddress CXAllocatorAllocate(CXAllocatorRef allocator, CXSize size);

/*
 * @description: The CXAllocator method to change the size of a previously allocated block of memory.
 *
 * @parameter allocator: The allocator which initially allocated the memory region
 * @parameter memory: A pointer to the block of memory to be operated upon
 * @parameter newSize: The minimum size of the newly allocated region
 *
 * @return: The address of the reallocated memory region, or kOSNullPointer if the memory could not be
 *     reallocated to the given size. The new region may or may not have the same address as the origial block.
 */
CXExport OSAddress CXAllocatorReallocate(CXAllocatorRef allocator, OSAddress memory, CXSize newSize);

/*
 * @description: The CXAllocator method to free a block of memory
 *
 * @parameter allocator: The allocator which initially allocated the memory region
 * @parameter memory: A pointer to the block of memory to be operated upon
 */
CXExport void CXAllocatorDeallocate(CXAllocatorRef allocator, OSAddress memory);

#pragma mark - Predefined Allocators

/*
 * @description: The default allocator for the system
 */
CXExport CXAllocatorRef kCXAllocatorSystemDefault;

/*
 * @description: An allocator which never allocates or frees any memory
 */
CXExport CXAllocatorRef kCXAllocatorNull;

/*
 * @description: An allocator which, if passed to CXAllocatorCreate will use the new allocator's context in order to allocate the new allocator
 */
CXExport CXAllocatorRef kCXAllocatorUseContext;

OSFinishCBlock

#endif /* __CX_CXALLOCATOR__ */
