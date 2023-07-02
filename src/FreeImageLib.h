/**
 * @file
 *
 * Copyright 2018-2023 Alexey Gruzdev
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FREEIMAGELIB_H
#define FREEIMAGELIB_H

/**
 * Provides access to dynamic symbols from the FreeImage.dll
 */
class FreeImageLib
{
public:
    // singleton
    static FreeImageLib& getInstance();

    /**
     * Find symbol by name in the currently loaded instance of the FreeImage.dll
     */
    void* findSymbol(const char* name);


    FreeImageLib(const FreeImageLib&) = delete;
    FreeImageLib(FreeImageLib&&) = delete;

    FreeImageLib& operator=(const FreeImageLib&) = delete;
    FreeImageLib& operator=(FreeImageLib&&) = delete;

private:
    FreeImageLib();
    ~FreeImageLib();

    void* mHandle = nullptr;
};

#endif // FREEIMAGELIB_H
