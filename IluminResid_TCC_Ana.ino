/************************************************************
*UNIVERSIDADE FEDERAL DE OURO PRETO - UFOP
*DECAT - DEPARTAMENTO DE ENGENHARIA DE CONTROLE E AUTOMAÇÃO
*TRABALHO FINAL DE CONCLUSÃO DE CURSO
*ANA LUIZA FERREIRA LEITE
*DESENVOLVIMENTO DE UM SISTEMA DE ILUMINAÇÃO INTELIGENTE COM CONTROLE DE INTENSIDADE LUMINOSA
************************************************************/

//INCLUSÃO DAS BIBLIOTECAS

#include <DM02A.h>  //Biblioteca que realiza a comunicação com o módulo dimmer DM02A
#include "RMaker.h" //Biblioteca que habilita o uso do ESP RainMaker
#include "WiFi.h" //Biblioteca quepermite a comunicação do programa com redes Wi-Fi.
#include "WiFiProv.h" //Biblioteca que permite o provisionamento de redes Wi-Fi


//---------------------------------------------------
//DECLARAÇÃO DE VARIÁVEIS

const char *service_name = "TccAna_BLE";
//Declara a variável *service_name que irá armazenar um ponteiro a localização na memória onde a string "TccAna_BLE" está armazenada.
//O nome do dispositivo será TccAna_BLE.
const char *pop = "12345678";
//Declara a variável *pop que irá armazenar um ponteiro a localização na memória onde a string "12345678" está armazenada.
//A senha para conexão com o dispositivo será 12345678.


static uint8_t WIFI_LED = 2;  
//Declara a variável WIFI_LED do tipo estática que será usada para representar um pino de hardware específico e atribui o valor 2 para a variável.
static uint8_t gpio_reset = 0;
//Declara a variável gpio_reset do tipo estática que será usada para fazer o reset do programa e atribui o valor 0 para a variável.
DM02A dimmer(23, 22);  
//Cria um novo objeto de nome dimmer e passa como parâmetros as portas associadas ao ESP32, sendo a porta 23 conectada ao SIG do módulo dimmer e 22 ao CH.
char pot2 = 33;    
//Porta associada a ligação do potenciômetro utilizado para ajuste manual.

//---------------------------------------------------
//DECLARAÇÃO DO DISPOSITIVO
static LightBulb my_ligthbulb("Lamp1");
//Cria uma variável estática da classe LightBulb chamada my_ligthbulb e a inicializa com o nome "Lamp1". 
#define DEFAULT_DIMMER_LEVEL 0
//Define uma constante chamada DEFAULT_DIMMER_LEVEL com o valor 0.

/****************************************************************************************************
 * FUNÇÃO SYSPROVEVENT
 *Essa é uma função que irá trabalhar com eventos relacionados à configuração inicial do sistema e à conexão Wi-Fi.
*****************************************************************************************************/

void sysProvEvent(arduino_event_t *sys_event) {//
  switch (sys_event->event_id) {
    case ARDUINO_EVENT_PROV_START:
#if CONFIG_IDF_TARGET_ESP32
      Serial.printf("\nProvisioning Started with name \"%s\" and PoP \"%s\" on BLE\n", service_name, pop);
      printQR(service_name, pop, "ble");
#else
      Serial.printf("\nProvisioning Started with name \"%s\" and PoP \"%s\" on SoftAP\n", service_name, pop);
      printQR(service_name, pop, "softap");
#endif
      break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      Serial.printf("\nConnected to Wi-Fi!\n");
      digitalWrite(WIFI_LED, HIGH);
      break;
  }
}




/****************************************************************************************************
 * Função write_callback 
 *Essa função é chamada quando ocorre uma escrita (ou atualização) de um parâmetro do dispositivo
*****************************************************************************************************/

void write_callback(Device *device, Param *param, const param_val_t val, void *priv_data, write_ctx_t *ctx) {
  //Ela recebe vários argumentos, incluindo um ponteiro para o dispositivo em questão, o parâmetro que foi atualizado, o novo valor do parâmetro, dados privados, e um contexto de escrita.
  const char *device_name = device->getDeviceName();
  const char *param_name = param->getParamName();
  //----------------------------------------------------------------------------------
  if (strcmp(device_name, "Lamp1") == 0) {
    Serial.printf("Lamp1 = %s\n", val.val.b ? "true" : "false");
    if (strcmp(param_name, "Power") == 0) {
      int ligthbulb_on_off_value = val.val.b;//
      Serial.printf("\nReceived value = %d for %s - %s\n", ligthbulb_on_off_value, device_name, param_name);
      dimmer.EnviaNivel(ligthbulb_on_off_value, 1);  //Atualiza o nível no dimmer
      param->updateAndReport(val);

    } else if (strcmp(param_name, "Level") == 0) {
      int pos = val.val.i;
      Serial.printf("\nReceived value = %d for %s - %s\n", pos, device_name, param_name);
      Serial.println(pos);
      dimmer.EnviaNivel(pos, 1);  //Atualiza o nível no dimmer
      delay(15);                  // waits 15ms for the servo to reach the position
      //param->updateAndReport(val);
    }
  }
}




unsigned int med = 0;  //Armazena a média das leituras no pino de entrada analógica
char cont = 0;         //Variável auxiliar

unsigned long last_tempo = 0;  ////Armazena o momento da última verificação dos potenciômetros, a função time() reinicia em 50 dias com o Arduino constantemente ligado

int nivel_pot2_tmp = 0;

int last_nivel_pot2 = 0;

void setup() {
  //Força os dois canais iniciarem desligados
  dimmer.EnviaNivel(0, 0);
  dimmer.EnviaNivel(0, 1);
  //analogReference(DEFAULT); //5V para placas que funcionam com 5V, se sua placa usar 3.3V, precisará refazer os cálculos

  //------------------------------------------------------------------------------
  uint32_t chipId = 0;
  Serial.begin(115200);
  //------------------------------------------------------------------------------
 
  //------------------------------------------------------------------------------
  pinMode(gpio_reset, INPUT);
  pinMode(WIFI_LED, OUTPUT);
  digitalWrite(WIFI_LED, LOW);
  //------------------------------------------------------------------------------
  Node my_node;
  my_node = RMaker.initNode("TCC - Ana Leite");


  //Standard switch device. Add switch device to the node
  Param level_param("Level", "custom.param.level", value(DEFAULT_DIMMER_LEVEL), PROP_FLAG_READ | PROP_FLAG_WRITE);
  level_param.addBounds(value(0), value(70), value(1));  //sart_value, end_value, interval
  level_param.addUIType(ESP_RMAKER_UI_SLIDER);
  my_ligthbulb.addParam(level_param);

  my_ligthbulb.addCb(write_callback);

  my_node.addDevice(my_ligthbulb);
  my_ligthbulb.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, 0);

  //------------------------------------------------------------------------------
  //This is optional
  RMaker.enableOTA(OTA_USING_PARAMS);
  //If you want to enable scheduling, set time zone for your region using setTimeZone().
  //The list of available values are provided here https://rainmaker.espressif.com/docs/time-service.html
  // RMaker.setTimeZone("Asia/Shanghai");
  // Alternatively, enable the Timezone service and let the phone apps set the appropriate timezone
  RMaker.enableTZService();
  RMaker.enableSchedule();
  //------------------------------------------------------------------------------
  //Service Name
  for (int i = 0; i < 17; i = i + 8) {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }

  Serial.printf("\nChip ID:  %d Service Name: %s\n", chipId, service_name);
  //------------------------------------------------------------------------------
  Serial.printf("\nStarting ESP-RainMaker\n");
  RMaker.start();
  //------------------------------------------------------------------------------
  WiFi.onEvent(sysProvEvent);
#if CONFIG_IDF_TARGET_ESP32
  WiFiProv.beginProvision(WIFI_PROV_SCHEME_BLE, WIFI_PROV_SCHEME_HANDLER_FREE_BTDM, WIFI_PROV_SECURITY_1, pop, service_name);
#else
  WiFiProv.beginProvision(WIFI_PROV_SCHEME_SOFTAP, WIFI_PROV_SCHEME_HANDLER_NONE, WIFI_PROV_SECURITY_1, pop, service_name);
#endif
  //------------------------------------------------------------------------------
}

void loop() {
  //O valor retornado pela leitura nas portas analógicas serão entre 0 e 1023, sendo 1024 possibilidades. Os níveis para o dimmer são entre 0 e 70
  //A cada 14 passos na leitura analógica iremos considerar um nível do dimmer, então chegará no limite em 980 (14 x 70);

  //A cada 500ms verifica se algum dos potenciômetros mudaram de posição
  if (millis() > (last_tempo + 500)) {  //Verifica se já se passou 500ms, se sim, verifica se houve atualização do nível pelos botões do módulo dimmer ou por chaves externas
    last_tempo = millis();              //Atualiza o tempo registrado para o tempo atual
   
    nivel_pot2_tmp = lePot(pot2);  //Lê o valor do potenciômetro 1
    if (last_nivel_pot2 != nivel_pot2_tmp) {
      delay(100);
      nivel_pot2_tmp = lePot(pot2);  //Lê novamente o valor do potenciômetro 1
      if (last_nivel_pot2 != nivel_pot2_tmp) {
        last_nivel_pot2 = nivel_pot2_tmp;             //Atualiza o nível lido no potenciômetro
        dimmer.EnviaNivel((nivel_pot2_tmp / 14), 1);  //Atualiza o nível no dimmer
      }
    }
  }
  //------------------------------------------------------------------------------
  // Read GPIO0 (external button to reset device
  if (digitalRead(gpio_reset) == LOW) {  //Push button pressed
    Serial.printf("Reset Button Pressed!\n");
    // Key debounce handling
    delay(100);
    int startTime = millis();
    while (digitalRead(gpio_reset) == LOW) delay(50);
    int endTime = millis();
    //_______________________________________________________________________
    if ((endTime - startTime) > 10000) {
      // If key pressed for more than 10secs, reset all
      Serial.printf("Reset to factory.\n");
      RMakerFactoryReset(2);
    }
    //_______________________________________________________________________
    else if ((endTime - startTime) > 3000) {
      Serial.printf("Reset Wi-Fi.\n");
      // If key pressed for more than 3secs, but less than 10, reset Wi-Fi
      RMakerWiFiReset(2);
    }
    //_______________________________________________________________________
  }
  //------------------------------------------------------------------------------
  delay(100);

  if (WiFi.status() != WL_CONNECTED) {
    //Serial.println("WiFi Not Connected");
    digitalWrite(WIFI_LED, LOW);
  } else {
    //Serial.println("WiFi Connected");
    digitalWrite(WIFI_LED, HIGH);
  }
  //------------------------------------------------------------------------------
}

//Faz 40 leituras no pino de entrada analógica e faz uma média.
//Isso se faz necessário porque os valores lidos no pino de entrada analógica tem variações
int lePot(char _pot) {
  cont = 0;
  med = 0;
  while (cont < 40) {
    med = med + analogRead(_pot);
    delay(1);
    cont = cont + 1;
  }
  med = med / 40;
  return med;
}
