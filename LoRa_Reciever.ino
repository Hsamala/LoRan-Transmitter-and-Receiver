#include <SPI.h>
#include <LoRa.h>

/* 
  Remember, this code uses the ZY-ESP32 board

  NOT NODEMCU!!!!!!

*/

#define LORA_SCK   14
#define LORA_MISO  33   // <--- CHANGE THIS: Safest available GPIO
#define LORA_MOSI  23
#define LORA_NSS   5

// --- LORA CONTROL PINS ---
#define LORA_RST   13
#define LORA_DIO0  32
 
const long frequency = 433E6; 

// Flag and Buffer for Interrupt Handling
volatile bool packetReceived = false;
volatile int receivedPacketSize = 0;
// Buffer to hold the data string: ID:X|T:X.X|H:X.X|M:X
char dataBuffer[100]; 

// =========================================================================
// FUNCTION PROTOTYPES
// =========================================================================
void onReceive(int packetSize);

// =========================================================================
// SETUP
// =========================================================================

void setup() {
  
  Serial.begin(9600); 
  Serial.println("\n--- LoRa Receiver Ready (433 MHz) ---");
  
  // 1. Initialize the SPI bus
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS); 
  
  // 2. Set LoRa control pins and initialize radio
  LoRa.setPins(LORA_NSS, LORA_RST, LORA_DIO0); 
  if (!LoRa.begin(frequency)) {
    Serial.println("❌ LoRa Initialization FAILED!");
    while (true); 
  }

  // 3. Attach the interrupt and set RX mode
  LoRa.onReceive(onReceive);
  LoRa.receive(); 
  Serial.println("✅ Module is in RECEIVE mode. Waiting for packets...");
}

// =========================================================================
// LOOP (Main Processing Thread)
// =========================================================================

void loop() {
  // Check the flag set by the interrupt
  if (packetReceived) {
    
    // --- STEP 1: Process Metadata and Content ---
    Serial.println("\n----------------------------------------");
    Serial.print("Received [");
    Serial.print(receivedPacketSize);
    Serial.print(" bytes] RSSI: ");
    Serial.print(LoRa.packetRssi());
    Serial.print(" dBm | SNR: ");
    Serial.println(LoRa.packetSnr());
    
    // Convert the received char array buffer to a String for easy parsing
    String message = String(dataBuffer);
    Serial.print("Raw Content: ");
    Serial.println(message);

    // --- STEP 2: PARSE THE SENSOR DATA ---
    // Format is: ID:X|T:X.X|H:X.X|M:X

    int tIndex = message.indexOf("T:") + 2; // Find "T:" and advance 2 chars
    int hIndex = message.indexOf("H:") + 2;
    int mIndex = message.indexOf("M:") + 2;
    int nextPipe = message.indexOf('|', tIndex - 2);

    // Extract Temperature (from T: to next |)
    String tempStr = message.substring(tIndex, nextPipe);
    
    // Extract Humidity (from H: to next |)
    nextPipe = message.indexOf('|', hIndex - 2);
    String humiStr = message.substring(hIndex, nextPipe);
    
    // Extract Moisture (from M: to end of string)
    String moistureStr = message.substring(mIndex);

    // --- STEP 3: DISPLAY RESULTS ---
    Serial.println("--- PARSED SENSOR DATA ---");
    Serial.print("Temperature (T): ");
    Serial.println(tempStr);
    Serial.print("Humidity (H): ");
    Serial.println(humiStr);
    Serial.print("Moisture (M): ");
    Serial.println(moistureStr);
    Serial.println("----------------------------------------");

    // --- STEP 4: CLEAN UP AND RESUME LISTENING ---
    packetReceived = false; 
    receivedPacketSize = 0;
    memset(dataBuffer, 0, sizeof(dataBuffer)); // Clear buffer
    
    LoRa.receive(); // Resume listening for the next packet
  }
}

// =========================================================================
// INTERRUPT SERVICE ROUTINE (ISR) - Executes instantly when a packet arrives
// =========================================================================

void onReceive(int packetSize) {
  if (packetSize == 0) return;

  // 1. Read packet data into the volatile buffer
  int i = 0;
  while (LoRa.available() && i < sizeof(dataBuffer) - 1) {
    dataBuffer[i++] = (char)LoRa.read();
  }
  dataBuffer[i] = '\0'; // Null-terminate the string

  // 2. Set the flags to signal the main loop
  receivedPacketSize = packetSize;
  packetReceived = true; 
}
