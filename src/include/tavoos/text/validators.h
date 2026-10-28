#pragma once

#include <tavoos/export.hpp>
#include <tavoos/text/texteditmodel.h>

#include <string>

namespace Tavoos {

TAVOOS_EXPORT TextValidator intValidator(long long bottom, long long top);
TAVOOS_EXPORT TextValidator doubleValidator(double bottom, double top, int decimals);
TAVOOS_EXPORT TextValidator regexValidator(const std::string& pattern);

}
