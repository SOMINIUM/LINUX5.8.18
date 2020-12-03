/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_SLUB_DEF_H
#define _LINUX_SLUB_DEF_H

/*
 * SLUB : A Slab allocator without object queues.
 *
 * (C) 2007 SGI, Christoph Lameter
 */
#include <linux/kobject.h>

enum stat_item {
	ALLOC_FASTPATH,		/* Allocation from cpu slab */
	ALLOC_SLOWPATH,		/* Allocation by getting a new cpu slab */
	FREE_FASTPATH,		/* Free to cpu slab */
	FREE_SLOWPATH,		/* Freeing not to cpu slab */
	FREE_FROZEN,		/* Freeing to frozen slab */
	FREE_ADD_PARTIAL,	/* Freeing moves slab to partial list */
	FREE_REMOVE_PARTIAL,	/* Freeing removes last object */
	ALLOC_FROM_PARTIAL,	/* Cpu slab acquired from node partial list */
	ALLOC_SLAB,		/* Cpu slab acquired from page allocator */
	ALLOC_REFILL,		/* Refill cpu slab from slab freelist */
	ALLOC_NODE_MISMATCH,	/* Switching cpu slab */
	FREE_SLAB,		/* Slab freed to the page allocator */
	CPUSLAB_FLUSH,		/* Abandoning of the cpu slab */
	DEACTIVATE_FULL,	/* Cpu slab was full when deactivated */
	DEACTIVATE_EMPTY,	/* Cpu slab was empty when deactivated */
	DEACTIVATE_TO_HEAD,	/* Cpu slab was moved to the head of partials */
	DEACTIVATE_TO_TAIL,	/* Cpu slab was moved to the tail of partials */
	DEACTIVATE_REMOTE_FREES,/* Slab contained remotely freed objects */
	DEACTIVATE_BYPASS,	/* Implicit deactivation */
	ORDER_FALLBACK,		/* Number of times fallback was necessary */
	CMPXCHG_DOUBLE_CPU_FAIL,/* Failure of this_cpu_cmpxchg_double */
	CMPXCHG_DOUBLE_FAIL,	/* Number of times that cmpxchg double did not match */
	CPU_PARTIAL_ALLOC,	/* Used cpu partial on alloc */
	CPU_PARTIAL_FREE,	/* Refill cpu partial on free */
	CPU_PARTIAL_NODE,	/* Refill cpu partial from node partial */
	CPU_PARTIAL_DRAIN,	/* Drain cpu partial to node partial */
	NR_SLUB_STAT_ITEMS };

struct kmem_cache_cpu {
	void **freelist;	/* Pointer to next available object */
	unsigned long tid;	/* Globally unique transaction id */
	struct page *page;	/* The slab from which we are allocating */
#ifdef CONFIG_SLUB_CPU_PARTIAL
	struct page *partial;	/* Partially allocated frozen slabs */
#endif
#ifdef CONFIG_SLUB_STATS
	unsigned stat[NR_SLUB_STAT_ITEMS];
#endif
};

#ifdef CONFIG_SLUB_CPU_PARTIAL
#define slub_percpu_partial(c)		((c)->partial)

#define slub_set_percpu_partial(c, p)		\
({						\
	slub_percpu_partial(c) = (p)->next;	\
})

#define slub_percpu_partial_read_once(c)     READ_ONCE(slub_percpu_partial(c))
#else
#define slub_percpu_partial(c)			NULL

#define slub_set_percpu_partial(c, p)

#define slub_percpu_partial_read_once(c)	NULL
#endif // CONFIG_SLUB_CPU_PARTIAL

/*
 * Word size structure that can be atomically updated or read and that
 * contains both the order and the number of objects that a slab of the
 * given order would contain.
 */
struct kmem_cache_order_objects {
	unsigned int x;
};

/*
 * Slab cache management.
 */
struct kmem_cache {
    /*
     * percpu 变量 ，对于每个cpu来讲就是一个本地缓存池，当分配内存时优先
     * 从本地分配从而保证cache命中率。
     * */
	struct kmem_cache_cpu __percpu *cpu_slab;
	/* Used for retrieving partial slabs, etc. */
    /*
     * object分配掩码,经常使用的SLAB_HWCACHE_ALIGN标志位，代表创建的kmem_cache
     * 管理的object按照硬件cache 对齐
     *
     * */
	slab_flags_t flags;
    /*
     * kmem_cache_node中partial链表最大slab数量，如果大于这个mini_partial的值，
     * 那么多余的slab就会被释放.
     *
     * */
	unsigned long min_partial;
    /*
     * 分配的object的大小, 包括对齐时补的大小，所以这个size应试>=object_size
     *
     * */
	unsigned int size;	/* The size of an object including metadata */
    /*
     * object 直正的大小
     * */
	unsigned int object_size;/* The size of an object without metadata */

    /*
     * 一个slab中会有N个object,这些object形成一链表，第个object中都存储了
     * 下一个object相对当前object首地址的offset值。
     *
     * */
	unsigned int offset;	/* Free pointer offset */
#ifdef CONFIG_SLUB_CPU_PARTIAL
	/* Number of per cpu partial objects to keep around */

    /*
     * 每个cpu中的所有slab中所有free object的数量最大值。超过这个值就会将所有的
     * slab转移到kmem_cache_node的partial链表
     *
     * */
	unsigned int cpu_partial;
#endif
    /*
     * 一个slab需要的页数和一个slab中object数
     * 0----------------15 16----------------31
     *       低16位               高16位
     * 一个slab中object数  一个slab需要的页数
     * */
	struct kmem_cache_order_objects oo;

	/* Allocation and freeing of slabs */
	struct kmem_cache_order_objects max;
	struct kmem_cache_order_objects min;
    /*
     * 从伙伴系统分配内存的掩码
     * */
	gfp_t allocflags;	/* gfp flags to use on each alloc */
	int refcount;		/* Refcount for slab cache destroy */
	void (*ctor)(void *);
    /*
     * 对齐到字的大小
     * */
	unsigned int inuse;		/* Offset to metadata */
    /*
     * 如果flags配置了SLAB_HWCACHE_ALIGN,则先对齐到cache的大小
     * 再对齐到字(sizeof(void *))的大小
     *
     * */
	unsigned int align;		/* Alignment */
	unsigned int red_left_pad;	/* Left redzone padding size */
	const char *name;	/* Name (only for display!) */
	struct list_head list;	/* List of slab caches */
#ifdef CONFIG_SYSFS
	struct kobject kobj;	/* For sysfs */
	struct work_struct kobj_remove_work;
#endif
#ifdef CONFIG_MEMCG
	struct memcg_cache_params memcg_params;
	/* For propagation, maximum size of a stored attr */
	unsigned int max_attr_size;
#ifdef CONFIG_SYSFS
	struct kset *memcg_kset;
#endif
#endif

#ifdef CONFIG_SLAB_FREELIST_HARDENED
	unsigned long random;
#endif

#ifdef CONFIG_NUMA
	/*
	 * Defragmentation by allocating from a remote node.
	 */
	unsigned int remote_node_defrag_ratio;
#endif

#ifdef CONFIG_SLAB_FREELIST_RANDOM
	unsigned int *random_seq;
#endif

#ifdef CONFIG_KASAN
	struct kasan_cache kasan_info;
#endif

	unsigned int useroffset;	/* Usercopy region offset */
	unsigned int usersize;		/* Usercopy region size */

	struct kmem_cache_node *node[MAX_NUMNODES];
};

#ifdef CONFIG_SLUB_CPU_PARTIAL
#define slub_cpu_partial(s)		((s)->cpu_partial)
#define slub_set_cpu_partial(s, n)		\
({						\
	slub_cpu_partial(s) = (n);		\
})
#else
#define slub_cpu_partial(s)		(0)
#define slub_set_cpu_partial(s, n)
#endif /* CONFIG_SLUB_CPU_PARTIAL */

#ifdef CONFIG_SYSFS
#define SLAB_SUPPORTS_SYSFS
void sysfs_slab_unlink(struct kmem_cache *);
void sysfs_slab_release(struct kmem_cache *);
#else
static inline void sysfs_slab_unlink(struct kmem_cache *s)
{
}
static inline void sysfs_slab_release(struct kmem_cache *s)
{
}
#endif

void object_err(struct kmem_cache *s, struct page *page,
		u8 *object, char *reason);

void *fixup_red_left(struct kmem_cache *s, void *p);

static inline void *nearest_obj(struct kmem_cache *cache, struct page *page,
				void *x) {
	void *object = x - (x - page_address(page)) % cache->size;
	void *last_object = page_address(page) +
		(page->objects - 1) * cache->size;
	void *result = (unlikely(object > last_object)) ? last_object : object;

	result = fixup_red_left(cache, result);
	return result;
}

#endif /* _LINUX_SLUB_DEF_H */
