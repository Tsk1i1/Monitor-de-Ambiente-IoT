# Monitor de Ambiente – ESP32 + FIWARE

Este projeto implementa um **sistema de monitoramento de um armazém de vinhos** utilizando um **ESP32** para coleta de **temperatura**, **umidade** e **luminosidade**.  
Os dados são enviados para a **plataforma FIWARE** hospedada em uma VM no **Microsoft Azure**, onde são processados e armazenados.  
Um **dashboard em Python/Dash** consome as APIs do FIWARE para apresentar dados históricos em gráficos dinâmicos.
A integração foi feita com o **FIWARE** e testada via **Postman**, conforme orientado pelo professor **Fábio Cabrini** em seu [repositório FIWARE Descomplicado](https://github.com/fabiocabrini/fiware).  

---

## 📋 Descrição da Solução

A solução é composta por:

- **ESP32** que lê sensores de **luminosidade (LDR)** e **temperatura/umidade (DHT22)**.
- **Alertas visuais e sonoros locais**:
  - **LEDs** (verde, amarelo, vermelho) indicam níveis de normalidade, atenção ou perigo.
  - **Buzzer** emite sons diferenciados para cada tipo de anomalia (temperatura, umidade ou luminosidade).
- **Conexão Wi-Fi** para envio dos dados via **MQTT** ao FIWARE.
- **Plataforma FIWARE** (VM Azure) que armazena e disponibiliza os dados via Orion Context Broker e STH-Comet.
- **Dashboard Web em Python/Dash** que consome os dados históricos (porta 8666) e exibe gráficos em tempo real.

---

## 🏗️ Arquitetura

A arquitetura é baseada em componentes FIWARE:

1. **ESP32**: coleta e envia dados via MQTT.
2. **IoT Agent MQTT**: recebe dados do ESP32 e os traduz para o Orion.
3. **Orion Context Broker**: gerencia o contexto em tempo real.
4. **STH-Comet**: armazena o histórico dos atributos para consultas.
5. **Dashboard Python/Dash**: acessa o STH-Comet e apresenta os gráficos.
---

![Arquitetura da Solução](https://github.com/Tsk1i1/Monitor-de-Ambiente-IoT/blob/main/Images/Esquema%20de%20Arquitetura.png)
[👉 Clique aqui para baixar o diagrama](https://github.com/Tsk1i1/Monitor-de-Ambiente-IoT/blob/main/Esquema.drawio)

---

## 🔌 Diagrama de Ligação (Hardware)

Diagrama completo disponível no [Wokwi](diagram.json).  
Componentes e conexões principais:

| Componente       | Pino ESP32 |
|------------------|------------|
| LDR (AO)         | 34         |
| DHT22 (Data)     | 32         |
| LED Verde        | 25         |
| LED Amarelo      | 26         |
| LED Vermelho     | 27         |
| Buzzer           | 12         |
| LED Onboard      | 2          |
| VCC dos sensores | 3V3        |
| GND              | GND        |

> **Resistores de 220 Ω** em série com cada LED.  

---

## 💻 Manual de Instalação e Operação

### 1. Hardware
- Monte o circuito conforme o diagrama (.drawio ou Wokwi).
- Conecte o ESP32 à USB para programação e alimentação.

### 2. Software – ESP32
1. Instale a [Arduino IDE](https://www.arduino.cc/en/software).
2. Copie o código.
3. Instale as bibliotecas:
   - `WiFi`
   - `PubSubClient`
   - `DHT sensor library`
4. Abra o arquivo [`MAI.ino`](MAI.ino) e:
   - Configure **SSID** e **senha Wi-Fi**.
   - Configure o **IP do Broker MQTT** (FIWARE/Azure).
5. Faça o upload para o ESP32.
6. Abra o Serial Monitor para acompanhar logs de conexão e leituras.

### 3. Software – Nuvem (FIWARE + Dashboard)
1. **VM Azure**:
   - Instale o stack FIWARE (Orion, IoT Agent MQTT, STH-Comet).
   - Configure portas: `1883` (MQTT), `4041` (IoT Agent), `1026` (Orion), `8666` (STH-Comet).
2. **Dashboard**:
   - Copie os arquivos [`dashboard.py`](dashboard.py) e `requirements.txt` para a VM.
   - Instale dependências:
     ```bash
     pip install -r requirements.txt
     ```
   - Execute:
     ```bash
     python3 dashboard.py
     ```
   - Acesse via navegador em `http://<IP_DA_VM>:5000`.
     
![Dashboard](https://github.com/Tsk1i1/Monitor-de-Ambiente-IoT/blob/main/Images/Dashboard_img)

### 4. Operação
- O ESP32 envia leituras a cada **5 segundos**.
- LEDs e buzzer indicam o estado local:
  - **Verde**: parâmetros normais.
  - **Amarelo**: nível de atenção.
  - **Vermelho + Buzzer**: nível crítico.
- O dashboard exibe gráficos de **temperatura**, **umidade** e **luminosidade** com média calculada.

---

## Simulação do Código
Simule esse projeto em https://wokwi.com/projects/442976754385019905

## Colaboradores

- Cesar Aaron Herrera
- Kaue Soares Madarazzo
- Rafael Seiji Aoke Arakaki
- Rafael Yuji Nakaya
- Nicolas Mendes dos Santos

## Agradecimentos

- Professor Fabio Cabrini (Disciplina: Edge Computing and Computer Systems, FIAP)
  

> **Observação:** Este projeto foi desenvolvido como parte da disciplina *Edge Computing and Computer Systems* na FIAP.
