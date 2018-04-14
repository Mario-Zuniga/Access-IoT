/* Librerias a Utilizar */
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <DS1302.h>
#include <Nokia_5110.h>
#include <SoftwareSerial.h>

/* Definiciones para valores de PINES para componentes */
#define RST_PIN  9    
#define SS_PIN  10  
#define RST 4
#define CE 5
#define DC 6
#define DIN 7
#define CLK 8

/* Simulacion de respuesta de la base de datos para verificaciones */
String info[20] = {"22118022838","1","1227","1600","2411","3821516489","1","1227","1600","2411","11495208","1","1227","1605","2411","0","0","0","0","0"};

String apiKey = "R1VVSJ0C14R19D6D";               /* Llave para acceso a sistema ThingSpeak */
String ssid="Mario";                              /* Nombre de la red */
String password ="hola2hola";                     /* Contraseña de red */

/* Variables globales a utilizar */
/* Banderas */
bool flag_TIME[4] = {false,false,false,false};
bool flag_MODIFY = false;
bool second_try = false;
bool flag_ID = true;

/* Variables tipo INT  */
int get_min_modify = 0;
int current_minute = 0;
int temperatura = 20;
int pulsador1 = 0;
int pulsador = 0;
int j = 0;

/* Variables tipo STRING */
String obtained_ID;
String ID_card;


/* Inicializacion para modulo WiFi */
SoftwareSerial Serial1(3,2);        /* RX|TX */

/* Definiciones para pantalla */
Nokia_5110 lcd = Nokia_5110(RST, CE, DC, DIN, CLK);

/* Definiciones para RTC */
DS1302 rtc(A3, A2, 8);  //Pin definiton RTC
Time t;
 
/* Creacion de objeto para lectura de credenciales */
MFRC522 Lector1(SS_PIN, RST_PIN); ///Creamos el objeto para el RC522 al cual llamamos Lector1


/* Configuracion inicial */
void setup() {
   /* Tanto el serial de Arduino como del ESP deben empezar en 9600 */
  Serial.begin(9600);
  Serial1.begin(9600);

  /* Modulo WiFi se configura como cliente */
  Serial1.println("AT+CWMODE=3");   
  showResponse(1000);

  /* Nos conectamos a la red deseada */
  Serial1.println("AT+CWJAP=\""+ssid+"\",\""+password+"\""); 
  showResponse(1000);

  /* Configuracion e inicialización de RTC */
  rtc.halt(false);
  rtc.writeProtect(false);
  rtc.setDOW(WEDNESDAY);         
  rtc.setTime(12,26,50);      
  rtc.setDate(5,11,2017);    
  Wire.begin();
  delay(100);

  /* Inicializacion de Pantalla Nokia */
  pinMode(A5, INPUT);
  digitalWrite(A5, INPUT_PULLUP);
  pinMode(A4, INPUT);
  digitalWrite(A4, INPUT_PULLUP);
  pinMode(A3, OUTPUT);
  digitalWrite(A3, HIGH);
  pinMode(A2, OUTPUT);
  digitalWrite(A2, LOW);
  lcd.setCursor(18, 0);
  lcd.print("Acercar");
  lcd.setCursor(12, 1);
  lcd.print("credencial");
  lcd.setCursor(0,3);
  SPI.begin();   
  
  /* Inicialización del lector de chip */
  Lector1.PCD_Init();   
}


/* Funcion en ciclo infinito, donde siempre estaremos esperando una credencial para validar */
void loop() { 

  /* Tomamos el tiempo del RTC, si este va a cumplir 1 minuto, mandamos a llamar la funcion de envio
     de temperatura, de lo contrario, seguimos en el código */
  getTime_RTC();
  if(t.sec == 59){
    send_Temperature();
  }

  /* Verificamos si el alumno que puso su credencial tiene el acceso al aire acondicionado */
  if(flag_TIME[j/5] == true && get_min_modify + 1 > t.min && flag_MODIFY == true) {

    /* De tener el acceso, comenzaremos a leer los botones de ajuste para la temperatura */
    pulsador = digitalRead(A5);
    pulsador1 = digitalRead(A4);

    /* Verificamos a continuación con las dos comparaciones si se presionaron */

    /* Boton presionado -> Sube la temperatura */
    if(pulsador == LOW)     temperatura++;

    /* Boton presionado -> Baja la temperatura */
    if(pulsador1 == LOW)    temperatura--;

    /* Imprimimos en la pantalla que el alumno tiene permitido el acceso al aire acondicionado,
       mostrando también la temperatura del aire */
    lcd.clear(0,18,60);
    lcd.clear(1,10,80);
    lcd.clear(3,10,50);
    lcd.setCursor(18, 0);
    lcd.print("Acceso");
    lcd.setCursor(12, 1);
    lcd.print("Permitido");
    lcd.setCursor(30, 3);
    lcd.print(temperatura);
    lcd.setCursor(45, 3);
    lcd.print("C");


  /* Validamos que el alumno con acceso se acabe su tiempo permitido de modificación */
  } else if(get_min_modify + 1 == t.min && flag_MODIFY == true) {

    /* De ser cierto, su autorización para modificar el aire acondicionado es removida */
    flag_MODIFY = false;


  /* Si el alumno tuvo acceso al aire acondicionado, pero paso su tiempo limite de modificación */
  } else if(get_min_modify+2>t.min && flag_MODIFY == false && flag_TIME[j/5] == true) {

    /* Si el caso se cumple, se imprime en pantalla que los controles del aire han sido bloqueados */
    lcd.clear(0,18,60);
    lcd.clear(1,10,80);
    lcd.clear(3,10,50);
    lcd.setCursor(18, 0);
    lcd.print("Controles");
    lcd.setCursor(12, 1);
    lcd.print("Bloqueados");
    lcd.setCursor(30, 3);
    lcd.print(temperatura);
    lcd.setCursor(45, 3);
    lcd.print("C");

  
  } else if(get_min_modify+2 == t.min && flag_MODIFY == false)  {
    
    flag_MODIFY = true;
    get_min_modify = t.min;

  /* Verificamos que haya una tarjeta presente */
  } else if (Lector1.PICC_IsNewCardPresent()) { 

    /* De estar presente comenzamos a leer la tarjeta */
    if (Lector1.PICC_ReadCardSerial()) {

      /* La variable del valor de la credencial es limpiada, al igual que la variable de acceso */
      ID_card = "";
      j = 0;

      /* Guardamos en la variable 'ID_card' la informacion obtenida del lector */
      for (int i = 0; i < Lector1.uid.size; i++) {
        ID_card+=Lector1.uid.uidByte[i];
      } 

      /* Dentro de este ciclo comenzaremos a buscar el valor de la credencial en nuestra base de datos */                  
      do {

        /* Aumentamos la posicion en el arreglo de la base de datos */
        j += 5;

        /* Verificamos que no estemos exediendo el tamaño de la base de datos */
        if(j > 25) {

          /* Si nos exedimos del valor, significa que la tarjeta no esta registrada dentro de nuestra base de datos,
             notificamos en pantalla que el acceso es negado y volvemos al inicio de la verificacio */
          lcd.clear(0,18,60);
          lcd.clear(1,12,70);
          lcd.setCursor(18, 0);
          lcd.print("Acceso");
          lcd.setCursor(12, 1);
          lcd.print("Denegado");
          flag_ID = false;
          break;
        }

        /* Comprobamos si el valor obtenido esta en la base de datos
           De ser correcto, guardamos la informacion de autorizacion ubicado en la siguiente posicion */
        if(ID_card == info[j])      obtained_ID = info[j+1];

      /* Condiciones para el ciclo */
      } while(ID_card != info[j]);

      /* Reiniciamos el valor para futuras validaciones */
      j = 0;

      /* Validamos si el alumno registrado tiene la autorizacion de acceder al aire acondicionado y si no lo ha modificado */
      if(obtained_ID == "1" && flag_ID && flag_MODIFY == false) {

        /* De ser cierta la validacion, ahora se validara que el alumno autorizado este pidiendo el acceso en su respectiva hora */
        if((info[j+2].toInt() / 100) == t.hour && (info[j+2].toInt() % 100) == t.min) {

          /* De ser cierto, el alumno obtiene la autorizacion de modificar el aire por un tiempo limitado */
          flag_MODIFY = true;
          flag_TIME[j/5] = true;
          get_min_modify = t.min;

          /* Notificamos al usuario que tiene acceso al aire acondicionado */
          lcd.clear(0,18,60);
          lcd.clear(1,10,80);
          lcd.setCursor(18, 0);
          lcd.print("Acceso");
          lcd.setCursor(12, 1);
          lcd.print("Permitido");

        /* En caso de no cumplir el requisito, significa que el alumno autorizado esta 
           pidiendo acceso fuera de su hora asignada */
        } else {

          /* Notificamos al alumno que no esta en una hora habil */
          lcd.clear(0,18,60);
          lcd.clear(1,12,70);
          lcd.setCursor(18, 0);
          lcd.print("Horario");
          lcd.setCursor(10, 1);
          lcd.print("No habilitado");
        }

      /* Si no se cumplio la condición previa, sabemos que el alumno no esta autorizado
         para el uso del aire acondicionado */
      } else  {

        /* Notificamos al alumno que no tiene permitido utilizar el aire acondicionaod */
        lcd.clear(0,18,60);
        lcd.clear(1,12,70);
        lcd.setCursor(18, 0);
        lcd.print("Acceso");
        lcd.setCursor(12, 1);
        lcd.print("Denegado");
      }                                              
    }
  }
}


/* Esta funcion tiene el objetivo de conectarse con el servidor de Thingspeak para
   mandarle la temperatura a nuestro canal privado */
void send_Temperature(){

  /* La variable 'cmd' es asignada con el comando de conectarse al servidor de 
     Thingspeak */
  String  cmd = "AT+CIPSTART=\"TCP\",\"";                  
  cmd += "184.106.153.149";                             
  cmd += "\",80";

  /* Comprobamos que el comando se mande de manera correcta y esperamos una respuesta */
  Serial1.println(cmd);
  showResponse(5000);

  /* Ahora mandamos en 'getStr' el comando de comunicacion a nuestro canal privado
     en el, mandamos nuestro valor de la temperatura actual */
  String getStr = "GET /update?api_key="; 
  getStr += apiKey;
  getStr +="&field1=";
  getStr += String(temperatura);
  getStr += "\r\n";

  /* Mandamos la instruccion previamente establecida */
  cmd = "AT+CIPSEND=";
  cmd += String(getStr.length());

  /* Verificamos que la instruccion haya sido enviada bien */
  Serial1.println(cmd);
 
  /* Esperamos 100 microsegundos para permitir que la instruccion sea enviada */
  delay(100);

  /* Buscamos en el serial de WiFi el caracter '>'*/
  if(Serial1.find(">")) {

    /* De ser encontrado, imprimimos la instruccion de comunicación con el servidor,
       esto puede significar que la instruccion no fue enviada correctamente */
    Serial1.print(getStr);
    
    /* Esperamos la respuesta de nuestro módulo WiFi */
    showResponse(5000);

  /* De no ser cierta la validacion pasado, sabemos que la instruccion fue enviada con exito */
  } else {

    /* Cerramos nuestra conexión con el servidor, hasta volver a necesitarla */
    Serial1.println("AT+CIPCLOSE");
  }  
}


/* Esta funcion toma el nuevo valor del RTC, y actualiza la pantalla con el */
void getTime_RTC() {
  /* Es obtenido el nuevo tiempo */
  t = rtc.getTime();

  /* Posición y esritura de la hora */
  lcd.clear(5,0,45);
  lcd.setCursor(0, 5);
  lcd.print(t.hour);

  /* Posición y esritura de los minutos */
  lcd.setCursor(12, 5);
  lcd.print(":");
  lcd.setCursor(15, 5);
  lcd.print(t.min);

  /* Posición y esritura de los segundos */
  lcd.setCursor(27, 5);
  lcd.print(":");
  lcd.setCursor(30, 5);
  lcd.print(t.sec);

  /* Posición y esritura del dia */
  lcd.setCursor(60, 5);
  lcd.print(t.date);

  /* Posición y esritura del mes */
  lcd.setCursor(67, 5);
  lcd.print("/");
  lcd.setCursor(70, 5);
  lcd.print(t.mon);
}


/* Esta funcion tiene la finalidad de mostrar la respuesta del módulo WiFi, usada para
   cuestiones de prueba, la respuesta solo es accesible a nosotros */
void showResponse(int waitTime) {

  /* Variables locales */
  long t = millis();
  char c;

  /* Ciclo que espera el tiempo indicado */
  while (t + waitTime > millis()) {

    /* Verificamos que el módulo WiFi este disponible */
    if (Serial1.available()) {

      /* Si lo esta, nos devuelve la información de respuesta */
      c = Serial1.read();
      Serial.print(c);
    }
  }                  
}
