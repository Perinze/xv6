// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define HASH(x) ((x) % NBUCKET)
#define MIN(x, y) ((x) < (y) ? (x) : (y))

struct bucket {
  struct spinlock lock;
  struct buf buf[BUCKETSIZE];
};

struct {
  struct spinlock lock;
  struct bucket bucket[NBUCKET];
} bcache;

void
touch(struct buf *buf)
{
  acquire(&tickslock);
  buf->timestamp = ticks;
  release(&tickslock);
}

void
binit(void)
{
  struct bucket *bucket;
  struct buf *b;

  initlock(&bcache.lock, "bcache");

  for(bucket = bcache.bucket; bucket < bcache.bucket+NBUCKET; bucket++){
    initlock(&bucket->lock, "bcache.bucket");
    for(b = bucket->buf; b < bucket->buf+BUCKETSIZE; b++){
      initsleeplock(&b->lock, "buffer");
      touch(b);
    }
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct bucket *bucket = &bcache.bucket[HASH(blockno)];
  struct buf *b;
  struct buf *lru;
  uint timestamp;

  acquire(&bucket->lock);

  // Is the block already cached?
  for(b = bucket->buf; b < bucket->buf+BUCKETSIZE; b++){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bucket->lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  timestamp = ~0;
  lru = 0;
  for(b = bucket->buf; b < bucket->buf+BUCKETSIZE; b++){
    if(b->refcnt == 0 && b->timestamp < timestamp){
      timestamp = b->timestamp;
      lru = b;
    }
  }

  if(lru){
    lru->dev = dev;
    lru->blockno = blockno;
    lru->valid = 0;
    lru->refcnt = 1;
    release(&bucket->lock);
    acquiresleep(&lru->lock);
    return lru;
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  struct bucket *bucket;

  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  bucket = &bcache.bucket[HASH(b->blockno)];

  acquire(&bucket->lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    touch(b);
  }

  release(&bucket->lock);
}

void
bpin(struct buf *b) {
  struct bucket *bucket = &bcache.bucket[HASH(b->blockno)];
  acquire(&bucket->lock);
  b->refcnt++;
  release(&bucket->lock);
}

void
bunpin(struct buf *b) {
  struct bucket *bucket = &bcache.bucket[HASH(b->blockno)];
  acquire(&bucket->lock);
  b->refcnt--;
  release(&bucket->lock);
}


