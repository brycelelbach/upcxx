#include "Amr.h"

AmrLevel *AmrLevel::getLevel(int lev) {
  return parent->getLevel(lev);
}
