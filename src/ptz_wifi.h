#pragma once

namespace ptz {

class PtzWifi {
 public:
  void begin(bool forcePortal);
  void resetAndProvision();
};

} // namespace ptz
