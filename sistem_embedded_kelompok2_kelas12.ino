#include <Wire.h> // Library untuk komunikasi I2C
#include <LiquidCrystal_I2C.h> // Library untuk kontrol LCD I2C
#include <Servo.h> // Library untuk kontrol motor servo

// Inisialisasi LCD dengan alamat 0x27, ukuran 16x2 karakter
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Inisialisasi Servo
Servo servo;

// Definisi pin
#define UltrasonicTrig 2  // Pin trig untuk sensor ultrasonik
#define UltrasonicEcho 3  // Pin echo untuk sensor ultrasonik
#define Pump1Relay 4      // Pin relay untuk pompa 1
#define Pump2Relay 5      // Pin relay untuk pompa 2
#define WaterLevelPin 6   // Pin untuk sensor level air
#define pHSensorPin A0    // Pin analog untuk sensor pH

// Variabel untuk penghitungan jarak
long duration; // Durasi pulsa dari sensor ultrasonik
float distance; // Jarak yang dihitung berdasarkan durasi pulsa

// Kalibrasi sensor pH
float calibration_value = 21.34 + 0.7; // Nilai kalibrasi untuk sensor pH

// Fungsi setup
void setup() {
  // Konfigurasi pin sensor ultrasonik
  pinMode(UltrasonicTrig, OUTPUT);
  pinMode(UltrasonicEcho, INPUT);
  
  // Konfigurasi pin relay untuk pompa
  pinMode(Pump1Relay, OUTPUT);
  pinMode(Pump2Relay, OUTPUT);
  
  // Konfigurasi pin sensor level air
  pinMode(WaterLevelPin, INPUT_PULLUP);
  
  // Konfigurasi pin analog untuk sensor pH
  pinMode(pHSensorPin, INPUT);

  // Matikan pompa di awal
  digitalWrite(Pump1Relay, HIGH); // Pompa 1 mati
  digitalWrite(Pump2Relay, HIGH); // Pompa 2 mati

  // Inisialisasi servo dan set posisi awal
  servo.attach(9); // Hubungkan servo ke pin 9
  servo.write(0);  // Posisi awal servo di 0 derajat

  // Inisialisasi LCD
  lcd.init();        // Inisialisasi modul LCD
  lcd.backlight();   // Aktifkan lampu latar LCD
  lcd.clear();       // Bersihkan layar LCD
  lcd.setCursor(0, 0);
  lcd.print("System Starting"); // Pesan awal sistem
  delay(2000);       // Tunggu 2 detik
  lcd.clear();       // Bersihkan layar
}

// Fungsi untuk membaca jarak dari sensor ultrasonik
float readDistance() {
  // Kirim sinyal trigger
  digitalWrite(UltrasonicTrig, LOW);
  delayMicroseconds(2);
  digitalWrite(UltrasonicTrig, HIGH);
  delayMicroseconds(10);
  digitalWrite(UltrasonicTrig, LOW);

  // Hitung durasi pulsa dari echo
  duration = pulseIn(UltrasonicEcho, HIGH);
  
  // Hitung jarak dalam cm
  return (duration * 0.034 / 2);
}

// Fungsi untuk membaca nilai pH dari sensor
float readPH() {
  int buf[10], temp; // Buffer untuk data pH
  unsigned long avgValue = 0; // Nilai rata-rata pH

  // Ambil 10 sampel dari sensor pH
  for (int i = 0; i < 10; i++) {
    buf[i] = analogRead(pHSensorPin); // Baca nilai analog
    delay(10); // Tunggu 10ms
  }

  // Urutkan data sampel untuk keakuratan
  for (int i = 0; i < 9; i++) {
    for (int j = i + 1; j < 10; j++) {
      if (buf[i] > buf[j]) {
        temp = buf[i];
        buf[i] = buf[j];
        buf[j] = temp;
      }
    }
  }

  // Hitung rata-rata dari nilai tengah sampel
  for (int i = 2; i < 8; i++) avgValue += buf[i];

  // Konversi nilai analog menjadi pH
  float voltage = (float)avgValue * 5.0 / 1024 / 6;
  return -4.60 * voltage + calibration_value; // Kalibrasi nilai pH
}

// Fungsi utama loop
void loop() {
  // Cek sensor level air
  if (digitalRead(WaterLevelPin) == LOW) { // Jika air terdeteksi
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("System OFF"); // Tampilkan pesan sistem mati
    digitalWrite(Pump1Relay, HIGH); // Matikan pompa 1
    digitalWrite(Pump2Relay, HIGH); // Matikan pompa 2
    servo.detach(); // Matikan servo
    while (1); // Hentikan program
  }

  // Membaca jarak dari sensor ultrasonik
  distance = readDistance();

  // Jika jarak ≥ 5 cm, hidupkan pompa 1 selama 30 detik
  if (distance >= 5) {
    lcd.setCursor(0, 0);
    lcd.print("Pompa 1: ON    ");
    digitalWrite(Pump1Relay, LOW); // Hidupkan pompa 1
    delay(30000); // Pompa 1 hidup selama 30 detik
    digitalWrite(Pump1Relay, HIGH); // Matikan pompa 1
    lcd.setCursor(0, 0);
    lcd.print("Pompa 1: OFF   ");
  }

  // Membaca nilai pH dari sensor
  float phValue = readPH();
  lcd.setCursor(0, 1);
  lcd.print("pH: ");
  lcd.print(phValue, 2); // Tampilkan nilai pH di LCD

  // Analisis nilai pH
  if (phValue >= 6 && phValue <= 8) { // Jika pH aman
    lcd.setCursor(0, 1);
    lcd.print("Air Aman        ");
    digitalWrite(Pump2Relay, LOW); // Hidupkan pompa 2
    servo.write(0); // Servo tetap di posisi awal
  } else { // Jika pH berbahaya
    lcd.setCursor(0, 1);
    lcd.print("Air Berbahaya   ");
    digitalWrite(Pump2Relay, HIGH); // Matikan pompa 2
    servo.write(90); // Servo bergerak ke posisi 90 derajat
  }

  delay(500); // Waktu tunggu sebelum iterasi berikutnya
}
