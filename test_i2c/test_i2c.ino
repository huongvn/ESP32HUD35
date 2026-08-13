#include <Wire.h>

// Các cấu hình SDA/SCL cần test
struct TestConfig {
  const char* name;
  int sda;
  int scl;
  uint8_t pullup; // 0 = vô hiệu, 1 = kích hoạt internal pullup
};

TestConfig configs[] = {
  {"Default (33,32)", 33, 32, 0},      // Cấu hình mặc định từ board definition
  {"Swapped (32,33)", 32, 33, 0},       // Đảo SDA/SCL
  {"With pullup (33,32)", 33, 32, 1},  // Kích hoạt internal pullup
  {"Default low speed", 33, 32, 0},     // Tốc độ I2C 100kHz (mặc định)
};

void setup() {
  Serial.begin(115200);
  delay(2000); // Chờ màn hình khởi tạo

  Serial.println("=== Test I2C GT911 ===");

  for (auto& cfg : configs) {
    Serial.printf("\n--- Test: %s ---\n", cfg.name);

    // Cấu hình Wire
    Wire.begin(cfg.sda, cfg.scl);

    // Kích/pullup nếu cần
    if (cfg.pullup) {
      pinMode(cfg.sda, INPUT_PULLUP);
      pinMode(cfg.scl, INPUT_PULLUP);
    }

    // Quét I2C bus
    byte error, address;
    int nDevices = 0;

    Serial.println("Scanning I2C bus...");
    for (address = 1; address < 127; address++) {
      Wire.beginTransmission(address);
      error = Wire.endTransmission();

      if (error == 0) {
        Serial.printf("  ✓ Found address: 0x%02X (%d)\n", address, address);
        nDevices++;

        // Thử đọc GT911 product ID (địa chỉ thường là 0xD hoặc 0x8)
        Wire.beginTransmission(address);
        Wire.write(0xA1); // Register product ID for GT911
        error = Wire.endTransmission();

        if (error == 0) {
          // Đọc product ID
          Wire.requestFrom(address, 1);
          byte productId = Wire.read();
          Serial.printf("  → GT911 Product ID: 0x%02X (%d)\n", productId, productId);
        } else {
          Serial.println("  → Không thể đọc product ID (NACK)");
        }
      }
    }

    if (nDevices == 0) {
      Serial.println("  ⚠ Không tìm thấy thiết bị I2C nào");
    } else if (nDevices == 1) {
      Serial.printf("  ✅ 1 thiết bị I2C tìm thấy\n");
    } else {
      Serial.printf("  ⚠%d thiết bị I2C tìm thấy\n", nDevices);
    }
  }

  Serial.println("\n=== Kết thúc test ===");
  for(;;); // Vòng lặp vô hạn
}

void loop() {}