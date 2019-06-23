
#include "UnitsListener.h"
#include "UnitsProvider.h"

UnitsListener::UnitsListener() {
  UnitsProvider::getInstance().addListener(this);
}

UnitsListener::~UnitsListener() {
  UnitsProvider::getInstance().removeListener(this);
}