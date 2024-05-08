

#pragma once

/********************************************************************************/

#define sizeof_array 64
#define sizeof_cache 4096

/********************************************************************************/

class lua_allotor final {
  class mallotor {
  public:
    mallotor();
    ~mallotor();
    void* pop();
    bool  push(void*);

  private:
    size_t ri, wi;
    char mp[sizeof_cache];
  };

public:
  lua_allotor();
  ~lua_allotor();
  void  clear();
  void  p_free(void* ptr, size_t os);
  void* p_realloc(void* ptr, size_t os, size_t ns);

private:
  mallotor* allotor_array[sizeof_array];
};

/********************************************************************************/
