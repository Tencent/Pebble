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

#include <iostream>
#include <stdlib.h>

#include "common/error.h"
#include "framework/register_error.h"

/**
 * 错误码解析工具
 */
int main(int argc,  char* argv[]) {
    if (argc != 2) {
        std::cout << "usage : " << argv[0] << " error_code" << std::endl
            // << "        " << argv[0] << " -a" << std::endl // 暂不支持查询全部错误码
            ;
        return -1;
    }

    const char* begin = argv[1];
    char* end = NULL;

    int error_code = strtol(begin, &end, 0);
    if (end > begin) {
        pebble::RegisterErrorString();
        std::cout << error_code << " : " << pebble::GetErrorString(error_code) << std::endl;
    } else {
        std::cout << "can't parse " << argv[1] << std::endl;
    }

    return 0;
}

