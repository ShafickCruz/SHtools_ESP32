# SHtools_ESP32

A biblioteca `SHtools_ESP32` permite que o ESP32 exiba remotamente as informações do serial monitor e receba atualizações de firmware via Wi-Fi em modo AP. Com ela, você pode atualizar o firmware do seu dispositivo de forma remota, sem necessidade de uma rede Wi-Fi intermediária e internet, usando um botão físico para iniciar o processo de inicio do modo servidor.

**Prática**: Ao pressionar o botão de atualização por 3 segundos, um LED começa a piscar e um servidor web assíncrono em modo access point é criado. Você deve acessar a rede Wi-Fi criada e, no navegador, inserir o IP do ESP32 que está no SSID. Em seguida, selecionar uma das opções que deseja utilizar, entre autualizações OTA, serial monitor remoto, etc.

## Recursos

- **OTA via Wi-Fi**: Faça atualizações de firmware através de uma rede Wi-Fi e da tecnologia OTA.
- **Modo Async**: Utilize esses recursos de forma assíncrona através de um `AsyncWebServer` criado pela biblioteca `ESPAsyncWebServer`.
- **Modo AP**: Elimine a necessidade de uma rede Wi-Fi intermediária através do Access Point criado pelo ESP32.
- **Serial Monitor**: Visualize informações do serial monitor de forma remota.
- **Envio de comandos**: Utilize o serial monitor remoto para enviar comandos previamente definidos ao ESP32.

## Instalação

### Usando o Gerenciador de Bibliotecas do Arduino

1. Abra o Arduino IDE.
2. Vá para **Sketch** > **Include Library** > **Manage Libraries...**.
3. Na barra de pesquisa, digite `SHtools_ESP32`.
4. Selecione a biblioteca e clique em **Install**.

### Usando PlatformIO

Adicione a seguinte linha ao seu `platformio.ini`:
lib_deps = ayushsharma82/ElegantOTA@3.1.5

### Dependências

Certifique-se de que as seguintes bibliotecas estão instaladas em seu projeto:

```
Arduino
WiFi
AsyncTCP
ESPAsyncWebServer
littlefs
```

## Uso

### Exemplo Básico

Aqui está um exemplo de como usar a biblioteca para configurar o ESP32 para atualizações OTA via Wi-Fi:

```
#include <SHtools_ESP32.h>

// Defina os pinos do LED, do botão e o nome do sketch
const int ledPin = 23;
const int buttonPin = 27;
String nomeSketch = "Projeto teste";

// Crie uma instância da biblioteca
SHtools_ESP32 FirmwareUpdate(ledPin, buttonPin, nomeSketch);

void setup() {
  // Inicialize a biblioteca
  FirmwareUpdate.begin();
}

void loop() {
  // Chame a função handle no loop principal
  FirmwareUpdate.handle();
}
```
