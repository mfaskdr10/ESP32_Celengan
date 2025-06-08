#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
//#include <AsyncTCP.h>


#define S2 2
#define S3 15
#define sensorOut 4
#define EEPROM_SIZE 512
#define RESET_BUTTON 5

int Red = 0, Green = 0, Blue = 0;
int Frequency = 0;
int Uang = 0;
int lastUang = 0;
bool statusUang = 0;
bool msg = 0;

AsyncWebServer server(80);
DNSServer dnsServer;
const byte DNS_PORT = 53;

// Fungsi setup
void setup() {
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(sensorOut, INPUT);
  pinMode(RESET_BUTTON, INPUT_PULLUP);
  Serial.begin(115200);

  EEPROM.begin(EEPROM_SIZE);
  Uang = EEPROM.readInt(0);

  WiFi.softAP("ESP32_Celengan", "12345678");
  delay(100);
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

  Serial.println("Access Point started");
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());

  // Rute halaman utama
  server.on("/", HTTP_GET, handleRoot);

  // Rute untuk ambil saldo saja
  server.on("/saldo", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", String(Uang));
  });

  // Rute untuk reset saldo
  server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request) {
    Uang = 0;
    EEPROM.writeInt(0, Uang);
    EEPROM.commit();
    request->redirect("/");
  });

   if (!LittleFS.begin()) {
    Serial.println("Gagal mount LittleFS!");
    //agar webserv tetep jalan
  } else {
    Serial.println("LittleFS mounted successfully");
    server.on("/icon.jpg", HTTP_GET, [](AsyncWebServerRequest * request) {
      request->send(LittleFS, "icon.jpg", "image/jpg");
    });
  }

  server.onNotFound([](AsyncWebServerRequest *request) {
    request->redirect("/");
  }); //capative portal like mikrotik wifi

  server.begin();
}

// Fungsi loop
void loop() {
  dnsServer.processNextRequest();

  if (digitalRead(RESET_BUTTON) == LOW) {
    delay(500);
    if (digitalRead(RESET_BUTTON) == LOW) {
      Uang = 0;
      EEPROM.writeInt(0, Uang);
      EEPROM.commit();
      Serial.println("Saldo direset ke 0");
    }
  }

  // Deteksi warna
  Red = getRed();
  Green = getGreen();
  Blue = getBlue();

  Serial.print("R: ");
  Serial.print(Red);
  Serial.print(" G: ");
  Serial.print(Green);
  Serial.print(" B: ");
  Serial.println(Blue);

  if (detectNominal(2000, 22, 25, 20, 23, 16, 19)) {
    processNominal(2000);
  } else if (detectNominal(1000, 19, 22, 21, 24, 23, 26)) {
    processNominal(1000);
  } else if (detectNominal(5000, 17, 20, 24, 27, 24, 27)) {
    processNominal(5000);
  } else if (detectNominal(10000, 25, 28, 28, 31, 21, 24)) {
    processNominal(10000);
  } else if (detectNominal(20000, 26, 30, 22, 26, 21, 25)) {
    processNominal(20000);
  } else if (detectNominal(50000, 35, 39, 30, 34, 22, 26)) {
    processNominal(50000);
  } else if (detectNominal(100000, 19, 23, 31, 35, 24, 28)) {
    processNominal(100000);
  } else if (Red > 100 && Green > 100 && Blue > 100) {
    statusUang = 0;
    msg = 0;
  }
}

void processNominal(int nominal) {
  Uang += nominal;
  EEPROM.writeInt(0, Uang);
  EEPROM.commit();
  statusUang = 1;
}

bool detectNominal(int nominal, int rMin, int rMax, int gMin, int gMax, int bMin, int bMax) {
  if (rMin == 0 && rMax == 0 && gMin == 0 && gMax == 0 && bMin == 0 && bMax == 0) {
    return false;
  }
  return (Red > rMin && Red < rMax && Green > gMin && Green < gMax && Blue > bMin && Blue < bMax && statusUang == 0);
}

// Ambil warna dari sensor
int getRed() {
  digitalWrite(S2, LOW);
  digitalWrite(S3, LOW);
  return pulseIn(sensorOut, LOW);
}

int getGreen() {
  digitalWrite(S2, HIGH);
  digitalWrite(S3, HIGH);
  return pulseIn(sensorOut, LOW);
}

int getBlue() {
  digitalWrite(S2, LOW);
  digitalWrite(S3, HIGH);
  return pulseIn(sensorOut, LOW);
}

// Halaman web utama
void handleRoot(AsyncWebServerRequest *request) {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <title>Celengan Pintar</title>
    <script>
      function updateSaldo() {
        fetch("/saldo")
          .then((res) => res.text())
          .then((data) => {
            document.getElementById("saldo").innerText = "Rp " + data;
          });
      }
      setInterval(updateSaldo, 2000);
      window.onload = updateSaldo;

      // Format the number to Indonesian locale
      function formatNumberToLocale(id) {
        const element = document.getElementById(id);
        if (element) {
          const number = parseInt(element.innerText.replace(/[^0-9]/g, ""));
          element.innerText = number.toLocaleString("id-ID");
        }
      }
      document.addEventListener("DOMContentLoaded", function () {
        formatNumberToLocale("nominal");
      });
    </script>

    <style>
      * {
        margin: 0;
        padding: 0;
        box-sizing: border-box;
      }

      body {
        text-align: center;
        font-family: sans-serif;
        max-width: 500px;
        width: 100%;
        margin: 0 auto;
        /* background-color: aliceblue; */
      }

      .header-1 {
        position: absolute;
        color: #ffff;
        padding-top: 30px;
      }

      .atas {
        width: 100%;
        height: 250px;
        object-fit: cover;
        position: relative;
        display: flex;
        justify-content: center;
        background-color: #0d5b1a;
        border-bottom-left-radius: 50%;
        border-bottom-right-radius: 50%;
      }

      .bawah {
        padding: 80px 20px;
      }

      .bawah button {
        font-size: 20px;
        width: 100%;
        padding: 10px 0px;
        border-radius: 15px;
        background-color: #0d5b1a;
        color: #ffff;
        margin: 0 auto;
      }

      .saldo {
        border: 1px solid black;
        margin-bottom: 20px;
        border-radius: 15px;
        box-shadow: 0px 0px 4px 0px #3d3d3d;
        padding: 20px 10px;
        background-color: #ffffff;
      }

      .saldo h2 {
        font-size: 20px;
        text-align: start;
        margin-left: 12px;
        font-weight: 400;
        margin-bottom: 5px;
      }

      .saldo h1 {
        font-size: 30px;
        text-align: start;
        margin-left: 12px;
      }

      .circle {
        width: 200px;
        height: 200px;
        border-radius: 50%;
        background-color: #003008;
        border: 5px solid #e1ffe6;
        display: flex;
        justify-content: center;
        align-items: center;
        position: absolute;
        top: 100px;
      }
    </style>
  </head>
  <body>
    <div class="atas">
      <h1 class="header-1">Celengan Pintar</h1>
      <div class="circle">
        <svg
          version="1.1"
          id="Layer_1"
          xmlns="http://www.w3.org/2000/svg"
          xmlns:xlink="http://www.w3.org/1999/xlink"
          x="0px"
          y="0px"
          width="70%"
          viewBox="0 0 512 448"
          enable-background="new 0 0 512 448"
          xml:space="preserve"
        >
        <!-- FFA8D1 -->
          <path
            fill="#FFA8D1"
            opacity="1.000000"
            stroke="none"
            d="
M513.000000,231.000000 
	C513.000000,247.687561 513.000000,264.375122 512.664062,281.202393 
	C511.894257,282.004486 511.230438,282.613007 511.060669,283.337341 
	C507.790222,297.292053 505.462891,311.538788 501.176117,325.170227 
	C491.055725,357.351776 480.208527,389.310760 469.213013,421.207275 
	C465.939423,430.703583 466.445282,442.126801 456.618073,448.679077 
	C456.745392,448.786041 456.872681,448.893036 457.000000,449.000000 
	C435.979095,449.000000 414.958221,449.000000 393.785187,448.734314 
	C387.990936,444.766846 385.947876,439.321777 384.140472,432.892975 
	C379.621277,416.818420 373.866394,401.091309 368.599915,385.202515 
	C347.412140,385.202515 326.299225,385.202515 305.487213,385.202515 
	C301.011780,398.446045 296.791901,411.476105 292.186646,424.368500 
	C289.162292,432.835175 289.629913,443.041107 280.617279,448.678497 
	C280.744843,448.785675 280.872437,448.892822 281.000000,449.000000 
	C259.979095,449.000000 238.958206,449.000000 217.785248,448.733948 
	C211.959335,444.786407 209.955246,439.313995 208.145996,432.894928 
	C203.615784,416.821960 197.867371,401.092316 192.863586,385.997101 
	C180.232162,384.615021 167.843994,384.165222 155.853409,381.745422 
	C135.457474,377.629333 115.698669,371.024811 96.931915,361.937958 
	C70.246765,349.016937 45.450825,333.352173 24.897858,311.525757 
	C15.403437,301.443085 7.499609,290.552094 2.936985,277.384583 
	C2.724988,276.772766 1.665037,276.454803 1.000002,276.000000 
	C1.000000,268.645752 1.000000,261.291504 1.273548,253.787109 
	C3.172280,251.444916 4.692090,249.164642 6.440888,247.076080 
	C14.020648,238.023636 24.820438,234.713364 35.385483,231.008102 
	C48.408737,226.440720 62.261951,223.601410 73.416534,214.941223 
	C78.340729,211.118210 82.068871,205.760986 86.370583,201.128647 
	C98.918449,187.616333 112.892517,175.988586 130.536896,168.373627 
	C129.342422,160.518112 128.601013,152.712265 126.893852,145.123688 
	C124.523422,134.586868 121.589310,124.175865 114.324203,115.599274 
	C113.585976,114.727783 113.908920,111.755714 114.817299,110.891830 
	C119.650848,106.295013 125.953590,105.295288 132.133026,106.179359 
	C143.499451,107.805511 153.731766,112.743629 162.897095,119.508873 
	C173.263977,127.161034 183.242767,135.338959 192.594879,142.665146 
	C208.504715,134.220734 224.319077,124.716637 241.055389,117.290314 
	C254.790253,111.195801 269.621368,107.571869 284.302795,102.941254 
	C289.078491,107.334953 293.518005,111.640236 297.578186,116.044243 
	C291.905212,117.503304 286.405701,118.350594 281.375793,120.368446 
	C276.357147,122.381783 272.815369,126.474319 273.147278,132.330368 
	C273.473450,138.085770 277.084137,141.953827 282.405121,143.512848 
	C285.777771,144.501022 289.589386,144.701721 293.110779,144.341949 
	C305.149139,143.112045 317.109467,140.891617 329.167694,140.136856 
	C346.816589,139.032166 364.332458,140.806000 381.678345,144.490158 
	C384.877350,145.169632 388.652161,144.433701 391.871399,143.425583 
	C396.983734,141.824646 400.449371,138.069763 400.858124,132.551697 
	C401.292175,126.692535 397.866516,122.486710 392.842957,120.474365 
	C387.505005,118.336075 381.682495,117.407356 376.263245,115.688171 
	C380.978943,110.904510 385.503021,106.383125 390.439697,101.924225 
	C424.041931,108.593338 452.671234,123.456757 475.683990,148.683701 
	C491.153046,140.956787 501.437683,143.814056 511.191345,158.592712 
	C511.436676,158.964447 512.381409,158.874603 513.000000,159.000000 
	C513.000000,165.354401 513.000000,171.708786 512.730103,178.213989 
	C508.837769,182.657227 505.215302,186.949677 502.100769,190.640274 
	C505.058319,203.415375 508.006927,216.264435 511.067932,229.086670 
	C511.246674,229.835526 512.334961,230.367249 513.000000,231.000000 
M120.145050,238.561752 
	C128.178299,242.676773 135.902985,241.504272 140.944183,235.404755 
	C145.678482,229.676529 145.806488,221.036957 141.237396,215.613373 
	C134.513184,207.631622 124.653984,206.864883 117.638855,213.778122 
	C110.419006,220.893097 111.211266,231.030396 120.145050,238.561752 
z"
          />
          <path
            fill="#FFCC66"
            opacity="1.000000"
            stroke="none"
            d="
M352.531342,0.999998 
	C353.462799,1.661376 353.803040,2.749234 354.406067,2.922520 
	C376.366730,9.233248 391.419708,23.260288 398.153412,44.866138 
	C404.271454,64.496613 402.538696,83.787926 390.144653,101.540405 
	C385.503021,106.383125 380.978943,110.904510 375.946045,115.835487 
	C362.890015,125.290245 348.932892,129.711838 333.431244,128.947693 
	C320.268402,128.298813 308.672241,123.308067 297.957520,115.945511 
	C293.518005,111.640236 289.078491,107.334953 284.302795,102.941254 
	C280.146912,94.578995 274.986237,86.912491 273.802795,77.497559 
	C272.235535,65.028915 272.106659,52.761696 277.027863,40.935833 
	C283.045197,26.475807 292.686066,15.141873 306.701294,7.912532 
	C311.490143,5.442332 316.654297,3.699731 321.323486,1.312342 
	C331.354218,1.000000 341.708466,1.000000 352.531342,0.999998 
M362.606598,84.057121 
	C371.079620,71.524651 371.183167,58.733913 362.911713,46.368153 
	C356.135437,36.237675 343.385071,31.312006 330.529816,33.858513 
	C316.473328,36.642982 306.170990,48.276348 305.032166,62.650379 
	C303.920074,76.687332 312.281403,89.593819 325.990082,95.000725 
	C338.600311,99.974388 353.708618,95.659271 362.606598,84.057121 
z"
          />
          <path
            fill="#CD5F92"
            opacity="1.000000"
            stroke="none"
            d="
M297.578186,116.044235 
	C308.672241,123.308067 320.268402,128.298813 333.431244,128.947693 
	C348.932892,129.711838 362.890015,125.290245 375.754395,116.097778 
	C381.682495,117.407356 387.505005,118.336075 392.842957,120.474365 
	C397.866516,122.486710 401.292175,126.692535 400.858124,132.551697 
	C400.449371,138.069763 396.983734,141.824646 391.871399,143.425583 
	C388.652161,144.433701 384.877350,145.169632 381.678345,144.490158 
	C364.332458,140.806000 346.816589,139.032166 329.167694,140.136856 
	C317.109467,140.891617 305.149139,143.112045 293.110779,144.341949 
	C289.589386,144.701721 285.777771,144.501022 282.405121,143.512848 
	C277.084137,141.953827 273.473450,138.085770 273.147278,132.330368 
	C272.815369,126.474319 276.357147,122.381783 281.375793,120.368446 
	C286.405701,118.350594 291.905212,117.503304 297.578186,116.044235 
z"
          />
          <path
            fill="#CD5F92"
            opacity="1.000000"
            stroke="none"
            d="
M119.843109,238.340393 
	C111.211266,231.030396 110.419006,220.893097 117.638855,213.778122 
	C124.653984,206.864883 134.513184,207.631622 141.237396,215.613373 
	C145.806488,221.036957 145.678482,229.676529 140.944183,235.404755 
	C135.902985,241.504272 128.178299,242.676773 119.843109,238.340393 
z"
          />
          <path
            fill="#CCA352"
            opacity="1.000000"
            stroke="none"
            d="
M362.404480,84.357597 
	C353.708618,95.659271 338.600311,99.974388 325.990082,95.000725 
	C312.281403,89.593819 303.920074,76.687332 305.032166,62.650379 
	C306.170990,48.276348 316.473328,36.642982 330.529816,33.858513 
	C343.385071,31.312006 356.135437,36.237675 362.911713,46.368153 
	C371.183167,58.733913 371.079620,71.524651 362.404480,84.357597 
z"
          />
        </svg>
      </div>
    </div>
    <div class="bawah">
      <div class="saldo">
        <h2>Total Uang Sekarang :</h2>
        <h1 id="saldo">Rp 0</h1>
      </div>
      <a href="/reset"><button>Reset Saldo</button></a>
    </div>
  </body>
</html>
  )rawliteral";
  request->send(200, "text/html", html);
}
