/*
 * Copyright (C) 2018-2019 BiiLabs Co., Ltd. and Contributors
 * All Rights Reserved.
 * This is free software; you can redistribute it and/or modify it under the
 * terms of the MIT license. A copy of the license can be found in the file
 * "LICENSE" at the root of this distribution.
 */

#ifndef RESPONSE_TA_FIND_TRANSACTIONS_H_
#define RESPONSE_TA_FIND_TRANSACTIONS_H_

#include <stdlib.h>
#include "utils/containers/hash/hash243_queue.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file accelerator/core/response/ta_find_transactions.h
 */

/** struct of ta_find_transactions_by_tag_res_t */
typedef struct ta_find_transactions_res {
  /** Transaction hashes is a 243 long flex trits hash queue. */
  hash243_queue_t hashes;
} ta_find_transactions_by_tag_res_t;

/**
 * Allocate memory of ta_find_transactions_by_tag_res_t
 *
 * @return
 * - struct of ta_find_transactions_by_tag_res_t on success
 * - NULL on error
 */
ta_find_transactions_by_tag_res_t* ta_find_transactions_res_new();

/**
 * Free memory of ta_find_transactions_by_tag_res_t
 *
 * @param res Data type of ta_find_transactions_by_tag_res_t
 */
void ta_find_transactions_res_free(ta_find_transactions_by_tag_res_t** res);

#ifdef __cplusplus
}
#endif

#endif  // RESPONSE_TA_FIND_TRANSACTIONS_H_
