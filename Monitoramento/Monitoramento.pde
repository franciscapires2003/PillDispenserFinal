import gab.opencv.*;
import processing.video.*;
import java.util.*;
import java.io.*;
import java.text.*;
import java.net.*;
import java.awt.Rectangle; 

Capture camera;
OpenCV opencv;
ArrayList<String> horasDetecao;
boolean pessoaDetetadaHoje = false;
boolean jaVerificado = false;

String horaInicioEsperada = "09:00";
String horaFimEsperada = "09:15";


String ipESP = "192.168.100.107";

void setup() {
  size(640, 480);

  camera = new Capture(this, 640, 480);
  camera.start();

  opencv = new OpenCV(this, 640, 480);
  opencv.loadCascade(OpenCV.CASCADE_FRONTALFACE);

  horasDetecao = new ArrayList<String>();
  carregarHistoricoDetecoes();
  calcularIntervaloEsperado();
  println("Janela esperada: " + horaInicioEsperada + " - " + horaFimEsperada);
}

void draw() {
  if (camera.available()) {
    camera.read();
  }

  image(camera, 0, 0);
  opencv.loadImage(camera);

  Rectangle[] faces = opencv.detect();

  noFill();
  stroke(0, 255, 0);
  strokeWeight(3);

  if (faces.length > 0) {
    for (Rectangle face : faces) {
      rect(face.x, face.y, face.width, face.height);
    }

    if (deveRegistarHoraNova()) {
      String horaAtual = horaAtual();
      horasDetecao.add(horaAtual);
      guardarHoraNoFicheiro(horaAtual);
      println("Pessoa detetada às " + horaAtual);

      if (estaDentroDoIntervaloEsperado(horaAtual)) {
        pessoaDetetadaHoje = true;
      }

      
      enviarDeteccaoParaESP(horaAtual);
    }
  }

  mostrarHistoricoHoje();
  verificarAusencia();
}



String horaAtual() {
  return new SimpleDateFormat("HH:mm").format(new Date());
}

int tempoUltimoRegisto = 0;
boolean deveRegistarHoraNova() {
  int agora = millis();
  if (agora - tempoUltimoRegisto > 5000) {
    tempoUltimoRegisto = agora;
    return true;
  }
  return false;
}

boolean estaDentroDoIntervaloEsperado(String hora) {
  return hora.compareTo(horaInicioEsperada) >= 0 && hora.compareTo(horaFimEsperada) <= 0;
}

void mostrarHistoricoHoje() {
  fill(0, 150);
  rect(0, 0, 300, 20 + horasDetecao.size() * 15);
  fill(255);
  textSize(12);
  text("Hoje:", 10, 15);
  for (int i = 0; i < horasDetecao.size(); i++) {
    text(horasDetecao.get(i), 10, 30 + i * 15);
  }
}

void verificarAusencia() {
  String agora = horaAtual();
  if (!jaVerificado && agora.compareTo(horaFimEsperada) > 0) {
    if (!pessoaDetetadaHoje) {
      println("Pessoa NÃO DETETADA no horário!");
      fill(255, 0, 0);
      textSize(20);
      text("Pessoa NÃO DETETADA!", 10, height - 30);
    }
    jaVerificado = true;
  }
}



void guardarHoraNoFicheiro(String hora) {
  String ficheiro = sketchPath("detections.csv");
  try {
    PrintWriter writer = new PrintWriter(new FileOutputStream(ficheiro, true));
    writer.println(hora);
    writer.close();
    println("Hora gravada no ficheiro: " + hora);
  } catch (Exception e) {
    println("Erro ao guardar deteção: " + e.getMessage());
  }
}

void carregarHistoricoDetecoes() {
  try {
    String[] linhas = loadStrings("detections.csv");
    if (linhas != null) {
      for (String linha : linhas) {
        horasDetecao.add(linha.trim());
      }
    }
  } catch (Exception e) {
    println("Ficheiro ainda não existe.");
  }
}

void calcularIntervaloEsperado() {
  HashMap<String, Integer> contagem = new HashMap<String, Integer>();

  for (String hora : horasDetecao) {
    if (hora.length() >= 5) {
      String minuto = hora.substring(0, 5);
      contagem.put(minuto, contagem.getOrDefault(minuto, 0) + 1);
    }
  }

  ArrayList<String> minutosFrequentes = new ArrayList<String>();
  for (String minuto : contagem.keySet()) {
    if (contagem.get(minuto) >= 3) {
      minutosFrequentes.add(minuto);
    }
  }

  if (minutosFrequentes.size() > 0) {
    Collections.sort(minutosFrequentes);
    horaInicioEsperada = minutosFrequentes.get(0);
    horaFimEsperada = minutosFrequentes.get(minutosFrequentes.size() - 1);
  }
}



void enviarDeteccaoParaESP(String hora) {
  try {
    String url = "http://" + ipESP + "/log?id=1&hora=" + URLEncoder.encode(hora, "UTF-8");
    println("A enviar para ESP32: " + url);
    URL u = new URL(url);
    URLConnection conn = u.openConnection();
    conn.setConnectTimeout(2000);
    conn.getInputStream();  
  } catch (Exception e) {
    println("Erro ao enviar para ESP32: " + e.getMessage());
  }
}
