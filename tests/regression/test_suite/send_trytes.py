from common import *
import json
import unittest
import time
import logging


class SendTrytes(unittest.TestCase):

    # Single 2673 trytes legal transaction object (pass)
    @test_logger
    def test_single_legal_txn(self):
        res = API("/tryte",
                  post_data=map_field(self.post_field, [self.query_string[0]]))
        self.assertEqual(STATUS_CODE_200, res["status_code"])

        res_json = json.loads(res["content"])
        self.assertEqual(self.query_string[0], res_json[self.post_field[0]])

    # Multiple 2673 trytes legal transaction object (pass)
    @test_logger
    def test_mult_legal_txn(self):
        res = API("/tryte",
                  post_data=map_field(self.post_field, [self.query_string[1]]))
        self.assertEqual(STATUS_CODE_200, res["status_code"])

        res_json = json.loads(res["content"])
        self.assertEqual(self.query_string[1], res_json[self.post_field[0]])

    # Single 200 trytes illegal transaction object (fail)
    @test_logger
    def test_200_trytes_txn(self):
        res = API("/tryte",
                  post_data=map_field(self.post_field, [self.query_string[2]]))
        self.assertEqual(STATUS_CODE_500, res["status_code"])

    # Single 3000 trytes illegal transaction object (fail)
    @test_logger
    def test_3000_trytes_txn(self):
        res = API("/tryte",
                  post_data=map_field(self.post_field, [self.query_string[3]]))
        self.assertEqual(STATUS_CODE_500, res["status_code"])

    # Single unicode illegal transaction object (fail)
    @test_logger
    def test_unicode_txn(self):
        res = API("/tryte",
                  post_data=map_field(self.post_field, [self.query_string[4]]))
        self.assertEqual(STATUS_CODE_500, res["status_code"])

    # Empty trytes list (fail)
    @test_logger
    def test_empty_trytes(self):
        res = API("/tryte",
                  post_data=map_field(self.post_field, [self.query_string[5]]))
        self.assertEqual(STATUS_CODE_500, res["status_code"])

    # Empty not trytes list object (fail)
    @test_logger
    def test_empty_not_trytes_list(self):
        res = API("/tryte",
                  post_data=map_field(self.post_field, [self.query_string[6]]))
        self.assertEqual(STATUS_CODE_500, res["status_code"])

    # Time statistics
    @test_logger
    def test_time_statistics(self):
        time_cost = []
        post_data = {"trytes": self.query_string[0]}
        post_data_json = json.dumps(post_data)
        for i in range(TIMES_TOTAL):
            start_time = time.time()
            API("/tryte", post_data=post_data_json)
            time_cost.append(time.time() - start_time)

        eval_stat(time_cost, "send trytes")

    @classmethod
    def setUpClass(cls):
        rand_trytes = []
        for i in range(2):
            all_9_context = fill_nines("", 2673 - 81 * 3)
            res = API("/tips/pair/", get_data="")
            unittest.TestCase().assertEqual(STATUS_CODE_200,
                                            res["status_code"])
            res_json = json.loads(res["content"])
            rand_trytes.append(all_9_context + res_json["trunkTransaction"] +
                               res_json["branchTransaction"] +
                               fill_nines("", 81))

        cls.query_string = [[rand_trytes[0]], rand_trytes,
                            [gen_rand_trytes(200)], [gen_rand_trytes(3000)],
                            ["逼類不司"], [""], ""]
        cls.post_field = ["trytes"]
