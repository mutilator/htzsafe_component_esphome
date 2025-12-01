#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include <map>
#include <vector>

namespace esphome {
namespace htzsafe {

static const uint32_t HEADER = 0xebaf05;
static const uint32_t ACTIVATION_DURATION = 5000;  // 5 seconds in milliseconds

class HTZSafe;

class HTZSafeBinarySensor : public binary_sensor::BinarySensor, public Component {
 public:
  void set_parent(HTZSafe *parent) { parent_ = parent; }
  void set_device_id(uint16_t device_id) { device_id_ = device_id; }
  uint16_t get_device_id() const { return device_id_; }

 protected:
  HTZSafe *parent_;
  uint16_t device_id_;
};

class HTZSafe : public Component, public uart::UARTDevice {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;

  void register_sensor(HTZSafeBinarySensor *sensor, uint16_t identifier);

 protected:
  void process_data_();
  void activate_device_(uint16_t identifier);
  void deactivate_device_(uint16_t identifier);

  std::map<uint16_t, HTZSafeBinarySensor *> sensors_;
  std::map<uint16_t, uint32_t> active_devices_;  // Maps identifier to deactivation time
  uint8_t read_buffer_[5];
  uint8_t buffer_index_ = 0;
  uint32_t bytes_skipped_ = 0;
  uint32_t loop_count_ = 0;
  uint32_t last_log_time_ = 0;
};

}  // namespace htzsafe
}  // namespace esphome
