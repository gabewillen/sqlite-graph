# 1 "/home/gwillen/sqlite-graph/src/graph-advanced.c"
# 1 "<built-in>" 1
# 1 "<built-in>" 3
# 389 "<built-in>" 3
# 1 "<command line>" 1
# 1 "<built-in>" 2
# 1 "/home/gwillen/sqlite-graph/src/graph-advanced.c" 2
# 11 "/home/gwillen/sqlite-graph/src/graph-advanced.c"
# 1 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3ext.h" 1
# 20 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3ext.h"
# 1 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h" 1
# 35 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
# 1 "/usr/lib/llvm-18/lib/clang/18/include/stdarg.h" 1 3
# 55 "/usr/lib/llvm-18/lib/clang/18/include/stdarg.h" 3
# 1 "/usr/lib/llvm-18/lib/clang/18/include/__stdarg___gnuc_va_list.h" 1 3
# 12 "/usr/lib/llvm-18/lib/clang/18/include/__stdarg___gnuc_va_list.h" 3
typedef __builtin_va_list __gnuc_va_list;
# 56 "/usr/lib/llvm-18/lib/clang/18/include/stdarg.h" 2 3




# 1 "/usr/lib/llvm-18/lib/clang/18/include/__stdarg_va_list.h" 1 3
# 12 "/usr/lib/llvm-18/lib/clang/18/include/__stdarg_va_list.h" 3
typedef __builtin_va_list va_list;
# 61 "/usr/lib/llvm-18/lib/clang/18/include/stdarg.h" 2 3




# 1 "/usr/lib/llvm-18/lib/clang/18/include/__stdarg_va_arg.h" 1 3
# 66 "/usr/lib/llvm-18/lib/clang/18/include/stdarg.h" 2 3




# 1 "/usr/lib/llvm-18/lib/clang/18/include/__stdarg___va_copy.h" 1 3
# 71 "/usr/lib/llvm-18/lib/clang/18/include/stdarg.h" 2 3




# 1 "/usr/lib/llvm-18/lib/clang/18/include/__stdarg_va_copy.h" 1 3
# 76 "/usr/lib/llvm-18/lib/clang/18/include/stdarg.h" 2 3
# 36 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h" 2
# 185 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           extern const char sqlite3_version[];
           const char *sqlite3_libversion(void);
           const char *sqlite3_sourceid(void);
           int sqlite3_libversion_number(void);
# 213 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_compileoption_used(const char *zOptName);
           const char *sqlite3_compileoption_get(int N);
# 256 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_threadsafe(void);
# 272 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
typedef struct sqlite3 sqlite3;
# 301 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
  typedef long long int sqlite_int64;
  typedef unsigned long long int sqlite_uint64;

typedef sqlite_int64 sqlite3_int64;
typedef sqlite_uint64 sqlite3_uint64;
# 353 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_close(sqlite3*);
           int sqlite3_close_v2(sqlite3*);






typedef int (*sqlite3_callback)(void*,int,char**, char**);
# 425 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_exec(
  sqlite3*,
  const char *sql,
  int (*callback)(void*,int,char**,char**),
  void *,
  char **errmsg
);
# 728 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
typedef struct sqlite3_file sqlite3_file;
struct sqlite3_file {
  const struct sqlite3_io_methods *pMethods;
};
# 834 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
typedef struct sqlite3_io_methods sqlite3_io_methods;
struct sqlite3_io_methods {
  int iVersion;
  int (*xClose)(sqlite3_file*);
  int (*xRead)(sqlite3_file*, void*, int iAmt, sqlite3_int64 iOfst);
  int (*xWrite)(sqlite3_file*, const void*, int iAmt, sqlite3_int64 iOfst);
  int (*xTruncate)(sqlite3_file*, sqlite3_int64 size);
  int (*xSync)(sqlite3_file*, int flags);
  int (*xFileSize)(sqlite3_file*, sqlite3_int64 *pSize);
  int (*xLock)(sqlite3_file*, int);
  int (*xUnlock)(sqlite3_file*, int);
  int (*xCheckReservedLock)(sqlite3_file*, int *pResOut);
  int (*xFileControl)(sqlite3_file*, int op, void *pArg);
  int (*xSectorSize)(sqlite3_file*);
  int (*xDeviceCharacteristics)(sqlite3_file*);

  int (*xShmMap)(sqlite3_file*, int iPg, int pgsz, int, void volatile**);
  int (*xShmLock)(sqlite3_file*, int offset, int n, int flags);
  void (*xShmBarrier)(sqlite3_file*);
  int (*xShmUnmap)(sqlite3_file*, int deleteFlag);

  int (*xFetch)(sqlite3_file*, sqlite3_int64 iOfst, int iAmt, void **pp);
  int (*xUnfetch)(sqlite3_file*, sqlite3_int64 iOfst, void *p);


};
# 1262 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
typedef struct sqlite3_mutex sqlite3_mutex;
# 1272 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
typedef struct sqlite3_api_routines sqlite3_api_routines;
# 1292 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
typedef const char *sqlite3_filename;
# 1463 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
typedef struct sqlite3_vfs sqlite3_vfs;
typedef void (*sqlite3_syscall_ptr)(void);
struct sqlite3_vfs {
  int iVersion;
  int szOsFile;
  int mxPathname;
  sqlite3_vfs *pNext;
  const char *zName;
  void *pAppData;
  int (*xOpen)(sqlite3_vfs*, sqlite3_filename zName, sqlite3_file*,
               int flags, int *pOutFlags);
  int (*xDelete)(sqlite3_vfs*, const char *zName, int syncDir);
  int (*xAccess)(sqlite3_vfs*, const char *zName, int flags, int *pResOut);
  int (*xFullPathname)(sqlite3_vfs*, const char *zName, int nOut, char *zOut);
  void *(*xDlOpen)(sqlite3_vfs*, const char *zFilename);
  void (*xDlError)(sqlite3_vfs*, int nByte, char *zErrMsg);
  void (*(*xDlSym)(sqlite3_vfs*,void*, const char *zSymbol))(void);
  void (*xDlClose)(sqlite3_vfs*, void*);
  int (*xRandomness)(sqlite3_vfs*, int nByte, char *zOut);
  int (*xSleep)(sqlite3_vfs*, int microseconds);
  int (*xCurrentTime)(sqlite3_vfs*, double*);
  int (*xGetLastError)(sqlite3_vfs*, int, char *);




  int (*xCurrentTimeInt64)(sqlite3_vfs*, sqlite3_int64*);




  int (*xSetSystemCall)(sqlite3_vfs*, const char *zName, sqlite3_syscall_ptr);
  sqlite3_syscall_ptr (*xGetSystemCall)(sqlite3_vfs*, const char *zName);
  const char *(*xNextSystemCall)(sqlite3_vfs*, const char *zName);





};
# 1641 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_initialize(void);
           int sqlite3_shutdown(void);
           int sqlite3_os_init(void);
           int sqlite3_os_end(void);
# 1680 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_config(int, ...);
# 1699 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_db_config(sqlite3*, int op, ...);
# 1764 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
typedef struct sqlite3_mem_methods sqlite3_mem_methods;
struct sqlite3_mem_methods {
  void *(*xMalloc)(int);
  void (*xFree)(void*);
  void *(*xRealloc)(void*,int);
  int (*xSize)(void*);
  int (*xRoundup)(int);
  int (*xInit)(void*);
  void (*xShutdown)(void*);
  void *pAppData;
};
# 2524 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_extended_result_codes(sqlite3*, int onoff);
# 2586 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           sqlite3_int64 sqlite3_last_insert_rowid(sqlite3*);
# 2596 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           void sqlite3_set_last_insert_rowid(sqlite3*,sqlite3_int64);
# 2657 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_changes(sqlite3*);
           sqlite3_int64 sqlite3_changes64(sqlite3*);
# 2699 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_total_changes(sqlite3*);
           sqlite3_int64 sqlite3_total_changes64(sqlite3*);
# 2741 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           void sqlite3_interrupt(sqlite3*);
           int sqlite3_is_interrupted(sqlite3*);
# 2777 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_complete(const char *sql);
           int sqlite3_complete16(const void *sql);
# 2839 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_busy_handler(sqlite3*,int(*)(void*,int),void*);
# 2862 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_busy_timeout(sqlite3*, int ms);
# 2937 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_get_table(
  sqlite3 *db,
  const char *zSql,
  char ***pazResult,
  int *pnRow,
  int *pnColumn,
  char **pzErrmsg
);
           void sqlite3_free_table(char **result);
# 2987 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           char *sqlite3_mprintf(const char*,...);
           char *sqlite3_vmprintf(const char*, va_list);
           char *sqlite3_snprintf(int,char*,const char*, ...);
           char *sqlite3_vsnprintf(int,char*,const char*, va_list);
# 3067 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           void *sqlite3_malloc(int);
           void *sqlite3_malloc64(sqlite3_uint64);
           void *sqlite3_realloc(void*, int);
           void *sqlite3_realloc64(void*, sqlite3_uint64);
           void sqlite3_free(void*);
           sqlite3_uint64 sqlite3_msize(void*);
# 3097 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           sqlite3_int64 sqlite3_memory_used(void);
           sqlite3_int64 sqlite3_memory_highwater(int resetFlag);
# 3121 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           void sqlite3_randomness(int N, void *P);
# 3212 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_set_authorizer(
  sqlite3*,
  int (*xAuth)(void*,int,const char*,const char*,const char*,const char*),
  void *pUserData
);
# 3320 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
                             void *sqlite3_trace(sqlite3*,
   void(*xTrace)(void*,const char*), void*);
                             void *sqlite3_profile(sqlite3*,
   void(*xProfile)(void*,const char*,sqlite3_uint64), void*);
# 3413 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_trace_v2(
  sqlite3*,
  unsigned uMask,
  int(*xCallback)(unsigned,void*,void*,void*),
  void *pCtx
);
# 3459 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           void sqlite3_progress_handler(sqlite3*, int, int(*)(void*), void*);
# 3739 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_open(
  const char *filename,
  sqlite3 **ppDb
);
           int sqlite3_open16(
  const void *filename,
  sqlite3 **ppDb
);
           int sqlite3_open_v2(
  const char *filename,
  sqlite3 **ppDb,
  int flags,
  const char *zVfs
);
# 3820 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           const char *sqlite3_uri_parameter(sqlite3_filename z, const char *zParam);
           int sqlite3_uri_boolean(sqlite3_filename z, const char *zParam, int bDefault);
           sqlite3_int64 sqlite3_uri_int64(sqlite3_filename, const char*, sqlite3_int64);
           const char *sqlite3_uri_key(sqlite3_filename z, int N);
# 3852 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           const char *sqlite3_filename_database(sqlite3_filename);
           const char *sqlite3_filename_journal(sqlite3_filename);
           const char *sqlite3_filename_wal(sqlite3_filename);
# 3873 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           sqlite3_file *sqlite3_database_file_object(const char*);
# 3920 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           sqlite3_filename sqlite3_create_filename(
  const char *zDatabase,
  const char *zJournal,
  const char *zWal,
  int nParam,
  const char **azParam
);
           void sqlite3_free_filename(sqlite3_filename);
# 3992 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_errcode(sqlite3 *db);
           int sqlite3_extended_errcode(sqlite3 *db);
           const char *sqlite3_errmsg(sqlite3*);
           const void *sqlite3_errmsg16(sqlite3*);
           const char *sqlite3_errstr(int);
           int sqlite3_error_offset(sqlite3 *db);
# 4023 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
typedef struct sqlite3_stmt sqlite3_stmt;
# 4065 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_limit(sqlite3*, int id, int newVal);
# 4275 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_prepare(
  sqlite3 *db,
  const char *zSql,
  int nByte,
  sqlite3_stmt **ppStmt,
  const char **pzTail
);
           int sqlite3_prepare_v2(
  sqlite3 *db,
  const char *zSql,
  int nByte,
  sqlite3_stmt **ppStmt,
  const char **pzTail
);
           int sqlite3_prepare_v3(
  sqlite3 *db,
  const char *zSql,
  int nByte,
  unsigned int prepFlags,
  sqlite3_stmt **ppStmt,
  const char **pzTail
);
           int sqlite3_prepare16(
  sqlite3 *db,
  const void *zSql,
  int nByte,
  sqlite3_stmt **ppStmt,
  const void **pzTail
);
           int sqlite3_prepare16_v2(
  sqlite3 *db,
  const void *zSql,
  int nByte,
  sqlite3_stmt **ppStmt,
  const void **pzTail
);
           int sqlite3_prepare16_v3(
  sqlite3 *db,
  const void *zSql,
  int nByte,
  unsigned int prepFlags,
  sqlite3_stmt **ppStmt,
  const void **pzTail
);
# 4361 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           const char *sqlite3_sql(sqlite3_stmt *pStmt);
           char *sqlite3_expanded_sql(sqlite3_stmt *pStmt);
# 4414 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_stmt_readonly(sqlite3_stmt *pStmt);
# 4426 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_stmt_isexplain(sqlite3_stmt *pStmt);
# 4461 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_stmt_explain(sqlite3_stmt *pStmt, int eMode);
# 4482 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_stmt_busy(sqlite3_stmt*);
# 4526 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
typedef struct sqlite3_value sqlite3_value;
# 4540 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
typedef struct sqlite3_context sqlite3_context;
# 4682 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_bind_blob(sqlite3_stmt*, int, const void*, int n, void(*)(void*));
           int sqlite3_bind_blob64(sqlite3_stmt*, int, const void*, sqlite3_uint64,
                        void(*)(void*));
           int sqlite3_bind_double(sqlite3_stmt*, int, double);
           int sqlite3_bind_int(sqlite3_stmt*, int, int);
           int sqlite3_bind_int64(sqlite3_stmt*, int, sqlite3_int64);
           int sqlite3_bind_null(sqlite3_stmt*, int);
           int sqlite3_bind_text(sqlite3_stmt*,int,const char*,int,void(*)(void*));
           int sqlite3_bind_text16(sqlite3_stmt*, int, const void*, int, void(*)(void*));
           int sqlite3_bind_text64(sqlite3_stmt*, int, const char*, sqlite3_uint64,
                         void(*)(void*), unsigned char encoding);
           int sqlite3_bind_value(sqlite3_stmt*, int, const sqlite3_value*);
           int sqlite3_bind_pointer(sqlite3_stmt*, int, void*, const char*,void(*)(void*));
           int sqlite3_bind_zeroblob(sqlite3_stmt*, int, int n);
           int sqlite3_bind_zeroblob64(sqlite3_stmt*, int, sqlite3_uint64);
# 4717 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_bind_parameter_count(sqlite3_stmt*);
# 4745 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           const char *sqlite3_bind_parameter_name(sqlite3_stmt*, int);
# 4763 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_bind_parameter_index(sqlite3_stmt*, const char *zName);
# 4773 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_clear_bindings(sqlite3_stmt*);
# 4789 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_column_count(sqlite3_stmt *pStmt);
# 4818 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           const char *sqlite3_column_name(sqlite3_stmt*, int N);
           const void *sqlite3_column_name16(sqlite3_stmt*, int N);
# 4863 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           const char *sqlite3_column_database_name(sqlite3_stmt*,int);
           const void *sqlite3_column_database_name16(sqlite3_stmt*,int);
           const char *sqlite3_column_table_name(sqlite3_stmt*,int);
           const void *sqlite3_column_table_name16(sqlite3_stmt*,int);
           const char *sqlite3_column_origin_name(sqlite3_stmt*,int);
           const void *sqlite3_column_origin_name16(sqlite3_stmt*,int);
# 4900 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           const char *sqlite3_column_decltype(sqlite3_stmt*,int);
           const void *sqlite3_column_decltype16(sqlite3_stmt*,int);
# 4985 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_step(sqlite3_stmt*);
# 5006 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_data_count(sqlite3_stmt *pStmt);
# 5253 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           const void *sqlite3_column_blob(sqlite3_stmt*, int iCol);
           double sqlite3_column_double(sqlite3_stmt*, int iCol);
           int sqlite3_column_int(sqlite3_stmt*, int iCol);
           sqlite3_int64 sqlite3_column_int64(sqlite3_stmt*, int iCol);
           const unsigned char *sqlite3_column_text(sqlite3_stmt*, int iCol);
           const void *sqlite3_column_text16(sqlite3_stmt*, int iCol);
           sqlite3_value *sqlite3_column_value(sqlite3_stmt*, int iCol);
           int sqlite3_column_bytes(sqlite3_stmt*, int iCol);
           int sqlite3_column_bytes16(sqlite3_stmt*, int iCol);
           int sqlite3_column_type(sqlite3_stmt*, int iCol);
# 5290 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_finalize(sqlite3_stmt *pStmt);
# 5329 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_reset(sqlite3_stmt *pStmt);
# 5455 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_create_function(
  sqlite3 *db,
  const char *zFunctionName,
  int nArg,
  int eTextRep,
  void *pApp,
  void (*xFunc)(sqlite3_context*,int,sqlite3_value**),
  void (*xStep)(sqlite3_context*,int,sqlite3_value**),
  void (*xFinal)(sqlite3_context*)
);
           int sqlite3_create_function16(
  sqlite3 *db,
  const void *zFunctionName,
  int nArg,
  int eTextRep,
  void *pApp,
  void (*xFunc)(sqlite3_context*,int,sqlite3_value**),
  void (*xStep)(sqlite3_context*,int,sqlite3_value**),
  void (*xFinal)(sqlite3_context*)
);
           int sqlite3_create_function_v2(
  sqlite3 *db,
  const char *zFunctionName,
  int nArg,
  int eTextRep,
  void *pApp,
  void (*xFunc)(sqlite3_context*,int,sqlite3_value**),
  void (*xStep)(sqlite3_context*,int,sqlite3_value**),
  void (*xFinal)(sqlite3_context*),
  void(*xDestroy)(void*)
);
           int sqlite3_create_window_function(
  sqlite3 *db,
  const char *zFunctionName,
  int nArg,
  int eTextRep,
  void *pApp,
  void (*xStep)(sqlite3_context*,int,sqlite3_value**),
  void (*xFinal)(sqlite3_context*),
  void (*xValue)(sqlite3_context*),
  void (*xInverse)(sqlite3_context*,int,sqlite3_value**),
  void(*xDestroy)(void*)
);
# 5619 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
                             int sqlite3_aggregate_count(sqlite3_context*);
                             int sqlite3_expired(sqlite3_stmt*);
                             int sqlite3_transfer_bindings(sqlite3_stmt*, sqlite3_stmt*);
                             int sqlite3_global_recover(void);
                             void sqlite3_thread_cleanup(void);
                             int sqlite3_memory_alarm(void(*)(void*,sqlite3_int64,int),
                      void*,sqlite3_int64);
# 5756 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           const void *sqlite3_value_blob(sqlite3_value*);
           double sqlite3_value_double(sqlite3_value*);
           int sqlite3_value_int(sqlite3_value*);
           sqlite3_int64 sqlite3_value_int64(sqlite3_value*);
           void *sqlite3_value_pointer(sqlite3_value*, const char*);
           const unsigned char *sqlite3_value_text(sqlite3_value*);
           const void *sqlite3_value_text16(sqlite3_value*);
           const void *sqlite3_value_text16le(sqlite3_value*);
           const void *sqlite3_value_text16be(sqlite3_value*);
           int sqlite3_value_bytes(sqlite3_value*);
           int sqlite3_value_bytes16(sqlite3_value*);
           int sqlite3_value_type(sqlite3_value*);
           int sqlite3_value_numeric_type(sqlite3_value*);
           int sqlite3_value_nochange(sqlite3_value*);
           int sqlite3_value_frombind(sqlite3_value*);
# 5792 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_value_encoding(sqlite3_value*);
# 5810 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           unsigned int sqlite3_value_subtype(sqlite3_value*);
# 5827 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           sqlite3_value *sqlite3_value_dup(const sqlite3_value*);
           void sqlite3_value_free(sqlite3_value*);
# 5873 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           void *sqlite3_aggregate_context(sqlite3_context*, int nBytes);
# 5888 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           void *sqlite3_user_data(sqlite3_context*);
# 5900 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           sqlite3 *sqlite3_context_db_handle(sqlite3_context*);
# 5969 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           void *sqlite3_get_auxdata(sqlite3_context*, int N);
           void sqlite3_set_auxdata(sqlite3_context*, int N, void*, void (*)(void*));
# 6025 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           void *sqlite3_get_clientdata(sqlite3*,const char*);
           int sqlite3_set_clientdata(sqlite3*, const char*, void*, void(*)(void*));
# 6042 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
typedef void (*sqlite3_destructor_type)(void*);
# 6193 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           void sqlite3_result_blob(sqlite3_context*, const void*, int, void(*)(void*));
           void sqlite3_result_blob64(sqlite3_context*,const void*,
                           sqlite3_uint64,void(*)(void*));
           void sqlite3_result_double(sqlite3_context*, double);
           void sqlite3_result_error(sqlite3_context*, const char*, int);
           void sqlite3_result_error16(sqlite3_context*, const void*, int);
           void sqlite3_result_error_toobig(sqlite3_context*);
           void sqlite3_result_error_nomem(sqlite3_context*);
           void sqlite3_result_error_code(sqlite3_context*, int);
           void sqlite3_result_int(sqlite3_context*, int);
           void sqlite3_result_int64(sqlite3_context*, sqlite3_int64);
           void sqlite3_result_null(sqlite3_context*);
           void sqlite3_result_text(sqlite3_context*, const char*, int, void(*)(void*));
           void sqlite3_result_text64(sqlite3_context*, const char*,sqlite3_uint64,
                           void(*)(void*), unsigned char encoding);
           void sqlite3_result_text16(sqlite3_context*, const void*, int, void(*)(void*));
           void sqlite3_result_text16le(sqlite3_context*, const void*, int,void(*)(void*));
           void sqlite3_result_text16be(sqlite3_context*, const void*, int,void(*)(void*));
           void sqlite3_result_value(sqlite3_context*, sqlite3_value*);
           void sqlite3_result_pointer(sqlite3_context*, void*,const char*,void(*)(void*));
           void sqlite3_result_zeroblob(sqlite3_context*, int n);
           int sqlite3_result_zeroblob64(sqlite3_context*, sqlite3_uint64 n);
# 6243 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           void sqlite3_result_subtype(sqlite3_context*,unsigned int);
# 6326 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_create_collation(
  sqlite3*,
  const char *zName,
  int eTextRep,
  void *pArg,
  int(*xCompare)(void*,int,const void*,int,const void*)
);
           int sqlite3_create_collation_v2(
  sqlite3*,
  const char *zName,
  int eTextRep,
  void *pArg,
  int(*xCompare)(void*,int,const void*,int,const void*),
  void(*xDestroy)(void*)
);
           int sqlite3_create_collation16(
  sqlite3*,
  const void *zName,
  int eTextRep,
  void *pArg,
  int(*xCompare)(void*,int,const void*,int,const void*)
);
# 6376 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_collation_needed(
  sqlite3*,
  void*,
  void(*)(void*,sqlite3*,int eTextRep,const char*)
);
           int sqlite3_collation_needed16(
  sqlite3*,
  void*,
  void(*)(void*,sqlite3*,int eTextRep,const void*)
);
# 6421 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_sleep(int);
# 6479 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           extern char *sqlite3_temp_directory;
# 6516 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           extern char *sqlite3_data_directory;
# 6537 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_win32_set_directory(
  unsigned long type,
  void *zValue
);
           int sqlite3_win32_set_directory8(unsigned long type, const char *zValue);
           int sqlite3_win32_set_directory16(unsigned long type, const void *zValue);
# 6575 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_get_autocommit(sqlite3*);
# 6588 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           sqlite3 *sqlite3_db_handle(sqlite3_stmt*);
# 6610 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           const char *sqlite3_db_name(sqlite3 *db, int N);
# 6642 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           sqlite3_filename sqlite3_db_filename(sqlite3 *db, const char *zDbName);
# 6652 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_db_readonly(sqlite3 *db, const char *zDbName);
# 6670 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_txn_state(sqlite3*,const char *zSchema);
# 6719 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           sqlite3_stmt *sqlite3_next_stmt(sqlite3 *pDb, sqlite3_stmt *pStmt);
# 6768 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           void *sqlite3_commit_hook(sqlite3*, int(*)(void*), void*);
           void *sqlite3_rollback_hook(sqlite3*, void(*)(void *), void*);
# 6829 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_autovacuum_pages(
  sqlite3 *db,
  unsigned int(*)(void*,const char*,unsigned int,unsigned int,unsigned int),
  void*,
  void(*)(void*)
);
# 6886 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           void *sqlite3_update_hook(
  sqlite3*,
  void(*)(void *,int ,char const *,char const *,sqlite3_int64),
  void*
);
# 6936 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_enable_shared_cache(int);
# 6952 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_release_memory(int);
# 6966 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_db_release_memory(sqlite3*);
# 7032 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           sqlite3_int64 sqlite3_soft_heap_limit64(sqlite3_int64 N);
           sqlite3_int64 sqlite3_hard_heap_limit64(sqlite3_int64 N);
# 7044 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
                             void sqlite3_soft_heap_limit(int N);
# 7116 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_table_column_metadata(
  sqlite3 *db,
  const char *zDbName,
  const char *zTableName,
  const char *zColumnName,
  char const **pzDataType,
  char const **pzCollSeq,
  int *pNotNull,
  int *pPrimaryKey,
  int *pAutoinc
);
# 7172 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_load_extension(
  sqlite3 *db,
  const char *zFile,
  const char *zProc,
  char **pzErrMsg
);
# 7204 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_enable_load_extension(sqlite3 *db, int onoff);
# 7242 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_auto_extension(void(*xEntryPoint)(void));
# 7254 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_cancel_auto_extension(void(*xEntryPoint)(void));







           void sqlite3_reset_auto_extension(void);




typedef struct sqlite3_vtab sqlite3_vtab;
typedef struct sqlite3_index_info sqlite3_index_info;
typedef struct sqlite3_vtab_cursor sqlite3_vtab_cursor;
typedef struct sqlite3_module sqlite3_module;
# 7288 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
struct sqlite3_module {
  int iVersion;
  int (*xCreate)(sqlite3*, void *pAux,
               int argc, const char *const*argv,
               sqlite3_vtab **ppVTab, char**);
  int (*xConnect)(sqlite3*, void *pAux,
               int argc, const char *const*argv,
               sqlite3_vtab **ppVTab, char**);
  int (*xBestIndex)(sqlite3_vtab *pVTab, sqlite3_index_info*);
  int (*xDisconnect)(sqlite3_vtab *pVTab);
  int (*xDestroy)(sqlite3_vtab *pVTab);
  int (*xOpen)(sqlite3_vtab *pVTab, sqlite3_vtab_cursor **ppCursor);
  int (*xClose)(sqlite3_vtab_cursor*);
  int (*xFilter)(sqlite3_vtab_cursor*, int idxNum, const char *idxStr,
                int argc, sqlite3_value **argv);
  int (*xNext)(sqlite3_vtab_cursor*);
  int (*xEof)(sqlite3_vtab_cursor*);
  int (*xColumn)(sqlite3_vtab_cursor*, sqlite3_context*, int);
  int (*xRowid)(sqlite3_vtab_cursor*, sqlite3_int64 *pRowid);
  int (*xUpdate)(sqlite3_vtab *, int, sqlite3_value **, sqlite3_int64 *);
  int (*xBegin)(sqlite3_vtab *pVTab);
  int (*xSync)(sqlite3_vtab *pVTab);
  int (*xCommit)(sqlite3_vtab *pVTab);
  int (*xRollback)(sqlite3_vtab *pVTab);
  int (*xFindFunction)(sqlite3_vtab *pVtab, int nArg, const char *zName,
                       void (**pxFunc)(sqlite3_context*,int,sqlite3_value**),
                       void **ppArg);
  int (*xRename)(sqlite3_vtab *pVtab, const char *zNew);


  int (*xSavepoint)(sqlite3_vtab *pVTab, int);
  int (*xRelease)(sqlite3_vtab *pVTab, int);
  int (*xRollbackTo)(sqlite3_vtab *pVTab, int);


  int (*xShadowName)(const char*);


  int (*xIntegrity)(sqlite3_vtab *pVTab, const char *zSchema,
                    const char *zTabName, int mFlags, char **pzErr);
};
# 7432 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
struct sqlite3_index_info {

  int nConstraint;
  struct sqlite3_index_constraint {
     int iColumn;
     unsigned char op;
     unsigned char usable;
     int iTermOffset;
  } *aConstraint;
  int nOrderBy;
  struct sqlite3_index_orderby {
     int iColumn;
     unsigned char desc;
  } *aOrderBy;

  struct sqlite3_index_constraint_usage {
    int argvIndex;
    unsigned char omit;
  } *aConstraintUsage;
  int idxNum;
  char *idxStr;
  int needToFreeIdxStr;
  int orderByConsumed;
  double estimatedCost;

  sqlite3_int64 estimatedRows;

  int idxFlags;

  sqlite3_uint64 colUsed;
};
# 7561 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_create_module(
  sqlite3 *db,
  const char *zName,
  const sqlite3_module *p,
  void *pClientData
);
           int sqlite3_create_module_v2(
  sqlite3 *db,
  const char *zName,
  const sqlite3_module *p,
  void *pClientData,
  void(*xDestroy)(void*)
);
# 7587 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_drop_modules(
  sqlite3 *db,
  const char **azKeep
);
# 7610 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
struct sqlite3_vtab {
  const sqlite3_module *pModule;
  int nRef;
  char *zErrMsg;

};
# 7634 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
struct sqlite3_vtab_cursor {
  sqlite3_vtab *pVtab;

};
# 7647 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_declare_vtab(sqlite3*, const char *zSQL);
# 7666 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_overload_function(sqlite3*, const char *zFuncName, int nArg);
# 7680 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
typedef struct sqlite3_blob sqlite3_blob;
# 7765 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_blob_open(
  sqlite3*,
  const char *zDb,
  const char *zTable,
  const char *zColumn,
  sqlite3_int64 iRow,
  int flags,
  sqlite3_blob **ppBlob
);
# 7798 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_blob_reopen(sqlite3_blob *, sqlite3_int64);
# 7821 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_blob_close(sqlite3_blob *);
# 7837 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_blob_bytes(sqlite3_blob *);
# 7866 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_blob_read(sqlite3_blob *, void *Z, int N, int iOffset);
# 7908 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_blob_write(sqlite3_blob *, const void *z, int n, int iOffset);
# 7939 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           sqlite3_vfs *sqlite3_vfs_find(const char *zVfsName);
           int sqlite3_vfs_register(sqlite3_vfs*, int makeDflt);
           int sqlite3_vfs_unregister(sqlite3_vfs*);
# 8059 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           sqlite3_mutex *sqlite3_mutex_alloc(int);
           void sqlite3_mutex_free(sqlite3_mutex*);
           void sqlite3_mutex_enter(sqlite3_mutex*);
           int sqlite3_mutex_try(sqlite3_mutex*);
           void sqlite3_mutex_leave(sqlite3_mutex*);
# 8130 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
typedef struct sqlite3_mutex_methods sqlite3_mutex_methods;
struct sqlite3_mutex_methods {
  int (*xMutexInit)(void);
  int (*xMutexEnd)(void);
  sqlite3_mutex *(*xMutexAlloc)(int);
  void (*xMutexFree)(sqlite3_mutex *);
  void (*xMutexEnter)(sqlite3_mutex *);
  int (*xMutexTry)(sqlite3_mutex *);
  void (*xMutexLeave)(sqlite3_mutex *);
  int (*xMutexHeld)(sqlite3_mutex *);
  int (*xMutexNotheld)(sqlite3_mutex *);
};
# 8173 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_mutex_held(sqlite3_mutex*);
           int sqlite3_mutex_notheld(sqlite3_mutex*);
# 8218 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           sqlite3_mutex *sqlite3_db_mutex(sqlite3*);
# 8261 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_file_control(sqlite3*, const char *zDbName, int op, void*);
# 8280 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_test_control(int op, ...);
# 8377 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_keyword_count(void);
           int sqlite3_keyword_name(int,const char**,int*);
           int sqlite3_keyword_check(const char*,int);
# 8397 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
typedef struct sqlite3_str sqlite3_str;
# 8424 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           sqlite3_str *sqlite3_str_new(sqlite3*);
# 8439 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           char *sqlite3_str_finish(sqlite3_str*);
# 8473 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           void sqlite3_str_appendf(sqlite3_str*, const char *zFormat, ...);
           void sqlite3_str_vappendf(sqlite3_str*, const char *zFormat, va_list);
           void sqlite3_str_append(sqlite3_str*, const char *zIn, int N);
           void sqlite3_str_appendall(sqlite3_str*, const char *zIn);
           void sqlite3_str_appendchar(sqlite3_str*, int N, char C);
           void sqlite3_str_reset(sqlite3_str*);
# 8509 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_str_errcode(sqlite3_str*);
           int sqlite3_str_length(sqlite3_str*);
           char *sqlite3_str_value(sqlite3_str*);
# 8539 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_status(int op, int *pCurrent, int *pHighwater, int resetFlag);
           int sqlite3_status64(
  int op,
  sqlite3_int64 *pCurrent,
  sqlite3_int64 *pHighwater,
  int resetFlag
);
# 8649 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_db_status(sqlite3*, int op, int *pCur, int *pHiwtr, int resetFlg);
# 8802 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_stmt_status(sqlite3_stmt*, int op,int resetFlg);
# 8890 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
typedef struct sqlite3_pcache sqlite3_pcache;
# 8902 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
typedef struct sqlite3_pcache_page sqlite3_pcache_page;
struct sqlite3_pcache_page {
  void *pBuf;
  void *pExtra;
};
# 9067 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
typedef struct sqlite3_pcache_methods2 sqlite3_pcache_methods2;
struct sqlite3_pcache_methods2 {
  int iVersion;
  void *pArg;
  int (*xInit)(void*);
  void (*xShutdown)(void*);
  sqlite3_pcache *(*xCreate)(int szPage, int szExtra, int bPurgeable);
  void (*xCachesize)(sqlite3_pcache*, int nCachesize);
  int (*xPagecount)(sqlite3_pcache*);
  sqlite3_pcache_page *(*xFetch)(sqlite3_pcache*, unsigned key, int createFlag);
  void (*xUnpin)(sqlite3_pcache*, sqlite3_pcache_page*, int discard);
  void (*xRekey)(sqlite3_pcache*, sqlite3_pcache_page*,
      unsigned oldKey, unsigned newKey);
  void (*xTruncate)(sqlite3_pcache*, unsigned iLimit);
  void (*xDestroy)(sqlite3_pcache*);
  void (*xShrink)(sqlite3_pcache*);
};






typedef struct sqlite3_pcache_methods sqlite3_pcache_methods;
struct sqlite3_pcache_methods {
  void *pArg;
  int (*xInit)(void*);
  void (*xShutdown)(void*);
  sqlite3_pcache *(*xCreate)(int szPage, int bPurgeable);
  void (*xCachesize)(sqlite3_pcache*, int nCachesize);
  int (*xPagecount)(sqlite3_pcache*);
  void *(*xFetch)(sqlite3_pcache*, unsigned key, int createFlag);
  void (*xUnpin)(sqlite3_pcache*, void*, int discard);
  void (*xRekey)(sqlite3_pcache*, void*, unsigned oldKey, unsigned newKey);
  void (*xTruncate)(sqlite3_pcache*, unsigned iLimit);
  void (*xDestroy)(sqlite3_pcache*);
};
# 9116 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
typedef struct sqlite3_backup sqlite3_backup;
# 9304 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           sqlite3_backup *sqlite3_backup_init(
  sqlite3 *pDest,
  const char *zDestName,
  sqlite3 *pSource,
  const char *zSourceName
);
           int sqlite3_backup_step(sqlite3_backup *p, int nPage);
           int sqlite3_backup_finish(sqlite3_backup *p);
           int sqlite3_backup_remaining(sqlite3_backup *p);
           int sqlite3_backup_pagecount(sqlite3_backup *p);
# 9430 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_unlock_notify(
  sqlite3 *pBlocked,
  void (*xNotify)(void **apArg, int nArg),
  void *pNotifyArg
);
# 9445 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_stricmp(const char *, const char *);
           int sqlite3_strnicmp(const char *, const char *, int);
# 9463 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_strglob(const char *zGlob, const char *zStr);
# 9486 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_strlike(const char *zGlob, const char *zStr, unsigned int cEsc);
# 9509 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           void sqlite3_log(int iErrCode, const char *zFormat, ...);
# 9546 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           void *sqlite3_wal_hook(
  sqlite3*,
  int(*)(void *,sqlite3*,const char*,int),
  void*
);
# 9581 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_wal_autocheckpoint(sqlite3 *db, int N);
# 9603 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_wal_checkpoint(sqlite3 *db, const char *zDb);
# 9697 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_wal_checkpoint_v2(
  sqlite3 *db,
  const char *zDb,
  int eMode,
  int *pnLog,
  int *pnCkpt
);
# 9737 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_vtab_config(sqlite3*, int op, ...);
# 9825 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_vtab_on_conflict(sqlite3 *);
# 9851 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_vtab_nochange(sqlite3_context*);
# 9886 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           const char *sqlite3_vtab_collation(sqlite3_index_info*,int);
# 9959 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_vtab_distinct(sqlite3_index_info*);
# 10032 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_vtab_in(sqlite3_index_info*, int iCons, int bHandle);
# 10079 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_vtab_in_first(sqlite3_value *pVal, sqlite3_value **ppOut);
           int sqlite3_vtab_in_next(sqlite3_value *pVal, sqlite3_value **ppOut);
# 10122 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_vtab_rhs_value(sqlite3_index_info*, int, sqlite3_value **ppVal);
# 10251 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_stmt_scanstatus(
  sqlite3_stmt *pStmt,
  int idx,
  int iScanStatusOp,
  void *pOut
);
           int sqlite3_stmt_scanstatus_v2(
  sqlite3_stmt *pStmt,
  int idx,
  int iScanStatusOp,
  int flags,
  void *pOut
);
# 10280 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           void sqlite3_stmt_scanstatus_reset(sqlite3_stmt*);
# 10313 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_db_cacheflush(sqlite3*);
# 10443 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_system_errno(sqlite3*);
# 10465 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
typedef struct sqlite3_snapshot {
  unsigned char hidden[48];
} sqlite3_snapshot;
# 10512 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
                               int sqlite3_snapshot_get(
  sqlite3 *db,
  const char *zSchema,
  sqlite3_snapshot **ppSnapshot
);
# 10561 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
                               int sqlite3_snapshot_open(
  sqlite3 *db,
  const char *zSchema,
  sqlite3_snapshot *pSnapshot
);
# 10578 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
                               void sqlite3_snapshot_free(sqlite3_snapshot*);
# 10605 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
                               int sqlite3_snapshot_cmp(
  sqlite3_snapshot *p1,
  sqlite3_snapshot *p2
);
# 10633 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
                               int sqlite3_snapshot_recover(sqlite3 *db, const char *zDb);
# 10678 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           unsigned char *sqlite3_serialize(
  sqlite3 *db,
  const char *zSchema,
  sqlite3_int64 *piSize,
  unsigned int mFlags
);
# 10744 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_deserialize(
  sqlite3 *db,
  const char *zSchema,
  unsigned char *pData,
  sqlite3_int64 szDb,
  sqlite3_int64 szBuf,
  unsigned mFlags
);
# 10826 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
typedef struct sqlite3_rtree_geometry sqlite3_rtree_geometry;
typedef struct sqlite3_rtree_query_info sqlite3_rtree_query_info;







  typedef double sqlite3_rtree_dbl;
# 10844 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
           int sqlite3_rtree_geometry_callback(
  sqlite3 *db,
  const char *zGeom,
  int (*xGeom)(sqlite3_rtree_geometry*, int, sqlite3_rtree_dbl*,int*),
  void *pContext
);






struct sqlite3_rtree_geometry {
  void *pContext;
  int nParam;
  sqlite3_rtree_dbl *aParam;
  void *pUser;
  void (*xDelUser)(void *);
};







           int sqlite3_rtree_query_callback(
  sqlite3 *db,
  const char *zQueryFunc,
  int (*xQueryFunc)(sqlite3_rtree_query_info*),
  void *pContext,
  void (*xDestructor)(void*)
);
# 10888 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
struct sqlite3_rtree_query_info {
  void *pContext;
  int nParam;
  sqlite3_rtree_dbl *aParam;
  void *pUser;
  void (*xDelUser)(void*);
  sqlite3_rtree_dbl *aCoord;
  unsigned int *anQueue;
  int nCoord;
  int iLevel;
  int mxLevel;
  sqlite3_int64 iRowid;
  sqlite3_rtree_dbl rParentScore;
  int eParentWithin;
  int eWithin;
  sqlite3_rtree_dbl rScore;

  sqlite3_value **apSqlParam;
};
# 12765 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
typedef struct Fts5ExtensionApi Fts5ExtensionApi;
typedef struct Fts5Context Fts5Context;
typedef struct Fts5PhraseIter Fts5PhraseIter;

typedef void (*fts5_extension_function)(
  const Fts5ExtensionApi *pApi,
  Fts5Context *pFts,
  sqlite3_context *pCtx,
  int nVal,
  sqlite3_value **apVal
);

struct Fts5PhraseIter {
  const unsigned char *a;
  const unsigned char *b;
};
# 13036 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
struct Fts5ExtensionApi {
  int iVersion;

  void *(*xUserData)(Fts5Context*);

  int (*xColumnCount)(Fts5Context*);
  int (*xRowCount)(Fts5Context*, sqlite3_int64 *pnRow);
  int (*xColumnTotalSize)(Fts5Context*, int iCol, sqlite3_int64 *pnToken);

  int (*xTokenize)(Fts5Context*,
    const char *pText, int nText,
    void *pCtx,
    int (*xToken)(void*, int, const char*, int, int, int)
  );

  int (*xPhraseCount)(Fts5Context*);
  int (*xPhraseSize)(Fts5Context*, int iPhrase);

  int (*xInstCount)(Fts5Context*, int *pnInst);
  int (*xInst)(Fts5Context*, int iIdx, int *piPhrase, int *piCol, int *piOff);

  sqlite3_int64 (*xRowid)(Fts5Context*);
  int (*xColumnText)(Fts5Context*, int iCol, const char **pz, int *pn);
  int (*xColumnSize)(Fts5Context*, int iCol, int *pnToken);

  int (*xQueryPhrase)(Fts5Context*, int iPhrase, void *pUserData,
    int(*)(const Fts5ExtensionApi*,Fts5Context*,void*)
  );
  int (*xSetAuxdata)(Fts5Context*, void *pAux, void(*xDelete)(void*));
  void *(*xGetAuxdata)(Fts5Context*, int bClear);

  int (*xPhraseFirst)(Fts5Context*, int iPhrase, Fts5PhraseIter*, int*, int*);
  void (*xPhraseNext)(Fts5Context*, Fts5PhraseIter*, int *piCol, int *piOff);

  int (*xPhraseFirstColumn)(Fts5Context*, int iPhrase, Fts5PhraseIter*, int*);
  void (*xPhraseNextColumn)(Fts5Context*, Fts5PhraseIter*, int *piCol);


  int (*xQueryToken)(Fts5Context*,
      int iPhrase, int iToken,
      const char **ppToken, int *pnToken
  );
  int (*xInstToken)(Fts5Context*, int iIdx, int iToken, const char**, int*);
};
# 13277 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
typedef struct Fts5Tokenizer Fts5Tokenizer;
typedef struct fts5_tokenizer fts5_tokenizer;
struct fts5_tokenizer {
  int (*xCreate)(void*, const char **azArg, int nArg, Fts5Tokenizer **ppOut);
  void (*xDelete)(Fts5Tokenizer*);
  int (*xTokenize)(Fts5Tokenizer*,
      void *pCtx,
      int flags,
      const char *pText, int nText,
      int (*xToken)(
        void *pCtx,
        int tflags,
        const char *pToken,
        int nToken,
        int iStart,
        int iEnd
      )
  );
};
# 13314 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3.h"
typedef struct fts5_api fts5_api;
struct fts5_api {
  int iVersion;


  int (*xCreateTokenizer)(
    fts5_api *pApi,
    const char *zName,
    void *pUserData,
    fts5_tokenizer *pTokenizer,
    void (*xDestroy)(void*)
  );


  int (*xFindTokenizer)(
    fts5_api *pApi,
    const char *zName,
    void **ppUserData,
    fts5_tokenizer *pTokenizer
  );


  int (*xCreateFunction)(
    fts5_api *pApi,
    const char *zName,
    void *pUserData,
    fts5_extension_function xFunction,
    void (*xDestroy)(void*)
  );
};
# 21 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3ext.h" 2
# 32 "/home/gwillen/sqlite-graph/build/_deps/sqlite3-src/sqlite3ext.h"
struct sqlite3_api_routines {
  void * (*aggregate_context)(sqlite3_context*,int nBytes);
  int (*aggregate_count)(sqlite3_context*);
  int (*bind_blob)(sqlite3_stmt*,int,const void*,int n,void(*)(void*));
  int (*bind_double)(sqlite3_stmt*,int,double);
  int (*bind_int)(sqlite3_stmt*,int,int);
  int (*bind_int64)(sqlite3_stmt*,int,sqlite_int64);
  int (*bind_null)(sqlite3_stmt*,int);
  int (*bind_parameter_count)(sqlite3_stmt*);
  int (*bind_parameter_index)(sqlite3_stmt*,const char*zName);
  const char * (*bind_parameter_name)(sqlite3_stmt*,int);
  int (*bind_text)(sqlite3_stmt*,int,const char*,int n,void(*)(void*));
  int (*bind_text16)(sqlite3_stmt*,int,const void*,int,void(*)(void*));
  int (*bind_value)(sqlite3_stmt*,int,const sqlite3_value*);
  int (*busy_handler)(sqlite3*,int(*)(void*,int),void*);
  int (*busy_timeout)(sqlite3*,int ms);
  int (*changes)(sqlite3*);
  int (*close)(sqlite3*);
  int (*collation_needed)(sqlite3*,void*,void(*)(void*,sqlite3*,
                           int eTextRep,const char*));
  int (*collation_needed16)(sqlite3*,void*,void(*)(void*,sqlite3*,
                             int eTextRep,const void*));
  const void * (*column_blob)(sqlite3_stmt*,int iCol);
  int (*column_bytes)(sqlite3_stmt*,int iCol);
  int (*column_bytes16)(sqlite3_stmt*,int iCol);
  int (*column_count)(sqlite3_stmt*pStmt);
  const char * (*column_database_name)(sqlite3_stmt*,int);
  const void * (*column_database_name16)(sqlite3_stmt*,int);
  const char * (*column_decltype)(sqlite3_stmt*,int i);
  const void * (*column_decltype16)(sqlite3_stmt*,int);
  double (*column_double)(sqlite3_stmt*,int iCol);
  int (*column_int)(sqlite3_stmt*,int iCol);
  sqlite_int64 (*column_int64)(sqlite3_stmt*,int iCol);
  const char * (*column_name)(sqlite3_stmt*,int);
  const void * (*column_name16)(sqlite3_stmt*,int);
  const char * (*column_origin_name)(sqlite3_stmt*,int);
  const void * (*column_origin_name16)(sqlite3_stmt*,int);
  const char * (*column_table_name)(sqlite3_stmt*,int);
  const void * (*column_table_name16)(sqlite3_stmt*,int);
  const unsigned char * (*column_text)(sqlite3_stmt*,int iCol);
  const void * (*column_text16)(sqlite3_stmt*,int iCol);
  int (*column_type)(sqlite3_stmt*,int iCol);
  sqlite3_value* (*column_value)(sqlite3_stmt*,int iCol);
  void * (*commit_hook)(sqlite3*,int(*)(void*),void*);
  int (*complete)(const char*sql);
  int (*complete16)(const void*sql);
  int (*create_collation)(sqlite3*,const char*,int,void*,
                           int(*)(void*,int,const void*,int,const void*));
  int (*create_collation16)(sqlite3*,const void*,int,void*,
                             int(*)(void*,int,const void*,int,const void*));
  int (*create_function)(sqlite3*,const char*,int,int,void*,
                          void (*xFunc)(sqlite3_context*,int,sqlite3_value**),
                          void (*xStep)(sqlite3_context*,int,sqlite3_value**),
                          void (*xFinal)(sqlite3_context*));
  int (*create_function16)(sqlite3*,const void*,int,int,void*,
                            void (*xFunc)(sqlite3_context*,int,sqlite3_value**),
                            void (*xStep)(sqlite3_context*,int,sqlite3_value**),
                            void (*xFinal)(sqlite3_context*));
  int (*create_module)(sqlite3*,const char*,const sqlite3_module*,void*);
  int (*data_count)(sqlite3_stmt*pStmt);
  sqlite3 * (*db_handle)(sqlite3_stmt*);
  int (*declare_vtab)(sqlite3*,const char*);
  int (*enable_shared_cache)(int);
  int (*errcode)(sqlite3*db);
  const char * (*errmsg)(sqlite3*);
  const void * (*errmsg16)(sqlite3*);
  int (*exec)(sqlite3*,const char*,sqlite3_callback,void*,char**);
  int (*expired)(sqlite3_stmt*);
  int (*finalize)(sqlite3_stmt*pStmt);
  void (*free)(void*);
  void (*free_table)(char**result);
  int (*get_autocommit)(sqlite3*);
  void * (*get_auxdata)(sqlite3_context*,int);
  int (*get_table)(sqlite3*,const char*,char***,int*,int*,char**);
  int (*global_recover)(void);
  void (*interruptx)(sqlite3*);
  sqlite_int64 (*last_insert_rowid)(sqlite3*);
  const char * (*libversion)(void);
  int (*libversion_number)(void);
  void *(*malloc)(int);
  char * (*mprintf)(const char*,...);
  int (*open)(const char*,sqlite3**);
  int (*open16)(const void*,sqlite3**);
  int (*prepare)(sqlite3*,const char*,int,sqlite3_stmt**,const char**);
  int (*prepare16)(sqlite3*,const void*,int,sqlite3_stmt**,const void**);
  void * (*profile)(sqlite3*,void(*)(void*,const char*,sqlite_uint64),void*);
  void (*progress_handler)(sqlite3*,int,int(*)(void*),void*);
  void *(*realloc)(void*,int);
  int (*reset)(sqlite3_stmt*pStmt);
  void (*result_blob)(sqlite3_context*,const void*,int,void(*)(void*));
  void (*result_double)(sqlite3_context*,double);
  void (*result_error)(sqlite3_context*,const char*,int);
  void (*result_error16)(sqlite3_context*,const void*,int);
  void (*result_int)(sqlite3_context*,int);
  void (*result_int64)(sqlite3_context*,sqlite_int64);
  void (*result_null)(sqlite3_context*);
  void (*result_text)(sqlite3_context*,const char*,int,void(*)(void*));
  void (*result_text16)(sqlite3_context*,const void*,int,void(*)(void*));
  void (*result_text16be)(sqlite3_context*,const void*,int,void(*)(void*));
  void (*result_text16le)(sqlite3_context*,const void*,int,void(*)(void*));
  void (*result_value)(sqlite3_context*,sqlite3_value*);
  void * (*rollback_hook)(sqlite3*,void(*)(void*),void*);
  int (*set_authorizer)(sqlite3*,int(*)(void*,int,const char*,const char*,
                         const char*,const char*),void*);
  void (*set_auxdata)(sqlite3_context*,int,void*,void (*)(void*));
  char * (*xsnprintf)(int,char*,const char*,...);
  int (*step)(sqlite3_stmt*);
  int (*table_column_metadata)(sqlite3*,const char*,const char*,const char*,
                                char const**,char const**,int*,int*,int*);
  void (*thread_cleanup)(void);
  int (*total_changes)(sqlite3*);
  void * (*trace)(sqlite3*,void(*xTrace)(void*,const char*),void*);
  int (*transfer_bindings)(sqlite3_stmt*,sqlite3_stmt*);
  void * (*update_hook)(sqlite3*,void(*)(void*,int ,char const*,char const*,
                                         sqlite_int64),void*);
  void * (*user_data)(sqlite3_context*);
  const void * (*value_blob)(sqlite3_value*);
  int (*value_bytes)(sqlite3_value*);
  int (*value_bytes16)(sqlite3_value*);
  double (*value_double)(sqlite3_value*);
  int (*value_int)(sqlite3_value*);
  sqlite_int64 (*value_int64)(sqlite3_value*);
  int (*value_numeric_type)(sqlite3_value*);
  const unsigned char * (*value_text)(sqlite3_value*);
  const void * (*value_text16)(sqlite3_value*);
  const void * (*value_text16be)(sqlite3_value*);
  const void * (*value_text16le)(sqlite3_value*);
  int (*value_type)(sqlite3_value*);
  char *(*vmprintf)(const char*,va_list);

  int (*overload_function)(sqlite3*, const char *zFuncName, int nArg);

  int (*prepare_v2)(sqlite3*,const char*,int,sqlite3_stmt**,const char**);
  int (*prepare16_v2)(sqlite3*,const void*,int,sqlite3_stmt**,const void**);
  int (*clear_bindings)(sqlite3_stmt*);

  int (*create_module_v2)(sqlite3*,const char*,const sqlite3_module*,void*,
                          void (*xDestroy)(void *));

  int (*bind_zeroblob)(sqlite3_stmt*,int,int);
  int (*blob_bytes)(sqlite3_blob*);
  int (*blob_close)(sqlite3_blob*);
  int (*blob_open)(sqlite3*,const char*,const char*,const char*,sqlite3_int64,
                   int,sqlite3_blob**);
  int (*blob_read)(sqlite3_blob*,void*,int,int);
  int (*blob_write)(sqlite3_blob*,const void*,int,int);
  int (*create_collation_v2)(sqlite3*,const char*,int,void*,
                             int(*)(void*,int,const void*,int,const void*),
                             void(*)(void*));
  int (*file_control)(sqlite3*,const char*,int,void*);
  sqlite3_int64 (*memory_highwater)(int);
  sqlite3_int64 (*memory_used)(void);
  sqlite3_mutex *(*mutex_alloc)(int);
  void (*mutex_enter)(sqlite3_mutex*);
  void (*mutex_free)(sqlite3_mutex*);
  void (*mutex_leave)(sqlite3_mutex*);
  int (*mutex_try)(sqlite3_mutex*);
  int (*open_v2)(const char*,sqlite3**,int,const char*);
  int (*release_memory)(int);
  void (*result_error_nomem)(sqlite3_context*);
  void (*result_error_toobig)(sqlite3_context*);
  int (*sleep)(int);
  void (*soft_heap_limit)(int);
  sqlite3_vfs *(*vfs_find)(const char*);
  int (*vfs_register)(sqlite3_vfs*,int);
  int (*vfs_unregister)(sqlite3_vfs*);
  int (*xthreadsafe)(void);
  void (*result_zeroblob)(sqlite3_context*,int);
  void (*result_error_code)(sqlite3_context*,int);
  int (*test_control)(int, ...);
  void (*randomness)(int,void*);
  sqlite3 *(*context_db_handle)(sqlite3_context*);
  int (*extended_result_codes)(sqlite3*,int);
  int (*limit)(sqlite3*,int,int);
  sqlite3_stmt *(*next_stmt)(sqlite3*,sqlite3_stmt*);
  const char *(*sql)(sqlite3_stmt*);
  int (*status)(int,int*,int*,int);
  int (*backup_finish)(sqlite3_backup*);
  sqlite3_backup *(*backup_init)(sqlite3*,const char*,sqlite3*,const char*);
  int (*backup_pagecount)(sqlite3_backup*);
  int (*backup_remaining)(sqlite3_backup*);
  int (*backup_step)(sqlite3_backup*,int);
  const char *(*compileoption_get)(int);
  int (*compileoption_used)(const char*);
  int (*create_function_v2)(sqlite3*,const char*,int,int,void*,
                            void (*xFunc)(sqlite3_context*,int,sqlite3_value**),
                            void (*xStep)(sqlite3_context*,int,sqlite3_value**),
                            void (*xFinal)(sqlite3_context*),
                            void(*xDestroy)(void*));
  int (*db_config)(sqlite3*,int,...);
  sqlite3_mutex *(*db_mutex)(sqlite3*);
  int (*db_status)(sqlite3*,int,int*,int*,int);
  int (*extended_errcode)(sqlite3*);
  void (*log)(int,const char*,...);
  sqlite3_int64 (*soft_heap_limit64)(sqlite3_int64);
  const char *(*sourceid)(void);
  int (*stmt_status)(sqlite3_stmt*,int,int);
  int (*strnicmp)(const char*,const char*,int);
  int (*unlock_notify)(sqlite3*,void(*)(void**,int),void*);
  int (*wal_autocheckpoint)(sqlite3*,int);
  int (*wal_checkpoint)(sqlite3*,const char*);
  void *(*wal_hook)(sqlite3*,int(*)(void*,sqlite3*,const char*,int),void*);
  int (*blob_reopen)(sqlite3_blob*,sqlite3_int64);
  int (*vtab_config)(sqlite3*,int op,...);
  int (*vtab_on_conflict)(sqlite3*);

  int (*close_v2)(sqlite3*);
  const char *(*db_filename)(sqlite3*,const char*);
  int (*db_readonly)(sqlite3*,const char*);
  int (*db_release_memory)(sqlite3*);
  const char *(*errstr)(int);
  int (*stmt_busy)(sqlite3_stmt*);
  int (*stmt_readonly)(sqlite3_stmt*);
  int (*stricmp)(const char*,const char*);
  int (*uri_boolean)(const char*,const char*,int);
  sqlite3_int64 (*uri_int64)(const char*,const char*,sqlite3_int64);
  const char *(*uri_parameter)(const char*,const char*);
  char *(*xvsnprintf)(int,char*,const char*,va_list);
  int (*wal_checkpoint_v2)(sqlite3*,const char*,int,int*,int*);

  int (*auto_extension)(void(*)(void));
  int (*bind_blob64)(sqlite3_stmt*,int,const void*,sqlite3_uint64,
                     void(*)(void*));
  int (*bind_text64)(sqlite3_stmt*,int,const char*,sqlite3_uint64,
                      void(*)(void*),unsigned char);
  int (*cancel_auto_extension)(void(*)(void));
  int (*load_extension)(sqlite3*,const char*,const char*,char**);
  void *(*malloc64)(sqlite3_uint64);
  sqlite3_uint64 (*msize)(void*);
  void *(*realloc64)(void*,sqlite3_uint64);
  void (*reset_auto_extension)(void);
  void (*result_blob64)(sqlite3_context*,const void*,sqlite3_uint64,
                        void(*)(void*));
  void (*result_text64)(sqlite3_context*,const char*,sqlite3_uint64,
                         void(*)(void*), unsigned char);
  int (*strglob)(const char*,const char*);

  sqlite3_value *(*value_dup)(const sqlite3_value*);
  void (*value_free)(sqlite3_value*);
  int (*result_zeroblob64)(sqlite3_context*,sqlite3_uint64);
  int (*bind_zeroblob64)(sqlite3_stmt*, int, sqlite3_uint64);

  unsigned int (*value_subtype)(sqlite3_value*);
  void (*result_subtype)(sqlite3_context*,unsigned int);

  int (*status64)(int,sqlite3_int64*,sqlite3_int64*,int);
  int (*strlike)(const char*,const char*,unsigned int);
  int (*db_cacheflush)(sqlite3*);

  int (*system_errno)(sqlite3*);

  int (*trace_v2)(sqlite3*,unsigned,int(*)(unsigned,void*,void*,void*),void*);
  char *(*expanded_sql)(sqlite3_stmt*);

  void (*set_last_insert_rowid)(sqlite3*,sqlite3_int64);

  int (*prepare_v3)(sqlite3*,const char*,int,unsigned int,
                    sqlite3_stmt**,const char**);
  int (*prepare16_v3)(sqlite3*,const void*,int,unsigned int,
                      sqlite3_stmt**,const void**);
  int (*bind_pointer)(sqlite3_stmt*,int,void*,const char*,void(*)(void*));
  void (*result_pointer)(sqlite3_context*,void*,const char*,void(*)(void*));
  void *(*value_pointer)(sqlite3_value*,const char*);
  int (*vtab_nochange)(sqlite3_context*);
  int (*value_nochange)(sqlite3_value*);
  const char *(*vtab_collation)(sqlite3_index_info*,int);

  int (*keyword_count)(void);
  int (*keyword_name)(int,const char**,int*);
  int (*keyword_check)(const char*,int);
  sqlite3_str *(*str_new)(sqlite3*);
  char *(*str_finish)(sqlite3_str*);
  void (*str_appendf)(sqlite3_str*, const char *zFormat, ...);
  void (*str_vappendf)(sqlite3_str*, const char *zFormat, va_list);
  void (*str_append)(sqlite3_str*, const char *zIn, int N);
  void (*str_appendall)(sqlite3_str*, const char *zIn);
  void (*str_appendchar)(sqlite3_str*, int N, char C);
  void (*str_reset)(sqlite3_str*);
  int (*str_errcode)(sqlite3_str*);
  int (*str_length)(sqlite3_str*);
  char *(*str_value)(sqlite3_str*);

  int (*create_window_function)(sqlite3*,const char*,int,int,void*,
                            void (*xStep)(sqlite3_context*,int,sqlite3_value**),
                            void (*xFinal)(sqlite3_context*),
                            void (*xValue)(sqlite3_context*),
                            void (*xInv)(sqlite3_context*,int,sqlite3_value**),
                            void(*xDestroy)(void*));

  const char *(*normalized_sql)(sqlite3_stmt*);

  int (*stmt_isexplain)(sqlite3_stmt*);
  int (*value_frombind)(sqlite3_value*);

  int (*drop_modules)(sqlite3*,const char**);

  sqlite3_int64 (*hard_heap_limit64)(sqlite3_int64);
  const char *(*uri_key)(const char*,int);
  const char *(*filename_database)(const char*);
  const char *(*filename_journal)(const char*);
  const char *(*filename_wal)(const char*);

  const char *(*create_filename)(const char*,const char*,const char*,
                           int,const char**);
  void (*free_filename)(const char*);
  sqlite3_file *(*database_file_object)(const char*);

  int (*txn_state)(sqlite3*,const char*);

  sqlite3_int64 (*changes64)(sqlite3*);
  sqlite3_int64 (*total_changes64)(sqlite3*);

  int (*autovacuum_pages)(sqlite3*,
     unsigned int(*)(void*,const char*,unsigned int,unsigned int,unsigned int),
     void*, void(*)(void*));

  int (*error_offset)(sqlite3*);
  int (*vtab_rhs_value)(sqlite3_index_info*,int,sqlite3_value**);
  int (*vtab_distinct)(sqlite3_index_info*);
  int (*vtab_in)(sqlite3_index_info*,int,int);
  int (*vtab_in_first)(sqlite3_value*,sqlite3_value**);
  int (*vtab_in_next)(sqlite3_value*,sqlite3_value**);

  int (*deserialize)(sqlite3*,const char*,unsigned char*,
                     sqlite3_int64,sqlite3_int64,unsigned);
  unsigned char *(*serialize)(sqlite3*,const char *,sqlite3_int64*,
                              unsigned int);
  const char *(*db_name)(sqlite3*,int);

  int (*value_encoding)(sqlite3_value*);

  int (*is_interrupted)(sqlite3*);

  int (*stmt_explain)(sqlite3_stmt*,int);

  void *(*get_clientdata)(sqlite3*,const char*);
  int (*set_clientdata)(sqlite3*, const char*, void*, void(*)(void*));
};





typedef int (*sqlite3_loadext_entry)(
  sqlite3 *db,
  char **pzErrMsg,
  const sqlite3_api_routines *pThunk
);
# 12 "/home/gwillen/sqlite-graph/src/graph-advanced.c" 2




# 1 "/home/gwillen/sqlite-graph/include/graph.h" 1
# 33 "/home/gwillen/sqlite-graph/include/graph.h"
typedef struct GraphNode GraphNode;
struct GraphNode {
  sqlite3_int64 iNodeId;
  char **azLabels;
  int nLabels;
  char *zProperties;
  GraphNode *pNext;
  GraphNode *pLabelNext;
};






typedef struct GraphEdge GraphEdge;
struct GraphEdge {
  sqlite3_int64 iEdgeId;
  sqlite3_int64 iFromId;
  sqlite3_int64 iToId;
  char *zType;
  double rWeight;
  char *zProperties;
  GraphEdge *pNext;
  GraphEdge *pFromNext;
  GraphEdge *pToNext;
};




typedef struct CypherSchema CypherSchema;
typedef struct GraphIndex GraphIndex;
typedef struct GraphPropertySchema GraphPropertySchema;






typedef struct GraphVtab GraphVtab;
struct GraphVtab {
  sqlite3_vtab base;
  sqlite3 *pDb;
  char *zDbName;
  char *zTableName;
  int nRef;
  int nNodes;
  int nEdges;
  GraphNode *pNodeList;
  GraphEdge *pEdgeList;
  GraphIndex *pLabelIndex;
  GraphIndex *pPropertyIndex;
  CypherSchema *pSchema;
  sqlite3_int64 iNextEdgeId;
};





typedef struct GraphCursor GraphCursor;
struct GraphCursor {
  sqlite3_vtab_cursor base;
  GraphVtab *pVtab;
  int iRowid;
  GraphNode *pCurrentNode;
  GraphEdge *pCurrentEdge;
  int iIterMode;
};
# 116 "/home/gwillen/sqlite-graph/include/graph.h"
int graphAddNode(GraphVtab *pVtab, sqlite3_int64 iNodeId,
                 const char *zProperties);






int graphRemoveNode(GraphVtab *pVtab, sqlite3_int64 iNodeId);







int graphGetNode(GraphVtab *pVtab, sqlite3_int64 iNodeId,
                 char **pzProperties);







int graphAddEdge(GraphVtab *pVtab, sqlite3_int64 iFromId,
                 sqlite3_int64 iToId, double rWeight,
                 const char *zProperties);





int graphRemoveEdge(GraphVtab *pVtab, sqlite3_int64 iFromId,
                    sqlite3_int64 iToId);






int graphUpdateNode(GraphVtab *pVtab, sqlite3_int64 iNodeId,
                    const char *zProperties);






int graphGetEdge(GraphVtab *pVtab, sqlite3_int64 iFromId,
                 sqlite3_int64 iToId, double *prWeight,
                 char **pzProperties);





int graphCountNodes(GraphVtab *pVtab);
int graphCountEdges(GraphVtab *pVtab);





GraphNode *graphFindNode(GraphVtab *pVtab, sqlite3_int64 iNodeId);





GraphEdge *graphFindEdge(GraphVtab *pVtab, sqlite3_int64 iFromId,
                         sqlite3_int64 iToId);
# 200 "/home/gwillen/sqlite-graph/include/graph.h"
int graphDFS(GraphVtab *pVtab, sqlite3_int64 iStartId, int nMaxDepth,
             char **pzPath);







int graphBFS(GraphVtab *pVtab, sqlite3_int64 iStartId, int nMaxDepth,
             char **pzPath);
# 223 "/home/gwillen/sqlite-graph/include/graph.h"
int graphDijkstra(GraphVtab *pVtab, sqlite3_int64 iStartId,
                  sqlite3_int64 iEndId, char **pzPath, double *prDistance);





int graphShortestPathUnweighted(GraphVtab *pVtab, sqlite3_int64 iStartId,
                                sqlite3_int64 iEndId, char **pzPath);
# 240 "/home/gwillen/sqlite-graph/include/graph.h"
int graphPageRank(GraphVtab *pVtab, double rDamping, int nMaxIter,
                  double rEpsilon, char **pzResults);




int graphInDegree(GraphVtab *pVtab, sqlite3_int64 iNodeId);
int graphOutDegree(GraphVtab *pVtab, sqlite3_int64 iNodeId);
int graphTotalDegree(GraphVtab *pVtab, sqlite3_int64 iNodeId);
double graphDegreeCentrality(GraphVtab *pVtab, sqlite3_int64 iNodeId,
                            int bDirected);




int graphIsConnected(GraphVtab *pVtab);
double graphDensity(GraphVtab *pVtab, int bDirected);
# 267 "/home/gwillen/sqlite-graph/include/graph.h"
int graphBetweennessCentrality(GraphVtab *pVtab, char **pzResults);






int graphClosenessCentrality(GraphVtab *pVtab, char **pzResults);






int graphTopologicalSort(GraphVtab *pVtab, char **pzOrder);





int graphHasCycle(GraphVtab *pVtab);






int graphConnectedComponents(GraphVtab *pVtab, char **pzComponents);






int graphStronglyConnectedComponents(GraphVtab *pVtab, char **pzSCC);
# 311 "/home/gwillen/sqlite-graph/include/graph.h"
struct CypherSchema {
  char **azNodeLabels;
  int nNodeLabels;
  char **azRelTypes;
  int nRelTypes;
  GraphPropertySchema *pPropSchema;
};





struct GraphIndex {
  char *zIndexName;
  int iIndexType;
  GraphNode **apNodes;
  int nBuckets;
  int nEntries;
};





struct GraphPropertySchema {
  char *zLabelOrType;
  char **azProperties;
  int *aiPropertyTypes;
  int nProperties;
  GraphPropertySchema *pNext;
};
# 353 "/home/gwillen/sqlite-graph/include/graph.h"
int graphAddNodeWithLabels(GraphVtab *pVtab, sqlite3_int64 iNodeId,
                           const char **azLabels, int nLabels,
                           const char *zProperties);





int graphAddEdgeWithType(GraphVtab *pVtab, sqlite3_int64 iFromId,
                         sqlite3_int64 iToId, const char *zType,
                         double rWeight, const char *zProperties);





int graphSetNodeLabels(GraphVtab *pVtab, sqlite3_int64 iNodeId,
                       const char **azLabels, int nLabels);





int graphAddNodeLabel(GraphVtab *pVtab, sqlite3_int64 iNodeId,
                      const char *zLabel);




int graphRemoveNodeLabel(GraphVtab *pVtab, sqlite3_int64 iNodeId,
                         const char *zLabel);





int graphGetNodeLabels(GraphVtab *pVtab, sqlite3_int64 iNodeId,
                       char **pzLabels);





int graphNodeHasLabel(GraphVtab *pVtab, sqlite3_int64 iNodeId,
                      const char *zLabel);
# 407 "/home/gwillen/sqlite-graph/include/graph.h"
int graphInitSchema(GraphVtab *pVtab);




void graphDestroySchema(CypherSchema *pSchema);





int graphRegisterLabel(CypherSchema *pSchema, const char *zLabel);




int graphRegisterRelationshipType(CypherSchema *pSchema, const char *zType);
# 433 "/home/gwillen/sqlite-graph/include/graph.h"
int graphCreateLabelIndex(GraphVtab *pVtab, const char *zLabel);






int graphCreatePropertyIndex(GraphVtab *pVtab, const char *zLabel,
                             const char *zProperty);





GraphNode *graphFindNodesByLabel(GraphVtab *pVtab, const char *zLabel);





GraphEdge *graphFindEdgesByType(GraphVtab *pVtab, const char *zType);
# 463 "/home/gwillen/sqlite-graph/include/graph.h"
unsigned int graphHashString(const char *zString);





int graphLabelsEqual(const char **azLabels1, int nLabels1,
                     const char **azLabels2, int nLabels2);





char **graphCopyLabels(const char **azLabels, int nLabels);




void graphFreeLabels(char **azLabels, int nLabels);
# 491 "/home/gwillen/sqlite-graph/include/graph.h"
int graphDiscoverSchema(GraphVtab *pVtab);






int graphGetSchemaInfo(GraphVtab *pVtab, char **pzSchemaInfo);





int graphValidateNodeLabels(GraphVtab *pVtab, const char **azLabels, int nLabels);





int graphValidateRelationshipType(GraphVtab *pVtab, const char *zType);





int graphRebuildIndexes(GraphVtab *pVtab);
# 17 "/home/gwillen/sqlite-graph/src/graph-advanced.c" 2
# 1 "/usr/lib/llvm-18/lib/clang/18/include/float.h" 1 3
# 18 "/home/gwillen/sqlite-graph/src/graph-advanced.c" 2
# 1 "/usr/include/string.h" 1 3 4
# 26 "/usr/include/string.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/libc-header-start.h" 1 3 4
# 33 "/usr/include/x86_64-linux-gnu/bits/libc-header-start.h" 3 4
# 1 "/usr/include/features.h" 1 3 4
# 394 "/usr/include/features.h" 3 4
# 1 "/usr/include/features-time64.h" 1 3 4
# 20 "/usr/include/features-time64.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/wordsize.h" 1 3 4
# 21 "/usr/include/features-time64.h" 2 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/timesize.h" 1 3 4
# 19 "/usr/include/x86_64-linux-gnu/bits/timesize.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/wordsize.h" 1 3 4
# 20 "/usr/include/x86_64-linux-gnu/bits/timesize.h" 2 3 4
# 22 "/usr/include/features-time64.h" 2 3 4
# 395 "/usr/include/features.h" 2 3 4
# 480 "/usr/include/features.h" 3 4
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 481 "/usr/include/features.h" 2 3 4
# 502 "/usr/include/features.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/sys/cdefs.h" 1 3 4
# 576 "/usr/include/x86_64-linux-gnu/sys/cdefs.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/wordsize.h" 1 3 4
# 577 "/usr/include/x86_64-linux-gnu/sys/cdefs.h" 2 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/long-double.h" 1 3 4
# 578 "/usr/include/x86_64-linux-gnu/sys/cdefs.h" 2 3 4
# 503 "/usr/include/features.h" 2 3 4
# 526 "/usr/include/features.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/gnu/stubs.h" 1 3 4
# 10 "/usr/include/x86_64-linux-gnu/gnu/stubs.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/gnu/stubs-64.h" 1 3 4
# 11 "/usr/include/x86_64-linux-gnu/gnu/stubs.h" 2 3 4
# 527 "/usr/include/features.h" 2 3 4
# 34 "/usr/include/x86_64-linux-gnu/bits/libc-header-start.h" 2 3 4
# 27 "/usr/include/string.h" 2 3 4






# 1 "/usr/lib/llvm-18/lib/clang/18/include/stddef.h" 1 3 4
# 77 "/usr/lib/llvm-18/lib/clang/18/include/stddef.h" 3 4
# 1 "/usr/lib/llvm-18/lib/clang/18/include/__stddef_size_t.h" 1 3 4
# 18 "/usr/lib/llvm-18/lib/clang/18/include/__stddef_size_t.h" 3 4
typedef long unsigned int size_t;
# 78 "/usr/lib/llvm-18/lib/clang/18/include/stddef.h" 2 3 4
# 92 "/usr/lib/llvm-18/lib/clang/18/include/stddef.h" 3 4
# 1 "/usr/lib/llvm-18/lib/clang/18/include/__stddef_null.h" 1 3 4
# 93 "/usr/lib/llvm-18/lib/clang/18/include/stddef.h" 2 3 4
# 34 "/usr/include/string.h" 2 3 4
# 43 "/usr/include/string.h" 3 4
extern void *memcpy (void *__restrict __dest, const void *__restrict __src,
       size_t __n) __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1, 2)));


extern void *memmove (void *__dest, const void *__src, size_t __n)
     __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1, 2)));





extern void *memccpy (void *__restrict __dest, const void *__restrict __src,
        int __c, size_t __n)
    __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1, 2))) ;




extern void *memset (void *__s, int __c, size_t __n) __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1)));


extern int memcmp (const void *__s1, const void *__s2, size_t __n)
     __attribute__ ((__nothrow__ )) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));
# 80 "/usr/include/string.h" 3 4
extern int __memcmpeq (const void *__s1, const void *__s2, size_t __n)
     __attribute__ ((__nothrow__ )) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));
# 107 "/usr/include/string.h" 3 4
extern void *memchr (const void *__s, int __c, size_t __n)
      __attribute__ ((__nothrow__ )) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));
# 141 "/usr/include/string.h" 3 4
extern char *strcpy (char *__restrict __dest, const char *__restrict __src)
     __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1, 2)));

extern char *strncpy (char *__restrict __dest,
        const char *__restrict __src, size_t __n)
     __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1, 2)));


extern char *strcat (char *__restrict __dest, const char *__restrict __src)
     __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1, 2)));

extern char *strncat (char *__restrict __dest, const char *__restrict __src,
        size_t __n) __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1, 2)));


extern int strcmp (const char *__s1, const char *__s2)
     __attribute__ ((__nothrow__ )) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));

extern int strncmp (const char *__s1, const char *__s2, size_t __n)
     __attribute__ ((__nothrow__ )) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));


extern int strcoll (const char *__s1, const char *__s2)
     __attribute__ ((__nothrow__ )) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));

extern size_t strxfrm (char *__restrict __dest,
         const char *__restrict __src, size_t __n)
    __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (2))) ;



# 1 "/usr/include/x86_64-linux-gnu/bits/types/locale_t.h" 1 3 4
# 22 "/usr/include/x86_64-linux-gnu/bits/types/locale_t.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/types/__locale_t.h" 1 3 4
# 27 "/usr/include/x86_64-linux-gnu/bits/types/__locale_t.h" 3 4
struct __locale_struct
{

  struct __locale_data *__locales[13];


  const unsigned short int *__ctype_b;
  const int *__ctype_tolower;
  const int *__ctype_toupper;


  const char *__names[13];
};

typedef struct __locale_struct *__locale_t;
# 23 "/usr/include/x86_64-linux-gnu/bits/types/locale_t.h" 2 3 4

typedef __locale_t locale_t;
# 173 "/usr/include/string.h" 2 3 4


extern int strcoll_l (const char *__s1, const char *__s2, locale_t __l)
     __attribute__ ((__nothrow__ )) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2, 3)));


extern size_t strxfrm_l (char *__dest, const char *__src, size_t __n,
    locale_t __l) __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (2, 4)))
                                           ;





extern char *strdup (const char *__s)
     __attribute__ ((__nothrow__ )) __attribute__ ((__malloc__)) __attribute__ ((__nonnull__ (1)));






extern char *strndup (const char *__string, size_t __n)
     __attribute__ ((__nothrow__ )) __attribute__ ((__malloc__)) __attribute__ ((__nonnull__ (1)));
# 246 "/usr/include/string.h" 3 4
extern char *strchr (const char *__s, int __c)
     __attribute__ ((__nothrow__ )) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));
# 273 "/usr/include/string.h" 3 4
extern char *strrchr (const char *__s, int __c)
     __attribute__ ((__nothrow__ )) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));
# 286 "/usr/include/string.h" 3 4
extern char *strchrnul (const char *__s, int __c)
     __attribute__ ((__nothrow__ )) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));





extern size_t strcspn (const char *__s, const char *__reject)
     __attribute__ ((__nothrow__ )) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));


extern size_t strspn (const char *__s, const char *__accept)
     __attribute__ ((__nothrow__ )) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));
# 323 "/usr/include/string.h" 3 4
extern char *strpbrk (const char *__s, const char *__accept)
     __attribute__ ((__nothrow__ )) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));
# 350 "/usr/include/string.h" 3 4
extern char *strstr (const char *__haystack, const char *__needle)
     __attribute__ ((__nothrow__ )) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));




extern char *strtok (char *__restrict __s, const char *__restrict __delim)
     __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (2)));



extern char *__strtok_r (char *__restrict __s,
    const char *__restrict __delim,
    char **__restrict __save_ptr)
     __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (2, 3)));

extern char *strtok_r (char *__restrict __s, const char *__restrict __delim,
         char **__restrict __save_ptr)
     __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (2, 3)));
# 380 "/usr/include/string.h" 3 4
extern char *strcasestr (const char *__haystack, const char *__needle)
     __attribute__ ((__nothrow__ )) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));







extern void *memmem (const void *__haystack, size_t __haystacklen,
       const void *__needle, size_t __needlelen)
     __attribute__ ((__nothrow__ )) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 3)))

                                         ;



extern void *__mempcpy (void *__restrict __dest,
   const void *__restrict __src, size_t __n)
     __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1, 2)));
extern void *mempcpy (void *__restrict __dest,
        const void *__restrict __src, size_t __n)
     __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1, 2)));




extern size_t strlen (const char *__s)
     __attribute__ ((__nothrow__ )) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));




extern size_t strnlen (const char *__string, size_t __maxlen)
     __attribute__ ((__nothrow__ )) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));




extern char *strerror (int __errnum) __attribute__ ((__nothrow__ ));
# 432 "/usr/include/string.h" 3 4
extern int strerror_r (int __errnum, char *__buf, size_t __buflen) __asm__ ("" "__xpg_strerror_r") __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (2)))


                                          ;
# 458 "/usr/include/string.h" 3 4
extern char *strerror_l (int __errnum, locale_t __l) __attribute__ ((__nothrow__ ));



# 1 "/usr/include/strings.h" 1 3 4
# 23 "/usr/include/strings.h" 3 4
# 1 "/usr/lib/llvm-18/lib/clang/18/include/stddef.h" 1 3 4
# 77 "/usr/lib/llvm-18/lib/clang/18/include/stddef.h" 3 4
# 1 "/usr/lib/llvm-18/lib/clang/18/include/__stddef_size_t.h" 1 3 4
# 78 "/usr/lib/llvm-18/lib/clang/18/include/stddef.h" 2 3 4
# 24 "/usr/include/strings.h" 2 3 4
# 34 "/usr/include/strings.h" 3 4
extern int bcmp (const void *__s1, const void *__s2, size_t __n)
     __attribute__ ((__nothrow__ )) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));


extern void bcopy (const void *__src, void *__dest, size_t __n)
  __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1, 2)));


extern void bzero (void *__s, size_t __n) __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1)));
# 68 "/usr/include/strings.h" 3 4
extern char *index (const char *__s, int __c)
     __attribute__ ((__nothrow__ )) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));
# 96 "/usr/include/strings.h" 3 4
extern char *rindex (const char *__s, int __c)
     __attribute__ ((__nothrow__ )) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));






extern int ffs (int __i) __attribute__ ((__nothrow__ )) __attribute__ ((__const__));





extern int ffsl (long int __l) __attribute__ ((__nothrow__ )) __attribute__ ((__const__));
__extension__ extern int ffsll (long long int __ll)
     __attribute__ ((__nothrow__ )) __attribute__ ((__const__));



extern int strcasecmp (const char *__s1, const char *__s2)
     __attribute__ ((__nothrow__ )) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));


extern int strncasecmp (const char *__s1, const char *__s2, size_t __n)
     __attribute__ ((__nothrow__ )) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));






extern int strcasecmp_l (const char *__s1, const char *__s2, locale_t __loc)
     __attribute__ ((__nothrow__ )) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2, 3)));



extern int strncasecmp_l (const char *__s1, const char *__s2,
     size_t __n, locale_t __loc)
     __attribute__ ((__nothrow__ )) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2, 4)));
# 463 "/usr/include/string.h" 2 3 4



extern void explicit_bzero (void *__s, size_t __n) __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1)))
                                                  ;



extern char *strsep (char **__restrict __stringp,
       const char *__restrict __delim)
     __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1, 2)));




extern char *strsignal (int __sig) __attribute__ ((__nothrow__ ));
# 489 "/usr/include/string.h" 3 4
extern char *__stpcpy (char *__restrict __dest, const char *__restrict __src)
     __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1, 2)));
extern char *stpcpy (char *__restrict __dest, const char *__restrict __src)
     __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1, 2)));



extern char *__stpncpy (char *__restrict __dest,
   const char *__restrict __src, size_t __n)
     __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1, 2)));
extern char *stpncpy (char *__restrict __dest,
        const char *__restrict __src, size_t __n)
     __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1, 2)));




extern size_t strlcpy (char *__restrict __dest,
         const char *__restrict __src, size_t __n)
  __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1, 2))) ;



extern size_t strlcat (char *__restrict __dest,
         const char *__restrict __src, size_t __n)
  __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1, 2))) ;
# 19 "/home/gwillen/sqlite-graph/src/graph-advanced.c" 2
# 1 "/usr/include/stdlib.h" 1 3 4
# 26 "/usr/include/stdlib.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/libc-header-start.h" 1 3 4
# 27 "/usr/include/stdlib.h" 2 3 4





# 1 "/usr/lib/llvm-18/lib/clang/18/include/stddef.h" 1 3 4
# 77 "/usr/lib/llvm-18/lib/clang/18/include/stddef.h" 3 4
# 1 "/usr/lib/llvm-18/lib/clang/18/include/__stddef_size_t.h" 1 3 4
# 78 "/usr/lib/llvm-18/lib/clang/18/include/stddef.h" 2 3 4
# 87 "/usr/lib/llvm-18/lib/clang/18/include/stddef.h" 3 4
# 1 "/usr/lib/llvm-18/lib/clang/18/include/__stddef_wchar_t.h" 1 3 4
# 24 "/usr/lib/llvm-18/lib/clang/18/include/__stddef_wchar_t.h" 3 4
typedef int wchar_t;
# 88 "/usr/lib/llvm-18/lib/clang/18/include/stddef.h" 2 3 4




# 1 "/usr/lib/llvm-18/lib/clang/18/include/__stddef_null.h" 1 3 4
# 93 "/usr/lib/llvm-18/lib/clang/18/include/stddef.h" 2 3 4
# 33 "/usr/include/stdlib.h" 2 3 4







# 1 "/usr/include/x86_64-linux-gnu/bits/waitflags.h" 1 3 4
# 41 "/usr/include/stdlib.h" 2 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/waitstatus.h" 1 3 4
# 42 "/usr/include/stdlib.h" 2 3 4
# 56 "/usr/include/stdlib.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/floatn.h" 1 3 4
# 119 "/usr/include/x86_64-linux-gnu/bits/floatn.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/floatn-common.h" 1 3 4
# 24 "/usr/include/x86_64-linux-gnu/bits/floatn-common.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/long-double.h" 1 3 4
# 25 "/usr/include/x86_64-linux-gnu/bits/floatn-common.h" 2 3 4
# 214 "/usr/include/x86_64-linux-gnu/bits/floatn-common.h" 3 4
typedef float _Float32;
# 251 "/usr/include/x86_64-linux-gnu/bits/floatn-common.h" 3 4
typedef double _Float64;
# 268 "/usr/include/x86_64-linux-gnu/bits/floatn-common.h" 3 4
typedef double _Float32x;
# 285 "/usr/include/x86_64-linux-gnu/bits/floatn-common.h" 3 4
typedef long double _Float64x;
# 120 "/usr/include/x86_64-linux-gnu/bits/floatn.h" 2 3 4
# 57 "/usr/include/stdlib.h" 2 3 4


typedef struct
  {
    int quot;
    int rem;
  } div_t;



typedef struct
  {
    long int quot;
    long int rem;
  } ldiv_t;





__extension__ typedef struct
  {
    long long int quot;
    long long int rem;
  } lldiv_t;
# 98 "/usr/include/stdlib.h" 3 4
extern size_t __ctype_get_mb_cur_max (void) __attribute__ ((__nothrow__ )) ;



extern double atof (const char *__nptr)
     __attribute__ ((__nothrow__ )) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1))) ;

extern int atoi (const char *__nptr)
     __attribute__ ((__nothrow__ )) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1))) ;

extern long int atol (const char *__nptr)
     __attribute__ ((__nothrow__ )) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1))) ;



__extension__ extern long long int atoll (const char *__nptr)
     __attribute__ ((__nothrow__ )) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1))) ;



extern double strtod (const char *__restrict __nptr,
        char **__restrict __endptr)
     __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1)));



extern float strtof (const char *__restrict __nptr,
       char **__restrict __endptr) __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1)));

extern long double strtold (const char *__restrict __nptr,
       char **__restrict __endptr)
     __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1)));
# 177 "/usr/include/stdlib.h" 3 4
extern long int strtol (const char *__restrict __nptr,
   char **__restrict __endptr, int __base)
     __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1)));

extern unsigned long int strtoul (const char *__restrict __nptr,
      char **__restrict __endptr, int __base)
     __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1)));



__extension__
extern long long int strtoq (const char *__restrict __nptr,
        char **__restrict __endptr, int __base)
     __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1)));

__extension__
extern unsigned long long int strtouq (const char *__restrict __nptr,
           char **__restrict __endptr, int __base)
     __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1)));




__extension__
extern long long int strtoll (const char *__restrict __nptr,
         char **__restrict __endptr, int __base)
     __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1)));

__extension__
extern unsigned long long int strtoull (const char *__restrict __nptr,
     char **__restrict __endptr, int __base)
     __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1)));
# 505 "/usr/include/stdlib.h" 3 4
extern char *l64a (long int __n) __attribute__ ((__nothrow__ )) ;


extern long int a64l (const char *__s)
     __attribute__ ((__nothrow__ )) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1))) ;




# 1 "/usr/include/x86_64-linux-gnu/sys/types.h" 1 3 4
# 29 "/usr/include/x86_64-linux-gnu/sys/types.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/types.h" 1 3 4
# 27 "/usr/include/x86_64-linux-gnu/bits/types.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/wordsize.h" 1 3 4
# 28 "/usr/include/x86_64-linux-gnu/bits/types.h" 2 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/timesize.h" 1 3 4
# 19 "/usr/include/x86_64-linux-gnu/bits/timesize.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/wordsize.h" 1 3 4
# 20 "/usr/include/x86_64-linux-gnu/bits/timesize.h" 2 3 4
# 29 "/usr/include/x86_64-linux-gnu/bits/types.h" 2 3 4


typedef unsigned char __u_char;
typedef unsigned short int __u_short;
typedef unsigned int __u_int;
typedef unsigned long int __u_long;


typedef signed char __int8_t;
typedef unsigned char __uint8_t;
typedef signed short int __int16_t;
typedef unsigned short int __uint16_t;
typedef signed int __int32_t;
typedef unsigned int __uint32_t;

typedef signed long int __int64_t;
typedef unsigned long int __uint64_t;






typedef __int8_t __int_least8_t;
typedef __uint8_t __uint_least8_t;
typedef __int16_t __int_least16_t;
typedef __uint16_t __uint_least16_t;
typedef __int32_t __int_least32_t;
typedef __uint32_t __uint_least32_t;
typedef __int64_t __int_least64_t;
typedef __uint64_t __uint_least64_t;



typedef long int __quad_t;
typedef unsigned long int __u_quad_t;







typedef long int __intmax_t;
typedef unsigned long int __uintmax_t;
# 141 "/usr/include/x86_64-linux-gnu/bits/types.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/typesizes.h" 1 3 4
# 142 "/usr/include/x86_64-linux-gnu/bits/types.h" 2 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/time64.h" 1 3 4
# 143 "/usr/include/x86_64-linux-gnu/bits/types.h" 2 3 4


typedef unsigned long int __dev_t;
typedef unsigned int __uid_t;
typedef unsigned int __gid_t;
typedef unsigned long int __ino_t;
typedef unsigned long int __ino64_t;
typedef unsigned int __mode_t;
typedef unsigned long int __nlink_t;
typedef long int __off_t;
typedef long int __off64_t;
typedef int __pid_t;
typedef struct { int __val[2]; } __fsid_t;
typedef long int __clock_t;
typedef unsigned long int __rlim_t;
typedef unsigned long int __rlim64_t;
typedef unsigned int __id_t;
typedef long int __time_t;
typedef unsigned int __useconds_t;
typedef long int __suseconds_t;
typedef long int __suseconds64_t;

typedef int __daddr_t;
typedef int __key_t;


typedef int __clockid_t;


typedef void * __timer_t;


typedef long int __blksize_t;




typedef long int __blkcnt_t;
typedef long int __blkcnt64_t;


typedef unsigned long int __fsblkcnt_t;
typedef unsigned long int __fsblkcnt64_t;


typedef unsigned long int __fsfilcnt_t;
typedef unsigned long int __fsfilcnt64_t;


typedef long int __fsword_t;

typedef long int __ssize_t;


typedef long int __syscall_slong_t;

typedef unsigned long int __syscall_ulong_t;



typedef __off64_t __loff_t;
typedef char *__caddr_t;


typedef long int __intptr_t;


typedef unsigned int __socklen_t;




typedef int __sig_atomic_t;
# 30 "/usr/include/x86_64-linux-gnu/sys/types.h" 2 3 4



typedef __u_char u_char;
typedef __u_short u_short;
typedef __u_int u_int;
typedef __u_long u_long;
typedef __quad_t quad_t;
typedef __u_quad_t u_quad_t;
typedef __fsid_t fsid_t;


typedef __loff_t loff_t;




typedef __ino_t ino_t;
# 59 "/usr/include/x86_64-linux-gnu/sys/types.h" 3 4
typedef __dev_t dev_t;




typedef __gid_t gid_t;




typedef __mode_t mode_t;




typedef __nlink_t nlink_t;




typedef __uid_t uid_t;





typedef __off_t off_t;
# 97 "/usr/include/x86_64-linux-gnu/sys/types.h" 3 4
typedef __pid_t pid_t;





typedef __id_t id_t;




typedef __ssize_t ssize_t;





typedef __daddr_t daddr_t;
typedef __caddr_t caddr_t;





typedef __key_t key_t;




# 1 "/usr/include/x86_64-linux-gnu/bits/types/clock_t.h" 1 3 4






typedef __clock_t clock_t;
# 127 "/usr/include/x86_64-linux-gnu/sys/types.h" 2 3 4

# 1 "/usr/include/x86_64-linux-gnu/bits/types/clockid_t.h" 1 3 4






typedef __clockid_t clockid_t;
# 129 "/usr/include/x86_64-linux-gnu/sys/types.h" 2 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/types/time_t.h" 1 3 4
# 10 "/usr/include/x86_64-linux-gnu/bits/types/time_t.h" 3 4
typedef __time_t time_t;
# 130 "/usr/include/x86_64-linux-gnu/sys/types.h" 2 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/types/timer_t.h" 1 3 4






typedef __timer_t timer_t;
# 131 "/usr/include/x86_64-linux-gnu/sys/types.h" 2 3 4
# 144 "/usr/include/x86_64-linux-gnu/sys/types.h" 3 4
# 1 "/usr/lib/llvm-18/lib/clang/18/include/stddef.h" 1 3 4
# 77 "/usr/lib/llvm-18/lib/clang/18/include/stddef.h" 3 4
# 1 "/usr/lib/llvm-18/lib/clang/18/include/__stddef_size_t.h" 1 3 4
# 78 "/usr/lib/llvm-18/lib/clang/18/include/stddef.h" 2 3 4
# 145 "/usr/include/x86_64-linux-gnu/sys/types.h" 2 3 4



typedef unsigned long int ulong;
typedef unsigned short int ushort;
typedef unsigned int uint;




# 1 "/usr/include/x86_64-linux-gnu/bits/stdint-intn.h" 1 3 4
# 24 "/usr/include/x86_64-linux-gnu/bits/stdint-intn.h" 3 4
typedef __int8_t int8_t;
typedef __int16_t int16_t;
typedef __int32_t int32_t;
typedef __int64_t int64_t;
# 156 "/usr/include/x86_64-linux-gnu/sys/types.h" 2 3 4


typedef __uint8_t u_int8_t;
typedef __uint16_t u_int16_t;
typedef __uint32_t u_int32_t;
typedef __uint64_t u_int64_t;


typedef int register_t __attribute__ ((__mode__ (__word__)));
# 176 "/usr/include/x86_64-linux-gnu/sys/types.h" 3 4
# 1 "/usr/include/endian.h" 1 3 4
# 24 "/usr/include/endian.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/endian.h" 1 3 4
# 35 "/usr/include/x86_64-linux-gnu/bits/endian.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/endianness.h" 1 3 4
# 36 "/usr/include/x86_64-linux-gnu/bits/endian.h" 2 3 4
# 25 "/usr/include/endian.h" 2 3 4
# 35 "/usr/include/endian.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/byteswap.h" 1 3 4
# 33 "/usr/include/x86_64-linux-gnu/bits/byteswap.h" 3 4
static __inline __uint16_t
__bswap_16 (__uint16_t __bsx)
{



  return ((__uint16_t) ((((__bsx) >> 8) & 0xff) | (((__bsx) & 0xff) << 8)));

}






static __inline __uint32_t
__bswap_32 (__uint32_t __bsx)
{



  return ((((__bsx) & 0xff000000u) >> 24) | (((__bsx) & 0x00ff0000u) >> 8) | (((__bsx) & 0x0000ff00u) << 8) | (((__bsx) & 0x000000ffu) << 24));

}
# 69 "/usr/include/x86_64-linux-gnu/bits/byteswap.h" 3 4
__extension__ static __inline __uint64_t
__bswap_64 (__uint64_t __bsx)
{



  return ((((__bsx) & 0xff00000000000000ull) >> 56) | (((__bsx) & 0x00ff000000000000ull) >> 40) | (((__bsx) & 0x0000ff0000000000ull) >> 24) | (((__bsx) & 0x000000ff00000000ull) >> 8) | (((__bsx) & 0x00000000ff000000ull) << 8) | (((__bsx) & 0x0000000000ff0000ull) << 24) | (((__bsx) & 0x000000000000ff00ull) << 40) | (((__bsx) & 0x00000000000000ffull) << 56));

}
# 36 "/usr/include/endian.h" 2 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/uintn-identity.h" 1 3 4
# 32 "/usr/include/x86_64-linux-gnu/bits/uintn-identity.h" 3 4
static __inline __uint16_t
__uint16_identity (__uint16_t __x)
{
  return __x;
}

static __inline __uint32_t
__uint32_identity (__uint32_t __x)
{
  return __x;
}

static __inline __uint64_t
__uint64_identity (__uint64_t __x)
{
  return __x;
}
# 37 "/usr/include/endian.h" 2 3 4
# 177 "/usr/include/x86_64-linux-gnu/sys/types.h" 2 3 4


# 1 "/usr/include/x86_64-linux-gnu/sys/select.h" 1 3 4
# 30 "/usr/include/x86_64-linux-gnu/sys/select.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/select.h" 1 3 4
# 31 "/usr/include/x86_64-linux-gnu/sys/select.h" 2 3 4


# 1 "/usr/include/x86_64-linux-gnu/bits/types/sigset_t.h" 1 3 4



# 1 "/usr/include/x86_64-linux-gnu/bits/types/__sigset_t.h" 1 3 4




typedef struct
{
  unsigned long int __val[(1024 / (8 * sizeof (unsigned long int)))];
} __sigset_t;
# 5 "/usr/include/x86_64-linux-gnu/bits/types/sigset_t.h" 2 3 4


typedef __sigset_t sigset_t;
# 34 "/usr/include/x86_64-linux-gnu/sys/select.h" 2 3 4



# 1 "/usr/include/x86_64-linux-gnu/bits/types/struct_timeval.h" 1 3 4







struct timeval
{




  __time_t tv_sec;
  __suseconds_t tv_usec;

};
# 38 "/usr/include/x86_64-linux-gnu/sys/select.h" 2 3 4

# 1 "/usr/include/x86_64-linux-gnu/bits/types/struct_timespec.h" 1 3 4
# 11 "/usr/include/x86_64-linux-gnu/bits/types/struct_timespec.h" 3 4
struct timespec
{



  __time_t tv_sec;




  __syscall_slong_t tv_nsec;
# 31 "/usr/include/x86_64-linux-gnu/bits/types/struct_timespec.h" 3 4
};
# 40 "/usr/include/x86_64-linux-gnu/sys/select.h" 2 3 4



typedef __suseconds_t suseconds_t;





typedef long int __fd_mask;
# 59 "/usr/include/x86_64-linux-gnu/sys/select.h" 3 4
typedef struct
  {






    __fd_mask __fds_bits[1024 / (8 * (int) sizeof (__fd_mask))];


  } fd_set;






typedef __fd_mask fd_mask;
# 102 "/usr/include/x86_64-linux-gnu/sys/select.h" 3 4
extern int select (int __nfds, fd_set *__restrict __readfds,
     fd_set *__restrict __writefds,
     fd_set *__restrict __exceptfds,
     struct timeval *__restrict __timeout);
# 127 "/usr/include/x86_64-linux-gnu/sys/select.h" 3 4
extern int pselect (int __nfds, fd_set *__restrict __readfds,
      fd_set *__restrict __writefds,
      fd_set *__restrict __exceptfds,
      const struct timespec *__restrict __timeout,
      const __sigset_t *__restrict __sigmask);
# 180 "/usr/include/x86_64-linux-gnu/sys/types.h" 2 3 4





typedef __blksize_t blksize_t;






typedef __blkcnt_t blkcnt_t;



typedef __fsblkcnt_t fsblkcnt_t;



typedef __fsfilcnt_t fsfilcnt_t;
# 227 "/usr/include/x86_64-linux-gnu/sys/types.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/pthreadtypes.h" 1 3 4
# 23 "/usr/include/x86_64-linux-gnu/bits/pthreadtypes.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/thread-shared-types.h" 1 3 4
# 44 "/usr/include/x86_64-linux-gnu/bits/thread-shared-types.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/pthreadtypes-arch.h" 1 3 4
# 21 "/usr/include/x86_64-linux-gnu/bits/pthreadtypes-arch.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/wordsize.h" 1 3 4
# 22 "/usr/include/x86_64-linux-gnu/bits/pthreadtypes-arch.h" 2 3 4
# 45 "/usr/include/x86_64-linux-gnu/bits/thread-shared-types.h" 2 3 4

# 1 "/usr/include/x86_64-linux-gnu/bits/atomic_wide_counter.h" 1 3 4
# 25 "/usr/include/x86_64-linux-gnu/bits/atomic_wide_counter.h" 3 4
typedef union
{
  __extension__ unsigned long long int __value64;
  struct
  {
    unsigned int __low;
    unsigned int __high;
  } __value32;
} __atomic_wide_counter;
# 47 "/usr/include/x86_64-linux-gnu/bits/thread-shared-types.h" 2 3 4




typedef struct __pthread_internal_list
{
  struct __pthread_internal_list *__prev;
  struct __pthread_internal_list *__next;
} __pthread_list_t;

typedef struct __pthread_internal_slist
{
  struct __pthread_internal_slist *__next;
} __pthread_slist_t;
# 76 "/usr/include/x86_64-linux-gnu/bits/thread-shared-types.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/struct_mutex.h" 1 3 4
# 22 "/usr/include/x86_64-linux-gnu/bits/struct_mutex.h" 3 4
struct __pthread_mutex_s
{
  int __lock;
  unsigned int __count;
  int __owner;

  unsigned int __nusers;



  int __kind;

  short __spins;
  short __elision;
  __pthread_list_t __list;
# 53 "/usr/include/x86_64-linux-gnu/bits/struct_mutex.h" 3 4
};
# 77 "/usr/include/x86_64-linux-gnu/bits/thread-shared-types.h" 2 3 4
# 89 "/usr/include/x86_64-linux-gnu/bits/thread-shared-types.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/struct_rwlock.h" 1 3 4
# 23 "/usr/include/x86_64-linux-gnu/bits/struct_rwlock.h" 3 4
struct __pthread_rwlock_arch_t
{
  unsigned int __readers;
  unsigned int __writers;
  unsigned int __wrphase_futex;
  unsigned int __writers_futex;
  unsigned int __pad3;
  unsigned int __pad4;

  int __cur_writer;
  int __shared;
  signed char __rwelision;




  unsigned char __pad1[7];


  unsigned long int __pad2;


  unsigned int __flags;
# 55 "/usr/include/x86_64-linux-gnu/bits/struct_rwlock.h" 3 4
};
# 90 "/usr/include/x86_64-linux-gnu/bits/thread-shared-types.h" 2 3 4




struct __pthread_cond_s
{
  __atomic_wide_counter __wseq;
  __atomic_wide_counter __g1_start;
  unsigned int __g_refs[2] ;
  unsigned int __g_size[2];
  unsigned int __g1_orig_size;
  unsigned int __wrefs;
  unsigned int __g_signals[2];
};

typedef unsigned int __tss_t;
typedef unsigned long int __thrd_t;

typedef struct
{
  int __data ;
} __once_flag;
# 24 "/usr/include/x86_64-linux-gnu/bits/pthreadtypes.h" 2 3 4



typedef unsigned long int pthread_t;




typedef union
{
  char __size[4];
  int __align;
} pthread_mutexattr_t;




typedef union
{
  char __size[4];
  int __align;
} pthread_condattr_t;



typedef unsigned int pthread_key_t;



typedef int pthread_once_t;


union pthread_attr_t
{
  char __size[56];
  long int __align;
};

typedef union pthread_attr_t pthread_attr_t;




typedef union
{
  struct __pthread_mutex_s __data;
  char __size[40];
  long int __align;
} pthread_mutex_t;


typedef union
{
  struct __pthread_cond_s __data;
  char __size[48];
  __extension__ long long int __align;
} pthread_cond_t;





typedef union
{
  struct __pthread_rwlock_arch_t __data;
  char __size[56];
  long int __align;
} pthread_rwlock_t;

typedef union
{
  char __size[8];
  long int __align;
} pthread_rwlockattr_t;





typedef volatile int pthread_spinlock_t;




typedef union
{
  char __size[32];
  long int __align;
} pthread_barrier_t;

typedef union
{
  char __size[4];
  int __align;
} pthread_barrierattr_t;
# 228 "/usr/include/x86_64-linux-gnu/sys/types.h" 2 3 4
# 515 "/usr/include/stdlib.h" 2 3 4






extern long int random (void) __attribute__ ((__nothrow__ ));


extern void srandom (unsigned int __seed) __attribute__ ((__nothrow__ ));





extern char *initstate (unsigned int __seed, char *__statebuf,
   size_t __statelen) __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (2)));



extern char *setstate (char *__statebuf) __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1)));







struct random_data
  {
    int32_t *fptr;
    int32_t *rptr;
    int32_t *state;
    int rand_type;
    int rand_deg;
    int rand_sep;
    int32_t *end_ptr;
  };

extern int random_r (struct random_data *__restrict __buf,
       int32_t *__restrict __result) __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1, 2)));

extern int srandom_r (unsigned int __seed, struct random_data *__buf)
     __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (2)));

extern int initstate_r (unsigned int __seed, char *__restrict __statebuf,
   size_t __statelen,
   struct random_data *__restrict __buf)
     __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (2, 4)));

extern int setstate_r (char *__restrict __statebuf,
         struct random_data *__restrict __buf)
     __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1, 2)));





extern int rand (void) __attribute__ ((__nothrow__ ));

extern void srand (unsigned int __seed) __attribute__ ((__nothrow__ ));



extern int rand_r (unsigned int *__seed) __attribute__ ((__nothrow__ ));







extern double drand48 (void) __attribute__ ((__nothrow__ ));
extern double erand48 (unsigned short int __xsubi[3]) __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1)));


extern long int lrand48 (void) __attribute__ ((__nothrow__ ));
extern long int nrand48 (unsigned short int __xsubi[3])
     __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1)));


extern long int mrand48 (void) __attribute__ ((__nothrow__ ));
extern long int jrand48 (unsigned short int __xsubi[3])
     __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1)));


extern void srand48 (long int __seedval) __attribute__ ((__nothrow__ ));
extern unsigned short int *seed48 (unsigned short int __seed16v[3])
     __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1)));
extern void lcong48 (unsigned short int __param[7]) __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1)));





struct drand48_data
  {
    unsigned short int __x[3];
    unsigned short int __old_x[3];
    unsigned short int __c;
    unsigned short int __init;
    __extension__ unsigned long long int __a;

  };


extern int drand48_r (struct drand48_data *__restrict __buffer,
        double *__restrict __result) __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1, 2)));
extern int erand48_r (unsigned short int __xsubi[3],
        struct drand48_data *__restrict __buffer,
        double *__restrict __result) __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1, 2)));


extern int lrand48_r (struct drand48_data *__restrict __buffer,
        long int *__restrict __result)
     __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1, 2)));
extern int nrand48_r (unsigned short int __xsubi[3],
        struct drand48_data *__restrict __buffer,
        long int *__restrict __result)
     __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1, 2)));


extern int mrand48_r (struct drand48_data *__restrict __buffer,
        long int *__restrict __result)
     __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1, 2)));
extern int jrand48_r (unsigned short int __xsubi[3],
        struct drand48_data *__restrict __buffer,
        long int *__restrict __result)
     __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1, 2)));


extern int srand48_r (long int __seedval, struct drand48_data *__buffer)
     __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (2)));

extern int seed48_r (unsigned short int __seed16v[3],
       struct drand48_data *__buffer) __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1, 2)));

extern int lcong48_r (unsigned short int __param[7],
        struct drand48_data *__buffer)
     __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1, 2)));


extern __uint32_t arc4random (void)
     __attribute__ ((__nothrow__ )) ;


extern void arc4random_buf (void *__buf, size_t __size)
     __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1)));



extern __uint32_t arc4random_uniform (__uint32_t __upper_bound)
     __attribute__ ((__nothrow__ )) ;




extern void *malloc (size_t __size) __attribute__ ((__nothrow__ )) __attribute__ ((__malloc__))
                                         ;

extern void *calloc (size_t __nmemb, size_t __size)
     __attribute__ ((__nothrow__ )) __attribute__ ((__malloc__)) ;






extern void *realloc (void *__ptr, size_t __size)
     __attribute__ ((__nothrow__ )) __attribute__ ((__warn_unused_result__)) ;


extern void free (void *__ptr) __attribute__ ((__nothrow__ ));







extern void *reallocarray (void *__ptr, size_t __nmemb, size_t __size)
     __attribute__ ((__nothrow__ )) __attribute__ ((__warn_unused_result__))

                       ;


extern void *reallocarray (void *__ptr, size_t __nmemb, size_t __size)
     __attribute__ ((__nothrow__ )) ;



# 1 "/usr/include/alloca.h" 1 3 4
# 24 "/usr/include/alloca.h" 3 4
# 1 "/usr/lib/llvm-18/lib/clang/18/include/stddef.h" 1 3 4
# 77 "/usr/lib/llvm-18/lib/clang/18/include/stddef.h" 3 4
# 1 "/usr/lib/llvm-18/lib/clang/18/include/__stddef_size_t.h" 1 3 4
# 78 "/usr/lib/llvm-18/lib/clang/18/include/stddef.h" 2 3 4
# 25 "/usr/include/alloca.h" 2 3 4







extern void *alloca (size_t __size) __attribute__ ((__nothrow__ ));
# 707 "/usr/include/stdlib.h" 2 3 4





extern void *valloc (size_t __size) __attribute__ ((__nothrow__ )) __attribute__ ((__malloc__))
                                         ;




extern int posix_memalign (void **__memptr, size_t __alignment, size_t __size)
     __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1))) ;
# 730 "/usr/include/stdlib.h" 3 4
extern void abort (void) __attribute__ ((__nothrow__ )) __attribute__ ((__noreturn__));



extern int atexit (void (*__func) (void)) __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1)));
# 749 "/usr/include/stdlib.h" 3 4
extern int on_exit (void (*__func) (int __status, void *__arg), void *__arg)
     __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1)));





extern void exit (int __status) __attribute__ ((__nothrow__ )) __attribute__ ((__noreturn__));
# 768 "/usr/include/stdlib.h" 3 4
extern void _Exit (int __status) __attribute__ ((__nothrow__ )) __attribute__ ((__noreturn__));




extern char *getenv (const char *__name) __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1))) ;
# 786 "/usr/include/stdlib.h" 3 4
extern int putenv (char *__string) __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1)));





extern int setenv (const char *__name, const char *__value, int __replace)
     __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (2)));


extern int unsetenv (const char *__name) __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1)));






extern int clearenv (void) __attribute__ ((__nothrow__ ));
# 814 "/usr/include/stdlib.h" 3 4
extern char *mktemp (char *__template) __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1)));
# 827 "/usr/include/stdlib.h" 3 4
extern int mkstemp (char *__template) __attribute__ ((__nonnull__ (1))) ;
# 849 "/usr/include/stdlib.h" 3 4
extern int mkstemps (char *__template, int __suffixlen) __attribute__ ((__nonnull__ (1))) ;
# 870 "/usr/include/stdlib.h" 3 4
extern char *mkdtemp (char *__template) __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1))) ;
# 923 "/usr/include/stdlib.h" 3 4
extern int system (const char *__command) ;
# 940 "/usr/include/stdlib.h" 3 4
extern char *realpath (const char *__restrict __name,
         char *__restrict __resolved) __attribute__ ((__nothrow__ )) ;






typedef int (*__compar_fn_t) (const void *, const void *);
# 960 "/usr/include/stdlib.h" 3 4
extern void *bsearch (const void *__key, const void *__base,
        size_t __nmemb, size_t __size, __compar_fn_t __compar)
     __attribute__ ((__nonnull__ (1, 2, 5))) ;







extern void qsort (void *__base, size_t __nmemb, size_t __size,
     __compar_fn_t __compar) __attribute__ ((__nonnull__ (1, 4)));
# 980 "/usr/include/stdlib.h" 3 4
extern int abs (int __x) __attribute__ ((__nothrow__ )) __attribute__ ((__const__)) ;
extern long int labs (long int __x) __attribute__ ((__nothrow__ )) __attribute__ ((__const__)) ;


__extension__ extern long long int llabs (long long int __x)
     __attribute__ ((__nothrow__ )) __attribute__ ((__const__)) ;






extern div_t div (int __numer, int __denom)
     __attribute__ ((__nothrow__ )) __attribute__ ((__const__)) ;
extern ldiv_t ldiv (long int __numer, long int __denom)
     __attribute__ ((__nothrow__ )) __attribute__ ((__const__)) ;


__extension__ extern lldiv_t lldiv (long long int __numer,
        long long int __denom)
     __attribute__ ((__nothrow__ )) __attribute__ ((__const__)) ;
# 1012 "/usr/include/stdlib.h" 3 4
extern char *ecvt (double __value, int __ndigit, int *__restrict __decpt,
     int *__restrict __sign) __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (3, 4))) ;




extern char *fcvt (double __value, int __ndigit, int *__restrict __decpt,
     int *__restrict __sign) __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (3, 4))) ;




extern char *gcvt (double __value, int __ndigit, char *__buf)
     __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (3))) ;




extern char *qecvt (long double __value, int __ndigit,
      int *__restrict __decpt, int *__restrict __sign)
     __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (3, 4))) ;
extern char *qfcvt (long double __value, int __ndigit,
      int *__restrict __decpt, int *__restrict __sign)
     __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (3, 4))) ;
extern char *qgcvt (long double __value, int __ndigit, char *__buf)
     __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (3))) ;




extern int ecvt_r (double __value, int __ndigit, int *__restrict __decpt,
     int *__restrict __sign, char *__restrict __buf,
     size_t __len) __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (3, 4, 5)));
extern int fcvt_r (double __value, int __ndigit, int *__restrict __decpt,
     int *__restrict __sign, char *__restrict __buf,
     size_t __len) __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (3, 4, 5)));

extern int qecvt_r (long double __value, int __ndigit,
      int *__restrict __decpt, int *__restrict __sign,
      char *__restrict __buf, size_t __len)
     __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (3, 4, 5)));
extern int qfcvt_r (long double __value, int __ndigit,
      int *__restrict __decpt, int *__restrict __sign,
      char *__restrict __buf, size_t __len)
     __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (3, 4, 5)));





extern int mblen (const char *__s, size_t __n) __attribute__ ((__nothrow__ ));


extern int mbtowc (wchar_t *__restrict __pwc,
     const char *__restrict __s, size_t __n) __attribute__ ((__nothrow__ ));


extern int wctomb (char *__s, wchar_t __wchar) __attribute__ ((__nothrow__ ));



extern size_t mbstowcs (wchar_t *__restrict __pwcs,
   const char *__restrict __s, size_t __n) __attribute__ ((__nothrow__ ))
                                      ;

extern size_t wcstombs (char *__restrict __s,
   const wchar_t *__restrict __pwcs, size_t __n)
     __attribute__ ((__nothrow__ ))

                                    ;






extern int rpmatch (const char *__response) __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1))) ;
# 1099 "/usr/include/stdlib.h" 3 4
extern int getsubopt (char **__restrict __optionp,
        char *const *__restrict __tokens,
        char **__restrict __valuep)
     __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1, 2, 3))) ;
# 1145 "/usr/include/stdlib.h" 3 4
extern int getloadavg (double __loadavg[], int __nelem)
     __attribute__ ((__nothrow__ )) __attribute__ ((__nonnull__ (1)));
# 1155 "/usr/include/stdlib.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/stdlib-float.h" 1 3 4
# 1156 "/usr/include/stdlib.h" 2 3 4
# 20 "/home/gwillen/sqlite-graph/src/graph-advanced.c" 2
# 30 "/home/gwillen/sqlite-graph/src/graph-advanced.c"
typedef struct BrandesStack BrandesStack;
struct BrandesStack {
  sqlite3_int64 iNodeId;
  BrandesStack *pNext;
};





typedef struct PredecessorList PredecessorList;
struct PredecessorList {
  sqlite3_int64 iPredId;
  PredecessorList *pNext;
};





typedef struct BrandesState BrandesState;
struct BrandesState {
  sqlite3_int64 iSourceId;
  double *aDistance;
  int *aSigma;
  double *aDelta;
  PredecessorList **aPred;
  int nNodes;
};





typedef struct NodeIndexMap NodeIndexMap;
struct NodeIndexMap {
  sqlite3_int64 *aNodeIds;
  int nNodes;
};





static NodeIndexMap *createNodeIndexMap(GraphVtab *pVtab){
  NodeIndexMap *pMap;
  GraphNode *pNode;
  int i = 0;

  pMap = sqlite3_malloc(sizeof(*pMap));
  if( pMap==0 ) return 0;

  pMap->nNodes = pVtab->nNodes;
  pMap->aNodeIds = sqlite3_malloc(sizeof(sqlite3_int64) * pMap->nNodes);
  if( pMap->aNodeIds==0 ){
    sqlite3_free(pMap);
    return 0;
  }


  for( pNode = pVtab->pNodeList; pNode; pNode = pNode->pNext ){
    pMap->aNodeIds[i++] = pNode->iNodeId;
  }

  return pMap;
}





static int getNodeIndex(NodeIndexMap *pMap, sqlite3_int64 iNodeId){
  int i;
  for( i = 0; i < pMap->nNodes; i++ ){
    if( pMap->aNodeIds[i] == iNodeId ) return i;
  }
  return -1;
}




static void freeNodeIndexMap(NodeIndexMap *pMap){
  if( pMap ){
    sqlite3_free(pMap->aNodeIds);
    sqlite3_free(pMap);
  }
}




static BrandesState *brandesInitState(GraphVtab *pVtab,
                                     sqlite3_int64 iSourceId,
                                     NodeIndexMap *pMap){
  BrandesState *pState;
  int nNodes = pVtab->nNodes;
  int i;

  pState = sqlite3_malloc(sizeof(*pState));
  if( pState==0 ) return 0;
  memset(pState, 0, sizeof(*pState));

  pState->iSourceId = iSourceId;
  pState->nNodes = nNodes;


  pState->aDistance = sqlite3_malloc(sizeof(double) * nNodes);
  pState->aSigma = sqlite3_malloc(sizeof(int) * nNodes);
  pState->aDelta = sqlite3_malloc(sizeof(double) * nNodes);
  pState->aPred = sqlite3_malloc(sizeof(PredecessorList*) * nNodes);

  if( pState->aDistance==0 || pState->aSigma==0 ||
      pState->aDelta==0 || pState->aPred==0 ){
    sqlite3_free(pState->aDistance);
    sqlite3_free(pState->aSigma);
    sqlite3_free(pState->aDelta);
    sqlite3_free(pState->aPred);
    sqlite3_free(pState);
    return 0;
  }


  for( i = 0; i < nNodes; i++ ){
    pState->aDistance[i] = 1.7976931348623157e+308;
    pState->aSigma[i] = 0;
    pState->aDelta[i] = 0.0;
    pState->aPred[i] = 0;
  }


  i = getNodeIndex(pMap, iSourceId);
  if( i >= 0 ){
    pState->aDistance[i] = 0.0;
    pState->aSigma[i] = 1;
  }

  return pState;
}




static void brandesFreeState(BrandesState *pState){
  int i;
  if( pState ){

    if( pState->aPred ){
      for( i = 0; i < pState->nNodes; i++ ){
        PredecessorList *pPred = pState->aPred[i];
        while( pPred ){
          PredecessorList *pNext = pPred->pNext;
          sqlite3_free(pPred);
          pPred = pNext;
        }
      }
    }
    sqlite3_free(pState->aDistance);
    sqlite3_free(pState->aSigma);
    sqlite3_free(pState->aDelta);
    sqlite3_free(pState->aPred);
    sqlite3_free(pState);
  }
}




static int brandesAddPredecessor(BrandesState *pState, int iNodeIdx,
                                sqlite3_int64 iPredId){
  PredecessorList *pPred;

  pPred = sqlite3_malloc(sizeof(*pPred));
  if( pPred==0 ) return 7;

  pPred->iPredId = iPredId;
  pPred->pNext = pState->aPred[iNodeIdx];
  pState->aPred[iNodeIdx] = pPred;

  return 0;
}




typedef struct BrandesQueue BrandesQueue;
struct BrandesQueue {
  int *aItems;
  int nFront;
  int nRear;
  int nCapacity;
};




static BrandesQueue *brandesQueueCreate(int nCapacity){
  BrandesQueue *pQueue;

  pQueue = sqlite3_malloc(sizeof(*pQueue));
  if( pQueue==0 ) return 0;

  pQueue->aItems = sqlite3_malloc(sizeof(int) * nCapacity);
  if( pQueue->aItems==0 ){
    sqlite3_free(pQueue);
    return 0;
  }

  pQueue->nFront = 0;
  pQueue->nRear = 0;
  pQueue->nCapacity = nCapacity;

  return pQueue;
}




static void brandesQueuePush(BrandesQueue *pQueue, int iItem){
  pQueue->aItems[pQueue->nRear] = iItem;
  pQueue->nRear = (pQueue->nRear + 1) % pQueue->nCapacity;
}




static int brandesQueuePop(BrandesQueue *pQueue){
  int iItem = pQueue->aItems[pQueue->nFront];
  pQueue->nFront = (pQueue->nFront + 1) % pQueue->nCapacity;
  return iItem;
}




static int brandesQueueIsEmpty(BrandesQueue *pQueue){
  return pQueue->nFront == pQueue->nRear;
}




static void brandesQueueFree(BrandesQueue *pQueue){
  if( pQueue ){
    sqlite3_free(pQueue->aItems);
    sqlite3_free(pQueue);
  }
}






int graphBetweennessCentrality(GraphVtab *pVtab, char **pzResults){
  NodeIndexMap *pMap = 0;
  double *aCentrality = 0;
  GraphNode *pNode;
  char *zResults = 0;
  int rc = 0;
  int nNodes = pVtab->nNodes;
  int i;


  if( nNodes == 0 ){
    *pzResults = sqlite3_mprintf("{}");
    return *pzResults ? 0 : 7;
  }


  pMap = createNodeIndexMap(pVtab);
  if( pMap==0 ){
    return 7;
  }


  aCentrality = sqlite3_malloc(sizeof(double) * nNodes);
  if( aCentrality==0 ){
    freeNodeIndexMap(pMap);
    return 7;
  }
  memset(aCentrality, 0, sizeof(double) * nNodes);


  for( pNode = pVtab->pNodeList; pNode; pNode = pNode->pNext ){
    BrandesState *pState;
    BrandesQueue *pQueue;
    BrandesStack *pStack = 0;
    GraphEdge *pEdge;
    int iSrcIdx;


    pState = brandesInitState(pVtab, pNode->iNodeId, pMap);
    if( pState==0 ){
      rc = 7;
      goto betweenness_cleanup;
    }


    pQueue = brandesQueueCreate(nNodes + 1);
    if( pQueue==0 ){
      brandesFreeState(pState);
      rc = 7;
      goto betweenness_cleanup;
    }


    iSrcIdx = getNodeIndex(pMap, pNode->iNodeId);
    if( iSrcIdx < 0 ){
      brandesQueueFree(pQueue);
      brandesFreeState(pState);
      continue;
    }


    brandesQueuePush(pQueue, iSrcIdx);

    while( !brandesQueueIsEmpty(pQueue) ){
      int iCurIdx = brandesQueuePop(pQueue);
      sqlite3_int64 iCurId = pMap->aNodeIds[iCurIdx];


      BrandesStack *pStackNode = sqlite3_malloc(sizeof(*pStackNode));
      if( pStackNode ){
        pStackNode->iNodeId = iCurId;
        pStackNode->pNext = pStack;
        pStack = pStackNode;
      }


      for( pEdge = pVtab->pEdgeList; pEdge; pEdge = pEdge->pNext ){
        int iNextIdx;
        if( pEdge->iFromId != iCurId ) continue;

        iNextIdx = getNodeIndex(pMap, pEdge->iToId);
        if( iNextIdx < 0 ) continue;


        if( pState->aDistance[iNextIdx] == 1.7976931348623157e+308 ){
          pState->aDistance[iNextIdx] = pState->aDistance[iCurIdx] + 1;
          brandesQueuePush(pQueue, iNextIdx);
        }


        if( pState->aDistance[iNextIdx] == pState->aDistance[iCurIdx] + 1 ){
          pState->aSigma[iNextIdx] += pState->aSigma[iCurIdx];
          brandesAddPredecessor(pState, iNextIdx, iCurId);
        }
      }
    }


    while( pStack ){
      BrandesStack *pNext = pStack->pNext;
      int iNodeIdx = getNodeIndex(pMap, pStack->iNodeId);
      PredecessorList *pPred;

      if( iNodeIdx >= 0 ){

        for( pPred = pState->aPred[iNodeIdx]; pPred; pPred = pPred->pNext ){
          int iPredIdx = getNodeIndex(pMap, pPred->iPredId);
          if( iPredIdx >= 0 && pState->aSigma[iNodeIdx] > 0 ){
            double factor = (double)pState->aSigma[iPredIdx] /
                           pState->aSigma[iNodeIdx];
            pState->aDelta[iPredIdx] += factor * (1.0 + pState->aDelta[iNodeIdx]);
          }
        }


        if( iNodeIdx != iSrcIdx ){
          aCentrality[iNodeIdx] += pState->aDelta[iNodeIdx];
        }
      }

      sqlite3_free(pStack);
      pStack = pNext;
    }

    brandesQueueFree(pQueue);
    brandesFreeState(pState);
  }


  zResults = sqlite3_mprintf("{");
  for( i = 0; i < nNodes && zResults; i++ ){
    char *zOld = zResults;
    zResults = sqlite3_mprintf("%s%s\"%lld\":%.6f", zOld,
                              i > 0 ? "," : "",
                              pMap->aNodeIds[i],
                              aCentrality[i] / 2.0);
    sqlite3_free(zOld);
  }

  if( zResults ){
    char *zOld = zResults;
    zResults = sqlite3_mprintf("%s}", zOld);
    sqlite3_free(zOld);
  }

  if( zResults==0 ){
    rc = 7;
  } else {
    *pzResults = zResults;
  }

betweenness_cleanup:
  sqlite3_free(aCentrality);
  freeNodeIndexMap(pMap);
  return rc;
}






int graphClosenessCentrality(GraphVtab *pVtab, char **pzResults){
  NodeIndexMap *pMap = 0;
  double *aCentrality = 0;
  GraphNode *pNode;
  char *zResults = 0;
  int rc = 0;
  int nNodes = pVtab->nNodes;
  int i;


  if( nNodes == 0 ){
    *pzResults = sqlite3_mprintf("{}");
    return *pzResults ? 0 : 7;
  }


  pMap = createNodeIndexMap(pVtab);
  if( pMap==0 ){
    return 7;
  }


  aCentrality = sqlite3_malloc(sizeof(double) * nNodes);
  if( aCentrality==0 ){
    freeNodeIndexMap(pMap);
    return 7;
  }


  for( i = 0; i < nNodes; i++ ){
    char *zPath = 0;
    double rTotalDistance = 0.0;
    int nReachable = 0;


    sqlite3_int64 nodeId = pMap->aNodeIds[i];
    for (pNode = pVtab->pNodeList; pNode && pNode->iNodeId != nodeId; pNode = pNode->pNext);
    if (!pNode) continue;


    extern int graphDijkstra(GraphVtab*, sqlite3_int64, sqlite3_int64,
                           char**, double*);
    rc = graphDijkstra(pVtab, pMap->aNodeIds[i], -1, &zPath, ((void*)0));

    if( rc == 0 && zPath ){


      const char *p = zPath;
      while( *p ){
        if( *p == ':' ){
          p++;
          while( *p == ' ' ) p++;
          double dist = 0.0;
          char *endptr;
          dist = strtod(p, &endptr);
          if( endptr > p && dist < 1.7976931348623157e+308 ){
            rTotalDistance += dist;
            nReachable++;
          }
          p = endptr;
        } else {
          p++;
        }
      }
    }

    sqlite3_free(zPath);


    if( nReachable > 0 && rTotalDistance > 0 ){
      aCentrality[i] = (double)(nReachable) / rTotalDistance;
    } else {
      aCentrality[i] = 0.0;
    }
  }


  zResults = sqlite3_mprintf("{");
  for( i = 0; i < nNodes && zResults; i++ ){
    char *zOld = zResults;
    zResults = sqlite3_mprintf("%s%s\"%lld\":%.6f", zOld,
                              i > 0 ? "," : "",
                              pMap->aNodeIds[i],
                              aCentrality[i]);
    sqlite3_free(zOld);
  }

  if( zResults ){
    char *zOld = zResults;
    zResults = sqlite3_mprintf("%s}", zOld);
    sqlite3_free(zOld);
  }

  if( zResults==0 ){
    rc = 7;
  } else {
    *pzResults = zResults;
  }

  sqlite3_free(aCentrality);
  freeNodeIndexMap(pMap);
  return rc;
}






int graphTopologicalSort(GraphVtab *pVtab, char **pzOrder){
  int *aInDegree = 0;
  int *aQueue = 0;
  char *zOrder = 0;
  NodeIndexMap *pMap = 0;
  GraphEdge *pEdge;
  int nProcessed = 0;
  int nFront = 0, nRear = 0;
  int rc = 0;
  int i;


  if( pVtab->nNodes == 0 ){
    *pzOrder = sqlite3_mprintf("[]");
    return *pzOrder ? 0 : 7;
  }


  pMap = createNodeIndexMap(pVtab);
  if( pMap==0 ){
    return 7;
  }


  aInDegree = sqlite3_malloc(sizeof(int) * pVtab->nNodes);
  aQueue = sqlite3_malloc(sizeof(int) * pVtab->nNodes);

  if( aInDegree==0 || aQueue==0 ){
    rc = 7;
    goto topo_cleanup;
  }


  memset(aInDegree, 0, sizeof(int) * pVtab->nNodes);
  for( pEdge = pVtab->pEdgeList; pEdge; pEdge = pEdge->pNext ){
    int iToIdx = getNodeIndex(pMap, pEdge->iToId);
    if( iToIdx >= 0 ){
      aInDegree[iToIdx]++;
    }
  }


  for( i = 0; i < pVtab->nNodes; i++ ){
    if( aInDegree[i] == 0 ){
      aQueue[nRear++] = i;
    }
  }


  zOrder = sqlite3_mprintf("[");
  while( nFront < nRear && zOrder ){
    int iCurIdx = aQueue[nFront++];
    sqlite3_int64 iCurId = pMap->aNodeIds[iCurIdx];


    char *zOld = zOrder;
    zOrder = sqlite3_mprintf("%s%s%lld", zOld,
                            nProcessed > 0 ? "," : "",
                            iCurId);
    sqlite3_free(zOld);
    nProcessed++;


    for( pEdge = pVtab->pEdgeList; pEdge; pEdge = pEdge->pNext ){
      if( pEdge->iFromId == iCurId ){
        int iToIdx = getNodeIndex(pMap, pEdge->iToId);
        if( iToIdx >= 0 ){
          aInDegree[iToIdx]--;
          if( aInDegree[iToIdx] == 0 ){
            aQueue[nRear++] = iToIdx;
          }
        }
      }
    }
  }


  if( nProcessed < pVtab->nNodes ){
    sqlite3_free(zOrder);
    rc = 19;
    *pzOrder = sqlite3_mprintf("Graph contains cycles");
  } else if( zOrder ){
    char *zOld = zOrder;
    zOrder = sqlite3_mprintf("%s]", zOld);
    sqlite3_free(zOld);
    *pzOrder = zOrder;
    if( zOrder==0 ) rc = 7;
  } else {
    rc = 7;
  }

topo_cleanup:
  sqlite3_free(aInDegree);
  sqlite3_free(aQueue);
  freeNodeIndexMap(pMap);
  return rc;
}





int graphHasCycle(GraphVtab *pVtab){
  NodeIndexMap *pMap;
  int *aColor;
  GraphNode *pNode;
  int hasCycle = 0;

  if( pVtab->nNodes == 0 ) return 0;

  pMap = createNodeIndexMap(pVtab);
  if( pMap==0 ) return 0;

  aColor = sqlite3_malloc(sizeof(int) * pVtab->nNodes);
  if( aColor==0 ){
    freeNodeIndexMap(pMap);
    return 0;
  }
  memset(aColor, 0, sizeof(int) * pVtab->nNodes);


  int dfsVisit(int iNodeIdx){
    GraphEdge *pEdge;
    sqlite3_int64 iNodeId = pMap->aNodeIds[iNodeIdx];

    aColor[iNodeIdx] = 1;

    for( pEdge = pVtab->pEdgeList; pEdge; pEdge = pEdge->pNext ){
      if( pEdge->iFromId == iNodeId ){
        int iToIdx = getNodeIndex(pMap, pEdge->iToId);
        if( iToIdx >= 0 ){
          if( aColor[iToIdx] == 1 ){

            return 1;
          }
          if( aColor[iToIdx] == 0 ){
            if( dfsVisit(iToIdx) ) return 1;
          }
        }
      }
    }

    aColor[iNodeIdx] = 2;
    return 0;
  }


  for( int i = 0; i < pVtab->nNodes && !hasCycle; i++ ){
    if( aColor[i] == 0 ){

      sqlite3_int64 nodeId = pMap->aNodeIds[i];
      for (pNode = pVtab->pNodeList; pNode && pNode->iNodeId != nodeId; pNode = pNode->pNext);
      hasCycle = dfsVisit(i);
    }
  }

  sqlite3_free(aColor);
  freeNodeIndexMap(pMap);
  return hasCycle;
}






int graphConnectedComponents(GraphVtab *pVtab, char **pzComponents){
  NodeIndexMap *pMap = 0;
  int *aComponent = 0;
  char *zComponents = 0;
  int nComponents = 0;
  int rc = 0;
  int i;


  if( pVtab->nNodes == 0 ){
    *pzComponents = sqlite3_mprintf("{}");
    return *pzComponents ? 0 : 7;
  }

  pMap = createNodeIndexMap(pVtab);
  if( pMap==0 ) return 7;

  aComponent = sqlite3_malloc(sizeof(int) * pVtab->nNodes);
  if( aComponent==0 ){
    freeNodeIndexMap(pMap);
    return 7;
  }


  for( i = 0; i < pVtab->nNodes; i++ ){
    aComponent[i] = -1;
  }


  for( i = 0; i < pVtab->nNodes; i++ ){
    if( aComponent[i] == -1 ){

      int *aStack = sqlite3_malloc(sizeof(int) * pVtab->nNodes);
      int nStack = 0;

      if( aStack==0 ){
        rc = 7;
        goto comp_cleanup;
      }


      aStack[nStack++] = i;
      aComponent[i] = nComponents;

      while( nStack > 0 ){
        int iCurIdx = aStack[--nStack];
        sqlite3_int64 iCurId = pMap->aNodeIds[iCurIdx];
        GraphEdge *pEdge;


        for( pEdge = pVtab->pEdgeList; pEdge; pEdge = pEdge->pNext ){
          int iNextIdx = -1;

          if( pEdge->iFromId == iCurId ){
            iNextIdx = getNodeIndex(pMap, pEdge->iToId);
          } else if( pEdge->iToId == iCurId ){
            iNextIdx = getNodeIndex(pMap, pEdge->iFromId);
          }

          if( iNextIdx >= 0 && aComponent[iNextIdx] == -1 ){
            aComponent[iNextIdx] = nComponents;
            aStack[nStack++] = iNextIdx;
          }
        }
      }

      sqlite3_free(aStack);
      nComponents++;
    }
  }


  zComponents = sqlite3_mprintf("{");
  for( int comp = 0; comp < nComponents && zComponents; comp++ ){
    char *zOld = zComponents;
    zComponents = sqlite3_mprintf("%s%s\"%d\":[", zOld,
                                 comp > 0 ? "," : "", comp);
    sqlite3_free(zOld);

    int first = 1;
    for( i = 0; i < pVtab->nNodes && zComponents; i++ ){
      if( aComponent[i] == comp ){
        zOld = zComponents;
        zComponents = sqlite3_mprintf("%s%s%lld", zOld,
                                     first ? "" : ",",
                                     pMap->aNodeIds[i]);
        sqlite3_free(zOld);
        first = 0;
      }
    }

    if( zComponents ){
      zOld = zComponents;
      zComponents = sqlite3_mprintf("%s]", zOld);
      sqlite3_free(zOld);
    }
  }

  if( zComponents ){
    char *zOld = zComponents;
    zComponents = sqlite3_mprintf("%s}", zOld);
    sqlite3_free(zOld);
  }

  if( zComponents==0 ){
    rc = 7;
  } else {
    *pzComponents = zComponents;
  }

comp_cleanup:
  sqlite3_free(aComponent);
  freeNodeIndexMap(pMap);
  return rc;
}




typedef struct TarjanNode TarjanNode;
struct TarjanNode {
  int iIndex;
  TarjanNode *pNext;
};




typedef struct TarjanState TarjanState;
struct TarjanState {
  int *aIndex;
  int *aLowLink;
  int *aOnStack;
  TarjanNode *pStack;
  int nIndex;
  int nSCC;
  char **azSCC;
  NodeIndexMap *pMap;
};




static void tarjanStrongConnect(GraphVtab *pVtab, TarjanState *pState,
                               int iNodeIdx){
  GraphEdge *pEdge;
  TarjanNode *pStackNode;
  sqlite3_int64 iNodeId = pState->pMap->aNodeIds[iNodeIdx];


  pState->aIndex[iNodeIdx] = pState->nIndex;
  pState->aLowLink[iNodeIdx] = pState->nIndex;
  pState->nIndex++;


  pStackNode = sqlite3_malloc(sizeof(*pStackNode));
  if( pStackNode ){
    pStackNode->iIndex = iNodeIdx;
    pStackNode->pNext = pState->pStack;
    pState->pStack = pStackNode;
    pState->aOnStack[iNodeIdx] = 1;
  }


  for( pEdge = pVtab->pEdgeList; pEdge; pEdge = pEdge->pNext ){
    if( pEdge->iFromId == iNodeId ){
      int iToIdx = getNodeIndex(pState->pMap, pEdge->iToId);
      if( iToIdx >= 0 ){
        if( pState->aIndex[iToIdx] == -1 ){

          tarjanStrongConnect(pVtab, pState, iToIdx);
          if( pState->aLowLink[iToIdx] < pState->aLowLink[iNodeIdx] ){
            pState->aLowLink[iNodeIdx] = pState->aLowLink[iToIdx];
          }
        } else if( pState->aOnStack[iToIdx] ){

          if( pState->aIndex[iToIdx] < pState->aLowLink[iNodeIdx] ){
            pState->aLowLink[iNodeIdx] = pState->aIndex[iToIdx];
          }
        }
      }
    }
  }


  if( pState->aLowLink[iNodeIdx] == pState->aIndex[iNodeIdx] ){
    char *zSCC = sqlite3_mprintf("[");
    int first = 1;


    while( pState->pStack ){
      TarjanNode *pNext = pState->pStack->pNext;
      int iIdx = pState->pStack->iIndex;

      if( zSCC ){
        char *zOld = zSCC;
        zSCC = sqlite3_mprintf("%s%s%lld", zOld,
                              first ? "" : ",",
                              pState->pMap->aNodeIds[iIdx]);
        sqlite3_free(zOld);
        first = 0;
      }

      pState->aOnStack[iIdx] = 0;
      sqlite3_free(pState->pStack);
      pState->pStack = pNext;

      if( iIdx == iNodeIdx ) break;
    }

    if( zSCC ){
      char *zOld = zSCC;
      zSCC = sqlite3_mprintf("%s]", zOld);
      sqlite3_free(zOld);


      char **azNew = sqlite3_realloc(pState->azSCC,
                                    sizeof(char*) * (pState->nSCC + 1));
      if( azNew ){
        pState->azSCC = azNew;
        pState->azSCC[pState->nSCC++] = zSCC;
      } else {
        sqlite3_free(zSCC);
      }
    }
  }
}





int graphStronglyConnectedComponents(GraphVtab *pVtab, char **pzSCC){
  TarjanState state;
  char *zResult = 0;
  int rc = 0;
  int i;


  if( pVtab->nNodes == 0 ){
    *pzSCC = sqlite3_mprintf("[]");
    return *pzSCC ? 0 : 7;
  }


  memset(&state, 0, sizeof(state));
  state.pMap = createNodeIndexMap(pVtab);
  if( state.pMap==0 ) return 7;

  state.aIndex = sqlite3_malloc(sizeof(int) * pVtab->nNodes);
  state.aLowLink = sqlite3_malloc(sizeof(int) * pVtab->nNodes);
  state.aOnStack = sqlite3_malloc(sizeof(int) * pVtab->nNodes);

  if( !state.aIndex || !state.aLowLink || !state.aOnStack ){
    rc = 7;
    goto scc_cleanup;
  }


  for( i = 0; i < pVtab->nNodes; i++ ){
    state.aIndex[i] = -1;
    state.aLowLink[i] = -1;
    state.aOnStack[i] = 0;
  }


  for( i = 0; i < pVtab->nNodes; i++ ){
    if( state.aIndex[i] == -1 ){
      tarjanStrongConnect(pVtab, &state, i);
    }
  }


  zResult = sqlite3_mprintf("[");
  for( i = 0; i < state.nSCC && zResult; i++ ){
    char *zOld = zResult;
    zResult = sqlite3_mprintf("%s%s%s", zOld,
                             i > 0 ? "," : "",
                             state.azSCC[i]);
    sqlite3_free(zOld);
  }

  if( zResult ){
    char *zOld = zResult;
    zResult = sqlite3_mprintf("%s]", zOld);
    sqlite3_free(zOld);
  }

  if( zResult==0 ){
    rc = 7;
  } else {
    *pzSCC = zResult;
  }

scc_cleanup:

  for( i = 0; i < state.nSCC; i++ ){
    sqlite3_free(state.azSCC[i]);
  }
  sqlite3_free(state.azSCC);


  while( state.pStack ){
    TarjanNode *pNext = state.pStack->pNext;
    sqlite3_free(state.pStack);
    state.pStack = pNext;
  }

  sqlite3_free(state.aIndex);
  sqlite3_free(state.aLowLink);
  sqlite3_free(state.aOnStack);
  freeNodeIndexMap(state.pMap);

  return rc;
}
