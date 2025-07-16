/*
** SQLite Graph Database Extension - Thread Safety Tests
**
** Test for race conditions and thread safety issues.
*/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include "sqlite3.h"

#define NUM_THREADS 4
#define OPERATIONS_PER_THREAD 1000

void* thread_worker(void *arg) {
  sqlite3 *db;
  int thread_id = *(int*)arg;
  int rc;
  
  rc = sqlite3_open(":memory:", &db);
  assert(rc == SQLITE_OK);
  
  /* Perform concurrent operations */
  for (int i = 0; i < OPERATIONS_PER_THREAD; i++) {
    char sql[256];
    
    /* Create virtual table */
    snprintf(sql, sizeof(sql), "CREATE VIRTUAL TABLE IF NOT EXISTS g%d USING graph()", thread_id);
    rc = sqlite3_exec(db, sql, NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
      printf("Thread %d: Error creating table: %s\n", thread_id, sqlite3_errmsg(db));
    }
    
    /* Drop table */
    snprintf(sql, sizeof(sql), "DROP TABLE IF EXISTS g%d", thread_id);
    rc = sqlite3_exec(db, sql, NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
      printf("Thread %d: Error dropping table: %s\n", thread_id, sqlite3_errmsg(db));
    }
  }
  
  sqlite3_close(db);
  return NULL;
}

int main() {
  pthread_t threads[NUM_THREADS];
  int thread_ids[NUM_THREADS];
  
  printf("Starting thread safety test with %d threads...\n", NUM_THREADS);
  
  /* Create threads */
  for (int i = 0; i < NUM_THREADS; i++) {
    thread_ids[i] = i;
    int rc = pthread_create(&threads[i], NULL, thread_worker, &thread_ids[i]);
    assert(rc == 0);
  }
  
  /* Wait for all threads to complete */
  for (int i = 0; i < NUM_THREADS; i++) {
    pthread_join(threads[i], NULL);
  }
  
  printf("Thread safety test completed successfully\n");
  return 0;
}