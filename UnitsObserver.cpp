
#include "UnitsObserver.h"
#include "UnitsProvider.h"

UnitsObserver::UnitsObserver() {
  UnitsProvider::getInstance()->attachObserver(this);
}

UnitsObserver::~UnitsObserver() {
  UnitsProvider::getInstance()->detachObserver(this);
}