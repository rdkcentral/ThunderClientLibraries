/*
* If not stated otherwise in this file or this component's license file the
* following copyright and licenses apply:
*
* Copyright 2024 RDK Management
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "open_cdm_impl.h"
#include "open_cdm_ext.h"
#include <gtest/gtest.h>
using namespace testing;

class OpenCDMExt_Test : public ::testing::Test {
protected:

    void SetUp() override {
    }

    void TearDown() override {
        }
};

TEST_F(OpenCDMExt_Test, opencdm_CreateSystem)
{
	int status = 0;
	const char keySystem[] = "playready";
	struct OpenCDMSystem* result;
	result = opencdm_create_system(keySystem);
	EXPECT_EQ(result, nullptr);
}
