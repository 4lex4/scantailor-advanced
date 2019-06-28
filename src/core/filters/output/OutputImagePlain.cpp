#include "OutputImagePlain.h"
#include <imageproc/Dpm.h>

namespace output {
OutputImagePlain::OutputImagePlain(const QImage& image) : m_image(image) {}

QImage OutputImagePlain::toImage() const {
  return m_image;
}

bool OutputImagePlain::isNull() const {
  return m_image.isNull();
}

void OutputImagePlain::setDpm(const Dpm& dpm) {
  m_image.setDotsPerMeterX(dpm.horizontal());
  m_image.setDotsPerMeterY(dpm.vertical());
}
}  // namespace output