
// 这个文件包含 系统heap和lua heap的默认实现


#include <stdlib.h>
#include <string.h>//add for memset
#include "bget.h"
#include "luat_malloc.h"

#define LUAT_LOG_TAG "vmheap"
#include "luat_log.h"

//------------------------------------------------
//  管理系统内存

void* luat_heap_malloc(size_t len) {
    return malloc(len);
}

void luat_heap_free(void* ptr) {
    free(ptr);
}

void* luat_heap_realloc(void* ptr, size_t len) {
    return realloc(ptr, len);
}

void* luat_heap_calloc(size_t count, size_t _size) {
    void *ptr = luat_heap_malloc(count * _size);
    if (ptr) {
        memset(ptr, 0, count * _size);
    }
    return ptr;
}

void* luat_heap_zalloc(size_t _size) {
    void* ptr = malloc(_size);
    if (ptr != NULL) {
        memset(ptr, 0, _size);
    }
    return ptr;
}
//------------------------------------------------

//------------------------------------------------
// ---------- 管理 LuaVM所使用的内存----------------
void* luat_heap_alloc(void *ud, void *ptr, size_t osize, size_t nsize) {
    (void)ud;
    if (0) {
        if (ptr) {
            if (nsize) {
                // 缩放内存块
                LLOGD("realloc %p from %d to %d", ptr, osize, nsize);
            }
            else {
                // 释放内存块
                LLOGD("free %p ", ptr);
                brel(ptr);
                return NULL;
            }
        }
        else {
            // 申请内存块
            ptr = bget(nsize);
            LLOGD("malloc %p type=%d size=%d", ptr, osize, nsize);
            return ptr;
        }
    }

    if (nsize)
    {
    	void* ptmp = bgetr(ptr, nsize);
    	if(ptmp == NULL && osize >= nsize)
    	{
    		return ptr;
    	}
        return ptmp;
    }
    brel(ptr);
    return NULL;
}

void luat_meminfo_luavm(size_t *total, size_t *used, size_t *max_used) {
	long curalloc, totfree, maxfree;
	unsigned long nget, nrel;
	bstats(&curalloc, &totfree, &maxfree, &nget, &nrel);
	*used = curalloc;
	*max_used = bstatsmaxget();
    *total = curalloc + totfree;
}

void luat_meminfo_sys(size_t *total, size_t *used, size_t *max_used) {
    // TODO 貌似真实不了-_-
	*used = 128*1024;
	*max_used = 64*1024;
    *total = 246*1024;
}

#include "luat_mem.h"
#include "luat_bget.h"

static void* psram_ptr;
static luat_bget_t psram_bget;

void luat_heap_opt_init(LUAT_HEAP_TYPE_E type){
    if (type == LUAT_HEAP_PSRAM && psram_ptr == NULL) {
        psram_ptr = malloc(2*1024*1024);
        luat_bget_init(&psram_bget);
        luat_bpool(&psram_bget, psram_ptr, 2*1024*1024);
    }
}

void* luat_heap_opt_malloc(LUAT_HEAP_TYPE_E type,size_t len){
    if (type == LUAT_HEAP_PSRAM) {
        return luat_bgetz(&psram_bget, len);
    }
    return luat_heap_malloc(len);
}

void luat_heap_opt_free(LUAT_HEAP_TYPE_E type,void* ptr){
    if (type == LUAT_HEAP_PSRAM) {
        return luat_brel(&psram_bget, ptr);
    }
    luat_heap_free(ptr);
}

void* luat_heap_opt_realloc(LUAT_HEAP_TYPE_E type,void* ptr, size_t len){
    if (type == LUAT_HEAP_PSRAM) {
        return luat_bgetr(&psram_bget, ptr, len);
    }
    return luat_heap_realloc(ptr, len);
}

void* luat_heap_opt_calloc(LUAT_HEAP_TYPE_E type,size_t count, size_t size){
    return luat_heap_opt_zalloc(type,count*size);
}

void* luat_heap_opt_zalloc(LUAT_HEAP_TYPE_E type,size_t size){
    void *ptr = luat_heap_opt_malloc(type,size);
    if (ptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}

void luat_meminfo_opt_sys(LUAT_HEAP_TYPE_E type,size_t* total, size_t* used, size_t* max_used){
    if (type == LUAT_HEAP_PSRAM) {
        long curalloc, totfree, maxfree;
	    unsigned long nget, nrel;
	    luat_bstats(&psram_bget, &curalloc, &totfree, &maxfree, &nget, &nrel);
	    *used = curalloc;
	    *max_used = luat_bstatsmaxget(&psram_bget);
        *total = curalloc + totfree;
    }
    else {
        luat_meminfo_sys(total, used, max_used);
    }
}



//-----------------------------------------------------------------------------
