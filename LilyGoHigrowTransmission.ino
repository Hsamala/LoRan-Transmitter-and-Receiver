#include <SPI.h>
#include <LoRa.h>
#include <DHT.h> // Ensure you have the DHT library installed

// --- LORA MODULE PINS (Final, Safest Configuration) ---
#define LORA_SCK   14
#define LORA_MISO  33
#define LORA_MOSI  23
#define LORA_NSS   5

#define LORA_RST   13
#define LORA_DIO0  -1 

// --- SENSOR PINS (Wired internally on the T-Higrow) ---
// We use GPIO34 (Fertility/EC) for the soil sensor reading as it is Input-Only.
#define MOISTURE_PIN 34  
#define DHT_PIN      16  
#define DHT_TYPE     DHT11 // Assuming DHT11 is the sensor on the T-Higrow

// LoRa Settings
const long frequency = 433E6; 
#define TRANSMIT_INTERVAL 30000 // Transmit every 30 seconds

DHT dht(DHT_PIN, DHT_TYPE);
int counter = 0;

// =========================================================================

void setup() {
  
  delay(2000); 
  Serial.begin(9600);
  while(!Serial); 
  
  Serial.println("\n--- T-Higrow LoRa Transmitter Ready (Data Mode) ---");

  // Initialize Sensors
  dht.begin();
  pinMode(MOISTURE_PIN, INPUT); 

  // Initialize the SPI bus
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS); 
  LoRa.setPins(LORA_NSS, LORA_RST, LORA_DIO0); 
  
  // Initialize LoRa radio
  if (!LoRa.begin(frequency)) {
    Serial.println("❌ LoRa Initialization FAILED!");
    while (true); 
  }

  // Set Tx Power for robust transmission
  LoRa.setTxPower(14); 
  Serial.println("✅ LoRa Initialized. Tx Power set to 14 dBm.");
}

// =========================================================================

void loop() {
  static unsigned long lastTransmitTime = 0;
  
  if (millis() - lastTransmitTime > TRANSMIT_INTERVAL) {
    
    // --- 1. READ SENSOR DATA ---
    
    // Read Temperature & Humidity (DHT)
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();
    
    // Read Soil Moisture/EC (Analog) - Raw ADC Value
    int rawMoisture = analogRead(MOISTURE_PIN);
    
    // --- 2. VALIDATE DATA ---
    if (isnan(humidity) || isnan(temperature)) {
      Serial.println("❌ Failed to read DHT sensor! Skipping packet.");
      lastTransmitTime = millis();
      return; 
    }

    // --- 3. PACKAGE AND TRANSMIT DATA ---
    // Packet Format: ID:1|T:24.5|H:60.0|M:650
    String message = "ID:" + String(counter) + 
                     "|T:" + String(temperature, 1) +  // Temp to 1 decimal place
                     "|H:" + String(humidity, 1) +     // Humidity to 1 decimal place
                     "|M:" + String(rawMoisture);      // Raw Moisture ADC value
    
    Serial.print("Sending Packet ");
    Serial.print(counter);
    Serial.print(" (");
    Serial.print(message.length());
    Serial.println(" bytes):");
    Serial.println(message);

    // Begin packet transmission
    LoRa.beginPacket();
    LoRa.print(message);
    
    // Use blocking send for maximum confirmation (true)
    bool txSuccess = LoRa.endPacket(true); 

    if (txSuccess) {
        Serial.println("--> ✅ TX Confirmed by Module.");
    } else {
        Serial.println("--> ❌ TX Error or Timeout.");
    }
    
    // --- 4. FINALIZE ---
    lastTransmitTime = millis();
    counter++;
  }
}
