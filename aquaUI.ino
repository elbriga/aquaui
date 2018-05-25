// WEMOS LOLIN32 - 80MHz

#define MAX_TEMPO_SEM_AGUA 5000 // ms

#define PINO_SENSOR_1 39 // ADC3
#define PINO_SENSOR_2 36 // ADC0

// Sensores
int sensor1 = 0, sensor2 = 0, sensor3 = 0;
int tempoSemAgua = 0, faltaAgua = 0;

int tsAcessoAPI = 0;


// DISPLAY ******************************************************************
// Include the correct display library, connection via I2C using Wire include
#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306Wire.h" // legacy include: `#include "SSD1306.h"`
// Include the UI lib
#include "OLEDDisplayUi.h"
// Include custom images
#include "images.h"

// Initialize the OLED display using Wire library
SSD1306Wire   display(0x3c, 5, 4);
OLEDDisplayUi ui(&display);

// OVERLAY - frame sempre visivel
void msOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
  display->setFont(ArialMT_Plain_10);

  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0, 0, "s1:" + String(sensor1));
  display->drawString(40, 0, "ml:" + String(millis()));

  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawString(128, 0,  "s2:" + String(sensor2));
}

void drawFrame1(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->drawXbm(x + 34, y + 14, WiFi_Logo_width, WiFi_Logo_height, WiFi_Logo_bits);
}

void drawFrame2(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  String alerta1 = !faltaAgua ? String("-") : String("Sem Agua!");

  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_16);
  display->drawString(0 + x, 20 + y, "Alerta: " + alerta1);
}

void drawFrame3(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setFont(ArialMT_Plain_10);

  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0 + x, 11 + y, "Aquaponia");

  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawString(128 + x, 33 + y, "DiCasa");
}

void drawFrame4(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_10);
  display->drawStringMaxWidth(0 + x, 10 + y, 128, "Central de monitoramento do ecossistema integrado. Tamos ai. PAZ");
}

void drawFrame5(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {

}

// This array keeps function pointers to all frames
// frames are the single views that slide in
FrameCallback frames[] = { drawFrame1, drawFrame2, drawFrame3, drawFrame4, drawFrame5 };
// how many frames are there?
int frameCount = 5;

// Overlays are statically drawn on top of a frame eg. a clock
OverlayCallback overlays[] = { msOverlay };
int overlaysCount = 1;

void initUI() {
  Serial.println("Inicializando display");
  
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
  ui.setFrames(frames, frameCount);

  // Add overlays
  ui.setOverlays(overlays, overlaysCount);

  // Initialising the UI will init the display too.
  ui.init();

  //display.flipScreenVertically();

  ui.switchToFrame(0);
  ui.disableAutoTransition();

  ui.update();
  Serial.print("Display inicializado: ");
  Serial.print(ui.update());
  Serial.println("!!!");
}
// ***************************************************************************










// WIFI ******************************************************************
#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid     = "foraTemer";
const char* password = "morcegopreto";
String IP;
WiFiServer server(80);
HTTPClient http;
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
  // We start by connecting to a WiFi network
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  IP = WiFi.localIP().toString();
  server.begin();
}

void handleHTTP() {
  WiFiClient client = server.available();   // listen for incoming clients
  if (!client) return;

  // if you get a client,
  Serial.println("New Client.");           // print a message out the serial port
  String currentLine = "";                // make a String to hold incoming data from the client
  while (client.connected()) {            // loop while the client's connected
    if (client.available()) {             // if there's bytes to read from the client,
      char c = client.read();             // read a byte, then
      Serial.write(c);                    // print it out the serial monitor
      if (c == '\n') {                    // if the byte is a newline character

        // if the current line is blank, you got two newline characters in a row.
        // that's the end of the client HTTP request, so send a response:
        if (currentLine.length() == 0) {
          // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
          // and a content-type so the client knows what's coming, then a blank line:
          client.println("HTTP/1.1 200 OK");
          client.println("Content-type:text/html");
          client.println();

          // the content of the HTTP response follows the header:
          client.print("Click <a href=\"/H\">here</a> to turn the LED on pin 5 on.<br>");
          client.print("Click <a href=\"/L\">here</a> to turn the LED on pin 5 off.<br>");

          // The HTTP response ends with another blank line:
          client.println();
          // break out of the while loop:
          break;
        } else {    // if you got a newline, then clear currentLine:
          currentLine = "";
        }
      } else if (c != '\r') {  // if you got anything else but a carriage return character,
        currentLine += c;      // add it to the end of the currentLine
      }

      // Check to see if the client request was "GET /H" or "GET /L":
      if (currentLine.endsWith("GET /H")) {
        digitalWrite(5, HIGH);               // GET /H turns the LED on
      }
      if (currentLine.endsWith("GET /L")) {
        digitalWrite(5, LOW);                // GET /L turns the LED off
      }
    }
  }

  // close the connection:
  client.stop();
  Serial.println("Client Disconnected.");
}


void pushbulletAPI() {
  if ((WiFi.status() != WL_CONNECTED)) { //Check the current connection status
    Serial.println("pushbulletAPI: Wifi n conectado");
    return;
  }

  http.begin("https://api.pushbullet.com/v2/pushes", root_ca);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Access-Token", "o.nsMkSWjqnZT4E8A2b1HDdUgtnMW0GiAX");
  int httpCode = http.POST("{\"body\":\"Esta faltando agua\",\"title\":\"Alertas\",\"type\":\"note\"}");

  if (httpCode > 0) { //Check for the returning code
    String payload = http.getString();
    Serial.println(httpCode);
    Serial.println(payload);
  } else {
    Serial.println("Error on HTTP request");
    Serial.println(httpCode);
  }

  http.end(); //Free the resources
}
// *********************************************************************************



int msecSensores;
void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();

  pinMode(39, INPUT);
  pinMode(36, INPUT);
  pinMode(0,  INPUT);

  initUI();

  for(int c=0; c<30; c++) {
    int d = ui.update();
    delay(d);
  }
 
  initWifi();

  Serial.println("init OK");
  msecSensores = millis();
}

void atualizaSensores() {
  int agora = millis();
  int tempoDesdeUltimaMedicao = agora - msecSensores;
  msecSensores = agora;

  // Limitar o "pooling" dos sensores aqui com 'tempoDesdeUltimaMedicao'
  //if(tempoDesdeUltimaMedicao < delayPoolingSensores) return;

  // ATUALIZAR os ADCs
  sensor1 = (int)((analogRead(PINO_SENSOR_1) / 4096.0) * 350.0);
  sensor2 = (int)((analogRead(PINO_SENSOR_2) / 4096.0) * 350.0);


  // Sensor de agua ************************************************************************************************************
  int temAgua = 1;// (sensor1 < 1000);

  if ( temAgua ) {
    // Temos Agua
    tempoSemAgua = 0;
  } else {
    // Acumular o tempo que estamos sem agua
    tempoSemAgua += tempoDesdeUltimaMedicao;
  }

  confereAlertas();
}

void confereAlertas() {
  int msecAtual = millis();

  // Flag de falta de agua
  faltaAgua = (tempoSemAgua > MAX_TEMPO_SEM_AGUA);

  if ( faltaAgua ) {
    if (tsAcessoAPI < msecAtual) {
      tsAcessoAPI = msecAtual + 10000;

      ui.switchToFrame(1);
      pushbulletAPI();
    }
    //} else {
    //tsAcessoAPI = 0;
  }
}

// Cenas
int tempoCena1 = 4000; // 4s na primeira "Cena"
int tempoCena2 = 6000; // 6s na  segunda "Cena"

int duracaoLoop       = tempoCena1 + tempoCena2; // 10s de LOOP
int timePerFrameCena2 = (tempoCena2 / frameCount);
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
}

void loop() {
  confereCenas();

  int remainingTimeBudget = ui.update();
  if (!(remainingTimeBudget > 0)) return; // Dar prioridade para o refresh do display

  int msecInicioProcessamento = millis();

  // Refresh do estado dos sensores e variaveis relacionadas
  atualizaSensores();

  // Atua conforme o estado dos sensores (envia alertas via HHTPs)
  confereAlertas();

  // Processamento de conexoes na 80
  handleHTTP();

  // Descontar do delay o tempo que levamos para processar
  remainingTimeBudget = remainingTimeBudget - (millis() - msecInicioProcessamento);

  // Ao inves de delay podemos ficar "em processamento" ate que acabe o "budget"
  if (remainingTimeBudget > 0)
    delay(remainingTimeBudget);
}
