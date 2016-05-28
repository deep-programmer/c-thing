#include "mpool.h"
#include "dbg.h"

static size_t _mpool_id = 0;

CT_EXPORT CT_MPool *ct_mpool_new() {
  return malloc(sizeof(CT_MPool));
}

CT_EXPORT size_t ct_mpool_init(CT_MPool *mp, size_t num, size_t blockSize) {
  CT_CHECK(blockSize >= sizeof(CT_MPoolFreeList),
           "blocksize must be >= %zd, was %zd", sizeof(CT_MPoolFreeList),
           blockSize);
  mp->numBlocks = num;
  mp->blockSize = blockSize;
  mp->freeList = NULL;
  mp->nextID = 0;
  mp->pool = malloc(num * mp->blockSize);
  CT_CHECK_MEM(mp->pool);
  mp->poolID = _mpool_id++;
  CT_DEBUG("init pool: %zd, mp: %p, bsize: %zd, num: %zd", mp->poolID, mp->pool,
           blockSize, num);
fail:
  return mp->pool == NULL;
}

CT_EXPORT void ct_mpool_free(CT_MPool *mp, void *block) {
  CT_CHECK(block >= (void *)mp->pool &&
               block < (void *)mp->pool + mp->numBlocks * mp->blockSize,
           "invalid block: %p", block);
  CT_DEBUG("pool: %zd, free block: %p", mp->poolID, block);
  CT_MPoolFreeList *fb = (CT_MPoolFreeList *)block;
  fb->next = mp->freeList;
  mp->freeList = fb;
fail:
  return;
}

CT_EXPORT void ct_mpool_free_all(CT_MPool *mp) {
  CT_CHECK(mp->pool, "pool already freed");
  CT_DEBUG("pool: %zd, free mp: %p", mp->poolID, mp->pool);
  free(mp->pool);
  mp->freeList = NULL;
fail:
  return;
}

CT_EXPORT void *ct_mpool_alloc(CT_MPool *mp) {
  void *ptr = NULL;
  if (mp->freeList != NULL) {
    ptr = mp->freeList;
    mp->freeList = mp->freeList->next;
  } else if (mp->nextID < mp->numBlocks) {
    ptr = mp->pool + mp->nextID * mp->blockSize;
    mp->nextID++;
  }
  CT_CHECK(ptr, "pool full");
  CT_DEBUG("pool: %zd, alloc block: %p", mp->poolID, ptr);
fail:
  return ptr;
}

CT_EXPORT void ct_mpool_trace(CT_MPool *mp) {
#ifndef NDEBUG
  CT_DEBUG("pool: %zd, nextID: %zd, addr: %p, free: %p, bsize: %zd, num: %zd",
           mp->poolID, mp->nextID, mp->pool, mp->freeList, mp->blockSize,
           mp->numBlocks);
  CT_MPoolFreeList *f = mp->freeList;
  size_t i = 0;
  while (f != NULL) {
    CT_DEBUG("\tfree list: %zd: %p -> %p", i, f, f->next);
    i++;
    f = f->next;
  }
#endif
}

CT_EXPORT size_t ct_rmpool_init(CT_RMPool *mp, size_t num, size_t blockSize) {
  CT_CHECK(blockSize >= sizeof(CT_MPoolFreeList),
           "blocksize must be >= %zd, was %zd", sizeof(CT_MPoolFreeList),
           blockSize);
  mp->numBlocks = num;
  mp->blockSize = blockSize;
  mp->freeList = NULL;
  mp->nextID = 0;
  CT_MPoolList *head = malloc(sizeof(CT_MPoolList));
  CT_CHECK_MEM(head);
  head->pool = malloc(num * mp->blockSize);
  CT_CHECK_MEM(head->pool);
  head->next = NULL;
  mp->head = head;
  mp->poolID = _mpool_id++;
  CT_DEBUG("init pool: %zd, head pool: %p, bsize: %zd, num: %zd", mp->poolID,
           mp->head->pool, blockSize, num);
  return 0;
fail:
  return 1;
}

CT_EXPORT void ct_rmpool_free(CT_RMPool *mp, void *block) {
  // TODO add valid ptr check (see mpool_free)
  CT_DEBUG("pool: %zd, free block: %p", mp->poolID, block);
  CT_MPoolFreeList *fb = (CT_MPoolFreeList *)block;
  fb->next = mp->freeList;
  mp->freeList = fb;
fail:
  return;
}

CT_EXPORT void ct_rmpool_free_all(CT_RMPool *mp) {
  CT_CHECK(mp->head, "pool already freed");
  CT_DEBUG("pool: %zd, freeing all...", mp->poolID);
  CT_MPoolList *p = mp->head, *q;
  while (p != NULL) {
    CT_DEBUG("\tfree sub-pool: %p, storage: %p", p, p->pool);
    q = p->next;
    free(p->pool);
    free(p);
    p = q;
  }
  mp->head = NULL;
  mp->freeList = NULL;
fail:
  return;
}

CT_EXPORT void *ct_rmpool_alloc(CT_RMPool *mp) {
  void *ptr = NULL;
  if (mp->freeList != NULL) {
    ptr = mp->freeList;
    mp->freeList = mp->freeList->next;
  } else if (mp->nextID < mp->numBlocks) {
    ptr = mp->head->pool + mp->nextID * mp->blockSize;
    mp->nextID++;
  } else {
    CT_MPoolList *head = malloc(sizeof(CT_MPoolList));
    CT_CHECK_MEM(head);
    head->pool = malloc(mp->numBlocks * mp->blockSize);
    CT_CHECK_MEM(head->pool);
    CT_DEBUG("adding new sub-pool: %p, %p", head, head->pool);
    ptr = head->pool;
    head->next = mp->head;
    mp->head = head;
    mp->nextID = 1;
  }
  CT_DEBUG("pool: %zd, alloc block: %p", mp->poolID, ptr);
fail:
  return ptr;
}

CT_EXPORT void ct_rmpool_trace(CT_RMPool *mp) {
#ifndef NDEBUG
  CT_DEBUG("pool: %zd, nextID: %zd, head: %p, free: %p, bsize: %zd, num: %zd",
           mp->poolID, mp->nextID, mp->head, mp->freeList, mp->blockSize,
           mp->numBlocks);
  CT_MPoolList *p = mp->head;
  while (p) {
    CT_DEBUG("\tsub-pool: %p -> %p storage: %p", p, p->next, p->pool);
    p = p->next;
  }
  CT_MPoolFreeList *f = mp->freeList;
  size_t i = 0;
  while (f != NULL) {
    CT_DEBUG("\tfree list: %zd: %p -> %p", i, f, f->next);
    i++;
    f = f->next;
  }
#endif
}
