#pragma once

#include <string>

namespace Auth
{
	enum eValidationStatus { Pending, Authorised, Unauthorised };

	std::string GetJwt();
	void SaveJwt(std::string jwt);
	eValidationStatus ValidateJwt();
}
