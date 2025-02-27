/** @file
 * Copyright (c) 2016-2018,2022 Arm Limited or its affiliates. All rights reserved.
 * SPDX-License-Identifier : Apache-2.0

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 **/

#include "val/include/sbsa_avs_val.h"
#include "val/include/val_interface.h"

#include "val/include/sbsa_avs_timer.h"
#include "val/include/sbsa_avs_pe.h"

#define TEST_NUM   (AVS_TIMER_TEST_NUM_BASE + 5)
#define TEST_DESC  "Check EL2-Virtual timer interrupt "

static uint32_t intid;

static
void
isr(void)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  /* We received our interrupt, so disable timer from generating further interrupts */
  val_timer_set_vir_el2(0);
  val_print(AVS_PRINT_INFO, "\n       Received interrupt    ", 0);
  val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
  val_gic_end_of_interrupt(intid);
}



static
void
payload(void)
{

  uint32_t timeout = TIMEOUT_LARGE;
  uint64_t timer_expire_val = 100;
  uint64_t data = 0;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  /* This test is run only when ARM v8.1 Virtualized Host Extensions are supported */
  data = val_pe_reg_read(ID_AA64MMFR1_EL1);

  if (((data >> 8) & 0xF) == 0) {
    val_print(AVS_PRINT_WARN, "\n       v8.1 VHE not supported on this PE ", 0);
    val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 01));
    return;
  }

  intid = val_timer_get_info(TIMER_INFO_VIR_EL2_INTID, 0);

  if (intid != 28) {
      val_print(AVS_PRINT_ERR, "\n       Incorrect PPI value %d     ", intid);
      val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
      return;
  }

  val_gic_install_isr(intid, isr);

  val_timer_set_vir_el2(timer_expire_val);

  while ((--timeout > 0) && (IS_RESULT_PENDING(val_get_status(index))));

  if (timeout == 0) {
    val_print(AVS_PRINT_ERR, "\n       EL2-Virtual timer interrupt not received on %d   ", intid);
    val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
  }

}

/**
  @brief  Check interrupt generation from EL2-Virtual timer.
          Depends on ARM V8.1 VHE Extensions.
**/
uint32_t
t005_entry(uint32_t num_pe)
{

  uint32_t status = AVS_STATUS_FAIL;

  num_pe = 1;  //This Timer test is run on single processor

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe, g_sbsa_level);
  if (status != AVS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe);

  val_report_status(0, SBSA_AVS_END(g_sbsa_level, TEST_NUM));

  return status;
}
