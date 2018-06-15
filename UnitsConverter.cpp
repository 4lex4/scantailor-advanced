
#include "UnitsConverter.h"
#include "Dpm.h"

UnitsConverter::UnitsConverter(const Dpi& dpi) : dpi(dpi) {}

void UnitsConverter::convert(double& horizontalValue, double& verticalValue, Units fromUnits, Units toUnits) const {
  if (dpi.isNull() || (fromUnits == toUnits)) {
    return;
  }

  auto dpm = Dpm(dpi);
  switch (fromUnits) {
    case PIXELS:
      switch (toUnits) {
        case MILLIMETRES:
          horizontalValue = horizontalValue / dpm.horizontal() * 1000.;
          verticalValue = verticalValue / dpm.vertical() * 1000.;
          break;
        case CENTIMETRES:
          horizontalValue = horizontalValue / dpm.horizontal() * 100.;
          verticalValue = verticalValue / dpm.vertical() * 100.;
          break;
        case INCHES:
          horizontalValue /= dpi.horizontal();
          verticalValue /= dpi.vertical();
          break;
        default:
          break;
      }
      break;
    case MILLIMETRES:
      switch (toUnits) {
        case PIXELS:
          horizontalValue = horizontalValue / 1000. * dpm.horizontal();
          verticalValue = verticalValue / 1000. * dpm.vertical();
          break;
        case CENTIMETRES:
          horizontalValue = horizontalValue / 10.;
          verticalValue = verticalValue / 10.;
          break;
        case INCHES:
          horizontalValue = horizontalValue / 1000. * dpm.horizontal() / dpi.horizontal();
          verticalValue = verticalValue / 1000. * dpm.vertical() / dpi.vertical();
          break;
        default:
          break;
      }
      break;
    case CENTIMETRES:
      switch (toUnits) {
        case PIXELS:
          horizontalValue = horizontalValue / 100. * dpm.horizontal();
          verticalValue = verticalValue / 100. * dpm.vertical();
          break;
        case MILLIMETRES:
          horizontalValue = horizontalValue * 10.;
          verticalValue = verticalValue * 10.;
          break;
        case INCHES:
          horizontalValue = horizontalValue / 100. * dpm.horizontal() / dpi.horizontal();
          verticalValue = verticalValue / 100. * dpm.vertical() / dpi.vertical();
          break;
        default:
          break;
      }
      break;
    case INCHES:
      switch (toUnits) {
        case PIXELS:
          horizontalValue *= dpi.horizontal();
          verticalValue *= dpi.vertical();
          break;
        case MILLIMETRES:
          horizontalValue = horizontalValue * dpi.horizontal() / dpm.horizontal() * 1000.;
          verticalValue = verticalValue * dpi.vertical() / dpm.vertical() * 1000.;
          break;
        case CENTIMETRES:
          horizontalValue = horizontalValue * dpi.horizontal() / dpm.horizontal() * 100.;
          verticalValue = verticalValue * dpi.vertical() / dpm.vertical() * 100.;
          break;
        default:
          break;
      }
      break;
  }
}

QTransform UnitsConverter::transform(Units fromUnits, Units toUnits) const {
  double xScaleFactor = 1.0;
  double yScaleFactor = 1.0;
  convert(xScaleFactor, yScaleFactor, fromUnits, toUnits);

  return QTransform().scale(xScaleFactor, yScaleFactor);
}

const Dpi& UnitsConverter::getDpi() const {
  return dpi;
}

void UnitsConverter::setDpi(const Dpi& dpi) {
  UnitsConverter::dpi = dpi;
}
