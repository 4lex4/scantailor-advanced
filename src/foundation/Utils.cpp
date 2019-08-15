
#include "Utils.h"

namespace foundation {
QString Utils::doubleToString(double val) {
  return QString::number(val, 'g', 16);
}
}