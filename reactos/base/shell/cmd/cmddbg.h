#ifdef _DEBUG

#define cmd_alloc(size) cmd_alloc_dbg(size, __FILE__, __LINE__)
#define cmd_realloc(ptr,size) cmd_realloc_dbg(ptr, size, __FILE__, __LINE__)
#define cmd_free(ptr) cmd_free_dbg(ptr, __FILE__, __LINE__)

void *
cmd_alloc_dbg(size_t size, const char *file, int line);

void *
cmd_realloc_dbg(void *ptr, size_t size, const char *file, int line);

void
cmd_free_dbg(void *ptr, const char *file, int line);

#else

#define cmd_alloc(size) malloc(size)
#define cmd_realloc(ptr,size) realloc(ptr, size)
#define cmd_free(ptr) free(ptr)

#endif
