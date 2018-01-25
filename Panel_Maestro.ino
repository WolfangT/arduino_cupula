// Panel_Maestro

#include <Wire.h>
#include <EEPROM.h>


const int  SWI = 2; //boton1, giro izquierda de cupula
const int  SWD = 3; //boton2, giro derecha de cupula
const int  SWV = 4; //boton3, ventana (sioloioogkjjjjjjju apreto una vez abre, si vuelvo a apretar cierra)
const int  SWO = 5; //no se usara
const int  SEL = 6; //switch selector modo manual/automatico
const int  EME = 7; //switch parada de emergencia,stop
const int  SIR = 8;
const int  LED = 13;gvgvr
const int  CLK = A0;
const int  UPD = A1;
const int  CLR = A2;
const int  PRE = A3;

int GradosM, GradosMA, OP, x = 0;
bool Boton1, Boton2, Boton3, Boton4, Modo, Correcion,
  Boton1A, Boton2A, Boton3A, Boton4A, ModoA, CorrecionA,
  Calibracion, CalibracionA, STOP, STOPA,
  tipo_automatico = false; //tipo_automatico: cero es seguimiento y uno es remoto
unsigned long TiempoA, TiempoP, TiempoPB = 0;
unsigned long  ultimoM=0;
const int Intervalo = 100;
const int IntervaloB = 200;
const int IntervaloM = 1000;
float GradosR, FCI, FCD, Operacion = 0.0;
int ventana = 0;
float telescopio = 0.0;
const int freqDebug = 10;
int conteoDebug = 0;


String getValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;
  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i+1 : i;
      }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void sendCommand(int OP) {
  Wire.beginTransmission(2);
  Wire.write(OP);
  Wire.endTransmission();
}

void setup() {
  pinMode(SWI,INPUT);
  pinMode(SWD,INPUT);
  pinMode(SWV,INPUT);
  pinMode(SWO,INPUT);
  pinMode(SEL,INPUT);
  pinMode(EME,INPUT);
  pinMode(CLK,OUTPUT);
  pinMode(UPD,OUTPUT);
  pinMode(CLR,OUTPUT);
  pinMode(PRE,OUTPUT);
  pinMode(SIR,OUTPUT);
  pinMode(LED,OUTPUT);

  digitalWrite(CLR,HIGH);
  delayMicroseconds(500);
  digitalWrite(CLR ,LOW);

  Wire.begin();
  Serial.begin(9600);
}

void loop() {
  TiempoA = millis();
    // Lectura de botones almenos cada 100ms
  if (TiempoA-TiempoP >= Intervalo) {
    TiempoP = TiempoA;
    Boton1 = digitalRead(SWI); //boton1, giro izquierda de cupula
    Boton2 = digitalRead(SWD); //boton2, giro derecha de cupula
    Boton3 = digitalRead(SWV); //boton3, ventana (si apreto una vez abre, si vuelvo a apretar cierra)
    Modo = digitalRead(SEL); //switch selector modo manual/automatico
    STOP = digitalRead(EME); //switch parada de emergencia,stop
    // STOP
    if (STOPA != STOP) {
      STOPA = STOP;
      if (STOP) sendCommand(9);
      else sendCommand(10);
    }
    // Cambio modo
    if (ModoA != Modo){
      Serial.println("modo:"+String(Modo));
      if (Modo==0) sendCommand(11);
      if (Modo==1) sendCommand(12);
      ModoA = Modo;
    }
    // Control Panel
    if (Modo==0) {
      if (Boton1 == true && Boton1A != Boton1) {
        Boton1A = Boton1;
        sendCommand(1);
      }
      if (Boton1 == false && Boton1A != Boton1) {
        Boton1A = Boton1;
        sendCommand(2);
      }
      if (Boton2 == true && Boton2A != Boton2) {
        Boton2A = Boton2;
        sendCommand(3);
      }
      if (Boton2 == false && Boton2A != Boton2) {
        Boton2A = Boton2;
        sendCommand(4);
      }
      if (Boton3 == true && Boton3A != Boton3) {
        Boton3A = Boton3;
        if (ventana) sendCommand(6);
        else sendCommand(5);
      }
      if (Boton3 == false && Boton3A != Boton3) {
        Boton3A = Boton3;
      }
    }
  }
  // Cada segundo envia a pc modo
  if (ultimoM-TiempoA >= IntervaloM){
    ultimoM = TiempoA;
    Serial.println("modo:"+String(Modo));
    Serial.println("modo_automatico:"+String(tipo_automatico));
  }
  // Comunicacion con pc
  // Recibe datos del pc y verfica su contenido
  if (Serial.available()) {
    // Codigo que la pc envia al arduino para que ejecute
    String data = Serial.readStringUntil('\n');
    data.trim();
    String comando = getValue(data, ':', 0);
    String valor = getValue(data, ':', 1);
    if (Modo==1) {
      if (comando == "modo_automatico") {
        if (valor == "seguimiento") {
          sendCommand(12);
          tipo_automatico=0;
        }
        else if (valor == "remoto") {
          sendCommand(13);
          tipo_automatico=1;
        }
        Serial.println("modo_automatico:"+String(tipo_automatico));
      }
      else if (comando == "ventana") {
        if (valor == "abrir") sendCommand(5);
        else if (valor == "cerrar") sendCommand(6);
      }
      else if (comando == "cupula") {
        int c = int(valor.toFloat() * 10);
        // Escribe comando del OP y el Entero del float de la cupula
        byte DE[3] = {highByte(c), lowByte(c)};
        Wire.beginTransmission(2);
        Wire.write(14);
        Wire.write(DE, 2);
        Wire.endTransmission();
      }
    }
  }
  // Lectura del valor enviado por el esclavo cada 300ms
  if (TiempoA-TiempoPB >= IntervaloB) {
    TiempoPB=TiempoA;
    Wire.requestFrom(2, 6); //Solicita 6 bytes (INT) del Arduino NANO
    byte DR[6];
    for (int i = 0; i < 6; i++) {
      DR[i] = Wire.read();
    }
    GradosR = word(DR[0], DR[1]) / 100.0; //Valor del angulo con dos decimales
    ventana = word(DR[2], DR[3]);
    telescopio = word(DR[4], DR[5]) / 100.0;
    Serial.println("cupula:"+String(GradosR));
    Serial.println("telescopio:"+String(telescopio));
    Serial.println("ventana:"+String(ventana));
    if (conteoDebug >= freqDebug) {
      Serial.println("debug:El maestro recibio datos del esclavo y su valor es: <C:" +
        String(GradosR) + ", T:" + String(telescopio) + ", V:" + String(ventana) + ">");
      conteoDebug = 0;
    }
    else conteoDebug++;
  }
  //-----Logica 7-seg---------
  if (GradosR > 0 && GradosR < 360) {
    GradosM = GradosR;
    if (GradosM != GradosMA) {
      if (GradosMA > GradosM) {
        Operacion = GradosMA-GradosM;
        x = int(Operacion);
        digitalWrite(UPD, LOW);
        delayMicroseconds(100);
        for (int i=0; i<x; i++) {
          digitalWrite(CLK, HIGH);
          delayMicroseconds(100);
          digitalWrite(CLK, LOW);
        }
      }
      if (GradosMA < GradosM) {
        Operacion = GradosM-GradosMA;
        x = int(Operacion);
        digitalWrite(UPD, HIGH);
        delayMicroseconds(100);
        for(int i=0; i<x; i++) {
          digitalWrite(CLK, HIGH);
          delayMicroseconds(100);
          digitalWrite(CLK, LOW);
        }
      }
      GradosMA = GradosM;
    }
  }
  //--------------------Final Logica 7Seg-------------
}
//----------------END------------------------------
