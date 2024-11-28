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

#include <stdint.h>
#include <vector>
#include <algorithm>
#include <numeric>
extern "C" {
#include "openair1/PHY/TOOLS/tools_defs.h"
struct configmodule_interface_s;
struct configmodule_interface_s *uniqCfg = NULL;
void exit_function(const char *file, const char *function, const int line, const char *s, const int assert)
{
  if (assert) {
    abort();
  } else {
    exit(EXIT_SUCCESS);
  }
}
}
#include <cstdio>
#include "common/utils/LOG/log.h"
#include "openair1/PHY/TOOLS/phy_test_tools.hpp"

int main()
{
  const int shift = 15; // it should always be 15 to keep int16 in same range
  for (int vector_size = 1237; vector_size < 1237 + 8; vector_size++) {
    auto input1 = generate_random_c16(vector_size);
    auto input2 = generate_random_c16(vector_size);

    // Clip the input for -32768 because this will make different result
    // C version promote the int16 to int before doing the conjugate (so negate imaginary part)
    // simd version do it on the int16, so the result overflows to -32768
    // other overflows exist, but they make the same
    // in C we do with int promotion, for real part:
    // (int)x.r*(int)y.r - (int)x.i*(int)y.i
    // that can overflow because each multiplication result is full int range
    // so when we sum, it overflows
    // _mm256_madd_epi16() overflows also, as do regular addition instruction
    // so the result will be the same (the same wrong value)
    
    for(auto it = input1.begin(); it != input1.end(); it++ ) {
      if (it->r == -32768)
	it->r=-32767;
      if (it->i == -32768)
	it->i=-32767;
    }
    for(auto it = input2.begin(); it != input2.end(); it++ ) {
      if (it->r == -32768)
	it->r=-32767;
      if (it->i == -32768)
	it->i=-32767;
    }
    AlignedVector512<c16_t> output;
    output.resize(vector_size);
    mult_complex_vectors(input1.data(), input2.data(), output.data(), vector_size, shift);
    for (int i = 0; i < vector_size; i++) {
      c16_t res = c16mulShift(input1[i], input2[i], shift);
      if (output[i].r != res.r || output[i].i != res.i) {
        printf("Error at %d: (%d,%d) * (%d,%d) = (%d,%d) (should be (%d,%d))\n",
               i,
               input1[i].r,
               input1[i].i,
               input2[i].r,
               input2[i].i,
               output[i].r,
               output[i].i,
               res.r,
               res.i);
        return 1;
      }
    }
  }
  return 0;
}
