#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <ESP8266WiFi.h>
#include <SinricPro.h>
#include <SinricProSwitch.h>
#include <SinricProLock.h>

// Include credentials from separate file
#include "credentials.h"

// Pin definitions for ESP8266 D1 Mini
#define RST_PIN D3   // Reset pin for RFID
#define SS_PIN D4    // SDA/SS pin for RFID
#define SERVO_PIN D2 // Servo control pin
#define RELAY_PIN D1 // Relay control pin
#define LED_PIN D0   // Optional status LED

// Create instances
MFRC522 mfrc522(SS_PIN, RST_PIN);
Servo doorServo;

// System state variables
unsigned long servoOpenTime = 0;
bool servoIsOpen = false;
bool bulbState = false;
const unsigned long SERVO_OPEN_DURATION = 10000;

// Servo position constants
const int LOCKED_POSITION = 0;
const int UNLOCKED_POSITION = 180;

// Function prototypes
bool onPowerState(const String &deviceId, bool &state);
bool onLockState(const String &deviceId, bool &state);
void setupWiFi();
void setupSinricPro();
void testServoMovement();
void checkServoTimer();
void unlockDoor();
void lockDoor();
void readCardData();
bool isAuthorizedCard(String uid);
void controlRelay(bool state);

void setup()
{
    Serial.begin(115200);
    while (!Serial)
        ;

    // Initialize pins with explicit modes
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(LED_PIN, OUTPUT);

    // Ensure relay starts in OFF state
    digitalWrite(RELAY_PIN, LOW);
    digitalWrite(LED_PIN, LOW);

    // Add a small delay to ensure pin initialization
    delay(100);

    Serial.println("🔧 Pin initialization complete");
    Serial.printf("🔌 Relay pin D1 (GPIO%d) set to OUTPUT\n", RELAY_PIN);

    SPI.begin();
    mfrc522.PCD_Init();

    Serial.println("🔧 Initializing servo...");
    // CRITICAL FIX: Use extended pulse widths for ESP8266
    doorServo.attach(SERVO_PIN, 500, 2500);
    delay(100);

    // Set initial locked position
    doorServo.write(LOCKED_POSITION);
    delay(2000); // Give servo time to reach position
    servoIsOpen = false;

    setupWiFi();
    setupSinricPro();

    Serial.println("✅ RFID + SinricPro Access Control System - Ready!");
    Serial.println("===================================================");
    Serial.println("🔧 ESP8266 Servo Fix Applied");
    Serial.println("🔒 Door is LOCKED (Servo at TRUE 0°)");
    Serial.println("💡 Bulb is OFF");
    Serial.printf("🔌 Relay on pin D1 (GPIO%d)\n", RELAY_PIN);
    Serial.printf("🔑 Monitoring %d authorized cards\n", NUM_AUTHORIZED_CARDS);
    Serial.println("Place an authorized card or use voice commands...");
    Serial.println();
}

void loop()
{
    SinricPro.handle();
    checkServoTimer();

    if (!mfrc522.PICC_IsNewCardPresent())
        return;
    if (!mfrc522.PICC_ReadCardSerial())
        return;

    Serial.println("🎯 CARD DETECTED!");
    Serial.println("==================");

    String uidString = "";
    for (byte i = 0; i < mfrc522.uid.size; i++)
    {
        if (mfrc522.uid.uidByte[i] < 0x10)
            uidString += "0";
        uidString += String(mfrc522.uid.uidByte[i], HEX);
    }
    uidString.toUpperCase();
    Serial.print("UID String: ");
    Serial.println(uidString);

    MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
    Serial.print("Card Type: ");
    Serial.println(mfrc522.PICC_GetTypeName(piccType));

    if (isAuthorizedCard(uidString))
    {
        Serial.println("✅ ACCESS GRANTED!");
        unlockDoor();
        SinricProLock &myLock = SinricPro[LOCK_ID];
        myLock.sendLockStateEvent(false); // false = unlocked
    }
    else
    {
        Serial.println("❌ ACCESS DENIED!");
    }

    if (piccType == MFRC522::PICC_TYPE_MIFARE_1K ||
        piccType == MFRC522::PICC_TYPE_MIFARE_4K ||
        piccType == MFRC522::PICC_TYPE_MIFARE_MINI)
    {
        readCardData();
    }

    Serial.println("==================");
    Serial.println();

    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    delay(1500);
}

void setupWiFi()
{
    Serial.printf("\n[WiFi]: Connecting to %s", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.printf(".");
        delay(250);
    }
    Serial.printf("connected!\n[WiFi]: IP-Address is %s\n", WiFi.localIP().toString().c_str());
}

void setupSinricPro()
{
    SinricProSwitch &mySwitch = SinricPro[SWITCH_ID];
    mySwitch.onPowerState(onPowerState);

    SinricProLock &myLock = SinricPro[LOCK_ID];
    myLock.onLockState(onLockState);

    SinricPro.onConnected([]()
                          { Serial.printf("Connected to SinricPro\n"); });
    SinricPro.onDisconnected([]()
                             { Serial.printf("Disconnected from SinricPro\n"); });

    SinricPro.begin(APP_KEY, APP_SECRET);
}

bool onPowerState(const String &deviceId, bool &state)
{
    Serial.printf("💡 Bulb turned %s (via SinricPro)\r\n", state ? "ON" : "OFF");
    Serial.printf("🔌 Setting relay pin D1 (GPIO%d) to %s\r\n", RELAY_PIN, state ? "HIGH" : "LOW");

    bulbState = state;
    controlRelay(state);

    Serial.printf("🔍 Relay pin actual state: %d\r\n", digitalRead(RELAY_PIN));

    return true;
}

bool onLockState(const String &deviceId, bool &state)
{
    Serial.printf("🔒 Door %s (via SinricPro)\r\n", state ? "LOCKED" : "UNLOCKED");
    if (state)
    {
        lockDoor();
    }
    else
    {
        unlockDoor();
    }
    return true;
}

void controlRelay(bool state)
{
    digitalWrite(RELAY_PIN, state ? HIGH : LOW);
    digitalWrite(LED_PIN, state ? HIGH : LOW);

    // Force a small delay to ensure the signal is stable
    delayMicroseconds(10);

    Serial.printf("🔧 Relay control executed - Pin: %d, State: %s\n",
                  RELAY_PIN, state ? "HIGH" : "LOW");
}

void checkServoTimer()
{
    if (servoIsOpen && (millis() - servoOpenTime >= SERVO_OPEN_DURATION))
    {
        lockDoor();
        // Send lock state update to SinricPro
        SinricProLock &myLock = SinricPro[LOCK_ID];
        myLock.sendLockStateEvent(true); // true = locked
    }
}

void unlockDoor()
{
    if (!servoIsOpen)
    {
        Serial.println("🔓 UNLOCKING DOOR...");
        doorServo.write(UNLOCKED_POSITION);
        delay(2000); // Give servo time to move
        servoOpenTime = millis();
        servoIsOpen = true;
        Serial.println("🔓 Door is UNLOCKED (Servo at TRUE 180°)");
        Serial.println("⏰ Door will auto-lock in 10 seconds");
    }
    else
    {
        servoOpenTime = millis();
        Serial.println("⏰ Door timer RESET - 10 seconds remaining");
    }
}

void lockDoor()
{
    Serial.println("🔒 LOCKING DOOR...");
    doorServo.write(LOCKED_POSITION); // Return to TRUE 0°
    delay(1000);                      // Give servo time to move
    servoIsOpen = false;
    Serial.printf("🔒 Door is LOCKED (Servo at TRUE %d°)\n", LOCKED_POSITION);
}

void readCardData()
{
    MFRC522::MIFARE_Key key;
    for (byte i = 0; i < 6; i++)
        key.keyByte[i] = 0xFF;

    byte blockAddr = 1;
    byte buffer[18];
    byte size = sizeof(buffer);

    MFRC522::StatusCode status = (MFRC522::StatusCode)mfrc522.PCD_Authenticate(
        MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockAddr, &key, &(mfrc522.uid));

    if (status != MFRC522::STATUS_OK)
    {
        Serial.print("Auth failed: ");
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
    }

    status = (MFRC522::StatusCode)mfrc522.MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK)
    {
        Serial.print("Read failed: ");
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
    }

    Serial.print("Block 1 Data: ");
    for (byte i = 0; i < 16; i++)
    {
        if (buffer[i] < 0x10)
            Serial.print("0");
        Serial.print(buffer[i], HEX);
        Serial.print(" ");
    }
    Serial.println();

    Serial.print("As text: ");
    for (byte i = 0; i < 16; i++)
    {
        if (buffer[i] >= 32 && buffer[i] <= 126)
        {
            Serial.print((char)buffer[i]);
        }
        else
        {
            Serial.print(".");
        }
    }
    Serial.println();
}

bool isAuthorizedCard(String uid)
{
    for (int i = 0; i < NUM_AUTHORIZED_CARDS; i++)
    {
        if (uid.equals(AUTHORIZED_CARDS[i]))
            return true;
    }
    return false;
}