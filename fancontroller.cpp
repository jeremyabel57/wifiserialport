#define FANCONTROLLER_C
#include "fancontroller.h"

#include <Adafruit_SHT31.h>
#include <Wire.h>
#include <math.h>
#include <driver/pcnt.h>

#if __has_include(<esp_arduino_version.h>)
#include <esp_arduino_version.h>
#endif

namespace {
Adafruit_SHT31 sht31;
const pcnt_unit_t TachPcntUnit = PCNT_UNIT_0;
const unsigned long SampleIntervalMs = 1000;
}

FanController fanController;

FanController::FanController()
  : curveCount(4),
    tachPulsesPerRevolution(2),
    sensorReady(false),
    tachReady(false),
    pwmReady(false),
    sensorFaultReported(false),
    lastSampleMs(0),
    currentTemperatureC(NAN),
    currentTachFrequencyHz(0),
    currentFanRpm(0),
    currentFanSpeedPercent(100) {
  curve[0] = {25.0f, 0.0f};
  curve[1] = {35.0f, 30.0f};
  curve[2] = {45.0f, 65.0f};
  curve[3] = {55.0f, 100.0f};
}

bool FanController::begin() {
  bool success = true;

  Wire.begin(SdaPin, SclPin);
  sensorReady = sht31.begin(0x44);
  if (!sensorReady) {
    Serial.println("Fan controller: SHT31 sensor not found at 0x44");
    success = false;
  }

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  pwmReady = ledcAttach(PwmPin, PwmFrequencyHz, PwmResolution);
#else
  const uint8_t pwmChannel = 0;
  pwmReady = ledcSetup(pwmChannel, PwmFrequencyHz, PwmResolution) != 0;
  if (pwmReady) {
    ledcAttachPin(PwmPin, pwmChannel);
  }
#endif
  if (!pwmReady) {
    Serial.println("Fan controller: unable to configure PWM on GPIO7");
    success = false;
  } else {
    setFanSpeedPercent(100.0f);
  }

  pcnt_config_t tachConfig = {};
  tachConfig.pulse_gpio_num = TachPin;
  tachConfig.ctrl_gpio_num = PCNT_PIN_NOT_USED;
  tachConfig.channel = PCNT_CHANNEL_0;
  tachConfig.unit = TachPcntUnit;
  tachConfig.pos_mode = PCNT_COUNT_INC;
  tachConfig.neg_mode = PCNT_COUNT_DIS;
  tachConfig.lctrl_mode = PCNT_MODE_KEEP;
  tachConfig.hctrl_mode = PCNT_MODE_KEEP;
  tachConfig.counter_h_lim = 32767;
  tachConfig.counter_l_lim = 0;

  esp_err_t                tachError = pcnt_unit_config(&tachConfig);
  if (tachError == ESP_OK) tachError = pcnt_set_filter_value(TachPcntUnit, 1000);
  if (tachError == ESP_OK) tachError = pcnt_filter_enable(TachPcntUnit);
  if (tachError == ESP_OK) tachError = pcnt_counter_clear(TachPcntUnit);
  if (tachError == ESP_OK) tachError = pcnt_counter_resume(TachPcntUnit);
  tachReady = tachError == ESP_OK;
  if (!tachReady) {
    Serial.printf("Fan controller: unable to configure tachometer PCNT (%d)\n", tachError);
    success = false;
  }

  lastSampleMs = millis();
  return success;
}

void FanController::loop() {
  const unsigned long now = millis();
  if (now - lastSampleMs < SampleIntervalMs) return;

  const unsigned long elapsedMs = now - lastSampleMs;
  lastSampleMs = now;

  if (sensorReady) {
    const float measuredTemperature = sht31.readTemperature();
    if (isfinite(measuredTemperature)) {
      currentTemperatureC = measuredTemperature;
      currentFanSpeedPercent = pwmForTemperature(measuredTemperature);
      setFanSpeedPercent(currentFanSpeedPercent);
      sensorFaultReported = false;
    } else {
      setFanSpeedPercent(100.0f);
      if (!sensorFaultReported) {
        Serial.println("Fan controller: SHT31 temperature read failed; setting fan to 100%");
        sensorFaultReported = true;
      }
    }
  } else {
    setFanSpeedPercent(100.0f);
  }

  if (tachReady) {
    int16_t pulses = 0;
    if (pcnt_get_counter_value(TachPcntUnit, &pulses) == ESP_OK &&
        pcnt_counter_clear(TachPcntUnit) == ESP_OK) {
      currentTachFrequencyHz = (float)pulses * 1000.0f / (float)elapsedMs;
      currentFanRpm = currentTachFrequencyHz * 60.0f / tachPulsesPerRevolution;
    } else {
      Serial.println("Fan controller: unable to read tachometer counter");
    }
  }
}

bool FanController::setCurve(const CurvePoint *points, size_t count) {
  if (points == nullptr || count < 2 || count > MaxCurvePoints) {
    return false;
  }

  for (size_t i = 0; i < count; ++i) {
    if (!isfinite(points[i].temperatureC) ||
        !isfinite(points[i].pwmPercent) ||
        points[i].pwmPercent < 0.0f ||
        points[i].pwmPercent > 100.0f) {
      return false;
    }
    if (i > 0 &&
        (points[i].temperatureC <= points[i - 1].temperatureC ||
         points[i].pwmPercent < points[i - 1].pwmPercent)) {
      return false;
    }
  }

  for (size_t i = 0; i < count; ++i) {
    curve[i] = points[i];
  }
  curveCount = count;
  return true;
}

void FanController::setTachPulsesPerRevolution(uint8_t pulses) {
  if (pulses > 0) tachPulsesPerRevolution = pulses;
}

void FanController::setFanSpeedPercent(float percent) {
  if (!isfinite(percent)) return;
  if (percent < 0.0f) percent = 0.0f;
  if (percent > 100.0f) percent = 100.0f;
  currentFanSpeedPercent = percent;
  if (!pwmReady) return;

  const uint32_t maxDuty = (1UL << PwmResolution) - 1;
  const uint32_t duty = (uint32_t)lroundf(percent * maxDuty / 100.0f);
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(PwmPin, duty);
#else
  ledcWrite(0, duty);
#endif
}

float FanController::temperatureC() const {
  return currentTemperatureC;
}

float FanController::tachFrequencyHz() const {
  return currentTachFrequencyHz;
}

float FanController::fanRpm() const {
  return currentFanRpm;
}

float FanController::fanSpeedPercent() const {
  return currentFanSpeedPercent;
}

float FanController::pwmForTemperature(float temperature) const {
  if (temperature <= curve[0].temperatureC) return curve[0].pwmPercent;
  if (temperature >= curve[curveCount - 1].temperatureC) return curve[curveCount - 1].pwmPercent;

  for (size_t i = 1; i < curveCount; ++i) {
    if (temperature <= curve[i].temperatureC) {
      const CurvePoint &low = curve[i - 1];
      const CurvePoint &high = curve[i];
      const float fraction = (temperature - low.temperatureC) /
                             (high.temperatureC - low.temperatureC);
      return low.pwmPercent + fraction * (high.pwmPercent - low.pwmPercent);
    }
  }
  return curve[curveCount - 1].pwmPercent;
}
