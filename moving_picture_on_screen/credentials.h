#ifndef CREDENTIALS_H
#define CREDENTIALS_H

// Caviscope WiFi credentials
struct WiFiCred {
  const char* SECRET_SSID;
  const char* SECRET_PASS;
};

// List of known WiFi networks
const WiFiCred knownNetworks[] = {
  {"UCI-RADIOTHERAPY", "Rad@2620_!"},
  {"uci bunkerboardroom", "uci@1234"},
  {"Galaxy S22 Ultra CD40", "solomon122"},
  {"IntelliSys Air", "intel_cool@2025!"},
  {"IntelliSys Air_5G", "intel_cool@2025!"},
  {"irrikit_Cloud", "IntelliSys@2025!"},
  {"IntelliSys Online", "qwerty0976_"},
  {"IntelliSys Pro 2025", "intel_cool@2025"},
  {"iPaul", "wrong_pass"}
};

const int knownCount = sizeof(knownNetworks) / sizeof(knownNetworks[0]);

#endif // CREDENTIALS_H
