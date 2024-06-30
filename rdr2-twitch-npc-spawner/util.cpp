#include "util.h"

#include <chrono>

namespace Util
{
	Hash GetHashKey(const char* string)
	{
		auto length = strlen(string);

		DWORD hash, i;
		for (hash = i = 0; i < length; ++i)
		{
			hash += tolower(string[i]);
			hash += (hash << 10);
			hash ^= (hash >> 6);
		}

		hash += (hash << 3);
		hash ^= (hash >> 11);
		hash += (hash << 15);

		return hash;
	}

	long long Now()
	{
		auto now = std::chrono::steady_clock::now();
		auto now_point_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
		auto now_epoch = now_point_ms.time_since_epoch();
		return std::chrono::duration_cast<std::chrono::milliseconds>(now_epoch).count();
	}

	std::string GetServerUrl()
	{
		return "http://localhost:5397";
	}
}
