/*
 * Copyright (C) 2020 BiiLabs Co., Ltd. and Contributors
 * All Rights Reserved.
 * This is free software; you can redistribute it and/or modify it under the
 * terms of the MIT license. A copy of the license can be found in the file
 * "LICENSE" at the root of this distribution.
 */

#include "scylladb_permanode_thpool.h"

#define logger_id scylladb_logger_id

status_t db_permanode_thpool_init(db_permanode_pool_t* pool) {
  if (pthread_mutex_init(&pool->request_mutex, NULL)) {
    ta_log_error("Failed to init pthread mutex\n");
    return SC_STORAGE_PTHREAD_ERROR;
  }
  pthread_cond_init(&pool->got_request, NULL);
  pthread_cond_init(&pool->finish_request, NULL);
  pool->working_thread_num = 0;
  pool->num_requests = 0;
  pool->requests = NULL;
  pool->last_request = NULL;
  return SC_OK;
}

status_t db_permanode_thpool_init_worker(db_worker_thread_t* thread_data, db_permanode_pool_t* pool, char* host) {
  status_t ret;
  pthread_mutex_init(&thread_data->thread_mutex, NULL);
  thread_data->pool = pool;
  thread_data->service.host = strdup(host);
  ret = db_client_service_init(&thread_data->service, DB_USAGE_PERMANODE);
  return ret;
}

status_t db_permanode_thpool_add(const tryte_t* hash, const tryte_t* trytes, db_permanode_pool_t* pool) {
#define MAX_REQUEST_QUEUE_SIZE 100
  status_t ret = SC_OK;
  struct request* a_request; /* pointer to newly added request.     */
  pthread_mutex_lock(&pool->request_mutex);
  if (pool->num_requests >= MAX_REQUEST_QUEUE_SIZE) {
    pthread_mutex_unlock(&pool->request_mutex);
    return SC_STORAGE_THPOOL_ADD_REQUEST_FAIL;
  }
  pthread_mutex_unlock(&pool->request_mutex);

  /* create structure with new request */
  a_request = (struct request*)malloc(sizeof(struct request));
  if (!a_request) {
    ta_log_error("%s\n", ta_error_to_string(SC_TA_OOM));
    return SC_TA_OOM;
  }
  memcpy(a_request->hash, hash, sizeof(a_request->hash));
  memcpy(a_request->trytes, trytes, sizeof(a_request->trytes));

  pthread_mutex_lock(&pool->request_mutex);

  if (pool->num_requests == 0) { /* special case - list is empty */
    pool->requests = a_request;
    pool->last_request = a_request;
  } else {
    pool->last_request->next = a_request;
    pool->last_request = a_request;
  }

  pool->num_requests++;
  pthread_mutex_unlock(&pool->request_mutex);

  /* signal the condition variable - there's a new request to handle */
  pthread_cond_broadcast(&pool->got_request);

  return ret;
}

static struct request* get_request(db_permanode_pool_t* pool) {
  struct request* a_request; /* pointer to request.                 */

  pthread_mutex_lock(&pool->request_mutex);

  if (pool->num_requests > 0) {
    a_request = pool->requests;
    pool->requests = a_request->next;
    if (pool->requests == NULL) { /* this was the last request on the list */
      pool->last_request = NULL;
    }
    /* decrease the total number of pending requests */
    pool->num_requests--;
    pool->working_thread_num++;
  } else { /* requests list is empty */
    a_request = NULL;
  }

  pthread_mutex_unlock(&pool->request_mutex);

  /* return the request to the caller. */
  return a_request;
}

static void handle_request(struct request* a_request, db_client_service_t* service) {
#define MAX_RETRY_CNT 10
  if (a_request) {
    status_t ret;
    int retry_cnt = 0;
    do {
      ret = db_permanode_insert_transaction(service, a_request->hash, a_request->trytes);
      if (ret != SC_OK) {
        ta_log_error("Failed to insert transaction %s\n", a_request->hash);
        retry_cnt++;

        if (retry_cnt >= MAX_RETRY_CNT) {
          do {
            ta_log_warning("Failed %d times, try to reconnect\n", MAX_RETRY_CNT);
            char* host = strdup(service->host);
            db_client_service_free(service);
            service->host = host;

          } while (db_client_service_init(service, DB_USAGE_PERMANODE) != SC_OK);

          ta_log_info("Init db service successed\n");
          retry_cnt = 0;
        }
      }
    } while (ret != SC_OK);
  }
}

void* db_permanode_worker_handler(void* data) {
  struct request* a_request;
  db_worker_thread_t* thread_data = (db_worker_thread_t*)data;
  pthread_mutex_lock(&thread_data->thread_mutex);

  while (1) {
    a_request = get_request(thread_data->pool);
    if (a_request) { /* got a request - handle it and free it */

      handle_request(a_request, &thread_data->service);
      free(a_request);
      pthread_mutex_lock(&thread_data->pool->request_mutex);
      thread_data->pool->working_thread_num--;
      pthread_mutex_unlock(&thread_data->pool->request_mutex);

      pthread_cond_signal(&thread_data->pool->finish_request);
    } else {
      pthread_cond_wait(&thread_data->pool->got_request, &thread_data->thread_mutex);
    }
  }
  return NULL;
}

void db_permanode_tpool_wait(db_permanode_pool_t* pool) {
  if (pool == NULL) {
    ta_log_error("%s\n", ta_error_to_string(SC_TA_NULL));
    return;
  }
  pthread_mutex_lock(&(pool->request_mutex));
  while (1) {
    if (pool->num_requests != 0 || pool->working_thread_num != 0) {
      pthread_cond_wait(&(pool->finish_request), &(pool->request_mutex));
    } else {
      break;
    }
  }
  pthread_mutex_unlock(&(pool->request_mutex));
}