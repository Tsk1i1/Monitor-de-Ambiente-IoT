#include <WiFi.h>          // Biblioteca para conectar o ESP32 a uma rede Wi-Fi
#include <PubSubClient.h>  // Biblioteca Biblioteca para comunicação via protocolo MQTT
#include <DHT.h>           // Biblioteca para o sensor de temperatura e umidade DHT

// ---------------------- Configurações de Wi-Fi ---------------------- //
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";
const char* MQTT_BROKER_IP = "20.151.77.156";
const int   MQTT_BROKER_PORT = 1883;
const char* MQTT_CLIENT_ID = "fiware_101";

// ---------------------- Configurações do MQTT ---------------------- //
const char* DEVICE_ID = "monitor101"; // Usado para compor comandos
const char* MQTT_TOPIC_CMD = "/TEF/monitor101/cmd";     // Tópico para receber comandos
const char* MQTT_TOPIC_ATTRS = "/TEF/monitor101/attrs"; // Tópico para publicar dados

// ---------------------- Pinos e Sensores ---------------------- //
const int PIN_LDR = 34;          // Pino analógico para o sensor de luminosidade (LDR)
const int PIN_DHT = 32;          // Pino conectado ao sensor DHT
const int PIN_LED_VERDE = 25;    // Status: OK
const int PIN_LED_AMARELO = 26;  // Status: Atenção
const int PIN_LED_VERMELHO = 27; // Status: Perigo/Crítico
const int PIN_BUZZER = 12;
const int PIN_LED_ONBOARD = 2;   // LED azul da placa (feedback de comando)

// =================================================================
// Níveis de Alerta 
// =================================================================
// Definição dos níveis de estado
#define NIVEL_OK 0
#define NIVEL_ATENCAO 1
#define NIVEL_PERIGO 2

// --- Luminosidade (em %) ---
// Faixa OK:      < 35%
// Faixa Atenção: 35% a 49%
// Faixa Perigo:  >= 50%
const int LUZ_ATENCAO_MIN = 35;
const int LUZ_PERIGO_MIN = 50;

// --- Temperatura (em °C) ---
// Faixa OK:      12.0 a 16.0 °C
// Faixa Atenção: 10.0 a 11.9 °C  OU  16.1 a 19.0 °C
// Faixa Perigo:  < 10.0 °C       OU  > 19.0 °C
const float TEMP_OK_MIN = 12.0;
const float TEMP_OK_MAX = 16.0;
const float TEMP_ATENCAO_MIN = 10.0;
const float TEMP_ATENCAO_MAX = 19.0;

// --- Umidade (em %) ---
// Faixa OK:      60.0 a 80.0 %
// Faixa Atenção: 50.0 a 59.9 % OU  80.1 a 85.0 %
// Faixa Perigo:  < 55.0 %      OU  > 85.0 %
const float UMID_OK_MIN = 60.0;
const float UMID_OK_MAX = 80.0;
const float UMID_ATENCAO_MIN = 50.0;
const float UMID_ATENCAO_MAX = 85.0;

// Frequências (Hz) do Buzzer para cada tipo de alerta crítico
const int FREQ_TEMP_ALERTA = 1200; // Tom agudo
const int FREQ_UMID_ALERTA = 800;  // Tom médio
const int FREQ_LUZ_ALERTA  = 400;  // Tom grave

// =================================================================
// Variáveis Globais e Objetos
// =================================================================
float temperatura_atual = 0.0;
float umidade_atual = 0.0;
int   luminosidade_atual = 0;
bool  alarmeSonoroSilenciado = false;

unsigned long lastPublish = 0;
const long PUBLISH_INTERVAL_MS = 5000; // Intervalo entre publicações (5 segundos)

int   nivel_alerta_geral = NIVEL_OK;       // Armazena o nível de alerta atual do sistema
unsigned long lastBlinkMillis = 0;         // Armazena o tempo do último piscar do LED
const int BLINK_INTERVAL_MS = 500;         // Intervalo do pisca-pisca (pisca 2x por segundo)
bool  ledAzulEstado = LOW;                 // Armazena o estado atual do LED azul

WiFiClient espClient;
PubSubClient mqttClient(espClient);
DHT dht(PIN_DHT, DHT22); //dht 11

// =================================================================
// Declaração de Funções 
// =================================================================
void setupWiFi();
void setupMQTT();
void setupHardware();
void reconnectWiFi();
void reconnectMQTT();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void lerSensores();
void publicarDadosSensores();
void gerenciarAlertasVisuaisESonoros();

void setup() {
    Serial.begin(115200);
    Serial.println("\nIniciando o Monitor de Ambiente...");

    setupHardware();
    setupWiFi();
    setupMQTT();

    Serial.println("Setup concluído. Dispositivo pronto para operar.");
}

void loop() {
    // Garante que as conexões estejam sempre ativas
    if (WiFi.status() != WL_CONNECTED) reconnectWiFi();
    if (!mqttClient.connected()) reconnectMQTT();
    
    // Processa mensagens MQTT recebidas
    mqttClient.loop();

    // Bloco de execução temporizado para leitura e publicação
    unsigned long now = millis();
    if (now - lastPublish > PUBLISH_INTERVAL_MS) {
        lastPublish = now; // Atualiza o tempo da última execução
        
        lerSensores();
        publicarDadosSensores();
        gerenciarAlertasVisuaisESonoros();
    }
    gerenciarBlinkLedAnomalia();
}

void setupHardware() {
    pinMode(PIN_LED_VERDE, OUTPUT);
    pinMode(PIN_LED_AMARELO, OUTPUT);
    pinMode(PIN_LED_VERMELHO, OUTPUT);
    pinMode(PIN_BUZZER, OUTPUT);
    pinMode(PIN_LED_ONBOARD, OUTPUT);
    
    // Inicia o sensor DHT
    dht.begin();

    // Sinaliza que a inicialização do hardware foi concluída
    digitalWrite(PIN_LED_VERDE, HIGH);
    delay(250);
    digitalWrite(PIN_LED_VERDE, LOW);
}

void setupWiFi() {
    Serial.print("Conectando à rede Wi-Fi: ");
    Serial.println(WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    reconnectWiFi(); // Garante a conexão inicial
}

void setupMQTT() {
    mqttClient.setServer(MQTT_BROKER_IP, MQTT_BROKER_PORT);
    mqttClient.setCallback(mqttCallback);
}

// Conexão e Reconexão
void reconnectWiFi() {
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWi-Fi conectado com sucesso!");
    Serial.print("Endereço IP: ");
    Serial.println(WiFi.localIP());
}

void reconnectMQTT() {
    while (!mqttClient.connected()) {
        Serial.print("Tentando conectar ao Broker MQTT...");
        if (mqttClient.connect(MQTT_CLIENT_ID)) {
            Serial.println(" Conectado!");
            // Inscreve-se no tópico de comandos após conectar/reconectar
            mqttClient.subscribe(MQTT_TOPIC_CMD);
            Serial.print("Inscrito no tópico: ");
            Serial.println(MQTT_TOPIC_CMD);
        } else {
            Serial.print(" Falha, rc=");
            Serial.print(mqttClient.state());
            Serial.println(". Nova tentativa em 5 segundos.");
            delay(5000);
        }
    }
}

// Processamento de Comandos MQTT
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    // Cria um buffer para a mensagem e garante que termina com nulo.
    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0'; // Adiciona o terminador de string nulo.

    Serial.println("Comando recebido: " + String(message)); // Mantemos esta linha essencial.

    // Comandos contextuais para o armazém - Usando strstr para comparação segura
    if (strstr(message, "@silenceAlarm") != NULL) {
        alarmeSonoroSilenciado = true; // Ativa o modo silencioso
    } else if (strstr(message, "@reactivateAlarm") != NULL) {
        alarmeSonoroSilenciado = false; // Desativa o modo silencioso
    }
    
    delay(100);
    digitalWrite(PIN_LED_ONBOARD, LOW);
}

// Lógica Principal de Monitoramento e Alertas
void lerSensores() {
    // Leitura LDR (0-4095) e mapeamento para porcentagem (0-100%)
    int ldrValue = analogRead(PIN_LDR);
    luminosidade_atual = map(ldrValue, 4000, 0, 0, 100);
    
    // Leitura DHT22
    temperatura_atual = dht.readTemperature();
    umidade_atual = dht.readHumidity();

    // Tratamento de erro para leituras inválidas do DHT
    if (isnan(temperatura_atual)) { temperatura_atual = -99; }
    if (isnan(umidade_atual)) { umidade_atual = -99; }
}

void publicarDadosSensores() {
    char payload[20];
    Serial.println("--- Publicando Dados ---");

    // Publica no formato "chave|valor"
    sprintf(payload, "l|%d", luminosidade_atual);
    mqttClient.publish(MQTT_TOPIC_ATTRS, payload);
    Serial.println("  Luminosidade: " + String(payload));

    dtostrf(temperatura_atual, 2, 2, payload);
    char temp_msg[20];
    sprintf(temp_msg, "t|%s", payload);
    mqttClient.publish(MQTT_TOPIC_ATTRS, temp_msg);
    Serial.println("  Temperatura: " + String(temp_msg));
    
    dtostrf(umidade_atual, 2, 2, payload);
    char hum_msg[20];
    sprintf(hum_msg, "h|%s", payload);
    mqttClient.publish(MQTT_TOPIC_ATTRS, hum_msg);
    Serial.println("  Umidade: " + String(hum_msg));
    Serial.println("------------------------");
}

void gerenciarAlertasVisuaisESonoros() {
    // Determina o nível de alerta para cada sensor individualmente.
    int nivel_luz = NIVEL_OK;
    if (luminosidade_atual >= LUZ_PERIGO_MIN) nivel_luz = NIVEL_PERIGO;
    else if (luminosidade_atual >= LUZ_ATENCAO_MIN) nivel_luz = NIVEL_ATENCAO;

    int nivel_temp = NIVEL_OK;
    if (temperatura_atual < TEMP_ATENCAO_MIN || temperatura_atual > TEMP_ATENCAO_MAX) nivel_temp = NIVEL_PERIGO;
    else if (temperatura_atual < TEMP_OK_MIN || temperatura_atual > TEMP_OK_MAX) nivel_temp = NIVEL_ATENCAO;

    int nivel_umid = NIVEL_OK;
    if (umidade_atual < UMID_ATENCAO_MIN || umidade_atual > UMID_ATENCAO_MAX) nivel_umid = NIVEL_PERIGO;
    else if (umidade_atual < UMID_OK_MIN || umidade_atual > UMID_OK_MAX) nivel_umid = NIVEL_ATENCAO;
    
    // Define o status geral do ambiente (o pior caso entre os sensores).
    int nivel_geral = max({nivel_luz, nivel_temp, nivel_umid});
    
    // Reseta a flag de silêncio se a condição de perigo desaparecer.
    if (nivel_geral != NIVEL_PERIGO) {
        alarmeSonoroSilenciado = false;
    }
    
    // Ativa o buzzer, mas APENAS se houver perigo E o alarme não estiver silenciado.
    if (nivel_geral == NIVEL_PERIGO && !alarmeSonoroSilenciado) {
        if (nivel_temp == NIVEL_PERIGO) {
            tone(PIN_BUZZER, FREQ_TEMP_ALERTA, 200);
        } else if (nivel_umid == NIVEL_PERIGO) {
            tone(PIN_BUZZER, FREQ_UMID_ALERTA, 200);
        } else if (nivel_luz == NIVEL_PERIGO) {
            tone(PIN_BUZZER, FREQ_LUZ_ALERTA, 200);
        }
    }
    
    // Acende o LED correspondente ao status geral.
    digitalWrite(PIN_LED_VERDE,   nivel_geral == NIVEL_OK);
    digitalWrite(PIN_LED_AMARELO, nivel_geral == NIVEL_ATENCAO);
    digitalWrite(PIN_LED_VERMELHO, nivel_geral == NIVEL_PERIGO);
}

void gerenciarBlinkLedAnomalia() {
    // Se o nível de alerta for OK, simplesmente desliga o LED e sai da função.
    if (nivel_alerta_geral == NIVEL_OK) {
        digitalWrite(PIN_LED_ONBOARD, LOW);
        ledAzulEstado = LOW;
        return;
    }

    // Se houver uma anomalia (Atenção ou Perigo), entra na lógica de piscar.
    unsigned long currentMillis = millis();
    if (currentMillis - lastBlinkMillis >= BLINK_INTERVAL_MS) {
        lastBlinkMillis = currentMillis; // Salva o tempo do último piscar

        // Inverte o estado do LED
        ledAzulEstado = !ledAzulEstado;
        digitalWrite(PIN_LED_ONBOARD, ledAzulEstado);
    }
}