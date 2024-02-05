#pragma once

#ifdef WIN32
#include "SinglePassSigScannerWin32.hpp"
#else
#include "SinglePassSigScannerLinux.hpp"
#endif