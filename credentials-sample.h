// credentials.h - Keep this file out of version control
#ifndef CREDENTIALS_H
#define CREDENTIALS_H

// WiFi credentials
#define WIFI_SSID "Your_WiFi_Name"
#define WIFI_PASS "Your_WiFi_Password"

// SinricPro credentials
#define APP_KEY "your-app-key-here"
#define APP_SECRET "your-app-secret-here"
#define LOCK_ID "your-lock-id-here"
#define SWITCH_ID "your-switch-id-here"

// Authorized RFID card UIDs
const String AUTHORIZED_CARDS[] = {
    "5474E900",
    "3ED70805",
    "93936A14"
    // Add more cards as needed
};

const int NUM_AUTHORIZED_CARDS = sizeof(AUTHORIZED_CARDS) / sizeof(AUTHORIZED_CARDS[0]);

#endif