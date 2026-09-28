/*
  Queue implemented in an array
  template <typename DataType, typename IndexType>
  DataType = data type
  IndexType = index type (uint8_t for max queue size of 254 items, uint16_t for max 65535, etc...)

functions:
  constructor([len]): len = size of queue.
  begin([len]): len = size of queue.  clear queue
  clear: empty the queue
  full: return true if queue is full
  empty: return true if queue is empty
  push: add one item to end of queue
  unpush: pull item from tail of queue
  peektail: peek at tail item
  peek: return the item that is at the begining of queue without removing it
  pull: pull one item from begining of queue
  unpull: put back one item at begining of queue
    if item is not given, then restore previous item that was there
  available: return the number of items currently stored in the queue
  space: return the number of items that can be added to make the queue full
  
*/

#ifndef ARRAYQUEUE_H
#define ARRAYQUEUE_H

#include <inttypes.h>


template<typename DataType, typename IndexType> class QueueArray {
public:                                   //
  QueueArray(IndexType len = 0);          // constructor - set max length of queue (0-254), allocate buffer memory
  ~QueueArray();                          // destructor - free buffer memory
  void begin(IndexType len = 0);          // set the size of the buffer
  void clear();                           // clear the queue, reset pointers so it appears empty
  bool full();                            // is the queue full
  bool empty();                           // is the queue empty
  void push(DataType c);                  // push one item into the back of the queue
  DataType unpush();                      // pull item from tail of the queue
  DataType peektail();                    // peek at tail item in queue
  DataType pull();                        // pull one item from the front of the queue
  DataType peek();                        // see the item at the front of the queue without removing it from the queue
  void unpull();                          // put back the previously pulled item at/into the front of the queue
  void unpull(DataType c);                // push a item back into the front of the queue
  IndexType available();                  // get how many items are in the queue
  IndexType space();                      // get how much space is left in the queue (total queue size minus number of items in queue)
  IndexType read(DataType *buf, IndexType len);   // buffer read from queue
  IndexType write(DataType *buf, IndexType len);  // buffer write to queue
  IndexType write(const DataType *buf, IndexType len);  // buffer write to queue
  
protected:                                //
  DataType *buf;                          // pointer to buffer / array of items to store queue data.  set by constructor
  IndexType bufn,                         // size of buffer
    head,                                 // index of item at head of queue
    tail;                                 // index one past the item at the end of the queue
  IndexType inc(IndexType i);             // increment an indexer and wrap around at end of data buffer
  IndexType dec(IndexType i);             // decrement an indexer and wrap around at beginning of data buffer
};

#include "queuearray.hpp"                 // template functions are like header definitions - code is not yet implemented like in a cpp file

#endif

//
