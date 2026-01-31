#pragma once

#include "../shared/logger.h"
#include <QString>

namespace suyan {
namespace log {

void setMaxFileSize(qint64 bytes);
void setMaxFileCount(int count);

}  // namespace log
}  // namespace suyan
