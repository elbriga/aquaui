#define VERSAO 99       // Versão 0.99
#define VCC     5 * 100 // 5V * 100 = 2 casas decimais de precisao nos sensores

#define FREQUENCIA_CHECK_SERIAL   5  // ms
#define FREQUENCIA_CHECK_SENSORES 25 // ms

#define TOTAL_RELES 8
int pinosReles[TOTAL_RELES] = { 4,5,6,7, 12,13, -1,-1 };

#include <stdarg.h>
void msgPC(char *fmt, ... );



#include <IRRemoteControl.h>
IRRecv irRecv;
IRSend irSend;



#include <Task.h>
#include <SoftTimer.h>




// Sensores OneWire
#include <OneWire.h>
#include <DallasTemperature.h>
// Data wire is conntec to the Arduino digital pin 0
#define ONE_WIRE_BUS 11
// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);
  // Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);




// Variaveis GLOBAIS
#define TOTAL_SENSORES_POR_TIPO 6
volatile int analog[TOTAL_SENSORES_POR_TIPO];
volatile int sensoresOW[TOTAL_SENSORES_POR_TIPO];
volatile int digital[TOTAL_SENSORES_POR_TIPO];

#define TOTAL_SENSORES (TOTAL_SENSORES_POR_TIPO * 3)




volatile unsigned short idxSensorPooling = 1000;
void leSensores_cb(Task *t) {
  idxSensorPooling++;
  if(idxSensorPooling >= TOTAL_SENSORES) idxSensorPooling = 0;

  if(idxSensorPooling < TOTAL_SENSORES_POR_TIPO) {
    int analogPin = idxSensorPooling;
    
    analog[analogPin] = ((analogRead(analogPin) / 1024.0) * 500.0);
  }
  else if(idxSensorPooling < (TOTAL_SENSORES_POR_TIPO * 2)) {
    int oneWireNum = idxSensorPooling - TOTAL_SENSORES_POR_TIPO;
    
    // OneWire
    sensors.requestTemperatures();
    float valor = sensors.getTempCByIndex(oneWireNum);
    sensoresOW[oneWireNum] = (valor != -127) ? (valor * 100.0) : -127;
  }
  else if(idxSensorPooling < (TOTAL_SENSORES_POR_TIPO * 3)) {
    int sBoolNum = idxSensorPooling - (TOTAL_SENSORES_POR_TIPO * 2);

    digital[sBoolNum] = 0;
  }
}
Task task_leSensores(FREQUENCIA_CHECK_SENSORES, leSensores_cb);









void imprimeDecimalNasSeriais(int valor, const char *prefixoNomeIO, int numNomeIO, int virgula=1) {
  char strbuff[32];
  
  if(numNomeIO >= 0) sprintf(strbuff, "%s%d:%d", prefixoNomeIO, numNomeIO, valor);
  else               sprintf(strbuff, "%s:%d",   prefixoNomeIO, valor);
  
  Serial.print(strbuff);
  Serial1.print(strbuff);

  if(virgula) {
    Serial.print(",");
    Serial1.print(",");
  }
}
void imprimeVariaveisNasSeriais() {
  msgPCsemNL("Enviando \"{");

  int usI = millis();
  Serial1.print("{");

  imprimeDecimalNasSeriais(VERSAO, "VER", -1);
  imprimeDecimalNasSeriais(VCC,    "VCC", -1);
  
  for(int analogPin=0; analogPin < TOTAL_SENSORES_POR_TIPO; analogPin++) {
    imprimeDecimalNasSeriais(analog[analogPin], "ADC", analogPin);
  }
  for(int oneWireIndex=0; oneWireIndex < TOTAL_SENSORES_POR_TIPO; oneWireIndex++) {
    imprimeDecimalNasSeriais(sensoresOW[oneWireIndex], "OW", oneWireIndex + 1);
  }
  for(int digitalPin=0; digitalPin < TOTAL_SENSORES_POR_TIPO; digitalPin++) {
    imprimeDecimalNasSeriais(digital[digitalPin], "DIG", digitalPin + 1, (digitalPin < TOTAL_SENSORES_POR_TIPO-1));
  }
  
  Serial1.println("}");
  
  msgPC("}\" para o WEMOSLOLIN32 (%d ms)", (millis() - usI));
}












void processaComando(char *origem, char *comando) {
  msgPC("[org:%s] comando: [%s]", origem, comando);

  if(!strncmp(comando, "get", 3)) {
    imprimeVariaveisNasSeriais();
  }

  
  else if(!strncmp(comando, "set", 3)) {
    char *cmd   = strtok(&comando[3], "=");
    char *valor = strtok(NULL, "\n\0");
    
    //msgPC("SETAR[%s]!!!", comando);

    if(!cmd || !valor) {
      //msgPC("Erro de parse");
      return;
    }

    if(!strncmp(cmd, "RELE", 4)) {
      short numRele = atoi(&cmd[4]) - 1;

      if (numRele >= 0 && numRele <= 7) {
        int pin = pinosReles[numRele];
        if(pin >= 0) {
          int estado = (valor[0] == '1');
          imprimeDecimalNasSeriais(estado, cmd, -1, 0);
          digitalWrite(pin, estado);
        } else {
          imprimeDecimalNasSeriais(1, "ERR", -1, 0);
        }
      } else {
        msgPC("Rele %d nao existe!", numRele);
        imprimeDecimalNasSeriais(2, "ERR", -1, 0);
      }
    } else {
      msgPC("SETAR[%s] -> desconhecido!!!", comando);
    }
  }


  else if(!strncmp(comando, "ver", 3)) {
    imprimeDecimalNasSeriais(VERSAO, "VER", -1);
  }

  
  else {
    msgPC("Comando [%s] Desconhecido!", comando);
  }
}




void checkSerial_cb() {
  if (!Serial.available()) return;

  int c = -1;
  char inBuff[32];
  do {
    c++;
    inBuff[c] = Serial.read();
  } while  (Serial.available() && inBuff[c] != '\n' && c < 31); // overflow?
  inBuff[c+1] = '\0';

  processaComando("Serial", inBuff);
}
Task task_checkSerial(FREQUENCIA_CHECK_SERIAL*2, checkSerial_cb);

void checkSerial1_cb() {
  if (!Serial1.available()) return;

  int c = -1;
  char inBuff[32];
  do {
    c++;
    inBuff[c] = Serial1.read();
  } while  (Serial1.available() && inBuff[c] != '\n' && c < 31); // overflow?
  inBuff[c+1] = '\0';

  processaComando("Serial1", inBuff);
}
Task task_checkSerial1(FREQUENCIA_CHECK_SERIAL, checkSerial1_cb);




void setup() {
  int timeout = millis() + 50;
  Serial.begin(9600); // USB PC
  while (!Serial && (millis() < timeout)) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  
  // start serial port at 115200 bps LINK WEMOS:
  Serial1.begin(115200);

  // Sensores OneWire em IO2
  sensors.begin();

  int pin;
  for(pin=0; pin<TOTAL_SENSORES_POR_TIPO; pin++)
    analog[pin] = 0;
  for(pin=0; pin<TOTAL_SENSORES_POR_TIPO; pin++)
    sensoresOW[pin] = 0;
  for(pin=0; pin<TOTAL_SENSORES_POR_TIPO; pin++)
    digital[pin] = 0;

  for(int r=0; r<TOTAL_RELES; r++) {
    pin = pinosReles[r];
    if(pin > 0)
      pinMode(pin, OUTPUT);
  }

  SoftTimer.add(&task_leSensores);
  SoftTimer.add(&task_checkSerial);
  SoftTimer.add(&task_checkSerial1);

  delay(500);
  
  msgPC("=======");
  msgPC("init OK");
  msgPC("=======");
}

void loop() {
  SoftTimer.loop();
}





void msgPC(char *fmt, ... ) {
        char buf[128]; // resulting string limited to 128 chars
        va_list args;
        va_start (args, fmt );
        vsnprintf(buf, 128, fmt, args);
        va_end (args);

        strcat(buf, "\n");
        
  Serial.print(buf);
}
void msgPCsemNL(char *fmt, ... ) {
        char buf[128]; // resulting string limited to 128 chars
        va_list args;
        va_start (args, fmt );
        vsnprintf(buf, 128, fmt, args);
        va_end (args);
        
  Serial.print(buf);  
}

