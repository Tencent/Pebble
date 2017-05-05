/*
 * Tencent is pleased to support the open source community by making Pebble available.
 * Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
 * Licensed under the MIT License (the "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 * http://opensource.org/licenses/MIT
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing permissions and limitations under
 * the License.
 *
 */

#include "framework/when_all.h"

namespace pebble {

void WhenAll(const std::vector<Call>& f_list) {
    WhenAllInit(f_list.size());

    for (std::vector<Call>::const_iterator it = f_list.begin();
            it != f_list.end(); ++it) {
        (*it)(&num_called, &num_parallel);
    }

    WhenAllCheck();
}

// f1
void WhenAll(const Call& f1) {
    WhenAllInit(1);

    WhenAllCall(1);

    WhenAllCheck();
}

// f2
void WhenAll(const Call& f1,
             const Call& f2) {
    WhenAllInit(2);

    WhenAllCall(1);
    WhenAllCall(2);

    WhenAllCheck();
}

// f3
void WhenAll(const Call& f1,
             const Call& f2,
             const Call& f3) {
    WhenAllInit(3);

    WhenAllCall(1);
    WhenAllCall(2);
    WhenAllCall(3);

    WhenAllCheck();
}

// f4
void WhenAll(const Call& f1,
             const Call& f2,
             const Call& f3,
             const Call& f4) {
    WhenAllInit(4);

    WhenAllCall(1);
    WhenAllCall(2);
    WhenAllCall(3);
    WhenAllCall(4);

    WhenAllCheck();
}

// f5
void WhenAll(const Call& f1,
             const Call& f2,
             const Call& f3,
             const Call& f4,
             const Call& f5) {
    WhenAllInit(5);

    WhenAllCall(1);
    WhenAllCall(2);
    WhenAllCall(3);
    WhenAllCall(4);
    WhenAllCall(5);

    WhenAllCheck();
}

// f6
void WhenAll(const Call& f1,
             const Call& f2,
             const Call& f3,
             const Call& f4,
             const Call& f5,
             const Call& f6) {
    WhenAllInit(6);

    WhenAllCall(1);
    WhenAllCall(2);
    WhenAllCall(3);
    WhenAllCall(4);
    WhenAllCall(5);
    WhenAllCall(6);

    WhenAllCheck();
}

// f7
void WhenAll(const Call& f1,
             const Call& f2,
             const Call& f3,
             const Call& f4,
             const Call& f5,
             const Call& f6,
             const Call& f7) {
    WhenAllInit(7);

    WhenAllCall(1);
    WhenAllCall(2);
    WhenAllCall(3);
    WhenAllCall(4);
    WhenAllCall(5);
    WhenAllCall(6);
    WhenAllCall(7);

    WhenAllCheck();
}

// f8
void WhenAll(const Call& f1,
             const Call& f2,
             const Call& f3,
             const Call& f4,
             const Call& f5,
             const Call& f6,
             const Call& f7,
             const Call& f8) {
    WhenAllInit(8);

    WhenAllCall(1);
    WhenAllCall(2);
    WhenAllCall(3);
    WhenAllCall(4);
    WhenAllCall(5);
    WhenAllCall(6);
    WhenAllCall(7);
    WhenAllCall(8);

    WhenAllCheck();
}

} // namespace pebble
