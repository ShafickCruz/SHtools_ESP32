#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <SHtools_ESP32.h>

// Defina os pinos do LED, do botão e o nome do sketch
const int ledPin = 23;
const int buttonPin = 27;
String nomeSketch = "Projeto teste";

// Crie uma instância da biblioteca
SHtools_ESP32 SHtools(ledPin, buttonPin, nomeSketch);

void setup()
{
    // Inicialize a biblioteca
    SHtools.begin();
}

void loop()
{
    // Mantém a biblioteca processando eventos e checando o botão
    SHtools.handle();
}
