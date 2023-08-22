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
//O nome do dispositivo bluetooth será TccAna_BLE.
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
  //Ela recebe como argumentos um ponteiro para o dispositivo em questão, o parâmetro que foi atualizado, o novo valor do parâmetro, dados privados, e um contexto de escrita.
  const char *device_name = device->getDeviceName();
  const char *param_name = param->getParamName();
  //As linhas 77 e 78 extraem o nome do dispositivo e o nome do parâmetro que estão sendo atualizados.
 

  if (strcmp(device_name, "Lamp1") == 0) {
    Serial.printf("Lamp1 = %s\n", val.val.b ? "true" : "false");
    //Imprime no monitor serial se a lâmpada "Lamp1" está ligada ou desligada.
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
      delay(15);                  // Delay de 15ms para acomodação
      
    }
  }
}




unsigned int med = 0;  
//Armazena a média das leituras no pino de entrada analógica
char cont = 0;         
//Variável auxiliar

unsigned long last_tempo = 0;  
//Armazena o momento da última verificação dos potenciômetros, a função time() reinicia em 50 dias com o Arduino constantemente ligado

int nivel_pot2_tmp = 0;
//Variável que armazena a leitura do potenciômetro;

int last_nivel_pot2 = 0;
////Variável que armazena a última leitura do potenciômetro;

void setup() {
  //Função do módulo dimmer
  
  dimmer.EnviaNivel(0, 0);
  dimmer.EnviaNivel(0, 1);
//Força os dois canais iniciarem desligados; passa como parâmetros o nível 0 e o canal (0 e 1 - 1 e 2)
    uint32_t chipId = 0;
  Serial.begin(115200);
  
 
  pinMode(gpio_reset, INPUT);
  //Variável que realiza o reset do programa setada como entrada.
  pinMode(WIFI_LED, OUTPUT);
  //Variável que indica o funcionamento da conexão wifi é saída.
  digitalWrite(WIFI_LED, LOW);
  //se o reset do programa for pressionado, o led que indica a conexão wifi é desligado.


  Node my_node;
  my_node = RMaker.initNode("TCC - Ana Leite");
  //Cria o node que irá representar o dispositivo.
  

  //Adiciona o dispositivo ao node
  Param level_param("Level", "custom.param.level", value(DEFAULT_DIMMER_LEVEL), PROP_FLAG_READ | PROP_FLAG_WRITE);
  //Cria level_param que passa como parâmetros o nome do parâmetro, o identificador, define um novo valor padrão para DEFAULT_DIMMER_LEVEL que foi definido como 0 no ínicio do código,
  //permite que o parâmetro seja lido e seja atualizado
  level_param.addBounds(value(0), value(70), value(1));  
  //Define que os valores de mínimo, máximo e o intervalo entre os valores permitidos que o parâmetro pode assumir+
  //0 seria a lâmpada desligada e 70 a lâmpada acesa com a máxima intensidade.
  level_param.addUIType(ESP_RMAKER_UI_SLIDER);
  //Define que o controle do parâmetro será feito pelo usuário (UI - user interface) por meio de um slider no aplicativo ESP RainMaker
    my_ligthbulb.addParam(level_param);
    //Associa o parâmetro de nível ao dispositivo my_ligthbulb que é a lâmpada
      my_ligthbulb.addCb(write_callback);
    //Chama a função write_callback para atualizar o status do dispositivo.

  my_node.addDevice(my_ligthbulb);
  //Adiciona o dispostivo my_ligthbulb ao node criado na linha 138.
  my_ligthbulb.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, 0);


  //------------------------------------------------------------------------------
  RMaker.enableOTA(OTA_USING_PARAMS);
  //Habilita a funcionalidade OTA que realiza a atualização do dispositivo remotamente por meio de parâmetros
  RMaker.enableTZService();
  //Ativa o serviço de fuso horário.
  RMaker.enableSchedule();
  //Habilita o agendamento de tarefas com o dispositivo.
  //------------------------------------------------------------------------------
  //Service Name
  for (int i = 0; i < 17; i = i + 8) {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }

  Serial.printf("\nChip ID:  %d Service Name: %s\n", chipId, service_name);
  //Imprime o valor do chip ID no monitor serial e o nome do dispositivo bluetooth.
  
  //------------------------------------------------------------------------------
  
  Serial.printf("\nStarting ESP-RainMaker\n");
  //Imprime a inicialização do ESP RainMaker.
  RMaker.start();
  //Inicia a biblioteca ESP RainMaker e permite que toda a comunicação/configuração aconteça.
 
  //------------------------------------------------------------------------------

  WiFi.onEvent(sysProvEvent);
  //Chama a função sysprovevent que faz a configuração inicial e conexão com wi-fi
#if CONFIG_IDF_TARGET_ESP32
  WiFiProv.beginProvision(WIFI_PROV_SCHEME_BLE, WIFI_PROV_SCHEME_HANDLER_FREE_BTDM, WIFI_PROV_SECURITY_1, pop, service_name);
  //Verifica se o microcontrolador usado é o ESP32
  //Se sim, a conexão é feita usando conexão BLE (bluetooth) e têm como parâmetros a configuração do wifi, nível de segurança, a senha e o nome do dispositivo bluetooth
#else
  WiFiProv.beginProvision(WIFI_PROV_SCHEME_SOFTAP, WIFI_PROV_SCHEME_HANDLER_NONE, WIFI_PROV_SECURITY_1, pop, service_name);
  //Se não, a conexão wifi é feita usando conexão SOFTAP(funciona como um roteador temporário) e seus parâmetros fazem a configuração necessária.
#endif
  //------------------------------------------------------------------------------
}

void loop() {
  //A cada 500ms verifica se algum dos potenciômetros mudaram de posição.
  if (millis() > (last_tempo + 500)) {  
    //Verifica se já se passou 500ms, se sim, verifica se houve atualização do nível pelos botões do módulo dimmer ou por chaves externas.
    last_tempo = millis();              
    //Atualiza o tempo registrado para o tempo atual.
    nivel_pot2_tmp = lePot(pot2);  
    //Lê o valor do potenciômetro e armazena o valor na variável nivel_pot2_tmp.
    if (last_nivel_pot2 != nivel_pot2_tmp) {
      //Se o valor lido for diferente do último valor registrado para o potenciômetro
      delay(100);
      //Aplica um delay de 100ms para evitar ruídos.
      nivel_pot2_tmp = lePot(pot2);  
      //Lê novamente o valor do potenciômetro 
      if (last_nivel_pot2 != nivel_pot2_tmp) {
        //Essa condição é repetida para verificar novamente se o valor lido do potenciômetro mudou após o atraso de 100ms.
        last_nivel_pot2 = nivel_pot2_tmp;             
        //Atualiza o nível lido no potenciômetro
        dimmer.EnviaNivel((nivel_pot2_tmp / 14), 1);  
        //Atualiza o nível no dimmer e envia ao canal 2
      }
    }
  }

  //------------------------------------------------------------------------------

  if (digitalRead(gpio_reset) == LOW) {  
    //Se o pino definido como gpio_reset estiver recebendo um sinal LOW.
    Serial.printf("Reset Button Pressed!\n");
    // Imprime que o botão de reset foi pressionado.
    delay(100);
    //Delay de 100ms para garantir que não tenha o efeito bouncing
    int startTime = millis();
    //Conta o tempo em ms que o botão começou a ser pressionado
    while (digitalRead(gpio_reset) == LOW) delay(50);
    //Aguarda até que o botão seja liberado
    int endTime = millis();
    //Conta o tempo em ms que o botão foi liberado.

    if ((endTime - startTime) > 10000) {
      //Se o botão ficar pressionado (tempo do final - tempo do início) mais que 10 segundos: resetar tudo
      Serial.printf("Reset to factory.\n");
      //Imprime que o reset de tudo foi feito.
      RMakerFactoryReset(2);
      //Reset "de fábrica"
    }

    else if ((endTime - startTime) > 3000) {
      //Se o botão ficar pressionado (tempo do final - tempo do início) mais que 3 e menos que 10 segundos: resetar conexão wi-fi
      Serial.printf("Reset Wi-Fi.\n");
      //Imprime que o reset Wi-fi foi feito.
      RMakerWiFiReset(2);
      //Reset Wi-fi
    }
  }
  delay(100);

  if (WiFi.status() != WL_CONNECTED) {
    //Verifica o status da conexão Wi-Fi e ajusta o estado de um LED com base nesse status.
    digitalWrite(WIFI_LED, LOW);
    //Indica que não está conectado à rede wi-fi e não acende o led indicador.
  } else {
    digitalWrite(WIFI_LED, HIGH);
    //Indica que está conectado à rede wi-fi e acende o led indicador.
  }
}


int lePot(char _pot) {
  //Função responsável por ler o valor analógico do potenciômetro
  //Faz 40 leituras no pino de entrada analógica e faz uma média.
  //Isso se faz necessário porque os valores lidos no pino de entrada analógica tem variações
  cont = 0;
  //Inicia o contador em 0
  med = 0;
  //Inicia a variável med em 0
  while (cont < 40) {
    //Loop enquanto o contador for menos que 40
    med = med + analogRead(_pot);
    //Med assumirá a soma do valor analógico do pino conectado ao potenciômetro + med
    delay(1);
    //Delay para evitar leituras rápidas.
    cont = cont + 1;
    //Atualiza o contador.
  }
  med = med / 40;
  //Calcula a média das leituras dividindo o valor total pelo número de leituras
  return med;
  //Retorna o valor de med
}
