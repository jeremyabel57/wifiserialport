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


#include "QueueArray.h"

template<typename DataType, typename IndexType> QueueArray<DataType, IndexType>::QueueArray(IndexType len) {
  buf = 0;
  bufn = 0;
  begin(len);
}
template<typename DataType, typename IndexType> QueueArray<DataType, IndexType>::~QueueArray() {
  if (buf) delete[] buf;
}
template<typename DataType, typename IndexType> void QueueArray<DataType, IndexType>::begin(IndexType len) {
  if (len > 0) {
    if (buf) delete[] buf;
    buf = new DataType[len + 1];
    bufn = (buf != 0) ? len + 1 : 0;
  }
  clear();
}
template<typename DataType, typename IndexType> void QueueArray<DataType, IndexType>::clear() {
  head = 0;
  tail = 0;
}
template<typename DataType, typename IndexType> bool QueueArray<DataType, IndexType>::full() {
  return head == inc(tail);
}
template<typename DataType, typename IndexType> bool QueueArray<DataType, IndexType>::empty() {
  return head == tail;
}
template<typename DataType, typename IndexType> void QueueArray<DataType, IndexType>::push(DataType d) {
  if (!full()) {
    buf[tail] = d;
    tail = inc(tail);
  }
}
template<typename DataType, typename IndexType> DataType QueueArray<DataType, IndexType>::unpush() {
  if (empty()) return 0xFF;
  tail = dec(tail);
  return buf[tail];
}
template<typename DataType, typename IndexType> DataType QueueArray<DataType, IndexType>::peektail() {
  return buf[dec(tail)];
}
template<typename DataType, typename IndexType> DataType QueueArray<DataType, IndexType>::pull() {
  if (empty()) return 0xFF;
  DataType d = buf[head];
  head = inc(head);
  return d;
}
template<typename DataType, typename IndexType> DataType QueueArray<DataType, IndexType>::peek() {
  return buf[head];
}
template<typename DataType, typename IndexType> void QueueArray<DataType, IndexType>::unpull() {
  if (!full()) head = dec(head);
}
template<typename DataType, typename IndexType> void QueueArray<DataType, IndexType>::unpull(DataType d) {
  if (!full()) {
    head = dec(head);
    buf[head] = d;
  }
}
template<typename DataType, typename IndexType> IndexType QueueArray<DataType, IndexType>::available() {
  return ((tail >= head) ? 0 : bufn) + tail - head;
}
template<typename DataType, typename IndexType> IndexType QueueArray<DataType, IndexType>::space() {
  return bufn - 1 - available();
}
template<typename DataType, typename IndexType> IndexType QueueArray<DataType, IndexType>::inc(IndexType i) {
  ++i;
  if (i >= bufn) i = 0;
  return i;
}
template<typename DataType, typename IndexType> IndexType QueueArray<DataType, IndexType>::dec(IndexType i) {
  if (i == 0) i = bufn;
  return --i;
}
template<typename DataType, typename IndexType> IndexType QueueArray<DataType, IndexType>::read(DataType *buf, IndexType len) {
  IndexType i = 0;
  while ((i<len) && (!empty()))
    buf[i++] = pull();
  return i;
}
template<typename DataType, typename IndexType> IndexType QueueArray<DataType, IndexType>::write(const DataType *buf, IndexType len) {
  IndexType i = 0;
  while ((i<len) && (!full()))
    push(buf[i++]);
  return i;
}
template<typename DataType, typename IndexType> IndexType QueueArray<DataType, IndexType>::write(DataType *buf, IndexType len) {
  return write((const DataType *) buf, len);
}

//
