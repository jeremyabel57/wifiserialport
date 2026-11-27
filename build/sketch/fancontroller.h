#line 1 "D:\\Users\\person\\Documents\\vs\\wifiserialport\\fancontroller.h"
#ifndef FANCONTROLLER_H
#define FANCONTROLLER_H

#include <Arduino.h>

class FanController {
  public:
    struct CurvePoint {
      float temperatureC;
      float pwmPercent;
    };

    FanController();
    bool begin();
    void loop();

    bool setCurve(const CurvePoint *points, size_t count);
    void setTachPulsesPerRevolution(uint8_t pulses);
    void setFanSpeedPercent(float percent);

    float temperatureC() const;
    float tachFrequencyHz() const;
    float fanRpm() const;
    float fanSpeedPercent() const;

  private:
    static const uint8_t MaxCurvePoints = 8;
    static const uint8_t SdaPin = 1;
    static const uint8_t SclPin = 2;
    static const uint8_t TachPin = 4;
    static const uint8_t PwmPin = 7;
    static const uint8_t PwmResolution = 10;
    static const uint32_t PwmFrequencyHz = 25000;

    CurvePoint curve[MaxCurvePoints];
    size_t curveCount;
    uint8_t tachPulsesPerRevolution;
    bool sensorReady;
    bool tachReady;
    bool pwmReady;
    bool sensorFaultReported;
    unsigned long lastSampleMs;
    float currentTemperatureC;
    float currentTachFrequencyHz;
    float currentFanRpm;
    float currentFanSpeedPercent;

    float pwmForTemperature(float temperature) const;
};

#ifndef FANCONTROLLER_C
extern FanController fanController;
#endif

#endif
