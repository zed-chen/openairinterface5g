/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

#include "gtest/gtest.h"
extern "C" {
#include "openair2/LAYER2/nr_rlc/nr_rlc_entity_am.h"
#include "openair2/LAYER2/nr_rlc/nr_rlc_entity.h"
#include "common/utils/LOG/log.h"
#include "common/config/config_load_configmodule.h"
extern configmodule_interface_t *uniqCfg;
}
#include <iostream>

int sdu_delivered_count = 0;
int sdu_acked_count = 0;
void deliver_sdu(void *deliver_sdu_data, struct nr_rlc_entity_t *entity, char *buf, int size)
{
  sdu_delivered_count++;
  char payload[300];
  ASSERT_LE(size, sizeof(payload));
  snprintf(payload, size, "%s", buf);
  std::cout << "Delivered sdu: " << payload << std::endl;
}

void sdu_successful_delivery(void *sdu_successful_delivery_data, struct nr_rlc_entity_t *entity, int sdu_id)
{
  sdu_acked_count++;
  std::cout << "SDU " << sdu_id << " acked" << std::endl;
}

void max_retx_reached(void *max_retx_reached_data, struct nr_rlc_entity_t *entity)
{
}

TEST(nr_rlc_am_entity, test_init)
{
  nr_rlc_entity_t *entity = new_nr_rlc_entity_am(100,
                                                 100,
                                                 deliver_sdu,
                                                 NULL,
                                                 sdu_successful_delivery,
                                                 NULL,
                                                 max_retx_reached,
                                                 NULL,
                                                 45,
                                                 35,
                                                 0,
                                                 -1,
                                                 -1,
                                                 8,
                                                 12);
  char buf[30];
  EXPECT_EQ(entity->generate_pdu(entity, buf, sizeof(buf)), 0) << "No PDCP SDU provided to RLC, expected no RLC PDUS";

  entity->delete_entity(entity);
}

TEST(nr_rlc_am_entity, test_segmentation_reassembly)
{
  nr_rlc_entity_t *tx_entity = new_nr_rlc_entity_am(100,
                                                    100,
                                                    deliver_sdu,
                                                    NULL,
                                                    sdu_successful_delivery,
                                                    NULL,
                                                    max_retx_reached,
                                                    NULL,
                                                    45,
                                                    35,
                                                    0,
                                                    -1,
                                                    -1,
                                                    8,
                                                    12);

  nr_rlc_entity_t *rx_entity = new_nr_rlc_entity_am(100,
                                                    100,
                                                    deliver_sdu,
                                                    NULL,
                                                    sdu_successful_delivery,
                                                    NULL,
                                                    max_retx_reached,
                                                    NULL,
                                                    45,
                                                    35,
                                                    0,
                                                    -1,
                                                    -1,
                                                    8,
                                                    12);
  char buf[30] = {0};
  snprintf(buf, sizeof(buf), "%s", "Message");
  EXPECT_EQ(tx_entity->generate_pdu(tx_entity, buf, sizeof(buf)), 0) << "No PDCP SDU provided to RLC, expected no RLC PDUS";

  // Higher layer IF -> deliver PDCP SDU
  tx_entity->recv_sdu(tx_entity, buf, sizeof(buf), 0);
  sdu_delivered_count = 0;
  for (auto i = 0; i < 10; i++) {
    if (sdu_delivered_count == 1) {
      break;
    }
    int size = tx_entity->generate_pdu(tx_entity, buf, 10);
    EXPECT_GT(size, 0);
    rx_entity->recv_pdu(rx_entity, buf, size);
  }
  EXPECT_EQ(sdu_delivered_count, 1);
  sdu_acked_count = 0;
  int size = rx_entity->generate_pdu(rx_entity, buf, 30);
  tx_entity->recv_pdu(tx_entity, buf, size);
  EXPECT_EQ(sdu_acked_count, 1);

  tx_entity->delete_entity(tx_entity);
  rx_entity->delete_entity(rx_entity);
}

TEST(nr_rlc_am_entity, test_ack_out_of_order)
{
  nr_rlc_entity_t *tx_entity = new_nr_rlc_entity_am(100,
                                                    100,
                                                    deliver_sdu,
                                                    NULL,
                                                    sdu_successful_delivery,
                                                    NULL,
                                                    max_retx_reached,
                                                    NULL,
                                                    5,
                                                    35,
                                                    0,
                                                    4,
                                                    -1,
                                                    8,
                                                    12);

  nr_rlc_entity_t *rx_entity = new_nr_rlc_entity_am(100,
                                                    100,
                                                    deliver_sdu,
                                                    NULL,
                                                    sdu_successful_delivery,
                                                    NULL,
                                                    max_retx_reached,
                                                    NULL,
                                                    5,
                                                    35,
                                                    0,
                                                    4,
                                                    -1,
                                                    8,
                                                    12);

  char buf[30] = {0};
  EXPECT_EQ(tx_entity->generate_pdu(tx_entity, buf, sizeof(buf)), 0) << "No PDCP SDU provided to RLC, expected no RLC PDUS";
  // Higher layer IF -> deliver PDCP SDU
  // Generate 4 PDCP SDUS
  snprintf(buf, sizeof(buf), "%s", "Message 1");
  tx_entity->recv_sdu(tx_entity, buf, sizeof(buf), 0);
  snprintf(buf, sizeof(buf), "%s", "Message 2");
  tx_entity->recv_sdu(tx_entity, buf, sizeof(buf), 1);
  snprintf(buf, sizeof(buf), "%s", "Message 3");
  tx_entity->recv_sdu(tx_entity, buf, sizeof(buf), 2);

  sdu_delivered_count = 0;
  sdu_acked_count = 0;
  char pdu_buf[40];
  uint64_t time = 0;
  for (auto i = 0; i < 3; i++) {
    int size = tx_entity->generate_pdu(tx_entity, pdu_buf, sizeof(pdu_buf));
    EXPECT_GT(size, 0);
    // Do not deliver PDU 2 to RX entity
    if (i != 1) {
      rx_entity->recv_pdu(rx_entity, pdu_buf, size);
    }
    tx_entity->set_time(tx_entity, time);
    rx_entity->set_time(tx_entity, time);
    time++;
  }
  EXPECT_EQ(sdu_delivered_count, 2) << "Expected 2 out of 3 SDUs delivered";

  int size = rx_entity->generate_pdu(rx_entity, pdu_buf, sizeof(pdu_buf));
  EXPECT_GT(size, 0);
  tx_entity->recv_pdu(tx_entity, pdu_buf, size);

  // Triggers t-poll-retransmit and retransmission of the lost PDU
  for (auto i = 0; i < 5; i++) {
    tx_entity->set_time(tx_entity, time);
    rx_entity->set_time(tx_entity, time);
    size = tx_entity->generate_pdu(tx_entity, pdu_buf, sizeof(pdu_buf));
    if (size != 0)
      rx_entity->recv_pdu(rx_entity, pdu_buf, size);
    size = rx_entity->generate_pdu(rx_entity, pdu_buf, sizeof(pdu_buf));
    if (size != 0)
      tx_entity->recv_pdu(tx_entity, pdu_buf, size);
    time++;
  }
  EXPECT_EQ(sdu_acked_count, 3);
  EXPECT_EQ(sdu_delivered_count, 3);

  tx_entity->delete_entity(tx_entity);
  rx_entity->delete_entity(rx_entity);
}

int main(int argc, char **argv)
{
  logInit();
  uniqCfg = load_configmodule(argc, argv, CONFIG_ENABLECMDLINEONLY);
  g_log->log_component[RLC].level = OAILOG_TRACE;
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
