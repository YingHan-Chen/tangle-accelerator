/*
 * Copyright (C) 2020 BiiLabs Co., Ltd. and Contributors
 * All Rights Reserved.
 * This is free software; you can redistribute it and/or modify it under the
 * terms of the MIT license. A copy of the license can be found in the file
 * "LICENSE" at the root of this distribution.
 */
#include <getopt.h>
#include "ta_storage.h"

#define logger_id scylladb_logger_id

static status_t divide_historical_data(char* file_path) {
  /* The format of each line in dump files is HASH,TRYTES,snapshot_index */
#define BUFFER_SIZE (NUM_FLEX_TRITS_HASH + 1 + NUM_TRYTES_SERIALIZED_TRANSACTION + 12)  // 12 is for snapshot_index
  status_t ret = SC_OK;
  FILE* list_file = NULL;
  char file_name_buffer[256];

  if ((list_file = fopen(file_path, "r")) == NULL) {
    /* The specified configuration file does not exist */
    ret = SC_CONF_FOPEN_ERROR;
    ta_log_error("Fail to open file %s\n", file_path);
    goto exit;
  }

  while (fgets(file_name_buffer, 256, list_file) != NULL) {
    char input_buffer[BUFFER_SIZE];
    FILE* fp_read = NULL;
    FILE* fp_w = NULL;
    char out_file[BUFFER_SIZE] = {0};
    int name_len = strlen(file_name_buffer);
    int cnt = 0;
    int cnt_base1000 = -1;

    if (name_len > 0) {
      file_name_buffer[name_len - 1] = 0;
    }

    if ((fp_read = fopen(file_name_buffer, "r")) == NULL) {
      /* The specified configuration file does not exist */
      ret = SC_CONF_FOPEN_ERROR;
      ta_log_error("open file %s fail\n", file_name_buffer);
      goto exit;
    }
    ta_log_info("%s %s\n", "starting to divide file : ", file_name_buffer);
    while (fgets(input_buffer, BUFFER_SIZE, fp_read) != NULL) {
      if (cnt % 1000 == 0) {
        char out_count[10];
        out_count[0] = 0;
        cnt = 0;
        cnt_base1000++;
        if (cnt_base1000 % 1000 == 0) {
          if (fp_w) {
            fclose(fp_w);
          }
          strncpy(out_file, file_name_buffer, strlen(file_name_buffer));
          out_file[strlen(file_name_buffer)] = 0;
          strcat(out_file, "_");
          int tmp = cnt_base1000 / 1000;
          int idx = 0;
          int base = 100000;
          while (base) {
            if (tmp >= base || idx != 0) {
              out_count[idx] = '0' + (tmp / base);
              idx++;
              tmp %= base;
            }
            base /= 10;
          }
          if (idx == 0) {
            out_count[idx] = '0';
            idx++;
          }
          out_count[idx] = 0;
          strcat(out_file, out_count);
          fp_w = fopen(out_file, "w+");
          printf("%s\n", out_file);
        }
      }

      fputs(input_buffer, fp_w);
      cnt++;
    }
    if (fp_w) {
      fclose(fp_w);
    }
    ta_log_info("successfully divide file : %s\n", file_name_buffer);
  }

exit:
  if (ret == SC_OK) {
    ta_log_info("%s %s\n", "successfully divide file : ", file_path);
  } else {
    ta_log_error("Fail to divide file : %s\n", file_path);
  }
  return ret;
}

int main(int argc, char* argv[]) {
  char* file_path = NULL;
  const struct option longOpt[] = {{"file", required_argument, NULL, 'f'}, {NULL, 0, NULL, 0}};
  /* Parse the command line options */
  while (1) {
    int cmdOpt;
    int optIdx;
    cmdOpt = getopt_long(argc, argv, "sf:", longOpt, &optIdx);
    if (cmdOpt == -1) break;

    /* Invalid option */
    if (cmdOpt == '?') break;

    if (cmdOpt == 'f') {
      file_path = optarg;
    }
  }
  if (file_path == NULL) {
    ta_log_error("No file path specified\n");
    return 0;
  }
  if (ta_logger_init() != SC_OK) {
    ta_log_error("Fail to init logger\n");
    return EXIT_FAILURE;
  }
  scylladb_logger_init();

  divide_historical_data(file_path);

  scylladb_logger_release();

  return 0;
}
