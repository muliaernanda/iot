#include <ESP32Servo.h>

Servo servos[12];
int servoPins[] = {13, 12, 14, 27, 26, 25, 33, 32, 4, 16, 17, 18};

void setup() {
  Serial.begin(115200);
  Serial.println("Memulai Tarian 12 Servo...");

  for (int i = 0; i < 12; i++) {
    servos[i].attach(servoPins[i], 500, 2400);
  }
}

void loop() {
  // Gerakan 1: Gelombang Beruntun (Wave) dari servo 0 sampai 11
  Serial.println("Tarian: Gerakan Gelombang (Wave)");
  for (int i = 0; i < 12; i++) {
    servos[i].write(150);
    delay(80);
    servos[i].write(30);
  }

  // Gerakan 2: Ganjil dan Genap Bergantian (Keluarga Bergerak)
  Serial.println("Tarian: Ganjil Genap Bergantian");
  for (int t = 0; t < 3; t++) {
    for (int i = 0; i < 12; i++) {
      if (i % 2 == 0) {
        servos[i].write(0);
      } else {
        servos[i].write(180);
      }
    }
    delay(300);
    
    for (int i = 0; i < 12; i++) {
      if (i % 2 == 0) {
        servos[i].write(180);
      } else {
        servos[i].write(0);
      }
    }
    delay(300);
  }

  // Gerakan 3: Semua Servo Berputar Bersamaan ke Kiri dan Kanan (Kompak)
  Serial.println("Tarian: Kompak Berputar");
  for (int sudut = 0; sudut <= 180; sudut += 30) {
    for (int i = 0; i < 12; i++) {
      servos[i].write(sudut);
    }
    delay(100);
  }
  
  for (int sudut = 180; sudut >= 0; sudut -= 30) {
    for (int i = 0; i < 12; i++) {
      servos[i].write(sudut);
    }
    delay(100);
  }

  // Jeda sejenak sebelum mengulang tarian dari awal
  delay(500);
}
