#pragma once
#include <ucos_ii.h>
#include <cstddef>
#include <array>

template <class T, size_t N>
class Heap {
public:
  INT8U initialize();
  T* get(INT8U*);
  INT8U put(T*);
private:
  bool initialized = false;
  std::array<T,N> heapData;
  OS_MEM* heap = nullptr;
};

template <class T, size_t N>
class Queue {
public:
  INT8U initialize();
  INT8U pop(T* msg = nullptr);
  INT8U push(T);
private:
  bool initialized = false;
  Heap<T,N> heap;
  std::array<T*,N> queueData;
  OS_EVENT* queue = nullptr;
};

template <class T, size_t N>
INT8U Heap<T,N>::initialize() {
  INT8U uCOSerr;
  if (initialized) return OS_ERR_NONE;

  // create mem partition
  heap = OSMemCreate(&heapData[0], N, sizeof(T), &uCOSerr);
  if (uCOSerr != OS_ERR_NONE) return uCOSerr;

  initialized = true;
  return OS_ERR_NONE;
}

template <class T, size_t N>
T* Heap<T,N>::get(INT8U *err) {
  return static_cast<T*>(OSMemGet(heap,err));
}

template <class T, size_t N>
INT8U Heap<T,N>::put(T* element) {
  return OSMemPut(heap, element);
}

template <class T, size_t N>
INT8U Queue<T,N>::initialize() {
  if (initialized) return OS_ERR_NONE;

  // initialize heap
  heap.initialize();

  // create queue
  queue = OSQCreate((void**)&queueData[0], N);
  if (queue == NULL) return OS_ERR_PEVENT_NULL;

  initialized = true;
  return OS_ERR_NONE;
}

template <class T, size_t N>
INT8U Queue<T,N>::push(T e) {
  INT8U uCOSerr;
  if (!initialized) return OS_ERR_PEVENT_NULL;

  // Obtain a T* from heap
  auto msg = heap.get(&uCOSerr);
  if (uCOSerr != OS_ERR_NONE) return uCOSerr;

  // copy element
  *msg = e;

  // Post event to queue
  uCOSerr = OSQPost(queue, msg);
  return uCOSerr;
}

template <class T, size_t N>
INT8U Queue<T,N>::pop(T* out) {
  INT8U uCOSerr = OS_ERR_NONE;
  // Pop element from queue
  auto msg = static_cast<T*>(OSQAccept(queue, &uCOSerr));
  if (!msg) { return uCOSerr; }

  // Copy element to output
  if (out) *out = *msg;

  // Free element from partition
  uCOSerr = heap.put(msg);
  if (uCOSerr != OS_ERR_NONE) {
    while (1);
  }
  return uCOSerr;
}
