#ifndef SHtools_ESP32_H
#define SHtools_ESP32_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <AsyncWebSocket.h>
#include <Update.h>
#include <esp_ota_ops.h>
#include <Preferences.h>
#include <FS.h>
#include <LittleFS.h>
#include <esp_system.h>
#include <esp_chip_info.h>
#include <esp_spi_flash.h>
#include <soc/rtc.h>
#include <mbedtls/sha256.h>

class SHtools_ESP32
{
public:
    SHtools_ESP32(int ledPin, int buttonPin, String nomeSketch);
    void begin();  // like setup
    void handle(); // like loop

    bool get_ServerMode_ON() const;                                       // Método getter para ServerMode_ON
    void printMSG(const String &_msg, bool newline, bool _debug = false); // Serial Monitor personalizado

    // Getters para server e websocket
    AsyncWebServer &get_server();
    AsyncWebSocket &get_ws();

private:
    bool ServerMode_ON;
    bool restartSolicitado;
    unsigned long ServerModeInicio;
    unsigned long buttonPressTime;
    unsigned long lastButtonStateChangeTime;
    unsigned long longPressDuration;
    unsigned long debounceDelay;
    int lastButtonState;
    const char *clientPassword;
    int ledPin;
    int buttonPin;
    String nomeSketch;
    bool DebugInicial;
    AsyncWebServer server; // Declaração do servidor como membro da classe
    AsyncWebSocket ws;     // Declaração do WebSocket como membro da classe
    Preferences config;    // instancia para configrações usando preferences

    String obterInformacoesPlaca();
    void bt_handle();
    void startServerMode(bool _softRestart); // Tenta iniciar o servidor AP
    bool ServerMode();
    void ServerMode_handle();
    bool SerialCMD(String _cmd);
    bool WifiSetup();
    void rotasEcallbacks();
    String generateSSID(); // Gera SSID
    void ReiniciarESP(int _tempoDelay = 1000);
    void OTA_FirmwareUpdate(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final);
    String OTA_FirmwareUpdate_ChecksumFinal(mbedtls_sha256_context *ctx);
    bool preferencias(int8_t _opcao, const char *_chave = "", bool _valor = false);
};

#endif
