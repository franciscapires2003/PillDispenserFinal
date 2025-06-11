#include <Wire.h>
#include <RTClib.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Stepper.h>

//alterar estas variaveis para a wifi que vai ser
const char* ssid = "NOME_DA_REDE";       
const char* password = "PASSWORD";          

RTC_DS3231 rtc;
WebServer server(80);

const int buzzerPin = 15;

const int passosPorRotacao = 2048; 
Stepper motor(passosPorRotacao, 13, 12, 14, 27);  

int alarmHour = -1;
int alarmMinute = -1;
bool alarmTriggered = false;


String ultimaHoraDetecao = "Nenhuma";
String ultimoID = "Desconhecido";

// Página de configuração do alarme
void rotacao() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="pt">

<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>PillDispenser - Configurar Alarme</title>
  <link rel="stylesheet" href="style.css">
  <style>
    /* Main content styles */
    .main-content {
      padding: 20px;
    }

    .page-title {
      font-size: 24px;
      font-weight: bold;
      margin-bottom: 30px;
      color: #333;
    }

    /* Form styles */
    .form-group {
      margin-bottom: 25px;
    }

    .form-label {
      display: block;
      font-weight: bold;
      margin-bottom: 15px;
      color: #333;
      font-size: 16px;
    }

    .form-label.required:after {
      content: " *";
      color: red;
    }

    .form-input {
      width: 100%;
      padding: 15px;
      border: 1px solid #ccc;
      border-radius: 10px;
      font-size: 16px;
      box-sizing: border-box;
      background-color: white;
    }

    /* Button styles */
    .button-container {
      margin-top: 40px;
    }

    .button {
      background-color: #081BAE;
      color: white;
      border: none;
      padding: 16px;
      border-radius: 10px;
      width: 100%;
      font-size: 16px;
      font-weight: bold;
      cursor: pointer;
    }

    .button:hover {
      background-color: #0a20d0;
    }
  </style>
</head>

<body>
  <div class="app">
    <!-- Header -->
    <header>
      <div class="logo"><strong>PillDispenser</strong></div>
    </header>

    <!-- Main content -->
    <main>
      <div class="main-content">
        <h1 class="page-title">Definir Alarme</h1>

        <form action="/setalarm" method="GET">
          <div class="form-group">
            <label for="hour" class="form-label required">Hora (0–23):</label>
            <input type="number" name="hour" id="hour" class="form-input" min="0" max="23" required>
          </div>

          <div class="form-group">
            <label for="minute" class="form-label required">Minuto (0–59):</label>
            <input type="number" name="minute" id="minute" class="form-input" min="0" max="59" required>
          </div>

          <div class="button-container">
            <button type="submit" class="button">Definir Alarme</button>
          </div>
        </form>
      </div>
    </main>

    <!-- Footer -->
    <footer>
      <a href="/monitoramento">Ir para Monitoramento</a>
    </footer>
  </div>
</body>

</html>
)rawliteral";

  server.send(200, "text/html", html);
}


void SetAlarm() {
  if (server.hasArg("hour") && server.hasArg("minute")) {
    alarmHour = server.arg("hour").toInt();
    alarmMinute = server.arg("minute").toInt();
    alarmTriggered = false;
    String response = "Alarme configurado para ";
    response += String(alarmHour) + ":" + (alarmMinute < 10 ? "0" : "") + String(alarmMinute);
    server.send(200, "text/plain", response);
    Serial.println(response);
  } else {
    server.send(400, "text/plain", "Parâmetros inválidos");
  }
}

// Logs do Processing
void logP() {
  if (server.hasArg("id") && server.hasArg("hora")) {
    ultimoID = server.arg("id");
    ultimaHoraDetecao = server.arg("hora");
    Serial.printf("Recebido: ID=%s, Hora=%s\n", ultimoID.c_str(), ultimaHoraDetecao.c_str());
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Faltam parâmetros");
  }
}

// Página de monitoramento
void Monitoramento() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="pt">
<head>
  <meta charset="UTF-8">
  <title>Monitoramento do Idoso</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      background-color: #f4f4f4;
      margin: 0;
      padding: 0;
    }
    header {
      background-color: #081BAE;
      color: white;
      padding: 20px;
      text-align: center;
      font-size: 24px;
      font-weight: bold;
    }
    .container {
      max-width: 500px;
      margin: 40px auto;
      background: white;
      padding: 30px;
      border-radius: 12px;
      box-shadow: 0 0 10px rgba(0,0,0,0.1);
      text-align: center;
    }
    .info {
      font-size: 18px;
      margin-bottom: 20px;
    }
    .label {
      font-weight: bold;
      color: #333;
    }
    .value {
      color: #555;
    }
    /* Button styles */
    .button-container {
      margin-top: 40px;
    }

    .button {
      background-color: #081BAE;
      color: white;
      border: none;
      padding: 16px;
      border-radius: 10px;
      width: 100%;
      font-size: 16px;
      font-weight: bold;
      cursor: pointer;
    }

    .button:hover {
      background-color: #0a20d0;
    }
  </style>
</head>
<body>
  <header>Monitoramento do Idoso</header>
  <div class="container">
    <div class="info">
      <span class="label">Último ID:</span>
      <span class="value">%ULTIMO_ID%</span>
    </div>
    <div class="info">
      <span class="label">Última hora de detecção:</span>
      <span class="value">%ULTIMA_HORA%</span>
    </div>
    <a href="/" class="button">Voltar para alarme</a>
  </div>
</body>
</html>
)rawliteral";

  
  html.replace("%ULTIMO_ID%", ultimoID);
  html.replace("%ULTIMA_HORA%", ultimaHoraDetecao);

  server.send(200, "text/html", html);
}


void setup() {
  Serial.begin(115200);
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);

  if (!rtc.begin()) {
    Serial.println("RTC não encontrado");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC perdeu energia. Ajustando hora...");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  motor.setSpeed(10);  

  Serial.print("Conectando WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Conectado");

  server.on("/", rotacao);
  server.on("/setalarm", SetAlarm);
  server.on("/log", logP);  
  server.on("/monitoramento", Monitoramento);  
  server.begin();
  Serial.println("Servidor iniciado");
}

void loop() {
  server.handleClient();

  DateTime now = rtc.now();
  Serial.printf("Hora: %02d:%02d:%02d  Data: %02d/%02d/%04d\n",
                now.hour(), now.minute(), now.second(),
                now.day(), now.month(), now.year());

  if (alarmHour != -1 && alarmMinute != -1 && !alarmTriggered) {
    if (now.hour() == alarmHour && now.minute() == alarmMinute) {
      Serial.println("Hora de tomar a medicação!");

      for (int i = 0; i < 6; i++) {
        digitalWrite(buzzerPin, HIGH);
        delay(250);
        digitalWrite(buzzerPin, LOW);
        delay(250);
      }

      int passos = passosPorRotacao / 8;
      motor.step(passos);  
      delay(500);
      Serial.println("Medicação dispensada.");

      alarmTriggered = true;
    }
  }

  delay(1000);
}
