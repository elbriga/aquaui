// WEMOS LOLIN32 - 80MHzs

// Formato dos valores dos sensores que vem do Arduino Leonardo
#define FORMATO_SENSORES "ADC0 ADC1 ADC2 ADC3 ADC4 ADC5 OW1 OW2 OW3 OW4 OW5 OW6 DIG1 DIG2 DIG3 DIG4 DIG5 DIG6 "

// Luz das plantas
#define TEMPO_LUZ_PLANTAS_LIGA     630  //  6:30 da manha
#define TEMPO_LUZ_PLANTAS_DESLIGA 1745  // 17:45 da tarde

// Aquecedor 150W 1
#define TEMPERATURA_MINIMA1 25.5
#define TEMPERATURA_MAXIMA1 26.5
// Aquecedor 150W 2
#define TEMPERATURA_MINIMA2 25.0
#define TEMPERATURA_MAXIMA2 27.0

#define MAX_TEMPO_TEMPERATURA_FORA_DA_FAIXA 30000 // Se ficar mais de X (ms) abaix ou acima = agir
#define MAX_TEMPO_SEM_AGUA 5000 // Tempo limite que o sensor de agua pode ficar sem agua (em ms) para disparar um alerta


// Pinagem dos reles
#define PINO_O_RELE1   12 // OUTPUT 12 = Luz das plantas
#define PINO_O_RELE2   13 // OUTPUT 13 = Aquecedor de agua 150W 1
#define PINO_O_RELE3   14 // OUTPUT 14 = Aquecedor de agua 150W 2
#define PINO_O_RELE4   15 // OUTPUT 15 = Bomba de agua (NF)


#define NTP_SERVER  "a.st1.ntp.br" // Servidor de NTP
#define UTC -3         // UTC -3:00 Brazil
#define NTP_REFRESH 60 // Refresh da hora (s)

#define DISPLAY_PINO1 5
#define DISPLAY_PINO2 4



#define TAMANHO_BUFFER_SERIAL 1024
char bufferSerial[TAMANHO_BUFFER_SERIAL];
int enviaComandoLeo(char *cmd, ...);




// Rede Wifi
//const char* ssid     = "foraTemer";
//const char* password = "morcegopreto";


// Sensores
float sensorPH     = -1;
float sensorTEMP   = -1;
float sensorTEMP_A = -1;

char faltaAgua1 = -1, faltaAgua2 = -1;
int tempoSemAgua1 = -1, tempoSemAgua2 = -1;

char aqc1 = -1, aqc2 = -1; // Estado dos reles dos aquecedores
int tempoTempBaixa1 = -1, tempoTempAlta1 = -1;
int tempoTempBaixa2 = -1, tempoTempAlta2 = -1;





// Declarado por ultimo, da pau pra compilar...
void msgPC(char *fmt, ... );


class IO {
  public:
    IO(const char *_nome, const char *_nomeIO, float valorDefault);

    String nome, nomeIO;
    float  valor;
    int    valorRAW;
};
IO::IO(const char *_nome, const char *_nomeIO, float valorDefault) {
  nome   = _nome;
  nomeIO = _nomeIO;
  valor    = valorDefault;
  valorRAW = valor;
}




#define TOTAL_SENSORES  8
#define TOTAL_ATUADORES 8


class Sensor : public IO {
  public:
    Sensor(const char *_nome, const char *_nomeIO, float valorDefault);
    Sensor(const char *_nome, const char *_nomeIO);

    float get();
    int _set(int _valorRAW);
};
Sensor::Sensor(const char *_nome, const char *_nomeIO, float valorDefault) : IO(_nome, _nomeIO, valorDefault) {}
Sensor::Sensor(const char *_nome, const char *_nomeIO) : IO(_nome, _nomeIO, 0) {}

int Sensor::_set(int _valorRAW) {
  valorRAW = _valorRAW;

  // TODO
  valor = valorRAW / 100.0;
  
  return 0;
}


static Sensor sensores[TOTAL_SENSORES] = { // 18 Sensores do Arduino Leonardo (ADC[0..5] - OW[1..6] - DIG[1..6])
  Sensor("pH",                 "ADC0", 7.00),
  Sensor("nivelTanque1",       "ADC1", 1.00),
  Sensor("temAguaTanque1",     "DIG1"),
  Sensor("nivelTanque2",       "ADC2"),
  Sensor("temAguaTanque2",     "DIG2"),
  Sensor("humidade",           "ADC3"),
  //Sensor("--vazio",          "ADC4"),
  Sensor("temperaturaProbePH", "ADC5"),
  Sensor("temperatura",        "OW1") // oneWire 01
};


class Atuador : public IO {
  public:
    Atuador(const char *nome, const char *nomeIO, float valorDefault) : IO(nome, nomeIO, valorDefault) {};
    Atuador(const char *nome, const char *nomeIO) : IO(nome, nomeIO, 0) {};


    float _get();
    int    set();

    
    void liga();
    void desliga();
};


void Atuador::liga() {
  valor = HIGH;
  valorRAW = valor;
  
  msgPC("Ligando %s (%s)", nome.c_str(), nomeIO.c_str());
  enviaComandoLeo("set%s=1", nomeIO.c_str());
}
void Atuador::desliga() {
  valor = LOW;
  valorRAW = valor;
  
  msgPC("DesLigando %s (%s)", nome.c_str(), nomeIO.c_str());
  enviaComandoLeo("set%s=0", nomeIO.c_str());
}

Atuador atuadores[] = {  // 8 Reles do Arduino Leonardo (RELE[1..8])
  Atuador("luzesPlantas",   "RELE1"),
  Atuador("aquecedor150W1", "RELE2"),
  Atuador("aquecedor150W2", "RELE3"),
  Atuador("bombaAgua",      "RELE4") // Ligada no NF do rele (INVERTIDA >> 0 = LIGADA)
};




#include <HardwareSerial.h>
HardwareSerial Serial1(1);


// DISPLAY ******************************************************************
// Include the correct display library, connection via I2C using Wire include
#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306Wire.h"
#include "OLEDDisplayUi.h" // Include the UI lib
#include "images.h"        // Include custom images

// Initialize the OLED display using Wire library
SSD1306Wire   display(0x3c, DISPLAY_PINO1, DISPLAY_PINO2);
OLEDDisplayUi ui(&display);



// WIFI ******************************************************************
#include <WiFi.h>
String IP = "---";


#include <HTTPClient.h>
WiFiServer server(80);
HTTPClient http;


#include <WiFiUdp.h>
#include <NTPClient.h>
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_SERVER, UTC * 3600, NTP_REFRESH * 1000);






#include <DNSServer.h>
#include <WebServer.h>
#include <WiFiManager.h>


WiFiManager wifiManager;







#include <SoftTimer.h> // TODO :: Remover "Debouncer" do src da biblioteca para nao precisar incluir PciListener.h
// Inicializacao das thread e callback em cada bloco de funcao
#include <Task.h>
#include <DelayRun.h>









// DISPLAY ******************************************************************
// Cenas
// how many frames are there?
int totalCenas = 5;

/*int tempoCena1 = 4000; // 4s na primeira "Cena"
  int tempoCena2 = 6000; // 6s na  segunda "Cena"

  int duracaoLoop       = tempoCena1 + tempoCena2; // 10s de LOOP
  int timePerFrameCena2 = (tempoCena2 / totalCenas);
  int cena = -1;

  void confereCenas() {
  int msecAtual = millis(); // ms desde o boot
  int msLoop    = (msecAtual % duracaoLoop); // = 0..(duracaoLoop-1) Num "frame" dentro do loop

  int _cena = cena;                          // "Backup" de cena
  int cena  = (msLoop < tempoCena1) ? 1 : 2; // Simples calculo de Cena atual baseado no tempo
  int mudouCena = (_cena != cena);           // Detector de mudança de Cena

  if (!mudouCena) return;

  // Mudança de "Cena"
  switch (cena) {
    default:
    case 1:
      ui.switchToFrame(cena);
      ui.disableAutoTransition();
      break;

    case 2:
      int timePerTransition = (timePerFrameCena2 * 0.2);
      ui.setTimePerFrame(timePerFrameCena2 - timePerTransition);
      ui.setTimePerTransition(timePerTransition);
      ui.enableAutoTransition();
      break;
  }
  }*/





// OVERLAY - frame sempre visivel
void msOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(60, 0, timeClient.getFormattedTime());
}

void drawCena0(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->drawXbm(x + 34, y + 14, WiFi_Logo_width, WiFi_Logo_height, WiFi_Logo_bits);
}

void drawCena1(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0 + x, 20 + y, "IP: " + IP);
}

#define NUM_CENA_DEFAULT 2
void drawCena2(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_16);
  display->drawString(0 + x, 12 + y, "pH:"  + String(sensorPH));   // probe pH
  display->drawString(60 + x, 12 + y, "T:"  + String(sensorTEMP_A)); // OneWire Dallas DS18B20 temperature sensor
  display->drawString(0 + x, 30 + y, "a1:"  + String(aqc1 ? "On" : "Off"));
  display->drawString(64 + x, 30 + y, "a2:"  + String(aqc2 ? "On" : "Off"));
  //display->drawString(0 + x, 44 + y, "Ag1:" + String(tempoSemAgua)); // Sensor de agua 1
}

#define NUM_CENA_ALERTA 3
String gMsgAlerta;
void drawCena3(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_16);
  display->drawString(0 + x, 10 + y, "Alerta!!!");
  display->setFont(ArialMT_Plain_10);
  display->drawStringMaxWidth(0 + x, 28 + y, 128, gMsgAlerta);
}

void drawCena4(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  // Cena Vazia!
}

// This array keeps function pointers to all frames
// frames are the single views that slide in
FrameCallback frames[] = { drawCena0, drawCena1, drawCena2, drawCena3, drawCena4 };

// Overlays are statically drawn on top of a frame eg. a clock
OverlayCallback overlays[] = { msOverlay };
int overlaysCount = 1;

void delayUI(int delayMS) {
  int d, msFinal = millis() + delayMS;

  do {
    d = ui.update();
    delay(d);
  } while (millis() < msFinal);
}

#define TEMPO_DURACAO_ALERTA 3000
boolean thread_apagarAlerta_callback(Task* task) {
  gMsgAlerta = "---";
  ui.switchToFrame(NUM_CENA_DEFAULT);
}

DelayRun thread_apagarAlerta(TEMPO_DURACAO_ALERTA, thread_apagarAlerta_callback);
void alerta(String msg, int duracao = TEMPO_DURACAO_ALERTA) {
  gMsgAlerta = msg;
  ui.switchToFrame(NUM_CENA_ALERTA);

  if (duracao < 1000) duracao = 1000;
  else if (duracao > 10000) duracao = 10000;
  thread_apagarAlerta.delayMs = duracao;

  thread_apagarAlerta.startDelayed();
}

void initUI(int delayInicial) {
  Serial.print("Inicializando display: ");

  // The ESP is capable of rendering 60fps in 80Mhz mode
  // but that won't give you much time for anything else
  // run it in 160Mhz mode or just set it to 30 fps
  ui.setTargetFPS(20);

  // Customize the active and inactive symbol
  ui.setActiveSymbol(activeSymbol);
  ui.setInactiveSymbol(inactiveSymbol);

  // You can change this to
  // TOP, LEFT, BOTTOM, RIGHT
  ui.setIndicatorPosition(BOTTOM);

  // Defines where the first frame is located in the bar.
  ui.setIndicatorDirection(LEFT_RIGHT);

  // You can change the transition that is used
  // SLIDE_LEFT, SLIDE_RIGHT, SLIDE_UP, SLIDE_DOWN
  ui.setFrameAnimation(SLIDE_UP);

  // Add frames
  ui.setFrames(frames, totalCenas);

  // Add overlays
  ui.setOverlays(overlays, overlaysCount);

  // Initialising the UI will init the display too.
  ui.init();

  //display.flipScreenVertically();

  //confereCenas();
  ui.setTimePerTransition(1000);
  ui.disableAutoTransition();

  gMsgAlerta = "Sem Problemas";

  delayUI(delayInicial);

  Serial.println("ok");
}
















// WIFI ******************************************************************
// Certificado do dominio do Pushbullet API
const char* root_ca = \
                      "-----BEGIN CERTIFICATE-----\n" \
                      "MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh\n" \
                      "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n" \
                      "d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD\n" \
                      "QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT\n" \
                      "MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n" \
                      "b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG\n" \
                      "9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB\n" \
                      "CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97\n" \
                      "nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt\n" \
                      "43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P\n" \
                      "T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4\n" \
                      "gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO\n" \
                      "BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR\n" \
                      "TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw\n" \
                      "DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr\n" \
                      "hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg\n" \
                      "06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF\n" \
                      "PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls\n" \
                      "YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk\n" \
                      "CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=\n" \
                      "-----END CERTIFICATE-----\n";

void initWifi() {
  /*Serial.print("Conectando a rede WiFi: ");
    Serial.println(ssid);

    // Wifi
    WiFi.begin(ssid, password);*/
  wifiManager.autoConnect("aquaUI");


  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
  }
  IP = WiFi.localIP().toString();

  Serial.println("");
  Serial.println("Yeah Yeah");
  Serial.println("IP: ");
  Serial.println(IP);

  // Porta 80
  server.begin();

  // NTP
  timeClient.begin();
  timeClient.update();
}

void jsonAddAtrInt(char *string, const char *nomeAtr, int valAtr, int virgula = 1) {
  int len = strlen(string);
  char *str = &string[len];

  sprintf(str, virgula ? "\"%s\": %d," : "\"%s\": %d", nomeAtr, valAtr);
}

// Processamento de conexoes na 80
void handleHTTP(Task *t = NULL) {
  WiFiClient client = server.available();   // listen for incoming clients
  if (!client) return;

  // if you get a client,
  msgPC("Conexao HTTP");          // print a message out the serial port
  String currentLine = "", URL = "";      // make a String to hold incoming data from the client
  int timeout = millis() + 50;
  while (client.connected() && millis() < timeout) {            // loop while the client's connected
    if (client.available()) {             // if there's bytes to read from the client,
      char c = client.read();             // read a byte, then
      //Serial.write(c);                    // print it out the serial monitor
      if (c == '\n') {                    // if the byte is a newline character

        if (currentLine.startsWith("GET ") && URL == "")
          URL = currentLine;

        // if the current line is blank, you got two newline characters in a row.
        // that's the end of the client HTTP request, so send a response:
        if (currentLine.length() == 0) {
          // break out of the while loop:
          break;
        } else {    // if you got a newline, then clear currentLine:
          currentLine = "";
        }
      } else if (c != '\r') {  // if you got anything else but a carriage return character,
        currentLine += c;      // add it to the end of the currentLine
      }
    }
  }

  msgPC("httpServer > URL: %s", URL.c_str());

  if (URL != "") {
    // Resposta HTML
    // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
    // and a content-type so the client knows what's coming, then a blank line:
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:text/html");
    client.println();

    if (URL.startsWith("GET /JSON")) {
      // Valores em JSON
      char bufferJSON[256];
      bufferJSON[0] = '{';
      bufferJSON[1] = '\0';

      jsonAddAtrInt(bufferJSON, "_TS",          timeClient.getEpochTime());
      jsonAddAtrInt(bufferJSON, "pH",           sensorPH     * 100);
      jsonAddAtrInt(bufferJSON, "temp",         sensorTEMP_A * 100);
      jsonAddAtrInt(bufferJSON, "aqc1",         aqc1 ? 1 : 0);
      jsonAddAtrInt(bufferJSON, "aqc2",         aqc2 ? 1 : 0);
      //    jsonAddAtrInt(bufferJSON, "tempoSemAgua", tempoSemAgua);
      jsonAddAtrInt(bufferJSON, "fim",          1, 0);

      int len = strlen(bufferJSON);
      bufferJSON[len]   = '}';
      bufferJSON[len + 1] = '\0';

      client.println(bufferJSON);
    } else {
      // Interface padrao
      client.print("<a href=\"/LP1\">Ligar Luzes das plantas</a><br>");
      client.print("<a href=\"/LP0\">Desligar Luzes das plantas</a><br><br>");

      client.print("<a href=\"/AQC11\">Ligar Aquecedor 1</a><br>");
      client.print("<a href=\"/AQC10\">Desligar Aquecedor 1</a><br>");
      client.print("<a href=\"/AQC21\">Ligar Aquecedor 2</a><br>");
      client.print("<a href=\"/AQC20\">Desligar Aquecedor 2</a><br><br>");
    }

    // The HTTP response ends with another blank line:
    client.println();
    // *******************************************************************************


    // Verificar Comandos HTTP
    if (URL.startsWith("GET /LP1")) digitalWrite(PINO_O_RELE1, HIGH);
    if (URL.startsWith("GET /LP0")) digitalWrite(PINO_O_RELE1, LOW);

    if (URL.startsWith("GET /AQC11")) digitalWrite(PINO_O_RELE2, HIGH);
    if (URL.startsWith("GET /AQC10")) digitalWrite(PINO_O_RELE2, LOW);

    if (URL.startsWith("GET /AQC21")) digitalWrite(PINO_O_RELE3, HIGH);
    if (URL.startsWith("GET /AQC20")) digitalWrite(PINO_O_RELE3, LOW);
  }

  // close the connection:
  client.stop();
  //Serial.println("Client Disconnected.");
}
#define TEMPO_POOLING_HTTP 100
Task thread_httpServer(TEMPO_POOLING_HTTP, handleHTTP);


// Alertas via celular
// Acesso HTTPS em "https://api.pushbullet.com/v2/pushes"
void pushbulletAPI(String msgAlerta) {
  if ((WiFi.status() != WL_CONNECTED)) { //Check the current connection status
    msgPC("pushbulletAPI: Wifi n conectado");
    return;
  }

  alerta(msgAlerta);

  http.begin("https://api.pushbullet.com/v2/pushes", root_ca);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Access-Token", "o.nsMkSWjqnZT4E8A2b1HDdUgtnMW0GiAX");
  int httpCode = http.POST("{\"body\":\"" + msgAlerta + "\",\"title\":\"Alertas\",\"type\":\"note\"}");

  if (httpCode > 0) { //Check for the returning code
    String payload = http.getString();
    Serial.println(httpCode);
    Serial.println(payload);
  } else {
    Serial.println("Error on HTTP request");
    Serial.println(httpCode);
  }

  http.end();
}
// *********************************************************************************















float VCC = 5.00;

int setSensor(char *nomeIO, int _valorRAW) {
  if(!strcmp(nomeIO, "VCC")) {
    VCC = _valorRAW / 100.0;
    return 0;
  }
  
  for(int s=0; s<TOTAL_SENSORES; s++) {
    if(sensores[s].nomeIO == nomeIO) {
      //msgPC("Setando sensor [%s] para [%d]", nomeIO, _valorRAW);
      sensores[s]._set(_valorRAW);
      return 0;
    }
  }

  // Erro??!?
  return 1;
}

Sensor *getSensor(char *nomeIO) {
  for(int s=0; s<TOTAL_SENSORES; s++) {
    if(sensores[s].nomeIO == nomeIO || sensores[s].nome == nomeIO) {
      //msgPC("Setando sensor [%s] para [%d]", nomeIO, _valorRAW);
      return &sensores[s];
    }
  }

  // Erro??!?
  return (Sensor *)NULL;
}
Atuador *getAtuador(char *nome) {
  for(int a=0; a<TOTAL_ATUADORES; a++) {
    if(atuadores[a].nome == nome || atuadores[a].nomeIO == nome) {
      return &atuadores[a];
    }
  }

  // Erro??!?
  return (Atuador *)NULL;
}





#define TAMANHO_BUFFER_MEDIA_PH 64
volatile short bufferPH[TAMANHO_BUFFER_MEDIA_PH], indexBufferPH = 0;

volatile int msecSensores;


void atualizaSensores(Task* task = NULL) {
  //msgPC("atzSens");

  int agora = millis();
  int tempoDesdeUltimaMedicao = agora - msecSensores;
  msecSensores = agora;

  
 
  int lenBuff = enviaComandoLeo("get"); // Solicitar String de sensores

  // Validar string recebida -- formato: "{LBL1:Val1,LBL2:Val2,...}\n"
  if (lenBuff < 10 || bufferSerial[0] != '{' || bufferSerial[lenBuff] != '\n') {
    msgPC("Erro!!! String sensores Leonardo invalida! [%d]", lenBuff);
    msgPC("str: \"%s\"", bufferSerial);
    return;
  }

  // Parse da string
  char *ptrLabel = strtok(&bufferSerial[1], ","); // Pular o "{" inicial
  char *ptrValor;

  int valor;
  while (ptrLabel) {
    ptrValor = strstr(ptrLabel, ":");
    if (ptrValor) {
      // Erro!!!
      *ptrValor = '\0';
      ptrValor++;

      valor = atoi(ptrValor);

      setSensor(ptrLabel, valor);
    }

    ptrLabel = strtok(NULL, ",");
  }












  float sensorVoltsPH    = getSensor("pH")->valor;
  float sensorAgua1      = getSensor("nivelTanque1")->valor;
  float sensorAgua2      = getSensor("nivelTanque2")->valor;
  
  float sensorVoltsTemp  = getSensor("temperaturaProbePH")->valor;
  float sensorTEMP_A_aux = getSensor("temperatura")->valor;
  




  /* pre Calcular/Formatar variaveis */

  // pH
  bufferPH[indexBufferPH++] = (1400.0 - ((sensorVoltsPH / VCC) * 1400.0)) + 30;

  /*Serial.print("bufferPH[indexBufferPH = ");
    Serial.print(String(indexBufferPH));
    Serial.print("] = ");
    Serial.println(String(bufferPH[indexBufferPH-1]));*/

  if (indexBufferPH >= TAMANHO_BUFFER_MEDIA_PH)
    indexBufferPH = 0;

  int max = 0, min = 10000, idxMax = 0, idxMin = 15;
  for (int c = 0; c < TAMANHO_BUFFER_MEDIA_PH; c++) {
    if (bufferPH[c] > max) {
      max = bufferPH[c];
      idxMax = c;
    }
    if (bufferPH[c] < min) {
      min = bufferPH[c];
      idxMin = c;
    }
  }
  int media = 0;
  for (int c = 0; c < TAMANHO_BUFFER_MEDIA_PH; c++) {
    if (c != idxMax && c != idxMin)
      media += bufferPH[c];
  }

  sensorPH = ((media / (TAMANHO_BUFFER_MEDIA_PH - 2)) + 0) / 100.0;
  //Serial.print("sensorPH: ");
  //Serial.println(String(sensorPH));


  // Temp
  sensorTEMP = sensorVoltsTemp;

  if (sensorTEMP_A_aux > -100) {
    sensorTEMP_A = sensorTEMP_A_aux;
  }

  // Sensor de agua1
  if ( sensorAgua1 < (VCC / 2) ) tempoSemAgua1 = 0;                        // Temos Agua
  else                           tempoSemAgua1 += tempoDesdeUltimaMedicao; // Acumular o tempo que estamos sem agua


  // Tempo1 com temperatura Baixa
  if ( sensorTEMP_A >= TEMPERATURA_MINIMA1 ) tempoTempBaixa1  = 0;
  else                                       tempoTempBaixa1 += tempoDesdeUltimaMedicao;
  // Tempo1 com temperatura Alta
  if ( sensorTEMP_A <= TEMPERATURA_MAXIMA1 ) tempoTempAlta1  = 0;
  else                                       tempoTempAlta1 += tempoDesdeUltimaMedicao;

  // Tempo2 com temperatura Baixa
  if ( sensorTEMP_A >= TEMPERATURA_MINIMA2 ) tempoTempBaixa2  = 0;
  else                                       tempoTempBaixa2 += tempoDesdeUltimaMedicao;
  // Tempo2 com temperatura Alta
  if ( sensorTEMP_A <= TEMPERATURA_MAXIMA2 ) tempoTempAlta2  = 0;
  else                                       tempoTempAlta2 += tempoDesdeUltimaMedicao;
}
#define TEMPO_REFRESH_SENSORES 2000
Task thread_atualizaSensores(TEMPO_REFRESH_SENSORES, atualizaSensores);
















// Atua conforme o estado dos sensores (envia alertas via HHTPs)
char aqc1_old = -1, aqc2_old = -1;
char luzPlantas_old = -1;
void confereAlertas(Task *t = NULL) {
  int msecAtual = millis();

  // Luz Plantas
  char luzPlantas;
  int tempo = (timeClient.getHours() * 100) + timeClient.getMinutes();

  //  printf("tempoLuzPlantas: %d \n", tempo);

  luzPlantas = ( tempo < TEMPO_LUZ_PLANTAS_LIGA ) ? LOW : (( tempo >= TEMPO_LUZ_PLANTAS_DESLIGA ) ? LOW : HIGH);

  if (luzPlantas != luzPlantas_old) {
    luzPlantas_old = luzPlantas;
    msgPC("LUZ PLANTAS MUDOU!!!");
    
    luzPlantas ?
      getAtuador("luzesPlantas")->liga() :
      getAtuador("luzesPlantas")->desliga();
  }

  /*  // Flag de falta de agua
    faltaAgua = (tempoSemAgua1 > MAX_TEMPO_SEM_AGUA);

    if ( faltaAgua ) {
      if (tsAcessoAPI < msecAtual) {
        tsAcessoAPI = msecAtual + 10000;

        //ui.switchToFrame(1);
        Serial.println("ALERTAAAAAAA");
        //pushbulletAPI();
        //alerta("Falta Agua!!!");
      }
      //} else {
      //tsAcessoAPI = 0;
    }*/


  // Aquecedor 1
  if (tempoTempBaixa1 > MAX_TEMPO_TEMPERATURA_FORA_DA_FAIXA) { // ligar?
    aqc1 = HIGH;
  } else {
    // Temp nao esta baixa, verificar se esta alta
    aqc1 = (tempoTempAlta1 > MAX_TEMPO_TEMPERATURA_FORA_DA_FAIXA) ? LOW : HIGH;
  }
  if (aqc1 != aqc1_old) {
    aqc1_old = aqc1;
    msgPC("TEMPERATURA 1 MUDOU!!!");

    aqc1 ?
      getAtuador("aquecedor150W1")->liga() :
      getAtuador("aquecedor150W1")->desliga();
  }

  // Aquecedor 2
  if (tempoTempBaixa2 > MAX_TEMPO_TEMPERATURA_FORA_DA_FAIXA) { // ligar?
    aqc2 = HIGH;
  } else {
    // Temp nao esta baixa, verificar se esta alta
    aqc2 = (tempoTempAlta2 > MAX_TEMPO_TEMPERATURA_FORA_DA_FAIXA) ? LOW : HIGH;
  }
  if (aqc2 != aqc2_old) {
    aqc2_old = aqc2;
    msgPC("TEMPERATURA 2 MUDOU!!!");

    aqc2 ?
      getAtuador("aquecedor150W2")->liga() :
      getAtuador("aquecedor150W2")->desliga();
  }
}
#define TEMPO_CONFERE_ALERTAS 3000
Task thread_confereAlertas(TEMPO_CONFERE_ALERTAS, confereAlertas);











void setup() {
  // Comunicacao serial para ler sensores do Arduino Leonardo
  Serial1.begin(9600, SERIAL_8N1, 0, 2);

  Serial.begin(9600);
  Serial.println();
  Serial.println();

  for (int c = 0; c < TAMANHO_BUFFER_MEDIA_PH; c++)
    bufferPH[c] = 700;

  // Pinagem IO
  pinMode(PINO_O_RELE1, OUTPUT);
  pinMode(PINO_O_RELE2, OUTPUT);
  pinMode(PINO_O_RELE3, OUTPUT);
  pinMode(PINO_O_RELE4, OUTPUT);

  SoftTimer.add(&thread_atualizaSensores);
  SoftTimer.add(&thread_confereAlertas);
  SoftTimer.add(&thread_httpServer);

  // Inicializar o display
  initUI(1000);

  ui.switchToFrame(0); // Logo WiFi
  // init WiFi
  initWifi();

  ui.switchToFrame(1); // Mostrar IP
  delayUI(1000);

  ui.switchToFrame(NUM_CENA_DEFAULT); // Tela Princiapl

  msecSensores = millis();

  msgPC("init OK");
}
























void loop() {
  int remainingTimeBudget = ui.update();
  //if (!(remainingTimeBudget > 0)) return; // Dar prioridade para o refresh do display

  int timeout = millis() + remainingTimeBudget;
  while (millis() < timeout)
    SoftTimer.loop();
}






#define TAMANHO_BUFFER_LEOOUT 32
int enviaComandoLeo(char *fmtCmd, ... ) {
  int timeout, cntBufferSerial = -1;

  char buffCmd[TAMANHO_BUFFER_LEOOUT]; // resulting string limited to 128 chars
  va_list args;
  va_start (args, fmtCmd );
  vsnprintf(buffCmd, TAMANHO_BUFFER_LEOOUT, fmtCmd, args);
  va_end (args);

  if(strcmp(fmtCmd, "get"))
    msgPC("Enviando [%s] para Leo", buffCmd);
    
  Serial1.println(buffCmd);
  delay(10);

  for (int c = 0; c < TAMANHO_BUFFER_SERIAL; c++)
    bufferSerial[c] = '\0';

  timeout = millis() + 1500;
  do {
    if (Serial1.available()) {
      cntBufferSerial++;
      bufferSerial[cntBufferSerial] = Serial1.read();
      timeout = millis() + 1500;
    } else
      delay(1);
  } while (millis() < timeout && (cntBufferSerial < 0 || bufferSerial[cntBufferSerial] != '\n') && cntBufferSerial < TAMANHO_BUFFER_SERIAL - 1);

  bufferSerial[cntBufferSerial + 1] = '\0';

  if(strcmp(fmtCmd, "get"))
    msgPC("RESP: [%s]", bufferSerial);
  
  return cntBufferSerial;
}


#define TAMANHO_BUFFER_MSGPC 1024
void msgPC(char *fmt, ... ) {
  char buf[TAMANHO_BUFFER_MSGPC]; // resulting string limited to 128 chars
  va_list args;
  va_start (args, fmt );
  vsnprintf(buf, TAMANHO_BUFFER_MSGPC, fmt, args);
  va_end (args);

  Serial.printf("[%s] > [%s]\n", timeClient.getFormattedTime().c_str(), buf);
}
