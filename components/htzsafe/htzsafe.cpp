#include "htzsafe.h"
#include "esphome/core/log.h"

namespace esphome {
namespace htzsafe {

static const char *TAG = "htzsafe";

void HTZSafe::setup() {
  ESP_LOGI(TAG, "Setting up HTZSafe component");
  
  // Log all registered devices
  for (const auto &entry : sensors_) {
    ESP_LOGI(TAG, "Registered device: %s (ID: 0x%04x)", entry.second->get_name().c_str(), entry.first);
  }
}

void HTZSafe::register_sensor(HTZSafeBinarySensor *sensor, uint16_t identifier) {
  sensors_[identifier] = sensor;
}

void HTZSafe::loop() {
  // Process any available data
  while (this->available()) {
    uint8_t byte = this->read();
    
    // Look for the start of the header (0xeb)
    if (buffer_index_ == 0) {
      if (byte == 0xeb) {
        read_buffer_[buffer_index_] = byte;
        buffer_index_++;
        bytes_skipped_ = 0;
      } else {
        bytes_skipped_++;
      }
      continue;
    }
    
    read_buffer_[buffer_index_] = byte;
    buffer_index_++;

    // Header: 0xebaf05 (3 bytes) + identifier (2 bytes) = 5 bytes total
    if (buffer_index_ >= 5) {
      process_data_();
      buffer_index_ = 0;
    }
  }

  // Check for devices that should be deactivated
  uint32_t now = millis();
  std::vector<uint16_t> to_deactivate;
  
  for (auto &entry : active_devices_) {
    if (now >= entry.second) {
      to_deactivate.push_back(entry.first);
    }
  }

  for (uint16_t id : to_deactivate) {
    deactivate_device_(id);
  }
}

void HTZSafe::process_data_() {
  // Extract header (3 bytes): 0xebaf05
  uint32_t header = (read_buffer_[0] << 16) | (read_buffer_[1] << 8) | read_buffer_[2];

  if (header != HEADER) {
    ESP_LOGW(TAG, "Invalid header: 0x%06x, expected: 0x%06x", header, HEADER);
    return;
  }

  // Extract identifier (2 bytes)
  uint16_t identifier = (read_buffer_[3] << 8) | read_buffer_[4];
  
  activate_device_(identifier);
}

void HTZSafe::activate_device_(uint16_t identifier) {
  auto it = sensors_.find(identifier);
  if (it != sensors_.end()) {
    ESP_LOGI(TAG, "Activated device: %s (ID: 0x%04x)", it->second->get_name().c_str(), identifier);
    it->second->publish_state(true);
    // Schedule deactivation in 5 seconds
    active_devices_[identifier] = millis() + ACTIVATION_DURATION;
  } else {
    ESP_LOGW(TAG, "Unknown device identifier: 0x%04x", identifier);
  }
}

void HTZSafe::deactivate_device_(uint16_t identifier) {
  auto it = sensors_.find(identifier);
  if (it != sensors_.end()) {
    ESP_LOGI(TAG, "Deactivated device: %s (ID: 0x%04x)", it->second->get_name().c_str(), identifier);
    it->second->publish_state(false);
    active_devices_.erase(identifier);
  }
}

void HTZSafe::dump_config() {
  ESP_LOGCONFIG(TAG, "HTZSafe:");
  ESP_LOGCONFIG(TAG, "  Registered Devices: %u", sensors_.size());
  for (const auto &entry : sensors_) {
    ESP_LOGCONFIG(TAG, "    - %s (ID: 0x%04x)", entry.second->get_name().c_str(), entry.first);
  }
}

}  // namespace htzsafe
}  // namespace esphome
