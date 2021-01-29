## ImageViewer

Fast and lightweighed image viewer for Windows

## License

Copyright 2018-2021 Alexey Gruzdev

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.


## Installation

Prebuilt ShibaView executable requires MSVC 2019 runtime on Windows, so please install the latest version of [Microsoft Visual C++ Redistributable for Visual Studio 2019](https://support.microsoft.com/en-us/help/2977003/the-latest-supported-visual-c-downloads)

## FreeImage changes

  1. Image load color space is chnaged to RGB by a compile definition, fixed appeared compilation errors
  2. Exported utility functions from DLL
