#pragma once

#include <string>

#include "inc/types.h"

namespace Util
{
	Hash GetHashKey(const char* string);
	long long Now();
	std::string GetServerUrl();
}
