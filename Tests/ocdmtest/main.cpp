/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2021 Metrological
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

#ifndef MODULE_NAME
#define MODULE_NAME OpenCDMTest
#endif

#include <ocdm/open_cdm.h>

#include <core/core.h>
#include <iostream>

using namespace std;
using namespace WPEFramework;

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

int main(int argc, const char* argv [])
{
    cout << "<keysystem> <0,1,2,3> [1=ocdm dispose, 2=singleton dispose, 3=both, 0 is none]" << endl;

    if( argc < 3) {
        cout << "invalid args" << endl;
        return -1;
    }

    cout << "using system " << argv[1] << endl;

    char o = argv[2][0];
    cout << "using option " << o << endl;

    struct OpenCDMSystem* s = opencdm_create_system(argv[1]);

    if( s !=  nullptr) {
        cout << "ocdm system created" << endl;
        opencdm_destruct_system(s);
    } else {
        cout << "ocdm system could not be created" << endl;
    }

    if(o == '1' || o == '3') {
        cout << "opencdm dispose" << endl;
        opencdm_dispose();
    }
    if(o == '2' || o == '3') {
        cout << "singleton dispose" << endl;
        Core::Singleton::Dispose();
    }
    return 0;
}
