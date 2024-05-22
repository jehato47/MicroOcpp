#include <Arduino.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
ESP8266WiFiMulti WiFiMulti;
#elif defined(ESP32)
#include <WiFi.h>
#else
#error only ESP32 or ESP8266 supported at the moment
#endif

#include <MicroOcpp.h>

#define STASSID  "TurkTelekom_ZTNTX3_2.4GHz"
#define STAPSK   "d4fzKRT3syEh"

#define OCPP_BACKEND_URL   "ws://192.168.1.60:8081/IonBee/6dd3911b691b4ef79305f473ecef8f81"
#define OCPP_CHARGE_BOX_ID "SN10052307112654"

// Initialize a variable to store the energy meter value
float energyMeterValue = 0.0;

void setup() {
    Serial.begin(115200);
    Serial.print(F("[main] Wait for WiFi: "));

#if defined(ESP8266)
    WiFiMulti.addAP(STASSID, STAPSK);
    while (WiFiMulti.run() != WL_CONNECTED) {
        Serial.print('.');
        delay(1000);
    }
#elif defined(ESP32)
    WiFi.begin(STASSID, STAPSK);
    while (!WiFi.isConnected()) {
        Serial.print('.');
        delay(1000);
    }
#else
#error only ESP32 or ESP8266 supported at the moment
#endif

    Serial.println(F(" connected!"));

    mocpp_initialize(OCPP_BACKEND_URL, OCPP_CHARGE_BOX_ID, "My Charging Station", "My company name");

    setEnergyMeterInput([]() {
        return energyMeterValue;
    });

    setSmartChargingCurrentOutput([](float limit) {
        Serial.printf("[main] Smart Charging allows maximum charge rate: %.0f\n", limit);
    });

    setConnectorPluggedInput([]() {
        return true;
    });

    // Add additional meter value input
    addMeterValueInput([]() {
        // Return the current energy meter value
        return energyMeterValue;
    }, "Energy.Active.Import.Register", "Wh", "Outlet", "L1", 1);
}

void loop() {
    mocpp_loop();

    if (ocppPermitsCharge()) {
        // OCPP set up and transaction running. Energize the EV plug here
    } else {
        // No transaction running at the moment. De-energize EV plug
    }

    // Update meter value periodically
    static unsigned long lastMeterUpdate = 0;
    unsigned long currentMillis = millis();
    if (currentMillis - lastMeterUpdate >= 1000) { // Update every second
        lastMeterUpdate = currentMillis;
        energyMeterValue += 1.0; // Increment the energy meter value (replace with actual logic)
    }

    if (/* RFID chip detected? */ false) {
        String idTag = "0123456789ABCD";

        if (!getTransaction()) {
            Serial.printf("[main] Begin Transaction with idTag %s\n", idTag.c_str());
            auto ret = beginTransaction(idTag.c_str());

            if (ret) {
                Serial.println(F("[main] Transaction initiated. OCPP lib will send a StartTransaction when ConnectorPlugged Input becomes true and if the Authorization succeeds"));
            } else {
                Serial.println(F("[main] No transaction initiated"));
            }
        } else {
            if (idTag.equals(getTransactionIdTag())) {
                Serial.println(F("[main] End transaction by RFID card"));
                endTransaction(idTag.c_str());
            } else {
                Serial.println(F("[main] Cannot end transaction by RFID card (different card?)"));
            }
        }
    }
}
