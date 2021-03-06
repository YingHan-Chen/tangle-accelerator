/*
 * Copyright (C) 2019-2020 BiiLabs Co., Ltd. and Contributors
 * All Rights Reserved.
 * This is free software; you can redistribute it and/or modify it under the
 * terms of the MIT license. A copy of the license can be found in the file
 * "LICENSE" at the root of this distribution.
 */

#ifdef DB_ENABLE

#include "storage/ta_storage.h"
#include "tests/test_define.h"

#define logger_id scylladb_logger_id

static char* host = "localhost";
static char* keyspace_name;

static struct identity_s {
  char uuid_string[DB_UUID_STRING_LENGTH];
  char* hash;
  int8_t status;
} identities[] = {
    {.hash = TRANSACTION_1, .status = PENDING_TXN},
    {.hash = TRANSACTION_2, .status = INSERTING_TXN},
    {.hash = TRANSACTION_3, .status = CONFIRMED_TXN},
};

int identity_num = sizeof(identities) / sizeof(struct identity_s);

void test_db_get_identity_objs_by_status(db_client_service_t* db_client_service) {
  db_identity_array_t* db_identity_array = db_identity_array_new();
  for (int i = 0; i < identity_num; i++) {
    db_get_identity_objs_by_status(db_client_service, identities[i].status, db_identity_array);
  }
  db_identity_t* itr;
  int idx = 0;
  IDENTITY_TABLE_ARRAY_FOREACH(db_identity_array, itr) {
    char uuid_string[DB_UUID_STRING_LENGTH];
    db_get_identity_uuid_string(itr, uuid_string);

    TEST_ASSERT_EQUAL_STRING(uuid_string, identities[idx].uuid_string);
    TEST_ASSERT_EQUAL_MEMORY(db_ret_identity_hash(itr), (cass_byte_t*)identities[idx].hash,
                             sizeof(cass_byte_t) * DB_NUM_TRYTES_HASH);
    idx++;
  }
  db_identity_array_free(&db_identity_array);
}

void test_db_get_identity_objs_by_uuid_string(db_client_service_t* db_client_service) {
  db_identity_array_t* db_identity_array = db_identity_array_new();

  for (int i = 0; i < identity_num; i++) {
    db_get_identity_objs_by_uuid_string(db_client_service, identities[i].uuid_string, db_identity_array);
  }
  db_identity_t* itr;
  int idx = 0;
  IDENTITY_TABLE_ARRAY_FOREACH(db_identity_array, itr) {
    TEST_ASSERT_EQUAL_MEMORY(db_ret_identity_hash(itr), (cass_byte_t*)identities[idx].hash,
                             sizeof(cass_byte_t) * DB_NUM_TRYTES_HASH);
    TEST_ASSERT_EQUAL_INT(db_ret_identity_status(itr), identities[idx].status);
    idx++;
  }
  db_identity_array_free(&db_identity_array);
}

void test_db_get_identity_objs_by_hash(db_client_service_t* db_client_service) {
  db_identity_array_t* db_identity_array = db_identity_array_new();

  for (int i = 0; i < identity_num; i++) {
    db_get_identity_objs_by_hash(db_client_service, (cass_byte_t*)identities[i].hash, db_identity_array);
  }
  db_identity_t* itr;
  int idx = 0;
  IDENTITY_TABLE_ARRAY_FOREACH(db_identity_array, itr) {
    char uuid_string[DB_UUID_STRING_LENGTH];
    db_get_identity_uuid_string(itr, uuid_string);
    TEST_ASSERT_EQUAL_STRING(uuid_string, identities[idx].uuid_string);
    TEST_ASSERT_EQUAL_INT(db_ret_identity_status(itr), identities[idx].status);
    idx++;
  }
  db_identity_array_free(&db_identity_array);
}

void test_db_identity_table(void) {
  db_client_service_t db_client_service;
  db_client_service.host = strdup(host);
  TEST_ASSERT_EQUAL_INT(db_client_service_init(&db_client_service, DB_USAGE_NULL), SC_OK);
  TEST_ASSERT_EQUAL_INT(db_init_identity_keyspace(&db_client_service, true, keyspace_name), SC_OK);
  for (int i = 0; i < identity_num; i++) {
    db_insert_tx_into_identity(&db_client_service, identities[i].hash, identities[i].status, identities[i].uuid_string);
  }
  test_db_get_identity_objs_by_status(&db_client_service);
  test_db_get_identity_objs_by_uuid_string(&db_client_service);
  test_db_get_identity_objs_by_hash(&db_client_service);

  db_client_service_free(&db_client_service);
}
#endif  // DB_ENABLE

int main(int argc, char** argv) {
#ifdef DB_ENABLE
  int cmdOpt;
  int optIdx;
  const struct option longOpt[] = {
      {"db_host", required_argument, NULL, 'h'}, {"keyspace", required_argument, NULL, 'k'}, {NULL, 0, NULL, 0}};

  keyspace_name = "test_scylla";
  /* Parse the command line options */
  /* TODO: Support macOS since getopt_long() is GNU extension */
  while (1) {
    cmdOpt = getopt_long(argc, argv, "b:", longOpt, &optIdx);
    if (cmdOpt == -1) break;

    /* Invalid option */
    if (cmdOpt == '?') continue;

    if (cmdOpt == 'h') {
      host = optarg;
    }
    if (cmdOpt == 'k') {
      keyspace_name = optarg;
    }
  }

  UNITY_BEGIN();
  if (ta_logger_init() != SC_OK) {
    ta_log_error("logger init fail\n");
    return EXIT_FAILURE;
  }
  scylladb_logger_init();
  RUN_TEST(test_db_identity_table);
  scylladb_logger_release();
  return UNITY_END();
#else
  return 0;
#endif  // DB_ENABLE
}
