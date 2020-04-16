//INCLUSÕES DE BIBLIOTECAS
#include <DS3231.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <SD.h>
//**************************************************************

// DEFINIÇÕES
#define endereco      0x27 // Endereços comuns: 0x27, 0x3F
#define colunas       16   // número de colunas do display
#define linhas        2    // número de linhas do display
#define rele          9
#define pinSensor     8
#define vermelho      7
#define amarelo       6
#define verde         5
//**************************************************************

//VARIÁVEIS PARA SENSOR DE FLUXO
float vazao; //Variável para armazenar o valor em L/min
int contaPulso; //Variável para a quantidade de pulsos
float Litros = 0; //Variável para Quantidade de agua
float MiliLitros = 0; //Variavel para Conversão
//**************************************************************

//VARIÁVEIS PARA O CONTADOR
unsigned long AtualMillis;
long AnteriorMillis = 0;
long Intervalo = 0;
int limite = 0;
unsigned long SomaIntervalo = 0;
//**************************************************************


// INSTANCIANDO LCD
LiquidCrystal_I2C lcd(endereco, colunas, linhas);

//ARQUIVO PARA SD CARD
File myFile;

// INSTANCIANDO RTC DS3231
//Uno, Ethernet  A4 (SDA), A5 (SCL)
DS3231  rtc(SDA, SCL);

//FUNÇÕES
bool ativaSensor(int porta);
/*    Pinagem do DK-80
    Preto   :   pino de sinal
    Azul    :   terra
    Marrom  :   VCC
*/
void incpulso ();

void setup()
{  
  // INICIANDO O RTC
  rtc.begin();
  
  /*
  The following lines can be uncommented to set the date and time
  rtc.setDOW(WEDNESDAY);     // Set Day-of-Week to SUNDAY
  rtc.setTime(13, 15, 0);     // Set the time to 12:00:00 (24hr format)
  rtc.setDate(29, 02, 2020);   // Set the date to January 1st, 2014
  */

  //INICIANDO COMUNICAÇÃO SERIAL
  Serial.begin(9600);

  //INICIA MODULO SD CARD E VERIFICA SE A CARTÃO DE MEMÓRIA
  if (!SD.begin(4)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");
  
  //INICIALIZA O SENSOR DE FLUXO
  pinMode(2, INPUT); //Configura o pino 2 como entrada de sinal
  attachInterrupt(digitalPinToInterrupt(2), incpulso, RISING); //Configura o pino 2(Interrupção 0) interrupção

  //iNICIALIZA O DISPLAY
  lcd.init(); // INICIA A COMUNICAÇÃO COM O DISPLAY
  lcd.backlight(); // LIGA A ILUMINAÇÃO DO DISPLAY
  lcd.clear(); // LIMPA O DISPLAY
  
  pinMode(rele, OUTPUT);
  pinMode(verde, OUTPUT);
  pinMode(amarelo, OUTPUT);
  pinMode(vermelho, OUTPUT);
}

void loop()
{ 
  contaPulso = 0;//Zera a variável
  interrupts(); //Habilita interrupção
  delay (1000); //Aguarda 1 segundo
  //noInterrupts(); //Desabilita interrupção

  vazao = contaPulso / 5.5; //Converte para L/min  
  MiliLitros = vazao / 60;
  Litros = Litros + MiliLitros;

  //INICIA CONTADOR
  AtualMillis = millis();
  Intervalo = AtualMillis - AnteriorMillis;
  AnteriorMillis = AtualMillis;
  SomaIntervalo = SomaIntervalo + Intervalo;

  /*STRING QUE CONTÉM DATA, HORA, TOTAL GASTO EM LITROS, VAZÃO ATUAL EM LITROS POR MINUTO*/
  String rtclog = String(rtc.getDateStr()) + ", " + String(rtc.getTimeStr()) + ", " + "Total: " + String(Litros) + " Litros" +
                  ", " + "Vazao: " + String(vazao) + ", " + "L/min";
  
  if (ativaSensor(pinSensor)) { 
         
              digitalWrite(rele,LOW); //ATIVA A TORNEIRA
              
          //*********************************************GRAVANDO DADOS NO CARTÃO DE MEMÓRIA*********************************************
              //CRIANDO ARQUIVO DATA.txt
              myFile = SD.open("DATA.txt", FILE_WRITE);
              if (myFile) {
              Serial.print("gravando");
              myFile.println(rtclog);
              // FECHANDO ARQUIVO
              myFile.close();
              Serial.println("done.");
            } else {
              //REPORTA ERRO SE O ARQUIVO NÃO FOR ABERTO
              Serial.println("error opening DATA.txt");
            }
          //*****************************************************************************************************************************
              
              //IMPRIME INFORMAÇÕES NO LCD
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print(rtc.getTimeStr());
              lcd.setCursor(11, 0);
              lcd.print("ON");
              lcd.setCursor(0, 1);
              lcd.print("Gasto: ");
              lcd.print(Litros);
              lcd.print("L/min");
          
              //DISPARA CONTADOR
              if(SomaIntervalo > 0){
                 digitalWrite(verde,HIGH);
              }
              if(SomaIntervalo > 3000){    
                digitalWrite(amarelo, HIGH);
              }
              if(SomaIntervalo > 6000){
                digitalWrite(vermelho, HIGH);    
              }
              if(SomaIntervalo > 9000){
              /*ETAPA DE BLOQUEIO:*/
                 while (limite == 0){
                    digitalWrite(rele,HIGH); //BLOQUEAR TORNEIRA
                    digitalWrite(verde,LOW);
                    digitalWrite(amarelo,LOW);
                    digitalWrite(vermelho,HIGH);
                    Serial.println("%%%%LIMITE DE USO%%%%");

                    /*PARA SAIR DA ETAPA DE BLOQUEIO CASO O SENSOR DK-80 NÃO IDENTIFIQUE NADA NA ZONA*/
                    if (!ativaSensor(pinSensor)){
                      limite = 1;
                      SomaIntervalo = 0;
                    }
                 }
              }
    } 
    else {
              /*Serial.print(vazao);
              Serial.println(" L/min");
              Serial.print("Data");
              Serial.println(rtc.getDateStr());*/
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print(rtc.getTimeStr());
              lcd.setCursor(11, 0);
              lcd.print("OFF");
              lcd.setCursor(0, 1);
              lcd.print("Gasto: ");
              lcd.print(Litros);
              lcd.print("L/min");
          
              digitalWrite(rele,HIGH);//DESLIGA A TORNEIRA
              digitalWrite(verde,LOW);
              digitalWrite(amarelo,LOW);
              digitalWrite(vermelho,LOW);
              
              Serial.println("Torneira OFF");
              SomaIntervalo = 0;
              limite = 0;
    }
  
  
}

//FUNÇÃO PARA USO DO SENSOR DE OBSTÁCULOS
bool ativaSensor(int porta){    
  bool sensor = digitalRead(porta);
  if (!sensor){
    return true;  
  }
  else{
    return false;
  }
}

//FUNÇÃO PARA USO DO SENSOR DE FLUXO
void incpulso ()
{
 contaPulso++; //Incrementa a variável de pulsos
}
