// WEMOS LOLIN32 - 80MHzs

// pH
#define PH_OFFSET 0.3

// Luz das plantas
#define TEMPO_LUZ_PLANTAS_LIGA     630  //  6:30 da manha
#define TEMPO_LUZ_PLANTAS_DESLIGA 1745  // 17:45 da tarde

// Aquecedor 150W 1
#define TEMPERATURA_MINIMA1 26.3
#define TEMPERATURA_MAXIMA1 26.7
// Aquecedor 150W 2
#define TEMPERATURA_MINIMA2 26.0
#define TEMPERATURA_MAXIMA2 27.0

#define MAX_TEMPO_TEMPERATURA_FORA_DA_FAIXA 30000 // Se ficar mais de X (ms) abaix ou acima = agir
#define MAX_TEMPO_SEM_AGUA 5000 // Tempo limite que o sensor de agua pode ficar sem agua (em ms) para disparar um alerta


#define TEMPO_DURACAO_ALERTA 3000


#define NTP_SERVER  "a.st1.ntp.br" // Servidor de NTP
#define UTC -3         // UTC -3:00 Brazil
#define NTP_REFRESH 60 // Refresh da hora (s)

#define DISPLAY_PINO1 5
#define DISPLAY_PINO2 4


float VCC = 500;

#define TAMANHO_BUFFER_SERIAL 1024
char bufferSerial[TAMANHO_BUFFER_SERIAL];
int enviaComandoLeo(char *cmd, ...);




// Rede Wifi
//const char* ssid     = "foraTemer";
//const char* password = "morcegopreto";


// Sensores
char faltaAgua1 = -1, faltaAgua2 = -1;
int tempoSemAgua1 = -1, tempoSemAgua2 = -1;
int tempoTempBaixa1 = -1, tempoTempAlta1 = -1;
int tempoTempBaixa2 = -1, tempoTempAlta2 = -1;





// Declarado por ultimo, da pau pra compilar...
void msgPC(char *fmt, ... );




#define TAMANHO_BUFFER_MEDIA_PH 64
float bufferPH[TAMANHO_BUFFER_MEDIA_PH];
int indexBufferPH = 0;


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
#define TOTAL_ATUADORES 4


class Sensor : public IO {
  public:
    Sensor(const char *_nome, const char *_nomeIO, float valorDefault);
    Sensor(const char *_nome, const char *_nomeIO);

    float get();
    int _set(int _valorRAW);
};
Sensor::Sensor(const char *_nome, const char *_nomeIO, float valorDefault) : IO(_nome, _nomeIO, valorDefault) {}
Sensor::Sensor(const char *_nome, const char *_nomeIO) : IO(_nome, _nomeIO, -1) {}

int Sensor::_set(int _valorRAW) {
  valorRAW = _valorRAW;

  if(nome == "pH") {
      // pH
      bufferPH[indexBufferPH] = (float)((14.0 - ((valorRAW / VCC) * 14.0)) + PH_OFFSET);
      if (indexBufferPH >= TAMANHO_BUFFER_MEDIA_PH)
        indexBufferPH = 0;

      // Descartar os valores minimos e maximos e calcular a MEDIA
      float max = -100000.0, min = 100000.0;
      short idxMax = 0, idxMin = 0;
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
      float media = 0;
      int   cntMedia = 0;
      for (int c = 0; c < TAMANHO_BUFFER_MEDIA_PH; c++) {
        if (c != idxMax && c != idxMin) {
          media += bufferPH[c];
          cntMedia++;
        }
      }
    
      valor = (media / cntMedia) + 0;
      //Serial.print("sensorPH: ");
      //Serial.println(valor);
  } else if(nome == "temperatura") {
    if (valorRAW > -100)
      valor = valorRAW / 100.0;
  } else {
    valor = valorRAW / 100.0;
  }
  
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

    
    void liga(int force, String motivo);
    void desliga(int force, String motivo);
};


void Atuador::liga(int force, String motivo) {
  if(!force && valor == HIGH)
    return;
  
  valor = HIGH;
  valorRAW = valor;

  if(!force) msgPC("Ligando [%s] :: Motivo [%s]", nome.c_str(), motivo.c_str());
  
//msgPC("   Ligando %s (%s)", nome.c_str(), nomeIO.c_str());
  enviaComandoLeo("set%s=1", nomeIO.c_str());
}
void Atuador::desliga(int force=0, String motivo="") {
  if(!force && valor == LOW)
    return;
  
  valor = LOW;
  valorRAW = valor;

  if(!force) msgPC("Desligando [%s] :: Motivo [%s]", nome.c_str(), motivo.c_str());
  
//msgPC("   DesLigando %s (%s)", nome.c_str(), nomeIO.c_str());
  enviaComandoLeo("set%s=0", nomeIO.c_str());
}

Atuador atuadores[TOTAL_ATUADORES] = {  // 8 Reles do Arduino Leonardo (RELE[1..8])
  Atuador("luzesPlantas",   "RELE1"),
  Atuador("aquecedor150W1", "RELE2"),
  Atuador("aquecedor150W2", "RELE3"),
  Atuador("bombaAgua",      "RELE4"), // Ligada no NF do rele (INVERTIDA >> 0 = LIGADA)
// Desconectados
/*  Atuador("Rele5",   "RELE5"),
  Atuador("Rele6",   "RELE6"),
  Atuador("Rele7",   "RELE7"),
  Atuador("Rele8",   "RELE8")*/
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
int totalCenas = 5;

// OVERLAY - frame sempre visivel
void msOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
  if((long)timeClient.getEpochTime() < 0) return;
  
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
  display->drawString(0  + x, 12 + y, "pH:" + String(getSensor("pH")->valor));
  display->drawString(60 + x, 12 + y, "T:"  + String(getSensor("temperatura")->valor));
  display->drawString(0  + x, 30 + y, "a1:" + String(getAtuador("aquecedor150W1")->valor ? "On" : "Off"));
  display->drawString(64 + x, 30 + y, "a2:" + String(getAtuador("aquecedor150W2")->valor ? "On" : "Off"));
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

boolean thread_apagarAlerta_callback(Task* task) {
  gMsgAlerta = "---";
  ui.transitionToFrame(NUM_CENA_DEFAULT);
}
DelayRun thread_apagarAlerta(TEMPO_DURACAO_ALERTA, thread_apagarAlerta_callback);

void alerta(String msg, int duracao = TEMPO_DURACAO_ALERTA) {
  gMsgAlerta = msg;
  ui.transitionToFrame(NUM_CENA_ALERTA);

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
  ui.disableAllIndicators();

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
  ui.setTimePerTransition(800);
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

void enviaRespostaHTTP(WiFiClient client, String html, int code=200) {
  // and a content-type so the client knows what's coming, then a blank line:
  client.print("HTTP/1.1 ");
  client.print(code);
  client.print(" ");
  if(code == 200)      client.println("OK");
  else if(code == 404) client.println("Not Found");
  else                 client.println("???");
  client.println("Content-type:text/html");
  client.println();
  client.println(html);
  client.println();

  msgPC("httpServer: Enviada resposta %d [%s]", code, html.c_str());
}

// Processamento de conexoes na 80
void handleHTTP(Task *t = NULL) {
  WiFiClient client = server.available();
  if (!client) return;

  // if you get a client,
  msgPC("Conexao HTTP");
  
  String currentLine = "", URL = "";
  int timeout = millis() + 250;
  char c;
  while (client.connected() && millis() < timeout) {
    if (client.available()) {
      c = client.read();
      if (c == '\n') {

        //msgPC("URL:[%s] - currentLine:[%s]", URL.c_str(), currentLine.c_str());
        
        if (currentLine.startsWith("GET "))
          URL = currentLine;

        if (currentLine.length() == 0) break; // 2 "\n" seguidos
        else                           currentLine = "";
      } else if (c != '\r') {
        currentLine += c;
      }
    } else {
      delay(1);
    }
  }

  msgPC("httpServer > URL: %s", URL.c_str());

  // Resposta HTML
  if (URL.startsWith("GET /JSON ")) {
    // Valores em JSON
    String bufferHTML = "{";
    for(int s=0; s<TOTAL_SENSORES; s++) {
      bufferHTML += sensores[s].nome;
      bufferHTML += ":";
      bufferHTML += sensores[s].valor;

      if(s < TOTAL_SENSORES - 1)
        bufferHTML += ",";
    }
    bufferHTML += "}";

    enviaRespostaHTTP(client, bufferHTML);
  } else if (URL.startsWith("GET / ")) {
    // Interface padrao
    String bufferHTML = "";
    for(int s=0; s<TOTAL_SENSORES; s++) {
      bufferHTML += sensores[s].nome;
      bufferHTML += " = ";
      bufferHTML += sensores[s].valor;
      bufferHTML += "<br>\n";
    }
    bufferHTML += "<br>\n";
    
    for(int a=0; a<TOTAL_ATUADORES; a++) {
      bufferHTML += "<h2>";
      bufferHTML += atuadores[a].nome;
      bufferHTML += " <a href=\"/set";
      bufferHTML += atuadores[a].nome;
      bufferHTML += "=1\">Ligar</a>  - <a href=\"/set";
      bufferHTML += atuadores[a].nome;
      bufferHTML += "=0\">Desligar</a></h2><br><br>\n";
    }

    enviaRespostaHTTP(client, bufferHTML);
  } else if (URL.startsWith("GET /set")) {
    // Comandos HTTP
    int posIgual = URL.indexOf("=");

    if(posIgual > 0) {
      Atuador *at = getAtuador(URL.substring(8, posIgual));
      if(at) {
        enviaRespostaHTTP(client, "Setando Atuador");
        
        // SUPORTE APENAS A VALORES BOLEANOS POR ENQUANTO
        if (URL.substring(posIgual + 1, posIgual + 2) == "1") at->liga(1, "Comando Web");
        else                                                  at->desliga(1, "Comando Web");
      } else {
        enviaRespostaHTTP(client, "atuador nao encontrado");
      }
    } else {
      enviaRespostaHTTP(client, "comando SET invalido");
    }
  } else if(URL != "") {
    // 404 NF
    enviaRespostaHTTP(client, "NF", 404);
  } else {
    enviaRespostaHTTP(client, "I'm a teapot", 418);
  }
  // *******************************************************************************

  // close the connection:
  client.stop();
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















int setSensor(char *nomeIO, int _valorRAW) {
  if(!strcmp(nomeIO, "VCC")) {
    VCC = _valorRAW;
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
Atuador *getAtuador(String nome) {
  for(int a=0; a<TOTAL_ATUADORES; a++) {
    if(atuadores[a].nome == nome || atuadores[a].nomeIO == nome) {
      return &atuadores[a];
    }
  }

  // Erro??!?
  return (Atuador *)NULL;
}







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

  //msgPC("bufferSerial cmd get: [%s]", bufferSerial);

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


  
/*
  // Sensor de agua1
  float sensorAgua1 = getSensor("nivelTanque1")->valor;
  if ( sensorAgua1 < (VCC / 2) ) tempoSemAgua1 = 0;                        // Temos Agua
  else                           tempoSemAgua1 += tempoDesdeUltimaMedicao; // Acumular o tempo que estamos sem agua
  // Sensor de agua2
  float sensorAgua2 = getSensor("nivelTanque2")->valor;
  if ( sensorAgua2 < (VCC / 2) ) tempoSemAgua2 = 0;
  else                           tempoSemAgua2 += tempoDesdeUltimaMedicao;
*/

  float temperatura = getSensor("temperatura")->valor;
  // Tempo1 com temperatura Baixa
  if ( temperatura >= TEMPERATURA_MINIMA1 ) tempoTempBaixa1  = 0;
  else                                      tempoTempBaixa1 += tempoDesdeUltimaMedicao;
  // Tempo1 com temperatura Alta
  if ( temperatura <= TEMPERATURA_MAXIMA1 ) tempoTempAlta1  = 0;
  else                                      tempoTempAlta1 += tempoDesdeUltimaMedicao;

  // Tempo2 com temperatura Baixa
  if ( temperatura >= TEMPERATURA_MINIMA2 ) tempoTempBaixa2  = 0;
  else                                      tempoTempBaixa2 += tempoDesdeUltimaMedicao;
  // Tempo2 com temperatura Alta
  if ( temperatura <= TEMPERATURA_MAXIMA2 ) tempoTempAlta2  = 0;
  else                                      tempoTempAlta2 += tempoDesdeUltimaMedicao;
}
#define TEMPO_REFRESH_SENSORES 2000
Task thread_atualizaSensores(TEMPO_REFRESH_SENSORES, atualizaSensores);








void atualizaAtuadores(Task *t=NULL) {
  //msgPC("refresh do estado dos Atuadores");
  for(int a=0; a<TOTAL_ATUADORES; a++) {
    //msgPC("setar Atuador \"%s\"(%s) = [%d]", atuadores[a].nome.c_str(), atuadores[a].nomeIO.c_str(), atuadores[a].valor);
//if(TIPO_RELE)
    atuadores[a].valor ?
      atuadores[a].liga(1, "refresh") :
      atuadores[a].desliga(1, "refresh");
  }
}
#define TEMPO_REFRESH_ATUADORES 33000
Task thread_atualizaAtuadores(TEMPO_REFRESH_ATUADORES, atualizaAtuadores);














// Atua conforme o estado dos sensores (envia alertas via HHTPs)
void confereAlertas(Task *t = NULL) {
  int msecAtual = millis();

  // Luz Plantas
  int tempo = (timeClient.getHours() * 100) + timeClient.getMinutes();

  char luzPlantas = ( tempo < TEMPO_LUZ_PLANTAS_LIGA ) ? LOW : (( tempo >= TEMPO_LUZ_PLANTAS_DESLIGA ) ? LOW : HIGH);
  luzPlantas ?
    getAtuador("luzesPlantas")->liga(0, "De dia!") :
    getAtuador("luzesPlantas")->desliga(0, "De noite!");

  /*  // Flag de falta de agua
    faltaAgua = (tempoSemAgua1 > MAX_TEMPO_SEM_AGUA);

    if ( faltaAgua ) {
      if (tsAcessoAPI < msecAtual) {
        tsAcessoAPI = msecAtual + 10000;

        Serial.println("ALERTAAAAAAA");
        //pushbulletAPI();
      }
      //} else {
      //tsAcessoAPI = 0;
    }*/


  // Aquecedor 1
  if(tempoTempBaixa1 > MAX_TEMPO_TEMPERATURA_FORA_DA_FAIXA)     getAtuador("aquecedor150W1")->liga(0, "Temperatura1 Baixa");
  else if(tempoTempAlta1 > MAX_TEMPO_TEMPERATURA_FORA_DA_FAIXA) getAtuador("aquecedor150W1")->desliga(0, "Temperatura1 Alta");


  // Aquecedor 2
  if(tempoTempBaixa2 > MAX_TEMPO_TEMPERATURA_FORA_DA_FAIXA)     getAtuador("aquecedor150W2")->liga(0, "Temperatura2 Baixa");
  else if(tempoTempAlta2 > MAX_TEMPO_TEMPERATURA_FORA_DA_FAIXA) getAtuador("aquecedor150W2")->desliga(0, "Temperatura2 Alta");
}
#define TEMPO_CONFERE_ALERTAS 3000
Task thread_confereAlertas(TEMPO_CONFERE_ALERTAS, confereAlertas);










void setup() {
  // Comunicacao serial para ler sensores do Arduino Leonardo
  Serial1.begin(115200, SERIAL_8N1, 0, 2);

  Serial.begin(9600);
  Serial.println();
  Serial.println();

  for (int c = 0; c < TAMANHO_BUFFER_MEDIA_PH; c++)
    bufferPH[c] = 7.0;

  // Inicializar o display
  initUI(500);

  ui.switchToFrame(0); // Logo WiFi
  // init WiFi
  initWifi();

  ui.transitionToFrame(1); // Mostrar IP
  delayUI(1200);

  SoftTimer.add(&thread_atualizaSensores);
  SoftTimer.add(&thread_atualizaAtuadores);
  SoftTimer.add(&thread_confereAlertas);
  SoftTimer.add(&thread_httpServer);
  SoftTimer.loop();

  ui.transitionToFrame(NUM_CENA_DEFAULT); // Tela Principal

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
    msgPC("     Enviando [%s] para Leo", buffCmd);
    
  Serial1.println(buffCmd);
  delay(10);

  for (int c = 0; c < TAMANHO_BUFFER_SERIAL; c++)
    bufferSerial[c] = '\0';

  timeout = millis() + 500;
  do {
    if (Serial1.available()) {
      cntBufferSerial++;
      bufferSerial[cntBufferSerial] = Serial1.read();
      timeout = millis() + 150;
    } else
      delay(1);
  } while (millis() < timeout && (cntBufferSerial < 0 || bufferSerial[cntBufferSerial] != '\n') && cntBufferSerial < TAMANHO_BUFFER_SERIAL - 1);

  bufferSerial[cntBufferSerial + 1] = '\0';

  if(strcmp(fmtCmd, "get"))
    msgPC("       RESP: [%s]", bufferSerial);
  
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
