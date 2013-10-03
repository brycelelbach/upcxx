add_library(upcxx STATIC
   active_coll.cpp
   async.cpp
   barrier.cpp
   dl_malloc.c
   event.cpp
   lock.cpp
   machine.cpp
   ptr_to_shared.cpp
   team.cpp
   upcxx_runtime.cpp
)

install (TARGETS upcxx DESTINATION lib)
