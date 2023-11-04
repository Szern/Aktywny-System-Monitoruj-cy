#include "esp_camera.h"
#include "soc/soc.h"           // Disable brownour problems
#include "soc/rtc_cntl_reg.h"  // Disable brownour problems
#include "driver/rtc_io.h"
#include <WiFi.h>
#include "time.h"
#include <WiFiClient.h>  
#include "ESP32_FTPClient.h"

// Pin definition for CAMERA_MODEL_AI_THINKER
#define CAM_PIN_PWDN    32
#define CAM_PIN_RESET   -1 //software reset will be performed
#define CAM_PIN_XCLK     0
#define CAM_PIN_SIOD    26
#define CAM_PIN_SIOC    27
#define CAM_PIN_D7      35
#define CAM_PIN_D6      34
#define CAM_PIN_D5      39
#define CAM_PIN_D4      36
#define CAM_PIN_D3      21
#define CAM_PIN_D2      19
#define CAM_PIN_D1      18
#define CAM_PIN_D0       5
#define CAM_PIN_VSYNC   25
#define CAM_PIN_HREF    23
#define CAM_PIN_PCLK    22

const bool debagowanie = true;

char* ftp_server = "domena.serwera.na.ktorym.znajduje.sie.ftp.pl";

char* ftp_user = "nazwa użytkownika ftp";
char* ftp_pass = "haslo ftp";

const char* ssid = "nazwa sieci wifi";
const char* password = "hasło do sieci wifi";

const char* ntpServer = "ntp1.tp.pl";
const long  gmtOffset_sec = 7200;   //Replace with your GMT offset (seconds)
const int   daylightOffset_sec = 0;  //Replace with your daylight offset (seconds)

ESP32_FTPClient ftp (ftp_server, ftp_user, ftp_pass, 3000, 0);

// można zmienić opóźnienia przy aktualizacji (przy starncie są ustawiane te poniżej)
int petla = 0;
int pauza_dla_ftpa = 1000;
int pauza_dla_wifi = 1000;

void kamera() { // #########################################################################################################################################################

  String rozdzielczosc = "", jakosc = "", jasnosc = "", kontrast = "", nasycenie = "", efekty = "", naswietlanie = "", kontrolanaswietlania = "", iso = "", balansbieli = "", pt = "", pw = "", pf = "";
  // String rozdzielczosc = "FRAMESIZE_SVGA", jakosc = "12", jasnosc = "1", kontrast = "2", nasycenie = "-2", efekty = "0"; // wartości domyślne

  // pobranie ustawień kamery z serwera
  while (!(ftp.isConnected())) {
    if (debagowanie) { Serial.println("Podłaczam się do FTP (kamera)"); }
    ftp.OpenConnection();
    delay(pauza_dla_ftpa);
    ftp.ChangeWorkDir("/obraz/");
  }
  ftp.InitFile("Type A");
  ftp.DownloadString("framesize.txt", rozdzielczosc); // jeśli pobierzemy większą ramkę niż zdefiniowany przy uruchomieniu kamery bufor to nastąpi restart kamery
  ftp.CloseFile();
  if (rozdzielczosc=="") { rozdzielczosc = "SVGA"; }
  ftp.InitFile("Type A");
  ftp.DownloadString("quality.txt", jakosc);
  ftp.CloseFile();
  if (jakosc=="") { jakosc = "12"; }
  ftp.InitFile("Type A");
  ftp.DownloadString("brightness.txt", jasnosc);
  ftp.CloseFile();
  if (jasnosc=="") { jasnosc = "1"; }
  ftp.InitFile("Type A");
  ftp.DownloadString("contrast.txt", kontrast);
  ftp.CloseFile();
  if (kontrast=="") { kontrast = "2"; }
  ftp.InitFile("Type A");
  ftp.DownloadString("saturation.txt", nasycenie);
  ftp.CloseFile();
  if (nasycenie=="") { nasycenie = "-2"; }
  ftp.InitFile("Type A");
  ftp.DownloadString("effects.txt", efekty);
  ftp.CloseFile();
  if (efekty=="") { efekty = "0"; }
  ftp.InitFile("Type A");
  ftp.DownloadString("exposure.txt", naswietlanie);
  ftp.CloseFile();
  if (naswietlanie=="") { naswietlanie = "300"; }
  ftp.InitFile("Type A");
  ftp.DownloadString("exposure_ctrl.txt", kontrolanaswietlania);
  ftp.CloseFile();
  if (kontrolanaswietlania=="") { kontrolanaswietlania = "1"; }
  ftp.InitFile("Type A");
  ftp.DownloadString("iso.txt", iso);
  ftp.CloseFile();
  if (iso=="") { iso = "0"; }
  ftp.InitFile("Type A");
  ftp.DownloadString("whitebalans.txt", balansbieli);
  ftp.CloseFile();
  if (balansbieli=="") { balansbieli = "0"; }
  ftp.InitFile("Type A");
  ftp.DownloadString("petla.txt", pt);
  ftp.CloseFile();
  if (pt=="") { petla = 0; } else { petla = atoi(pt.c_str()); }
  ftp.InitFile("Type A");
  ftp.DownloadString("pauza_dla_wifi.txt", pw);
  ftp.CloseFile();
  if (pw=="") { pauza_dla_wifi = 500; } else { pauza_dla_wifi = atoi(pt.c_str()); }
  ftp.InitFile("Type A");
  ftp.DownloadString("pauza_dla_ftpa.txt", pf);
  ftp.CloseFile();
  if (pf=="") { pauza_dla_ftpa = 1000; } else { pauza_dla_ftpa = atoi(pt.c_str()); }

  sensor_t * s = esp_camera_sensor_get();
  s->set_brightness(s, atoi(jasnosc.c_str()));     // -2 to 2
  s->set_contrast(s, atoi(kontrast.c_str()));       // -2 to 2
  s->set_saturation(s, atoi(nasycenie.c_str()));     // -2 to 2
  s->set_special_effect(s, atoi(efekty.c_str())); // 0 to 6 (0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia)
  s->set_whitebal(s, 1);       // 0 = disable , 1 = enable  Set white balance
  s->set_awb_gain(s, 1);       // 0 = disable , 1 = enable  Set white balance gain
  s->set_wb_mode(s, atoi(balansbieli.c_str()));        // 0 to 4 - The mode of white balace module. if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
  s->set_exposure_ctrl(s, atoi(kontrolanaswietlania.c_str()));  // 0 = disable , 1 = enable  Set exposure control
  s->set_aec2(s, 1);           // 0 = disable , 1 = enable  Auto Exposure Control
  s->set_ae_level(s, 0);       // -2 to 2  The auto exposure level to apply to the picture (when aec_mode is set to auto),
  s->set_aec_value(s, atoi(naswietlanie.c_str()));    // 0 to 1200 (300)  The Exposure value to apply to the picture (when aec_mode is set to manual), from 0 to 1200.
  s->set_gain_ctrl(s, 1);      // 0 = disable , 1 = enable
  s->set_agc_gain(s, 0);       // 0 to 30  The gain value to apply to the picture (when aec_mode is set to manual), from 0 to 30. Defaults to 0.
  s->set_gainceiling(s, (gainceiling_t)atoi(iso.c_str()));  // 0 to 6  The maximum gain allowed, when agc_mode is set to auto. This parameter seems act as “ISO” setting.
  s->set_bpc(s, 0);            // 0 = disable , 1 = enable
  s->set_wpc(s, 1);            // 0 = disable , 1 = enable
  s->set_raw_gma(s, 1);        // 0 = disable , 1 = enable
  s->set_lenc(s, 1);           // 0 = disable , 1 = enable
  s->set_hmirror(s, 0);        // 0 = disable , 1 = enable
  s->set_vflip(s, 0);          // 0 = disable , 1 = enable
  s->set_dcw(s, 1);            // 0 = disable , 1 = enable
  s->set_colorbar(s, 0);       // For tests purposes, it’s possible to replace picture get from sensor by a test color pattern.
  if (rozdzielczosc=="QVGA") {
    s->set_framesize(s, FRAMESIZE_QVGA);     // rozmiar ramki, musi się zmieścić w zadeklarowanym buforze; FRAMESIZE_ + QVGA = 5, VGA = 8, SVGA = 9, XGA = 10, SXGA = 12, UXGA = 13
  } else if (rozdzielczosc=="VGA") {
    s->set_framesize(s, FRAMESIZE_VGA);
  } else if (rozdzielczosc=="SVGA") {
    s->set_framesize(s, FRAMESIZE_SVGA);
  } else if (rozdzielczosc=="XGA") {
    s->set_framesize(s, FRAMESIZE_XGA);
  } else if (rozdzielczosc=="SXGA") {
    s->set_framesize(s, FRAMESIZE_SXGA);
  } else if (rozdzielczosc=="UXGA") {
    s->set_framesize(s, FRAMESIZE_UXGA);
  } else {
    s->set_framesize(s, FRAMESIZE_SVGA);
  }
  s->set_quality(s, atoi(jakosc.c_str())); // 10-63, for OV series camera sensors, lower number means higher quality
}

void setup() { // ##########################################################################################################################################################

  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector

  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (debagowanie) { Serial.print("Podłaczam się do WiFi..."); }
  while (WiFi.status() != WL_CONNECTED) {
    delay(pauza_dla_wifi);
    if (debagowanie) { Serial.print("."); }
  }
  if (debagowanie) { 
    Serial.print(" adres IP: ");
    Serial.println(WiFi.localIP());
  }

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  //power up the camera if PWDN pin is defined
  if (CAM_PIN_PWDN != -1) {
    pinMode(CAM_PIN_PWDN, OUTPUT);
    digitalWrite(CAM_PIN_PWDN, LOW);
  }

  static camera_config_t camera_config = {
    .pin_pwdn  = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sccb_sda = CAM_PIN_SIOD,
    .pin_sccb_scl = CAM_PIN_SIOC,
    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,
    .xclk_freq_hz = 20000000, //EXPERIMENTAL: Set to 16MHz on ESP32-S2 or ESP32-S3 to enable EDMA mode
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,
    .pixel_format = PIXFORMAT_JPEG,//YUV422,GRAYSCALE,RGB565,JPEG
    // .frame_size = FRAMESIZE_SVGA, // rozmiar bufora, musi być więszy lub równy niż rozmiar ramki; FRAMESIZE_SVGA 1s 2s 2s 3s 3s 4s 7s 10s 15s (przy q=12 FRAMESIZE_ + QVGA X 320 240 |CIF X 352 288 |VGA X 640 480 |SVGA X 800 600 |XGA X 1024 768 |SXGA X 1280 1024 |UXGA X 1600 1200, For ESP32, do not use sizes above QVGA when not JPEG. The performance of the ESP32-S series has improved a lot, but JPEG mode always gives better frame rates.
    // .jpeg_quality = 12, // 10-63, for OV series camera sensors, lower number means higher quality
    .fb_count = 1, //When jpeg mode is used, if fb_count more than one, the driver will work in continuous mode.
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY // CAMERA_GRAB_WHEN_EMPTY//CAMERA_GRAB_LATEST. Sets when buffers should be filled
  };

  String rozdzielczosc = "";

  // pobranie ustawień kamery z serwera
  while (!(ftp.isConnected())) {
    if (debagowanie) { Serial.println("Podłaczam się do FTP (setup)"); }
    ftp.OpenConnection();
    delay(pauza_dla_ftpa);
    ftp.ChangeWorkDir("/obraz/");
  }
  ftp.InitFile("Type A");
  ftp.DownloadString("framesize.txt", rozdzielczosc);
  ftp.CloseFile();
  if (rozdzielczosc=="") { rozdzielczosc = "SVGA"; }

  if (rozdzielczosc=="QVGA") {
    camera_config.frame_size = FRAMESIZE_QVGA; // 5
  } else if (rozdzielczosc=="VGA") {
    camera_config.frame_size = FRAMESIZE_VGA; // 8
  } else if (rozdzielczosc=="SVGA") {
    camera_config.frame_size = FRAMESIZE_SVGA; // 9
  } else if (rozdzielczosc=="XGA") {
    camera_config.frame_size = FRAMESIZE_XGA; // 10
  } else if (rozdzielczosc=="SXGA") {
    camera_config.frame_size = FRAMESIZE_SXGA; // 12
  } else if (rozdzielczosc=="UXGA") {
    camera_config.frame_size = FRAMESIZE_UXGA; // 13
  } else {
    camera_config.frame_size = FRAMESIZE_SVGA;
  }

  //initialize the camera
  esp_err_t err = esp_camera_init(&camera_config);

  kamera();

}

void loop() { // ###########################################################################################################################################################

  // acquire a frame
  camera_fb_t * fb = esp_camera_fb_get();

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    if (debagowanie) { Serial.print("Ponownie podłączam się do WiFi..."); }
    while (WiFi.status() != WL_CONNECTED) {
      delay(pauza_dla_wifi);
      if (debagowanie) { Serial.print("."); }
    }
    if (debagowanie) { 
      Serial.println(WiFi.localIP());
    }
  }

  String aktualizacja;
  ftp.InitFile("Type A");
  ftp.DownloadString("aktualizacja.txt", aktualizacja);
  ftp.CloseFile();
  if (aktualizacja=="tak") {
    if (debagowanie) { Serial.println("Aktualizuję"); }
    ftp.DeleteFile("aktualizacja.txt");
    ftp.InitFile("Type A");
    ftp.NewFile("aktualizacja.txt");
    ftp.CloseFile();
    kamera();
  }

  // przygotowanie nazwy pliku na podstawie aktualnego czasu
  time_t now;
  time(&now);
  char czas [12];
  int ret = snprintf(czas, sizeof(czas), "%ld", now);
  char* rozszerzenie = "\.jpg";
  int len = strlen(czas) + 5;
  char* nazwa_pliku = (char*) malloc(len);
  int i=0,j=0;
  for(i=0;i<len;i++){
    if(i<strlen(czas)){
      nazwa_pliku[i]=czas[i];
    } else {
      nazwa_pliku[i]=rozszerzenie[j++];
    }
  }
  while (!(ftp.isConnected())) {
    ftp.CloseConnection();
    delay(pauza_dla_ftpa);
    if (debagowanie) { Serial.println("Podłaczam się do FTP (loop2)"); }
    ftp.OpenConnection();
    ftp.ChangeWorkDir("/obraz/");
  }

  // wysłanie zdjęcia na serwer
  if (debagowanie) { Serial.println("wysyłam"); }
  ftp.InitFile("Type I");
  ftp.NewFile(nazwa_pliku);
  ftp.WriteData( fb->buf, fb->len );
  ftp.CloseFile();

  //return the frame buffer back to the driver for reuse
  esp_camera_fb_return(fb);

  if (debagowanie) {
    Serial.print("wysłany: ");
    Serial.println(nazwa_pliku);
  }
  // zapisanie pustego pliku informującego serwer o pojawieniu się nowego pliku z obrazem
  ftp.InitFile("Type A");
  ftp.NewFile("nowy.txt");
  ftp.CloseFile();

  delay(petla);

}
