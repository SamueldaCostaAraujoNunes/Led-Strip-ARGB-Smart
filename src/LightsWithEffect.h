#ifndef _LIGHTSWITHEFFECT_H_
#define _LIGHTSWITHEFFECT_H_

#include <SinricProDevice.h>
#include <Capabilities/PowerStateController.h>
#include <Capabilities/ModeController.h>
#include <Capabilities/BrightnessController.h>
#include <Capabilities/ColorController.h>
#include <Capabilities/PercentageController.h>

class LightsWithEffect 
: public SinricProDevice
, public PowerStateController<LightsWithEffect>
, public ModeController<LightsWithEffect>
, public BrightnessController<LightsWithEffect>
, public ColorController<LightsWithEffect>
, public PercentageController<LightsWithEffect> {
  friend class PowerStateController<LightsWithEffect>;
  friend class ModeController<LightsWithEffect>;
  friend class BrightnessController<LightsWithEffect>;
  friend class ColorController<LightsWithEffect>;
  friend class PercentageController<LightsWithEffect>;
public:
  LightsWithEffect(const DeviceId &deviceId) : SinricProDevice(deviceId, "LightsWithEffect") {};
};

#endif
