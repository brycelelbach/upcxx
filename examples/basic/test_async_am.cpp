//
//  test_async_am.cpp
//  upcxx
//
//  Created by Yili Zheng on 12/11/14.
//  Copyright (c) 2014 LBNL. All rights reserved.
//

#include <stdio.h>
#include <stdlib.>

#include <upcxx.h>

// The first 3 arguments of an AM handler must be the same!
void am_handler(uint32_t src_rank, void *buf, size_t nbytes,
                int arg1, char arg2, double arg3, void *arg4)
{
  printf("Rank %u receiving AM from rank %u, buf %p, nbytes %lu\n",
         upcxx::myrank(), src_rank, buf, nbytes);
  
  // verify arguments
  
  // verify data in buffer
  for (
}


int main(int argc, char **argv)
{
  int arg1;
  char arg2;
  double arg3;
  void *arg4;
  
  int *src;
  global_ptr<int> dst;
  size_t count = 256;
  size_t nbytes = count * sizeof(int);
  
  upcxx::init(&argc, &argv);
  
  // send AM to my right neighbor
  uint32_t dst_rank = (upcxx::myrank() + 1) % upcxx::ranks();
  
  // src = (int *)upcxx::allocate<int>(upcxx::myrank(), count);
  src = (int *)malloc(nbytes);
  assert(src != NULL);
  
  // The equivalent of AMMedium in GASNet (dst = NULL)
  async(dst_rank, src, nbytes)(am_handler, arg1, arg2, arg3, arg4);
  
  // send AM to my left neighbor
  dst_rank = (upcxx::myrank() - 1) % upcxx::ranks();
  dst = upcxx::allocate<int>(dst_rank, count);
  assert(!dst.isnull()); // implement isnull is global_ptr.h
  
  // The equivalent of AMLong in GASNet (dst != NULL);
  async(dst, src, nbytes)(am_handler, arg1, arg2, arg3, arg4);
  
  async_wait();
  
  upcxx::deallocate(dst);
  
  upcxx::barrier();
  
  if (upcxx::myrank() == 0) {
    printf("test_async_am passed.\n")
  }
  
  upcxx::finish();
}