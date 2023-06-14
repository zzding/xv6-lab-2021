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
struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct spinlock buketlock[NBUKETSIZE];
  //struct buflist buket[NBUKETSIZE];
  struct buf buket[NBUKETSIZE];
  struct buf tail[NBUKETSIZE];
} bcache;

int hash(int dev, int blockno){
  return (dev * blockno) % NBUKETSIZE;
}

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");

  // Create linked list of buffers
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    initsleeplock(&b->lock, "buffer");
  }
  for(int i = 0; i < NBUKETSIZE; ++i){
    initlock(&bcache.buketlock[i], "bcachebuket");
    bcache.buket[i].next = &bcache.tail[i];
    bcache.tail[i].prev = &bcache.buket[i];
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int index = hash(dev, blockno);

  acquire(&bcache.buketlock[index]);

  for(b = bcache.buket[index].next; b != &bcache.tail[index]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.buketlock[index]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  for(b = bcache.buf; b != &bcache.buf[NBUF]; ++b){
    int num = hash(b->dev, b->blockno);
    if(num != index){
      acquire(&bcache.buketlock[num]);
    }
    if(b->refcnt == 0){
      int olddev = b->dev;
      int oldblockno = b->blockno;
      int oldindex = hash(olddev, oldblockno);
      struct buf *oldb;

      for(oldb = bcache.buket[oldindex].next; oldb != &bcache.tail[oldindex]; oldb = oldb->next){
        if(oldb->dev == olddev && oldb->blockno == oldblockno){
          oldb->prev->next = oldb->next;
          oldb->next->prev = oldb->prev; 
          break;
        }
      }
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;


      b->next = bcache.buket[index].next;
      bcache.buket[index].next = b;
      b->next->prev = b;
      b->prev = &bcache.buket[index];
      release(&bcache.buketlock[index]);
      acquiresleep(&b->lock);
      if(num != index){
        release(&bcache.buketlock[num]);
      } 
      return b;
    }
    if(num != index){
      release(&bcache.buketlock[num]);
    }
  } 
  int indexs = 0;
  for(b = bcache.buf; b != &bcache.buf[NBUF]; ++b){
    printf("%d, %d\n",indexs++,b->refcnt);
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
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);
  int index = hash(b->dev, b->blockno);
  acquire(&bcache.buketlock[index]);
  b->refcnt--;
  release(&bcache.buketlock[index]);
}

void
bpin(struct buf *b) {
   int idx = hash(b->dev,b->blockno);
   acquire(&bcache.buketlock[idx]);
   b->refcnt++;
   release(&bcache.buketlock[idx]);
}

void
bunpin(struct buf *b) {
   int idx = hash(b->dev,b->blockno);
   acquire(&bcache.buketlock[idx]);
   b->refcnt--;
   release(&bcache.buketlock[idx]);
}


