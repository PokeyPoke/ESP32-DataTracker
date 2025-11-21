#include "security.h"

#define CODE_EXPIRATION_MS 300000    // 5 minutes
#define SESSION_EXPIRATION_MS 1800000 // 30 minutes
#define LOCKOUT_DURATION_MS 60000     // 1 minute
#define MAX_FAILED_ATTEMPTS 3

SecurityManager::SecurityManager() {
    currentCode.code = 0;
    currentCode.generatedAt = 0;
    currentCode.used = false;
    currentCode.failedAttempts = 0;
    currentCode.lockoutUntil = 0;

    currentSession.token[0] = '\0';
    currentSession.expiresAt = 0;
    currentSession.active = false;
}

uint32_t SecurityManager::generateNewCode() {
    // Reset previous code state
    currentCode.code = random(100000, 999999);  // 6-digit code
    currentCode.generatedAt = millis();
    currentCode.used = false;
    currentCode.failedAttempts = 0;
    currentCode.lockoutUntil = 0;

    Serial.print("New security code generated: ");
    Serial.println(currentCode.code);

    return currentCode.code;
}

bool SecurityManager::validateCode(uint32_t enteredCode) {
    unsigned long now = millis();

    // Check if locked out
    if (now < currentCode.lockoutUntil) {
        Serial.println("Code validation failed: locked out");
        return false;
    }

    // Check expiration (5 minutes)
    if (now - currentCode.generatedAt > CODE_EXPIRATION_MS) {
        Serial.println("Code validation failed: expired");
        return false;
    }

    // Check if already used
    if (currentCode.used) {
        Serial.println("Code validation failed: already used");
        return false;
    }

    // Validate code
    if (enteredCode == currentCode.code) {
        currentCode.used = true;
        Serial.println("Code validation successful!");
        return true;
    }

    // Failed attempt
    currentCode.failedAttempts++;
    Serial.print("Code validation failed: incorrect code (attempt ");
    Serial.print(currentCode.failedAttempts);
    Serial.println("/3)");

    // Lock after 3 failures
    if (currentCode.failedAttempts >= MAX_FAILED_ATTEMPTS) {
        currentCode.lockoutUntil = now + LOCKOUT_DURATION_MS;
        Serial.println("Too many failed attempts. Locked out for 1 minute.");
    }

    return false;
}

bool SecurityManager::isCodeValid() {
    unsigned long now = millis();

    // Check if locked out
    if (now < currentCode.lockoutUntil) {
        return false;
    }

    // Check expiration
    if (now - currentCode.generatedAt > CODE_EXPIRATION_MS) {
        return false;
    }

    // Check if already used
    if (currentCode.used) {
        return false;
    }

    return true;
}

uint32_t SecurityManager::getCurrentCode() {
    return currentCode.code;
}

unsigned long SecurityManager::getCodeTimeRemaining() {
    if (!isCodeValid()) {
        return 0;
    }

    unsigned long elapsed = millis() - currentCode.generatedAt;
    if (elapsed >= CODE_EXPIRATION_MS) {
        return 0;
    }

    return CODE_EXPIRATION_MS - elapsed;
}

bool SecurityManager::isLockedOut() {
    return millis() < currentCode.lockoutUntil;
}

unsigned long SecurityManager::getLockoutTimeRemaining() {
    if (!isLockedOut()) {
        return 0;
    }

    return currentCode.lockoutUntil - millis();
}

String SecurityManager::createSession() {
    // Generate random 32-character hex token
    char token[33];
    for (int i = 0; i < 32; i++) {
        token[i] = "0123456789abcdef"[random(0, 16)];
    }
    token[32] = '\0';

    // Store session
    strncpy(currentSession.token, token, 33);
    currentSession.expiresAt = millis() + SESSION_EXPIRATION_MS;
    currentSession.active = true;

    Serial.print("Session created: ");
    Serial.println(currentSession.token);

    return String(token);
}

bool SecurityManager::validateSession(const String& token) {
    if (!currentSession.active) {
        Serial.println("Session validation failed: no active session");
        return false;
    }

    if (millis() > currentSession.expiresAt) {
        currentSession.active = false;
        Serial.println("Session validation failed: expired");
        return false;
    }

    if (token != String(currentSession.token)) {
        Serial.println("Session validation failed: invalid token");
        return false;
    }

    return true;
}

void SecurityManager::expireSession() {
    currentSession.active = false;
    currentSession.token[0] = '\0';
    currentSession.expiresAt = 0;
    Serial.println("Session expired");
}

bool SecurityManager::isSessionActive() {
    if (!currentSession.active) {
        return false;
    }

    if (millis() > currentSession.expiresAt) {
        currentSession.active = false;
        return false;
    }

    return true;
}

unsigned long SecurityManager::getSessionTimeRemaining() {
    if (!isSessionActive()) {
        return 0;
    }

    return currentSession.expiresAt - millis();
}

void SecurityManager::resetCode() {
    currentCode.code = 0;
    currentCode.generatedAt = 0;
    currentCode.used = false;
    currentCode.failedAttempts = 0;
    currentCode.lockoutUntil = 0;
    Serial.println("Security code reset");
}
