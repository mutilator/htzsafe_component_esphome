#include "htzsafe.h"
#include "esphome/core/log.h"

namespace esphome {
namespace htzsafe {

static const char *TAG = "htzsafe";

void HTZSafe::setup() {
  ESP_LOGI(TAG, "Setting up HTZSafe component");
  
  // Log all registered devices and initialize state to false
  for (const auto &entry : sensors_) {
    ESP_LOGI(TAG, "Registered device: %s (ID: 0x%04x)", entry.second->get_name().c_str(), entry.first);
    entry.second->publish_state(false);
  }
}

void HTZSafe::register_sensor(HTZSafeBinarySensor *sensor, uint16_t identifier) {
  sensors_[identifier] = sensor;
}

void HTZSafe::loop() {
  // Check if pending activation window has expired
  if (pending_identifier_ != 0 && millis() - pending_activation_time_ > ACTIVATION_WINDOW) {
    ESP_LOGD(TAG, "Activation window expired for device 0x%04x (only %d 0x92 bytes received)", 
             pending_identifier_, consecutive_0x92_count_);
    pending_identifier_ = 0;
    consecutive_0x92_count_ = 0;
    extra_bytes_count_ = 0;
  }
  
  // Process any available data
  while (this->available()) {
    uint8_t byte = this->read();
    
    // If we're waiting for 0x92 bytes after a valid header
    if (pending_identifier_ != 0) {
      // Skip the 4 extra bytes after identifier
      if (extra_bytes_count_ < EXTRA_BYTES_COUNT) {
        extra_bytes_count_++;
        ESP_LOGV(TAG, "Skipping extra byte %d/4: 0x%02x", extra_bytes_count_, byte);
        continue;
      }
      
      // Now count 0x92 bytes
      if (byte == 0x92) {
        consecutive_0x92_count_++;
        if (consecutive_0x92_count_ >= REQUIRED_0x92_COUNT) {
          // Got enough 0x92 bytes, activate the device
          activate_device_(pending_identifier_);
          pending_identifier_ = 0;
          consecutive_0x92_count_ = 0;
          extra_bytes_count_ = 0;
        }
        continue;
      } else {
        // Non-0x92 byte received, reset pending activation
        ESP_LOGD(TAG, "Non-0x92 byte (0x%02x) received, resetting pending activation for 0x%04x", 
                 byte, pending_identifier_);
        pending_identifier_ = 0;
        consecutive_0x92_count_ = 0;
        extra_bytes_count_ = 0;
        // Fall through to process this byte normally
      }
    }
    
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
  
  // Check if this is a known device
  auto it = sensors_.find(identifier);
  if (it != sensors_.end()) {
    // Set up pending activation - wait for 4 extra bytes then 0x92 bytes
    pending_identifier_ = identifier;
    consecutive_0x92_count_ = 0;
    extra_bytes_count_ = 0;
    pending_activation_time_ = millis();
    ESP_LOGD(TAG, "Received valid header for device: %s (ID: 0x%04x), waiting for 4 extra bytes + 0x92 bytes", 
             it->second->get_name().c_str(), identifier);
  } else {
    ESP_LOGW(TAG, "Unknown device identifier: 0x%04x", identifier);
  }
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
