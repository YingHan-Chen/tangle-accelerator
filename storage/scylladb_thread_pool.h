/*
 * Copyright (C) 2020 BiiLabs Co., Ltd. and Contributors
 * All Rights Reserved.
 * This is free software; you can redistribute it and/or modify it under the
 * terms of the MIT license. A copy of the license can be found in the file
 * "LICENSE" at the root of this distribution.
 */
#ifndef STORAGE_SCYLLADB_THREAD_POOL_H_
#define STORAGE_SCYLLADB_THREAD_POOL_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>
#include "scylladb_chronicle.h"

/**
 * @file storage/scylladb_thread_pool.h
 * @brief
 */

struct request {
  tryte_t hash[NUM_TRYTES_HASH]; /* number of the request                  */
  tryte_t trytes[NUM_TRYTES_SERIALIZED_TRANSACTION];
  struct request* next; /* pointer to next request, NULL if none. */
};

typedef struct {
  pthread_mutex_t request_mutex;
  pthread_cond_t got_request;
  pthread_cond_t finish_request;
  int num_requests;             /* number of pending requests, initially none */
  struct request* requests;     /* head of linked list of requests. */
  struct request* last_request; /* pointer to last request.         */

} db_chronicle_pool_t;

typedef struct {
  pthread_mutex_t thread_mutex;
  db_chronicle_pool_t* pool;
  db_client_service_t service;
} db_worker_thread_t;

/**
 * @brief Initialize request, pthread mutex, pthread cond in threadpool struct
 *
 * @param[in] poll pointer to db_chronicle_pool_t
 *
 * - SC_OK on success
 * - non-zero on error
 */
status_t db_init_chronicle_threadpool(db_chronicle_pool_t* pool);

/**
 * @brief Initialize work thread data
 *
 * @param[in] thread_data target worker thread struct
 * @param[in] pool connected thread pool
 * @param[in] host ScyllaDB host name
 *
 * - SC_OK on success
 * - non-zero on error
 */
status_t db_init_worker_thread_data(db_worker_thread_t* thread_data, db_chronicle_pool_t* pool, char* host);

/**
 * @brief Add request into chronicle request threadpoll
 *
 * @param[in] hash input transaction hash
 * @param[in] tryte input transaction trytes
 * @param[in] pool connected thread pool
 *
 * - SC_OK on success
 * - non-zero on error
 */
status_t db_chronicle_thpool_add(const tryte_t* hash, const tryte_t* trytes, db_chronicle_pool_t* pool);

/**
 * @brief infinite loop of requests handling
 *
 * Loop forever, if there are requests to handle, take the first and handle it.
 * Then wait on the given condition variable, and when it is signaled, re-do the loop.
 *
 * @param[in] data pointer to db_chronicle_pool_t
 *
 * @return NULL
 */
void* db_chronicle_worker_handler(void* data);

#ifdef __cplusplus
}
#endif

#endif  // STORAGE_SCYLLADB_THREAD_POOL_H_