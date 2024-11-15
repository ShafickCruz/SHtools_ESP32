/*
Dependency Graph
|-- ESP Async WebServer @ 1.2.4
|-- LittleFS @ 2.0.0
|-- AsyncTCP @ 1.1.1
|-- Preferences @ 2.0.0
|-- Update @ 2.0.0
|-- WiFi @ 2.0.0
*/

#include "SHtools_ESP32.h"

/*****************************/
/***** Binário WebServer *****/
/*****************************/
#include "cmd_html.h"
#include "favicon_icon.h"
#include "index_html.h"
#include "info_html.h"
#include "ota_html.h"
#include "script_js.h"
#include "serial_html.h"
#include "sha_js.h"
#include "style_css.h"

/*****************************/
/******** Preferences ********/
/*****************************/
// Limite máximo de 15 caracteres para Namespace e Key
const char *PrefNameSpace_config = "_config";
const char *PrefKey_configOK = "_configOK";
const char *PrefKey_debugInicial = "DebugInicial";
const char *PrefKey_novoFirmware = "NovoFirmware";
const char *PrefKey_testeNovoFirmware = "tstNewFirmware";
const char *PrefKey_fezRollback = "fezRollback";

const char *HeaderOTA = "X-OTA-Header";
const String HeaderOTAassinatura = "SHtoolsOTA";
const char *HeaderOTAtestarFirmware = "X-TestarFirmware";
const String ParamChecksum = "checksum";

String controle_testeNovoFirmware_detalhe = "";
int controle_testeNovoFirmware = 1; // 1 = pong

SHtools_ESP32::SHtools_ESP32(int ledPin, int buttonPin, String nomeSketch)
    : HabilitarDebug(false),
      ServerMode_ON(false),
      restartSolicitado(false),
      ServerModeInicio(0),
      buttonPressTime(0),
      lastButtonStateChangeTime(0),
      longPressDuration(3000),
      debounceDelay(50),
      lastButtonState(HIGH),
      ledPin(ledPin),
      buttonPin(buttonPin),
      nomeSketch(nomeSketch),
      DebugInicial(0),
      server(80),
      ws("/ws_rota") {}

void SHtools_ESP32::begin()
{
  // Configura os pinos de I/O
  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);

  Serial.begin(115200);

  preferencias(-1);                                     // monta config se não estiver montado
  DebugInicial = preferencias(2, PrefKey_debugInicial); // Obtem o valor para DebugInicial

  // Marca firmware como válido sem testar o firmware
  // Cliente web não está esperando resposta
  if (preferencias(2, PrefKey_novoFirmware) && !preferencias(2, PrefKey_testeNovoFirmware))
  {

    printMSG("DETECTADO NOVO FIRMWARE SEM SOLICITAÇÃO DE TESTES", true, true); // debug

    // desmarca novo firmware em preferencias
    preferencias(1, PrefKey_novoFirmware, false);

    esp_err_t err = esp_ota_mark_app_valid_cancel_rollback();
    if (err == ESP_OK)
    {
      printMSG("Novo firmware definido como válidado sem testes de funcionalidades.", true);
    }
    else
    {
      printMSG("Erro ao definir novo firmware como válido. Iniciando processo de rollback com restart...\nDetalhes: " + String(esp_err_to_name(err)), true);
      // não há marcação de rollback efetuado porque sem solicitação de teste de novo firmware, o web cliente não aguarda resposta
      ESP.restart(); // restart rápido para tentar reestabelecer conexão com cliente antes de timout do servidor
    }
  }
}

void SHtools_ESP32::handle()
{
  /*
  Se estiver no modo Servidor, faz o LED piscar continuamente e processa as requisições. Se não estiver, verifica se debug inicial está habilitado.
  Se debug inicial estiver habilitado, inicia o processo de ServerMode e ignora o botão e se não estiver, keep watching o botão.
  */
  if (ServerMode_ON)
  {
    ServerMode_handle();
  }
  else
  {
    if (DebugInicial)
    {
      startServerMode(!(preferencias(2, PrefKey_testeNovoFirmware)));

      printMSG("SUCESSO NA INICIALIZAÇÃO DE SERVERMODE OU FALHA + SOLICITAÇÃO DE TESTE DE NOVO FIRMWARE", true, true); // debug

      /*
      Se chegoun até aqui é porque startServerMode obteve sucesso ou
      teste de novo firmware está marcadao como true em preferences.
      Quando startServerMode não obtem sucesso, ocorre restart em seu processo (ServerMode()).

      Formas de solicitar DebugInicial:
      - CMD (web cliente)
      - Botão físico
      - Teste de novo firmware (OTA)
      */

      /*********************
      TESTE DE NOVO FIRMWARE
      *********************/

      if (ServerMode_ON)
      {
        if (preferencias(2, PrefKey_testeNovoFirmware, true)) // teste de novo firmware obteve sucesso
        {
          printMSG("SUCESSO NO TESTE DE NOVO FIRMWARE", true, true); // debug

          // desmarca novo firmware em preferencias
          preferencias(1, PrefKey_novoFirmware, false);

          // desmarca teste de novo firmware em preferencias
          preferencias(1, PrefKey_testeNovoFirmware, false);

          printMSG("Novo firmware executou testes com sucesso. Iniciando tentativa de marcar firmware como válido...", true);

          // Mark firmware as valid and cancel rollback process
          esp_err_t err = esp_ota_mark_app_valid_cancel_rollback();
          if (err == ESP_OK)
          {
            printMSG("Novo firmware pronto a ser utilizado!", true);
            controle_testeNovoFirmware_detalhe = "Novo firmware pronto a ser utilizado!";
            controle_testeNovoFirmware = 2;
          }
          else
          {
            printMSG("Erro ao definir novo firmware como válido. Iniciando processo de rollback com restart...\nDetalhes: " + String(esp_err_to_name(err)), true);
            controle_testeNovoFirmware_detalhe = "Erro ao definir novo firmware como válido. Iniciando processo de rollback com restart...";
            controle_testeNovoFirmware = 0;
            ESP.restart(); // restart rápido para tentar reestabelecer conexão com cliente antes de timout do servidor
          }
        }
      }
      else
      // ServerMode_ON sendo false e não havido boot no processo de ServerMode, significa que teste de novo firmware falhou
      {
        printMSG("FALHA NO TESTE DE NOVO FIRMWARE", true, true); // debug

        printMSG("Teste de novo firmware falhou. Iniciando tentativa de roolback.", true);
        // sem possibilidades de informar ao web cliente

        // desmarca novo firmware em preferencias
        preferencias(1, PrefKey_novoFirmware, false);

        // desmarca teste de novo firmware em preferencias
        preferencias(1, PrefKey_testeNovoFirmware, false);

        /*
        Marca na preferencias que fez rollback
        Mesmo se não conseguir fazer, tratará como se tivesse feito, pois se o teste de novo firmware falhou
        e não conseguiu fazer rollback, o sistema estará inválido e não fará diferença esta marcação.
        */
        preferencias(1, PrefKey_fezRollback, true);

        printMSG("INICIO DE ROLLBACK SEGUIDO DE RESTART", true, true); // debug

        // Mark firmware as invalid and rollback to previous firmware
        esp_err_t err = esp_ota_mark_app_invalid_rollback_and_reboot();

        // Provavelmente não chegará até aqui, pois poderá haver restart imediato
        if (err == ESP_OK)
        {
          printMSG("Iniciando processo de rollback...", true);
          // sem possibilidades de informar ao web cliente
        }
        else
        {
          printMSG("Não há firmware disponível para processo de rollback ou processo de rollback falhou.\nDetalhes: " + String(esp_err_to_name(err)), true);
          // sem possibilidades de informar ao web cliente
        }
      }
    }
    else
    {
      bt_handle();
    }
  }
}

void SHtools_ESP32::ServerMode_handle()
{

  // Verifica se havia solicitação de teste de novo firmware e o mesmo falhou
  // Retorna mensagem para web cliente
  if (preferencias(2, PrefKey_fezRollback))
  {
    controle_testeNovoFirmware_detalhe = "Falha ao iniciar modo servidor. Rollback efetuado com sucesso.";
    controle_testeNovoFirmware = 0;
    preferencias(1, PrefKey_fezRollback, false); // reseta valor de chave de preferencia
  }

  ws.cleanupClients(); // Processa eventos do WebSocket e mantém o WebSocket ativo e limpa conexões inativas

  unsigned long cTime = millis();

  /*
  Se está em modo servidor há mais de 30 minutos,
  desativa o modo DebugInicial e reinicia o esp para sair do modo servidor
  */
  if ((cTime - ServerModeInicio) >= 1800000) // 30 minutos
  {
    if (WiFi.softAPgetStationNum() == 0) // Verifica se há clientes conectados
    {
      DebugInicial = preferencias(1, PrefKey_debugInicial, false); // Desativa DebugInicial
      ReiniciarESP();                                              // Reinicia o ESP32
    }
  }

  // Faz o LED piscar continuamente
  static unsigned long lastBlinkTime = 0;
  unsigned long blinkInterval = 150; // Piscar a cada 150ms
  cTime = millis();

  if ((cTime - lastBlinkTime) >= blinkInterval)
  {
    digitalWrite(ledPin, !digitalRead(ledPin)); // Inverte o estado do LED
    lastBlinkTime = cTime;
  }
}

void SHtools_ESP32::bt_handle()
{
  int currentButtonState = digitalRead(buttonPin); // Lê o estado atual do botão
  unsigned long currentTime = millis();            // Captura o tempo atual

  // Verifica se o estado do botão mudou e se passou o tempo suficiente para o debounce
  if (currentButtonState != lastButtonState)
  {
    if ((currentTime - lastButtonStateChangeTime) > debounceDelay)
    {
      if (currentButtonState == LOW)
        buttonPressTime = currentTime; // Registra o momento em que o botão foi pressionado

      lastButtonStateChangeTime = currentTime;
    }
  }

  // Verifica se o botão está pressionado e se o tempo de pressionamento excede longPressDuration
  if (currentButtonState == LOW && (currentTime - buttonPressTime) >= longPressDuration)
  {
    // aguarda o botão ser liberado (de LOW para HIGH)
    digitalWrite(ledPin, !digitalRead(ledPin)); // apaga ou acende o led para indicar que o longpress foi reconhecido
    while (digitalRead(buttonPin) == LOW)
    {
      delay(100);
    }

    // Tenta iniciar modo servidor e reinicia caso não consiga
    startServerMode(true);
  }

  // Atualiza o estado anterior do botão
  lastButtonState = currentButtonState;
}

void SHtools_ESP32::startServerMode(bool _softRestart)
{
  if (!ServerMode())
  {
    printMSG("Falha ao iniciar modo servidor!", true);

    // se DebugInicial estiver true e houver falha na inicialização do modo servidor,
    // ficará em loop infinito. Para evitar, deve-se desativar o DebugInicial
    if (DebugInicial)
    {
      DebugInicial = preferencias(1, PrefKey_debugInicial, false);
    }

    if (_softRestart)
    {
      printMSG("Restart em 5 segundos...", true);
      delay(4000);
      ReiniciarESP();
    }
  }
}

bool SHtools_ESP32::ServerMode()
{
  ServerMode_ON = false;

  printMSG("Entrando em modo Servidor...", true);

  if (!WifiSetup()) // configura e inicializa o servidor wifi
  {
    printMSG("Falha ao iniciar WiFi!", true);
    return false;
  }

  /*
  if (!LittleFS.begin(true)) // monta estrutura de arquivos
  {
    printMSG("Falha ao montar LittleFS!", true);
    return false;
  }
  */

  server.addHandler(&ws); // Inicializa o WebSocket
  
  rotasEcallbacks(); // define rotas e callbacks

  server.begin(); // Start webserver

  printMSG("Servidor iniciado", true);
  ServerMode_ON = true;
  ServerModeInicio = millis();

  return true;
}

void SHtools_ESP32::rotasEcallbacks()
{

  server.on("/server_status", HTTP_GET, [this](AsyncWebServerRequest *request)
            {
              // Verificar se o cabeçalho personalizado está presente
              if (request->hasHeader(HeaderOTA))
              {
                String customHeaderValue = request->header(HeaderOTA); // Obter o valor do cabeçalho

                // Verificar se o valor é o esperado
                if (customHeaderValue == HeaderOTAassinatura)
                {
                  switch (controle_testeNovoFirmware)
                  {
                  case 0:
                  controle_testeNovoFirmware = 1; // reseta marcador
                    request->send(500, "text/plain", "Falha no teste de novo firmware.<br><strong>Detalhes:</strong> " + controle_testeNovoFirmware_detalhe); // resposta final
                    break;

                  case 1:
                    request->send(202); // utilizado para pong
                    break;

                  case 2:
                  controle_testeNovoFirmware = 1; // reseta marcador
                    request->send(200, "text/plain", "Servidor reiniciado.<br><strong>Detalhes:</strong> " + controle_testeNovoFirmware_detalhe); // resposta final
                    break;
                  }
                }
                else
                {
                  request->send(403, "text/plain", "Credenciais inválidas!"); // assinatura invalida
                }
              }
              else
              {
                request->send(403, "text/plain", "Credenciais ausentes!"); // cabeçalho ausente
              } });

  //
  // Rota para obter informações da placa
  //
  server.on("/placa_info", HTTP_GET, [this](AsyncWebServerRequest *request)
            {
    // Verificar se o cabeçalho personalizado está presente
    if (request->hasHeader(HeaderOTA)) {
        String customHeaderValue = request->header(HeaderOTA); // Obter o valor do cabeçalho

        // Verificar se o valor é o esperado
        if (customHeaderValue == HeaderOTAassinatura) {
            // Obter informações da placa
            String jsonResponse = obterInformacoesPlaca();
            request->send(200, "application/json", jsonResponse); // Enviar a resposta como JSON
        } else {
            request->send(403, "text/plain", "Credenciais inválidas!"); // assinatura invalida
        }
    } else {
        request->send(403, "text/plain", "Credenciais ausentes!"); // cabeçalho ausente
    } });

  //
  // OTA: Rota para servir o upload de firmware
  //
  server.on("/upload_firmware", HTTP_POST, [this](AsyncWebServerRequest *request)
            {
      // Cabeçalhos e permissões de upload
      if (request->hasHeader(HeaderOTA)) {
          String customHeaderValue = request->header(HeaderOTA);
          if (customHeaderValue != HeaderOTAassinatura) {              
              request->send(403, "text/plain", "Credenciais inválidas!"); // assinatura invalida
          }
      } else {
          request->send(403, "text/plain", "Credenciais ausentes!"); // cabeçalho ausente
      } }, [this](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
            { OTA_FirmwareUpdate(request, filename, index, data, len, final); });

  //
  // SERVIR ARQUIVOS ESTATICOS SEM ROTAS PERSONALIZADAS
  //
  // server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
  // server.serveStatic("/img", LittleFS, "/img");
  // server.serveStatic("/libs", LittleFS, "/libs");
  // server.serveStatic("/ota", LittleFS, "/ota").setDefaultFile("ota.html");
  // server.serveStatic("/serial", LittleFS, "/serial").setDefaultFile("serial.html");

  //
  // WEBSOCKET
  //

  ws.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client,
                    AwsEventType type, void *arg, uint8_t *data, size_t len)
             {
        switch (type) {
            case WS_EVT_CONNECT:
                printMSG("Novo cliente conectado", true);
                break;
            case WS_EVT_DISCONNECT:
                printMSG("Cliente desconectado", true);
                break;
            case WS_EVT_DATA:  // Mensagens recebidas por websocket
            {
              String message((char*)data, len); // Captura a mensagem recebida
              printMSG(message, true); // Chama a função printMSG com a mensagem recebida
              break;
            }

            default:
                break;
        } });

  RotasBinarios();
}

void SHtools_ESP32::RotasBinarios()
{
  // Definir cada rota para servir o respectivo arquivo binário
  server.on("/cmd.html", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/html", cmd_html, cmd_html_len); });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/html", index_html, index_html_len); });

  server.on("/info.html", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/html", info_html, info_html_len); });

  server.on("/ota.html", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/html", ota_html, ota_html_len); });

  server.on("/serial.html", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/html", serial_html, serial_html_len); });

  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "application/javascript", script_js, script_js_len); });

  server.on("/sha.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "application/javascript", sha_js, sha_js_len); });

  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/css", style_css, style_css_len); });

  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "image/x-icon", favicon_ico, favicon_ico_len); });
}

bool SHtools_ESP32::WifiSetup()
{
  // desconecta WiFi
  if (WiFi.status() == WL_CONNECTED)
  {
    WiFi.disconnect();
    delay(2000);
  }

  // Configura o servidor AP
  IPAddress local_ip(192, 168, 100, 100);
  IPAddress local_mask(255, 255, 255, 0);
  IPAddress gateway(192, 168, 100, 1);
  String ssid = "192.168.100.100 -> " + generateSSID(); // Gera o SSID com identificador único

  // Aguarda a inicialização do servidor wifi
  printMSG("inicializando webserver.", true);
  unsigned long startTime = millis();  // Armazena o tempo de início
  const unsigned long timeout = 10000; // Tempo limite de 10 segundos

  WiFi.persistent(false); // Desativa a persistência das configurações do Wi-Fi para proteger a memoria flash

  while (!WiFi.softAP(ssid.c_str()))
  {
    printMSG(".", false);
    delay(100);

    // Verifica se o tempo limite foi atingido
    if (millis() - startTime > timeout)
    {
      printMSG("Falha ao iniciar o Access Point.", true);
      return false; // Retorna false se não conseguir iniciar
    }
  }

  // aplica as configurações
  WiFi.softAPConfig(local_ip, gateway, local_mask);

  printMSG("Access Point criado com sucesso.", true);
  printMSG("SSID: ", false);
  printMSG(ssid, true);
  printMSG("IP do ESP32: ", false);
  printMSG(WiFi.softAPIP().toString(), true);

  return true;
}

String SHtools_ESP32::generateSSID()
{
  uint64_t mac = ESP.getEfuseMac();   // Obtém o identificador único do ESP32
  String uniqueID = String(mac, HEX); // Converte para uma string hexadecimal
  uniqueID.toUpperCase();             // Opcional: converte para maiúsculas
  return uniqueID;
}

// Getter para o server
AsyncWebServer &SHtools_ESP32::get_server()
{
  return server;
}

// Getter para o websocket
AsyncWebSocket &SHtools_ESP32::get_ws()
{
  return ws;
}

// Getter para ServerMode_ON
bool SHtools_ESP32::get_ServerMode_ON() const
{
  return ServerMode_ON;
}

// Obtem informações da placa
String SHtools_ESP32::obterInformacoesPlaca()
{
  // Definindo a struct para armazenar as informações
  struct ESP32Info
  {
    String chipIsEP32 = "n/a";
    String chip = "n/a";
    String revisao = "n/a";
    String nucleos = "n/a";
    String cpuFreqMhz = "n/a";
    String cristal = "n/a";
    String flashType = "n/a";
    String flashSpeed = "n/a";
    String flashSize = "n/a";
    String flashDisponivel = "n/a";
    String ramTotal = "n/a";
    String ramDisponivel = "n/a";
    String mac = "n/a";
    String uptime = "n/a";
    bool wifi = false;
    bool ble = false;
    bool bt = false;
  } info;

  // Para obter informações detalhadas do chip
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);

  String json;

  // Obter o modelo do chip
  const char *chipModel = ESP.getChipModel(); // Armazena como const char*

  if (chipModel)
  {
    info.chip = String(chipModel); // Converte para String
    info.chip.toUpperCase();

    // Verifica se o chip é ESP32
    if (info.chip.indexOf("ESP32") >= 0)
    {
      info.chipIsEP32 = "true";
    }
    else
    {
      info.chipIsEP32 = "false";
      json = "{\"chipIsEP32\": \"" + info.chipIsEP32 + "\"}"; // Monta JSON para chip não ESP32
      return json;                                            // Retorna JSON imediatamente se não for ESP32
    }
  }
  else
  {
    printMSG("Erro ao tentar obter modelo do chip da placa", true);
    info.chipIsEP32 = "n/a";                                // Valor padrão em caso de erro
    json = "{\"chipIsEP32\": \"" + info.chipIsEP32 + "\"}"; // Monta JSON com erro
    return json;                                            // Retorna JSON imediatamente
  }

  // Atribuir valores às variáveis
  info.revisao = String(ESP.getChipRevision());
  info.nucleos = String(chip_info.cores);
  info.cpuFreqMhz = String(ESP.getCpuFreqMHz());  // MHz
  info.cristal = String(rtc_clk_xtal_freq_get()); // MHz
  info.flashType = String((chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "Embedded" : "External");
  info.flashSpeed = String(ESP.getFlashChipSpeed() / 1000000);        // MHz
  info.flashSize = String(spi_flash_get_chip_size() / (1024 * 1024)); // MB
  info.ramTotal = String(ESP.getHeapSize() / 1024);                   // KB
  info.ramDisponivel = String(ESP.getFreeHeap() / 1024);              // KB
  info.mac = WiFi.macAddress();
  info.uptime = String(millis() / 1000); // segundos;

  info.wifi = chip_info.features & CHIP_FEATURE_WIFI_BGN;
  info.ble = chip_info.features & CHIP_FEATURE_BLE;
  info.bt = chip_info.features & CHIP_FEATURE_BT;

  // Calcular flash disponível
  //uint32_t totalBytes = spi_flash_get_chip_size();
  info.flashDisponivel = String((ESP.getFlashChipSize() - ESP.getSketchSize()) / 1024); // KB

  // Montagem manual do objeto JSON
  json = "{";
  json += "\"chipIsEP32\": \"" + info.chipIsEP32 + "\",";
  json += "\"chip\": \"" + info.chip + "\",";
  json += "\"revisao\": \"" + info.revisao + "\",";
  json += "\"nucleos\": \"" + info.nucleos + "\",";
  json += "\"cpuFreqMhz\": \"" + info.cpuFreqMhz + "MHz\",";
  json += "\"cristal\": \"" + info.cristal + "MHz\",";
  json += "\"flashType\": \"" + info.flashType + "\",";
  json += "\"flashSpeed\": \"" + info.flashSpeed + "MHz\",";
  json += "\"flashSize\": \"" + info.flashSize + "MB\",";
  json += "\"flashDisponivel\": \"" + info.flashDisponivel + "KB\",";
  json += "\"ramTotal\": \"" + info.ramTotal + "KB\",";
  json += "\"ramDisponivel\": \"" + info.ramDisponivel + "KB\",";
  json += "\"mac\": \"" + info.mac + "\",";
  json += "\"uptime\": \"" + info.uptime + "s\",";
  json += "\"wifi\": " + String(info.wifi) + ",";
  json += "\"ble\": " + String(info.ble) + ",";
  json += "\"bt\": " + String(info.bt);
  json += "}";

  return json;
}

void SHtools_ESP32::OTA_FirmwareUpdate(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
{
  static bool hasError = false;
  static bool testarFirmwareSolicitado = false;
  static String clientChecksum;
  static mbedtls_sha256_context ctx;
  String msgRetorno = "";

  // Se ocorreu um erro anterior, aborta o processo imediatamente
  if (hasError)
  {
    msgRetorno = "Erro anterior no upload. Processo abortado.";
    request->send(500, "text/plain", msgRetorno);
    printMSG(msgRetorno, true);
    return;
  }

  if (index == 0)
  {
    // Inicialização
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0); // Inicia SHA-256

    if (request->hasParam(ParamChecksum, true))
    {
      clientChecksum = request->getParam(ParamChecksum, true)->value();
    }
    if (request->hasHeader(HeaderOTAtestarFirmware))
    {
      testarFirmwareSolicitado = request->getHeader(HeaderOTAtestarFirmware)->value() == "true";
    }

    // Verificações e inicializações
    if (!Update.begin(UPDATE_SIZE_UNKNOWN))
    {
      hasError = true;
      msgRetorno = "Erro ao iniciar Update OTA";
      request->send(500, "text/plain", msgRetorno);
      printMSG(msgRetorno, true);
      return;
    }
  }

  // Atualiza checksum e grava dados
  mbedtls_sha256_update(&ctx, data, len);
  if (Update.write(data, len) != len)
  {
    hasError = true;
    msgRetorno = "Erro ao gravar firmware";
    request->send(500, "text/plain", msgRetorno);
    printMSG(msgRetorno, true);
    return;
  }

  if (final)
  {
    // Verifica checksum
    if (!clientChecksum.isEmpty())
    {
      String calculatedChecksum = OTA_FirmwareUpdate_ChecksumFinal(&ctx);
      if (calculatedChecksum != clientChecksum)
      {
        Update.abort();
        hasError = true;
        msgRetorno = "Checksum inválido";
        request->send(400, "text/plain", msgRetorno);
        printMSG(msgRetorno, true);
        return;
      }
    }

    // Finaliza a atualização
    if (Update.end(true))
    {
      msgRetorno = "Upload via OTA bem sucedido! Ativando novo firmware...";
      request->send(200, "text/plain", msgRetorno);
      printMSG(msgRetorno, true);
    }
    else
    {
      hasError = true;
      msgRetorno = "Erro ao finalizar o update OTA";
      request->send(500, "text/plain", msgRetorno);
      printMSG(msgRetorno, true);
    }

    delay(2000); // delay para dar tempo de enviar resposta antes do restart

    // Faz restart para ativar novo firmware
    if (Update.isFinished())
    {
      preferencias(1, PrefKey_novoFirmware, true);                // informa que houve update de firmware
      DebugInicial = preferencias(1, PrefKey_debugInicial, true); // Ativa DebugInicial

      if (testarFirmwareSolicitado)
      {
        preferencias(1, PrefKey_testeNovoFirmware, true); // marca flag para testar novo firmware após restart
      }

      ESP.restart();
    }

    // Reinicializa as variáveis estáticas após sucesso ou falha
    hasError = false;
    testarFirmwareSolicitado = false;
    clientChecksum = "";
  }
}

// Função para calcular o checksum completo após upload
String SHtools_ESP32::OTA_FirmwareUpdate_ChecksumFinal(mbedtls_sha256_context *ctx)
{
  uint8_t hash[32];
  mbedtls_sha256_finish(ctx, hash); // Finaliza o cálculo do hash

  // Converte o hash para string hexadecimal
  String checksumStr;
  for (int i = 0; i < 32; i++)
  {
    char hex[3];
    sprintf(hex, "%02x", hash[i]);
    checksumStr += hex;
  }
  return checksumStr;
}

/// @brief Manipula <preferences.h> para escrita e leitura
/// @param _opcao -1 = monta _config se não estiver montado,
/// 0 = cria ou obtem o valor,
/// 1 = atualiza valor,
/// 2 = obtém valor
/// @param _chave const char
/// @param _valor bool
/// @return bool
bool SHtools_ESP32::preferencias(int8_t _opcao, const char *_chave, bool _valor)
{
  config.begin(PrefNameSpace_config, false); // Abre o Preferences para escrita
  bool valorRetorno = false;
  switch (_opcao)
  {
  case -1: // monta _config se não estiver montado
    if (!config.isKey(PrefKey_configOK))
    {
      config.putBool(PrefKey_debugInicial, false);
      config.putBool(PrefKey_novoFirmware, false);
      config.putBool(PrefKey_testeNovoFirmware, false);
      config.putBool(PrefKey_fezRollback, false);
      config.putBool(PrefKey_configOK, true);
    }
    break;

  case 0: // se não exisitir a chave, cria. Se existir, obtem o valor
    if (!config.isKey(_chave))
    {
      config.putBool(_chave, _valor);
      valorRetorno = _valor;
    }
    else
    {
      valorRetorno = config.getBool(_chave);
    }
    break;

  case 1: // atualiza valor
    config.putBool(_chave, _valor);
    valorRetorno = _valor;
    break;

  case 2: // obtem valor
    valorRetorno = config.getBool(_chave);
    break;

  default:
    break;
  }
  config.end();        // Fecha o Preferences após a gravação
  return valorRetorno; // Atualiza a variável interna
}

// Implementação de função para imprimir as informações em tela
void SHtools_ESP32::printMSG(const String &_msg, bool newline, bool _debug)
{
  String msg = _msg;

  // controle de imprimir mensagens de debug
  if (_debug)
  {
    if (HabilitarDebug)
    {
      msg = "DEBUG >>> " + msg;
    }
    else
    {
      return;
    }
  }

  // Verifica se a mensagem recebida é um comando (case insensitive)
  if (msg.substring(0, 4).equalsIgnoreCase("cmd:"))
  {
    msg = msg + ":resultado=" + String((SerialCMD(msg.substring(4)) ? 1 : 0));
  }

  if (ServerMode_ON)
    ws.textAll(msg); // Enviar para o WebSocket (serial remoto)
                     // não obedece "newline = false", cada envio é escrito em uma nova linha

  if (newline)
    Serial.println(msg); // Envia com nova linha
  else
    Serial.print(msg); // Envia sem nova linha

  if (restartSolicitado)
  {
    ReiniciarESP();
  }
}

bool SHtools_ESP32::SerialCMD(String _cmd)
{
  // Verifica qual comando foi enviado
  if (_cmd.equalsIgnoreCase("DebugInicial"))
  {
    printMSG("Comando DebugInicial: " + String(DebugInicial) + " >>>>> " + String(!DebugInicial), true);
    DebugInicial = preferencias(1, PrefKey_debugInicial, !DebugInicial);
    delay(1000);
    restartSolicitado = true;
    return true;
  }
  else if (_cmd.equalsIgnoreCase("Teste"))
  {
    printMSG("...teste...", true);
    return true;
  }
  else
  {
    printMSG("Comando desconhecido.", true);
    return false; // Retorna false para comando não reconhecido
  }
}

/// @brief Soft restart: Finaliza recusroso ativos e efetua restart do sistema
/// @param _tempoDelay Milesegundos (deafulr 1000)
void SHtools_ESP32::ReiniciarESP(int _tempoDelay)
{
  ws.closeAll();      // Fecha todas as conexões WebSocket, ignorando erros
  delay(50);          // delay de segurança
  server.end();       // Para o servidor, ignorando erros
  delay(50);          // delay de segurança
  WiFi.disconnect();  // Desconecta o WiFi, ignorando erros
  delay(50);          // delay de segurança
  Serial.flush();     // Garante que todos os dados da Serial sejam enviados
  delay(_tempoDelay); // Atraso opcional antes do reinício
  ESP.restart();      // Reinicia o ESP
}