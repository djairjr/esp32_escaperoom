// MotionController.h - Motor and actuator control via PCA9685
// Manages: servos (platforms, lid), linear actuators, relays
//
// Hardware: PCA9685 (I2C 0x40) + 5V relay modules
//
// Channel mapping:
//   CH0: Platform A servo (extend/retract)
//   CH1: Platform B servo
//   CH2: Platform C servo
//   CH3: Lid opening servo
//   CH4-CH7: Relays for linear actuators (direction)

#pragma once

#include <ESP32EscapeRoom.h>

using namespace esp32_escaperoom;

class MotionController {
public:
    MotionController() {}

    void begin() {
        if (!hw.hasPwmDriver()) {
            Serial.println("[MOTION] PCA9685 not detected!");
            return;
        }

        // PCA9685 already initialized by Hardware::begin()
        // Set the frequency for servos (50Hz)
        hw.pwmDriver().setToServoFrequency();

        Serial.println("[MOTION] PCA9685 OK");
    }

    // --- Servos ---
    // Pulse: 1000us = 0 graus, 2000us = 180 graus
    void setServoPulse(uint8_t channel, uint16_t pulseUs) {
        if (!hw.hasPwmDriver()) return;
        hw.pwmDriver().setChannelServoPulseDuration(channel, pulseUs);
    }

    void setServoAngle(uint8_t channel, uint8_t angle) {
        uint16_t pulse = map(angle, 0, 180, 1000, 2000);
        setServoPulse(channel, pulse);
    }

    void extendPlatform(uint8_t servoChannel) {
        Serial.printf("[MOTION] Platform CH%d EXTEND\n", servoChannel);
        setServoAngle(servoChannel, 180);
    }

    void retractPlatform(uint8_t servoChannel) {
        Serial.printf("[MOTION] Platform CH%d RETRACT\n", servoChannel);
        setServoAngle(servoChannel, 0);
    }

    void openLid(uint8_t servoChannel = 3) {
        Serial.println("[MOTION] Lid OPEN");
        for (uint8_t a = 0; a <= 180; a += 2) {
            setServoAngle(servoChannel, a);
            delay(100);
        }
    }

    // --- Linear Actuators (PWM + Relays) ---
    void linearExtend(uint8_t pwmChannel, uint8_t relayFwd, uint8_t relayBwd, uint8_t speed = 100) {
        if (!hw.hasPwmDriver()) return;
        hw.digitalWrite(relayBwd, HIGH);
        hw.digitalWrite(relayFwd, LOW);
        hw.pwmDriver().setChannelDutyCycle(pwmChannel, speed);
    }

    void linearRetract(uint8_t pwmChannel, uint8_t relayFwd, uint8_t relayBwd, uint8_t speed = 100) {
        if (!hw.hasPwmDriver()) return;
        hw.digitalWrite(relayFwd, HIGH);
        hw.digitalWrite(relayBwd, LOW);
        hw.pwmDriver().setChannelDutyCycle(pwmChannel, speed);
    }

    void linearStop(uint8_t pwmChannel, uint8_t relayFwd, uint8_t relayBwd) {
        if (!hw.hasPwmDriver()) return;
        hw.digitalWrite(relayFwd, HIGH);
        hw.digitalWrite(relayBwd, HIGH);
        hw.pwmDriver().setChannelDutyCycle(pwmChannel, 0);
    }

    // --- Shortcuts ---
    void extendAllPlatforms(uint8_t chA = 0, uint8_t chB = 1, uint8_t chC = 2) {
        extendPlatform(chA);
        extendPlatform(chB);
        extendPlatform(chC);
    }
};
