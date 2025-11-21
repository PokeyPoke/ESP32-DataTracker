#ifndef SECURITY_H
#define SECURITY_H

#include <Arduino.h>

// Security code state
struct SecurityCode {
    uint32_t code;               // 6-digit number (100000-999999)
    unsigned long generatedAt;   // millis() when created
    bool used;                   // true after successful auth
    uint8_t failedAttempts;      // count of failed attempts
    unsigned long lockoutUntil;  // millis() when lockout ends
};

// Session token state
struct Session {
    char token[33];              // 32-char hex string + null terminator
    unsigned long expiresAt;     // millis() when expires (30 min)
    bool active;                 // true if session is valid
};

class SecurityManager {
private:
    SecurityCode currentCode;
    Session currentSession;

public:
    SecurityManager();

    // Security code management
    uint32_t generateNewCode();
    bool validateCode(uint32_t enteredCode);
    bool isCodeValid();
    uint32_t getCurrentCode();
    unsigned long getCodeTimeRemaining();  // milliseconds until expiration
    bool isLockedOut();
    unsigned long getLockoutTimeRemaining();  // milliseconds until unlock

    // Session management
    String createSession();
    bool validateSession(const String& token);
    void expireSession();
    bool isSessionActive();
    unsigned long getSessionTimeRemaining();  // milliseconds until expiration

    // Reset on module change
    void resetCode();
};

#endif // SECURITY_H
