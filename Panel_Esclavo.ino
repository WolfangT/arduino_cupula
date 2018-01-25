// Panel_Esclavo

#include <Wire.h>
#include <EEPROM.h>

const int histerisis = 5;

//const int SEN = 2 // sensor del triac
const int FC  = 8; // final de carrera para marcar 0 grados
const int MVI = 3; // rele para mover la cupula a la izquierda
const int MVD = 4; // rele para mover la cupula a la derecha
const int ACT = 5; // rele de activacion de bobina de motor cupula
//const int TRI = 6 // no se usa
const int ENC = 7; // Sensor encoder

bool Sensor, SensorA, FinalC, Fase, FaseA, Test, InicioA, \
    InicioB, InicioC, END, ONIZQ, ONDER, Correcion, FCA, STOP, \
    Activador, Cfase, Secuencia, Secuencia2, Activador2, \
    F1, F2, F3, F4 = false;
unsigned long TiempoA, TiempoP, TiempoPB, TiempoD = 0;

byte Valor;
int MP = 0;
int Nmed = 0;
int Mvalor = 0;
int GiroI = 0;
int GiroD = 0;
int Modo = 1;

unsigned int GradosE = 0;
int Tmax = 40;
int AnguloD = 2;
int GradosD1, GradosD2 = 0;

const int IntervaloB = 50;
float GradosR, FCI, FCD, GradosC, Operacion, Acumulador, \
    FCIr, FCDr, FCIo, FCDo, GradosRA, GradosG = 0;

int ventana = 0;
float telescopio = 0;
float objetivoCupula = 0;

//-------Inicio Funciones---------

void receiveEvent(int howMany) {
  // Recepcion de datos (I2C) // HAY QUE ENVIAR 3 COSAS SOLO PARA EL MODO REMOTO
  // ESO SERIA EL VALOR DE LA INSTRUCCION (ENTERO) EL ESTADO VENTANA (ABRIR/CERRAR)
  // Y EL VALOR ARBITRARIO A COLOCAR LA CUPULA (FLOAT DE 2 DECIMALES)
  int Instruccion = Wire.read(); // receive 1 byte as an integer
  int valor = 0;
  if (howMany > 1) { // recive otros 2 bytes como integer
    byte DR[2];
    for (int i = 0; i < howMany -1; i++) {
      DR[i] = Wire.read();
    }
    valor = word(DR[0], DR[1]);
  }
  // Modos funcionamiento
  if (Instruccion == 11) Modo=1; //Modo Manual
  if (Instruccion == 12) Modo=2; //Modo Auto Seguimiento
  if (Instruccion == 13) Modo=3;//Modo Auto REMOTO
  if (Instruccion == 7) {Modo=4; Activador=true;} // Modo Configuracion
  // Control funciones
  if (Instruccion == 1) {ONIZQ=true; Activador=true;} // LISTO MOV. IZQ
  if (Instruccion == 2) {ONIZQ=false; Activador=true;} // LISTO DET. IZQ
  if (Instruccion == 3) {ONDER=true; Activador2=true;} // LISTO MOV. DER
  if (Instruccion == 4) {ONDER=false; Activador2=true;} // LISTO DET. DER
  if (Instruccion == 9) {STOP=true; Activador=true; Activador2=true;} // Listo ACT. STOP
  if (Instruccion == 10) {STOP=false; Activador=true; Activador2=true;} // LISTO DET. STOP
  // Control ventana
  if (Instruccion == 5) Serial.println("panel:abrir"); // ABRE VENTANA
  if (Instruccion == 6) Serial.println("panel:cerrar"); // CIERRA
  // Control cupula
  if (Instruccion == 14) objetivoCupula = (float) valor / 100.0;
}

void requestEvent() {
    // RESPUESTA AL SERVIDOR I2C ( VALOR REAL DE LA CUPULA, Ventana y TELESCOPIO CON 2 DEC)
    int g = GradosR*100;  // Responde el valor de la variable Grados (ENTERO) 2 BYTES
    int v = ventana; // Responde el valor de la ventana ( Podria se un solo byte pero whatever)
    int t = telescopio*100;  // Responde el valor de la variable Telescopio (ENTERO) 2 BYTES
    byte datosSensores[6] = {highByte(g), lowByte(g), highByte(v), lowByte(v), highByte(t), lowByte(t)};
    Wire.write(datosSensores, 6);
}

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
//-----------------------------------------------END------------------------------------------------------

void setup() {
  pinMode(ENC,INPUT);
  //pinMode(SEN,INPUT);
  pinMode(FC ,INPUT);
  pinMode(MVI,OUTPUT);
  pinMode(MVD,OUTPUT);
  pinMode(ACT,OUTPUT);
  //pinMode(TRI,OUTPUT);

  SensorA = digitalRead(ENC);
  //FaseA = digitalRead(SEN);
  Nmed = EEPROM.read(11);

  for (int i=1; i<=Nmed; i=i+2) {
      Valor = EEPROM.read(i);
      FCI = (FCI + int(Valor)/100);
      Acumulador++;
  }
  if (Acumulador > 0) {
    FCI = FCI / Acumulador;
    Acumulador = 0;
    Valor=0;
  }
  for (int i=2; i<=Nmed; i=i+2) {
    Valor = EEPROM.read(i);
    FCD = (FCD + int(Valor)/100);
    Acumulador++;
  }
  if (Acumulador > 0) {
    FCD=FCD/Acumulador;
    Acumulador=0;
    Valor=0;
  }
  Serial.begin(9600);
  Wire.begin(2);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);
}

void ModoConf () {
  //--------Inicio MODO CONF.----------------
  if(Activador == true) {
    Valor = EEPROM.read(11);
    Nmed = Valor;
    Activador=false;
    Secuencia=true;
    ONIZQ=true;
    if (Nmed == 10) {
      Secuencia=false;
      ONIZQ=false;
      Modo=0;
    }
  }
  if (ONIZQ == true && STOP == false) {
    if(Secuencia == true) {
      digitalWrite(ACT, HIGH);
      Secuencia = false;
    }
  }
  if (ONIZQ == false && Secuencia == true) {
    digitalWrite(ACT, LOW);
    Secuencia = false;
    Secuencia2 = true;
  }
  if (ONDER == true && STOP == false) {
    if (Secuencia2 == true) {
      digitalWrite(MVI, HIGH);
      digitalWrite(MVD, HIGH);
      digitalWrite(ACT, HIGH);
      Secuencia2 = false;
    }
  }
  if (ONDER == false && Secuencia2 == true) {
    digitalWrite(MVI, LOW);
    digitalWrite(MVD, LOW);
    digitalWrite(ACT, LOW);
    Secuencia2 = false;
    FCIr = 360/GiroI;
    FCIo = FCIr*100;
    FCI = int(FCIo);
    FCI = byte(FCD);
    FCDr = 360/GiroD;
    FCDo = FCDr*100;
    FCD = int(FCDo);
    FCD = byte(FCD);
    EEPROM.write(Nmed+1, FCI);
    EEPROM.write(Nmed+2, FCD);
    Nmed = Nmed+1;
    EEPROM.write(11, Nmed);
    Modo=0;
  }
  //------Fin Modo CONF. --------------------
}

void ModoManual () {
  //------------Inicio MODO MANUAL------------
  if (ONIZQ == true && STOP == false) {
    if (Activador == true) {
      digitalWrite(ACT, HIGH);
      Activador = false;
    }
  }
  if (ONIZQ == false && Activador == true) {
    digitalWrite(ACT, LOW);
    Activador = false;
  }
  if (ONDER == true && STOP == false) {
    if (Activador2 == true) {
      digitalWrite(MVI, HIGH);
      digitalWrite(MVD, HIGH);
      digitalWrite(ACT, HIGH);
      Activador2 = false;
    }
  }
  if (ONDER == false && Activador2 == true) {
    digitalWrite(MVI, LOW);
    digitalWrite(MVD, LOW);
    digitalWrite(ACT, LOW);
    Activador2 = false;
  }
  // ------------Final MODO MANUAL------------
}

void ModoAutomatico (float angulo) {
  // ------------Inicio AUTO SEGUIMIENTO------------
  if(angulo-GradosR<0 && angulo-GradosR>-histerisis && F1==false) {
    F1 = true;
    F2 = false;
    F3 = false;
    F4 = false;
    digitalWrite(MVD,LOW);
    digitalWrite(MVI,LOW);
    digitalWrite(ACT,LOW);
  }
  if (angulo-GradosR<-histerisis && F2==false) {
    F1 = false;
    F2 = true;
    F3 = false;
    F4 = false;
    digitalWrite(MVD, HIGH);
    digitalWrite(MVI, HIGH);
    digitalWrite(ACT, HIGH);
  }
  if (angulo-GradosR>0 && angulo-GradosR<histerisis && F3==false) {
    digitalWrite(ACT, LOW);
    F1 = false;
    F2 = false;
    F3 = true;
    F4 = false;
  }
  if (angulo-GradosR>histerisis && F4==false) {
    digitalWrite(ACT, HIGH);
    F1 = false;
    F2 = false;
    F3 = false;
    F4 = true;
  }
  // ------------Fin AUTO SEGUIMIENTO------------
}

void loop() {
  TiempoA = millis();
  // Lectura de encoder y Final de carrera almenos cada 100ms (Control Posicion)
  if (TiempoA-TiempoPB >= IntervaloB) {
    TiempoPB = TiempoA;
    Sensor = digitalRead(ENC);
    FinalC = digitalRead(FC);
    if(Sensor != SensorA) {
      SensorA = Sensor;
      if (ONIZQ==true && Modo!=4) GradosR = GradosR-FCI;
      if (ONDER==true && Modo!=4) GradosR = GradosR+FCD;
      if (ONIZQ==true && Modo==4) GiroI++;
      if (ONDER==true && Modo==4) GiroD++;
    }
  }
  if (FinalC==true && ONIZQ==true && Modo!=4) GradosR=360;
  if (FinalC==true && ONDER==true && Modo!=4) GradosR=0;
  if (FinalC==true && ONIZQ==true && Modo==4) {ONIZQ=false; Secuencia=true;}
  if (FinalC==true && ONDER==true && Modo==4) {ONDER=false; Secuencia2=true;}
  //----------------------- Final Lectura de ENTRADAS-----------------------------------

  //-------Parada de Emergencia--------------
  if (STOP==true && Activador==true)  {
    // STOP REGENERATIVO ( PERMITE ACTIVAR UNA VEZ DESELECIONADO)
    Activador = false;
    digitalWrite(MVD, LOW);
    digitalWrite(MVI, LOW);
    digitalWrite(ACT, LOW);
  }
  //------- Final Parada de Emergencia---------

  //---------- Control Funcionamiento ------------
  switch (Modo) {
    case 1:
      ModoManual();
      break;
    case 2:
      ModoAutomatico(telescopio);
      break;
    case 3:
      ModoAutomatico(objetivoCupula);
      break;
    case 4:
      ModoConf();
      break;
  }
  //---------- Fin Funcionamiento ------------

  // -----------Recepcion de datos desde el xbee --------
  while (Serial.available()) {
    String data = Serial.readStringUntil('\n');
    data.trim();
    String origen = getValue(data, ':', 0);
    String valor = getValue(data, ':', 1);
    if (origen == "ventana") {
      ventana = valor.toInt();
    }
    else if (origen == "telescopio") {
      telescopio = valor.toFloat();
    }
  }
  // -----------Fin de Recepcion de datos desde el xbee --------

} //Final  Programa Principal ( LOOP)
