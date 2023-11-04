#include "pico/stdlib.h"
#include <WiFi.h>

const char * ssid = "nazwa sieci wifi";
const char * pass = "hasło do sieci wifi";
int status = WL_IDLE_STATUS;
WiFiClient client;
int znacznik = 1;
int wyslij = 0;
int slady[100];
time_t ostatni_slad = 0;
time_t sladyt[100];
time_t ostatni;
static const char * const czujki[] = {"c2b", "c2c", "c3b", "c3c", "c4", "c3d", "c3e"};
static const char * const strefy[] = {"na drodze do domu", "na drodze przy północnej ścianie", "przed drzwiami (pszczelarnia}", "przed drzwiami (garaż)", "przed garażem", "przy południowej ścianie", "przed oknami"};
time_t  syrena[2];
time_t  lampa[3];

const uint s[2] = {12,13};
const uint kam[2] = {10,11};
const uint l[3] = {9,8,7};
const uint c[7] = {0,1,2,3,4,5,6};

void wifi() {
  while (status != WL_CONNECTED) {
    status = WiFi.begin(ssid, pass);
    delay(1000);
  }
}

// wysłanie informacji o naruszeniu spokoju czujek na serwer

void alarm(char * czujka) { // ###########################################################################################################
    if (client.connect("subdomena.serwera.na.ktorym.znajduje.sie.panel.pl", 80)) {
    client.print("GET /alarm.php?czujka=");
    client.println(czujka);
    client.println(" HTTP/1.1");
    client.println("Host: subdomena.serwera.na.ktorym.znajduje.sie.panel.pl");
    client.println("Connection: close");
    client.println();
    client.println();
    client.stop();
  }
}

// pobranie czasu z serwerka (dla potrzeb wysyłania SMS-ów)

char * podaj_czas() { // #################################################################################################################
  char tag[7] = "<body>";
  size_t r =  19;
  char * buf;
  buf = (char*)malloc(r);
  if (client.connect("subdomena.serwera.na.ktorym.znajduje.sie.panel.pl", 80)) {
    client.println("GET /czas.php HTTP/1.1");
    client.println("Host: subdomena.serwera.na.ktorym.znajduje.sie.panel.pl");
    client.println();
    client.println();
  }
  while (!client.available()) {
    delay(100);
  }
  while (client.available()) {
    char c = client.read();
    if ( c == tag[0] ) {
      int czy = 1;
      int i = 0;
      while (i < 5) { // długość tagu minus jeden
        if ( c == tag[i] ) {
          c = client.read();
        } else {
          czy = 0;
        }
        i++;
      }
      if (czy == 1) {
        int j = 0;
        c = client.read();
        while ( c != tag[0] ) {
          buf[j++] = c;
          c = client.read();
        }
        buf[j] = '\0';
      }
    }
  }
  client.stop();
  return buf;
}

// SMS-y - obsługa modemu
// pobranie_tokena()
// pobierz_smsindex()
// sprawdzenie_SMS()
// usun_SMS()
// wyslij_SMS
// odczytaj_SMS

char * pobranie_tokena() { // ############################################################################################################
  char tag[8] = "<token>";
  size_t r =  11;
  char * buf;
  buf = (char*)malloc(r);
  if (status != WL_CONNECTED) {
    wifi();
  }
  if (client.connect("192.168.2.1", 80)) {
    client.println("GET /api/webserver/token HTTP/1.1");
    client.println("Host: 192.168.2.1");
    client.println();
    client.println();
  }
  while (!client.available()) {
    delay(100);
  }
  while (client.available()) {
    char c = client.read();
    if ( c == tag[0] ) {
      int czy = 1;
      int i = 0;
      while ( i < 6 ) {
        if ( c == tag[i] ) {
          c = client.read();
        } else {
          czy = 0;
        }
        i++;
      }
      if (czy == 1) {
        int j = 0;
        c = client.read();
        while ( c != tag[0] ) {
          buf[j++] = c;
          c = client.read();
        }
        buf[j] = '\0';
      }
    }
  }
  client.stop();
  return buf;
}

char * pobierz_smsindex(int boks) { // ####################################################################################################
  char * token = pobranie_tokena();
  char tag[8] = "<Index>";
  size_t r =  6;
  char * buf;
  buf = (char*)malloc(r);
  if (status != WL_CONNECTED) {
    wifi();
  }
  if (client.connect("192.168.2.1", 80)) {
    client.println("POST /api/sms/sms-list HTTP/1.1");
    client.println("Host: 192.168.2.1");
    client.println("Content-Type: text/xml");
    client.print("__RequestVerificationToken: ");
    client.println(token);
    client.print("Content-Length: 227");
    client.println();
    client.println();
    client.println("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
    client.println("<request>");
    client.println("<PageIndex>1</PageIndex>");
    client.println("<ReadCount>1</ReadCount>");
    client.print("<BoxType>");
    client.print(boks); // 2 - wysłane,
    client.println("</BoxType>");
    client.println("<SortType>0</SortType>");
    client.println("<Ascending>0</Ascending>");
    client.println("<UnreadPreferred>0</UnreadPreferred>");
    client.println("</request>");
    client.println();
    while (!client.available()) {
      delay(100);
    }
    while (client.available()) {
      char c = client.read();
      if ( c == tag[0] ) {
        int czy = 1;
        int i = 0;
        while (i < 6) {
          if ( c == tag[i] ) {
            c = client.read();
          } else {
            czy = 0;
          }
          i++;
        }
        if (czy == 1) {
          int j = 0;
          c = client.read();
          while ( c != tag[0] ) {
            if ( c != tag[0] ) {
              buf[j] = c;
              c = client.read();
              j++;
            }
          }
        buf[j] = '\0';
        }
      }
    }
    client.stop();
  }
  return buf;
}

int sprawdzenie_SMS() { // ############################################################################################################
  char tag[14] = "<LocalUnread>";
  if (status != WL_CONNECTED) {
    wifi();
  }
  char buf[10];
  if (client.connect("192.168.2.1", 80)) {
    client.println("GET /api/sms/sms-count HTTP/1.1");
    client.println("Host: 192.168.2.1");
    client.println();
    client.println();
  }
  while (!client.available()) {
    delay(100);
  }
  while (client.available()) {
    char c = client.read();
    if ( c == tag[0] ) {
      int czy = 1;
      int i = 0;
      while (i < 12) {
        if ( c == tag[i] ) {
          c = client.read();
        } else {
          czy = 0;
        }
        i++;
      }
      if (czy == 1) {
        int j = 0;
        c = client.read();
        while ( c != tag[0] ) {
          if ( c != tag[0] ) {
            buf[j] = c;
            c = client.read();
            j++;
          }
        }
        buf[j] = '\0';
      }
    }
  }
  client.stop();
  int buffo = atoi(buf);
  return buffo;
}

void usun_SMS(char * smsindex) { // ########################################################################################################
  char * token = pobranie_tokena();
  if (status != WL_CONNECTED) {
    wifi();
  }
  if (client.connect("192.168.2.1", 80)) {
    client.println("POST /api/sms/delete-sms HTTP/1.1");
    client.println("Host: 192.168.2.1");
    client.println("Content-Type: text/xml");
    client.print("__RequestVerificationToken: ");
    client.println(token);
    client.println("Content-Length: 83");
    client.println();
    client.println();
    client.println("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
    client.print("<request><Index>");
    client.print(smsindex);
    client.println("</Index></request>");
    client.println();
    client.stop();
  }
}

void wyslij_SMS(char tekst[]) { // ########################################################################################################
  char * token = pobranie_tokena();
  char * czas = podaj_czas();
  int cl = 263 + strlen(tekst);
  if (status != WL_CONNECTED) {
    wifi();
  }
  if (client.connect("192.168.2.1", 80)) {
    client.println("POST /api/sms/send-sms HTTP/1.1");
    client.println("Host: 192.168.2.1");
    client.println("Content-Type: text/xml");
    client.print("__RequestVerificationToken: ");
    client.println(token);
    client.print("Content-Length: ");
    client.println(cl);
    client.println();
    client.println("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
    client.println("<request>");
    client.println("<Index>-1</Index>");
    client.println("<Phones><Phone>+48123456789</Phone></Phones>"); // numer telefonu, na który wysyłane są powiadomienia
    client.println("<Sca></Sca>");
    client.print("<Content>");
    if ( strcmp(tekst,strefy[0]) || strcmp(tekst,strefy[1]) || strcmp(tekst,strefy[2]) || strcmp(tekst,strefy[3]) || strcmp(tekst,strefy[4]) || strcmp(tekst,strefy[5]) || strcmp(tekst,strefy[6]) ) {
      client.print("Naruszono strefę: ");
    } else {
      client.print("Forward z modemu: ");
    }
    client.print(tekst);
    client.println("</Content>");
    client.print("<Length>");
    client.print(strlen(tekst)+1);
    client.println("</Length>");
    client.println("<Reserved>1</Reserved>");
    client.print("<Date>");
    client.print(czas);
    client.println("</Date>");
    client.println("</request>");
    client.println();
    client.println();

    char tag[11] = "<response>";
    char buf[3];
    while (!client.available()) {
      delay(100);
    }
    while (client.available()) {
      char c = client.read();
      if ( c == tag[0] ) {
        int czy = 1;
        int i = 0;
        while (i < 9) {
          if ( c == tag[i] ) {
            c = client.read();
          } else {
            czy = 0;
          }
          i++;
        }
        if (czy == 1) {
          int j = 0;
          c = client.read();
          while ( c != tag[0] ) {
            if ( c != tag[0] ) {
              buf[j] = c;
              c = client.read();
              j++; 
            }
          }
          buf[j] = '\0'; // odpowiedź to OK, ale jej nie sprawdzam, zależy mi tylko, aby poczekać aż modem skończy wysyłanie i będzie wolny
        }
      }
    }
    client.stop();
    int buffo = 0;
    while ( buffo == 0 ) {
      char tag[14] = "<LocalOutbox>";
      if (status != WL_CONNECTED) {
        wifi();
      }
      char buf[10];
      if (client.connect("192.168.2.1", 80)) {
        client.println("GET /api/sms/sms-count HTTP/1.1");
        client.println("Host: 192.168.2.1");
        client.println();
        client.println();
      }
      while (!client.available()) {
        delay(100);
      }
      while (client.available()) {
        char c = client.read();
        if ( c == tag[0] ) {
          int czy = 1;
          int i = 0;
          while (i < 12) {
            if ( c == tag[i] ) {
              c = client.read();
            } else {
              czy = 0;
            }
            i++;
          }
          if (czy == 1) {
            int j = 0;
            c = client.read();
            while ( c != tag[0] ) {
              if ( c != tag[0] ) {
                buf[j] = c;
                c = client.read();
                j++;
              }
            }
            buf[j] = '\0';
          }
        }
      }
      client.stop();
      buffo = atoi(buf);
    }
    if (buffo) {
    char * smsindex = pobierz_smsindex(2);
    usun_SMS(smsindex);
    }
  }
}

char * odczytaj_SMS() { // ###############################################################################################################
  char * token = pobranie_tokena();

  size_t r1 =  11;
  char * buf1;
  buf1 = (char*)malloc(r1);

  size_t r2 =  13;
  char * buf2;
  buf2 = (char*)malloc(r2);

  size_t r3 =  165;
  char * buf3;
  buf3 = (char*)malloc(r3);

  if (status != WL_CONNECTED) {
    wifi();
  }
  if (client.connect("192.168.2.1", 80)) {
    client.println("POST /api/sms/sms-list HTTP/1.1");
    client.println("Host: 192.168.2.1");
    client.println("Content-Type: text/xml");
    client.print("__RequestVerificationToken: ");
    client.println(token);
    client.println("Content-Length: 227");
    client.println();
    client.println("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
    client.println("<request>");
    client.println("<PageIndex>1</PageIndex>");
    client.println("<ReadCount>1</ReadCount>");
    client.println("<BoxType>1</BoxType>");
    client.println("<SortType>0</SortType>");
    client.println("<Ascending>0</Ascending>");
    client.println("<UnreadPreferred>0</UnreadPreferred>");
    client.println("</request>");
    client.println();
    client.println();

    while (!client.available()) {
      delay(100);
    }

    char tag1[8] = "<Index>";
    char c;
    int czy;
    int i;
    int znaczek = 0;
    while (client.available() && (znaczek == 0)) {
      if (znaczek == 0) {
        c = client.read();
        if ( c == tag1[0] ) {
          czy = 1;
          i = 0;
          while (i < 6) {
            if ( c == tag1[i] ) {
              c = client.read();
            } else {
              czy = 0;
            }
            i++;
          }
          if (czy == 1) {
            int j = 0;
            c = client.read();
            while ( c != tag1[0] ) {
              if ( c != tag1[0] ) {
                buf1[j] = c;
                c = client.read();
                j++;
              }
            }
            buf1[j] = '\0';
            znaczek = 1;
          }
        }
      }
    }

    char tag2[8] = "<Phone>";
    znaczek = 0;
    while (client.available() && (znaczek == 0)) {
      if (znaczek == 0) {
        c = client.read();
        if ( c == tag2[0] ) {
          czy = 1;
          i = 0;
          while (i < 6) {
            if ( c == tag2[i] ) {
              c = client.read();
            } else {
              czy = 0;
            }
            i++;
          }
          if (czy == 1) {
            int j = 0;
            c = client.read();
            while ( c != tag2[0] ) {
              if ( c != tag2[0] ) {
                buf2[j] = c;
                c = client.read();
                j++;
              }
            }
            buf2[j] = '\0';
            znaczek = 1;
          }
        }
      }
    }

    char tag3[10] = "<Content>";
    znaczek = 0;
    while (client.available() && (znaczek == 0)) {
      if (znaczek == 0) {
        c = client.read();
        if ( c == tag3[0] ) {
          czy = 1;
          i = 0;
          while (i < 8) {
            if ( c == tag3[i] ) {
              c = client.read();
            } else {
              czy = 0;
            }
            i++;
          }
          if (czy == 1) {
            int j = 0;
            c = client.read();
            while ( c != tag3[0] ) {
              if ( c != tag3[0] ) {
                buf3[j] = c;
                c = client.read();
                j++;
              }
            }
            buf3[j] = '\0';
            znaczek = 0;
          }
        }
      }
    }
    client.stop();
  }

  int buffo = 0;
  while ( buffo == 0 ) {
    char tag[14] = "<LocalUnread>";
    char buf[10];
    if (status != WL_CONNECTED) {
      wifi();
    }
    if (client.connect("192.168.2.1", 80)) {
      client.println("GET /api/sms/sms-count HTTP/1.1");
      client.println("Host: 192.168.2.1");
      client.println();
      client.println();
    }
    while (!client.available()) {
      delay(100);
    }
    while (client.available()) {
      char c = client.read();
      if ( c == tag[0] ) {
        int czy = 1;
        int i = 0;
        while (i < 12) {
          if ( c == tag[i] ) {
            c = client.read();
          } else {
            czy = 0;
          }
          i++;
        }
        if (czy == 1) {
          int j = 0;
          c = client.read();
          while ( c != tag[0] ) {
            if ( c != tag[0] ) {
              buf[j] = c;
              c = client.read();
              j++;
            }
          }
          buf[j] = '\0';
        }
      }
    }
    client.stop();
    buffo = atoi(buf);
  }
  usun_SMS(buf1);
  if ( strcmp(buf2,"+48123456789") && strcmp(buf2,"+48012345678") && strcmp(buf2,"+48234567890") ) { // numery telefonów, z których można sterować alarmem poprzez wysyłanie SMS-ów 
    wyslij_SMS(buf3);
    return strdup("inne");
  } else {
    return buf3;
  }
}

// debagowanie

void miganie(short int ile=1, float dlugo=1) { // ########################################################################################
  int x = 0;
  while (x++ < ile) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(dlugo*1000);
    digitalWrite(LED_BUILTIN, LOW);
    delay(dlugo*1000);
  }
}

void slad(int czujka) { // ###############################################################################################################
  if ( ostatni_slad > 99 ) {
    for ( int i = 1; i < 100; i++) {
      slady[i-1] = slady[i];
      sladyt[i-1] = sladyt[i];
    }
    slady[99] = czujka;
    sladyt[99] = time(NULL);
  } else {
    slady[ostatni_slad] = czujka;
    sladyt[ostatni_slad] = time(NULL);
  }
    ostatni_slad++;
}

void setup() { // ########################################################################################################################
  // BOOTSEL
  miganie(3,0.33);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(kam[0], OUTPUT);
  digitalWrite(kam[0], LOW);
  pinMode(kam[1], OUTPUT);
  digitalWrite(kam[1], LOW);
  pinMode(s[0], OUTPUT);
  digitalWrite(s[0], HIGH);
  pinMode(s[1], OUTPUT);
  digitalWrite(s[1], HIGH);
  pinMode(l[0], OUTPUT);
  digitalWrite(l[0], HIGH);
  pinMode(l[1], OUTPUT);
  digitalWrite(l[1], HIGH);
  pinMode(l[2], OUTPUT_12MA);
  digitalWrite(l[2], LOW);
  pinMode(c[0], INPUT);
  pinMode(c[1], INPUT);
  pinMode(c[2], INPUT);
  pinMode(c[3], INPUT);
  pinMode(c[4], INPUT);
  pinMode(c[5], INPUT);
  pinMode(c[6], INPUT);
  wifi();
  for ( int i = 0; i < 100; i++) {
    slad(9);
  }
  ostatni = time(NULL);
  syrena[0] = 0;
  syrena[1] = 0;
  miganie(2,0.5);
  delay(60000);
  miganie(1,1);
 }

void loop() { // ##########################################################################################################################
  int alert = 0;
// sprawdzanie stanu czujek

  if ((digitalRead(c[0])) || (digitalRead(c[1])) || (digitalRead(c[2])) || (digitalRead(c[3])) || (digitalRead(c[4])) || (digitalRead(c[5])) || (digitalRead(c[6]))) {
    for (int i = 0; i < 7; i++) {
      if ( digitalRead(c[i]) ) { // zawsze wysyłam informację na swewer  i zapisuję ślad
        alarm((char*)czujki[i]);
        if ( ( slady[99] != i ) || ( time(NULL) > sladyt[99] + 1 ) ) { // zapisuję ślad, jeśli jest różny od poprzedniego albo minęła sekunda od ostatniego zapisania śladu
          alert = 1;
          slad(i);
        }
      }
    }
  }

// sekwencje uruchamiające syreny;
/*
  if ( alert && ( time(NULL) - sladyt[97] < 10 ) ) {
    if ( slady[99] == 1 && slady[98] == 0 ) {
      syrena[1] = time(NULL) + 1;
    }
    if ( ( slady[99] == 2 || slady[99] == 3 ) && ( slady[98] == 0 || slady[98] == 1 ) ) {
      syrena[0] = time(NULL) + 10;
    } 
    if ( slady[99] == 4 && ( slady[98] == 2 || slady[98] == 3 || slady[98] == 5 ) ) {
      syrena[0] = time(NULL) + 20;
      syrena[1] = time(NULL) + 20;
    }
    if ( slady[99] == 6 && ( slady[98] == 5 || slady[98] == 0 ) ) {
      syrena[0] = time(NULL) + 20;
      syrena[1] = time(NULL) + 20;
    }
  }
*/

// timer pilnujący włączania i wyłączania syren w zadanym czasie

  if ( ( syrena[0] < time(NULL) ) && ( !digitalRead(s[0] ) ) ) {
    digitalWrite(s[0], HIGH);
    alarm((char*)"s1off");
  } else if ( ( syrena[0] > time(NULL) ) && ( digitalRead(s[0]) ) ) {
    digitalWrite(s[0], LOW);
    alarm((char*)"s1on");
  } else if ( ( syrena[1] < time(NULL) ) && ( !digitalRead(s[1] ) ) ) {
    digitalWrite(s[1], HIGH);
    alarm((char*)"s2off");
  } else if ( ( syrena[1] > time(NULL) ) && ( digitalRead(s[1]) ) ) {
    digitalWrite(s[1], LOW);
    alarm((char*)"s2on");
  }

// timer pilnujący włączania i wyłączania światła w zadanym czasie

  if ( ( lampa[0] < time(NULL) ) && ( !digitalRead(l[0] ) ) ) {
    digitalWrite(l[0], HIGH);
    alarm((char*)"l1off");
  } else if ( ( lampa[0] > time(NULL) ) && ( digitalRead(l[0]) ) ) {
    digitalWrite(l[0], LOW);
    alarm((char*)"l1on");
  } else if ( ( lampa[1] < time(NULL) ) && ( !digitalRead(l[1] ) ) ) {
    digitalWrite(l[1], HIGH);
    alarm((char*)"l2off");
  } else if ( ( lampa[1] > time(NULL) ) && ( digitalRead(l[1]) ) ) {
    digitalWrite(l[1], LOW);
    alarm((char*)"l2on");
  } else if ( ( lampa[2] < time(NULL) ) && ( !digitalRead(l[2] ) ) ) {
    digitalWrite(l[2], HIGH);
    alarm((char*)"l3off");
  } else if ( ( lampa[2] > time(NULL) ) && ( digitalRead(l[2]) ) ) {
    digitalWrite(l[3], LOW);
    alarm((char*)"l3on");
  }

// wysyłanie powiadomień o naruszeniu stref na telefony, bez powtórzeń z ostatnich 15 sekund

  if (alert) {
    time_t kiedy = time(NULL) - 15;
    int i = 98;
    wyslij = 1;
    while (i >= 0) {
      if ( sladyt[i] > kiedy ) {
        if ( slady[i] == slady[99] ) {
          wyslij = 0;
          break;
        }
      } else {
        break;
      }
      i--;
    }
    if (wyslij) { // jeśli w ciągu ostatnich dziesięciu sekund nie naruszono spokoju  tej samej czujki
      wyslij_SMS((char*)strefy[slady[99]]);
      wyslij = 0;
    }
  }

// sprawdzenie SMS-ów przychodzących i odpowiednio sterowanie według nich ustawieniami

  if ( time(NULL) - ostatni > 0 ) { // co sekundę
    miganie(1,0.01);
    ostatni = time(NULL);
    if ( sprawdzenie_SMS() ) {
      char * kontrolka = odczytaj_SMS();
      const char * testy[10] = {"Td", "Tn", "S", "Sw", "Se", "Sr", "K", "Lw", "Le", "Lr"};
       if ( !strcmp(kontrolka,testy[0]) ) { // dzień Td
        digitalWrite(kam[0], LOW);
        digitalWrite(kam[1], LOW);
        digitalWrite(s[0], HIGH);
        digitalWrite(s[1], HIGH);
        digitalWrite(l[0], HIGH);
        digitalWrite(l[1], HIGH);
        digitalWrite(l[2], LOW);
      } else if ( !strcmp(kontrolka,testy[1]) ) { // noc Tn
        digitalWrite(kam[0], HIGH);
        digitalWrite(kam[1], HIGH);
        digitalWrite(s[0], HIGH);
        digitalWrite(s[1], HIGH);
        digitalWrite(l[0], LOW);
        digitalWrite(l[1], HIGH);
        digitalWrite(l[2], LOW);
      } else if ( !strcmp(kontrolka,testy[2]) ) { // przełączanie syren S
        digitalWrite(s[0], LOW);
        alarm((char*)"s1on");
        syrena[0] = time(NULL) + 180;
        digitalWrite(s[1], LOW);
        alarm((char*)"s2on");
        syrena[1] = time(NULL) + 180;
      } else if ( !strcmp(kontrolka,testy[3]) ) { // Sw
        if ( digitalRead(s[0]) ) {
          digitalWrite(s[0], LOW);
          syrena[0] = time(NULL) + 60;
          alarm((char*)"s1on");
        } else {
          digitalWrite(s[0], HIGH);
          syrena[0] = time(NULL);
          alarm((char*)"s1off");
        }
      } else if ( !strcmp(kontrolka,testy[4]) ) { // Se
        if ( digitalRead(s[1]) ) {
          digitalWrite(s[1], LOW);
          syrena[1] = time(NULL) + 60;
          alarm((char*)"s2on");
        } else {
          digitalWrite(s[1], HIGH);
          syrena[1] = time(NULL);
          alarm((char*)"s2off");
        }
      } else if  ( !strcmp(kontrolka,testy[5]) ) { // Sr
        digitalWrite(s[0], HIGH);
        syrena[0] = time(NULL);
        alarm((char*)"s1off");
        digitalWrite(s[1], HIGH);
        syrena[1] = time(NULL);
        alarm((char*)"s2off");
      } else if ( !strcmp(kontrolka,testy[6]) ) { // przełączenie kamer K
        if ( digitalRead(kam[0]) ) {
          digitalWrite(kam[0], LOW);
        } else {
          digitalWrite(kam[0], HIGH);
        }
        if ( digitalRead(kam[1]) ) {
          digitalWrite(kam[1], LOW);
        } else {
          digitalWrite(kam[1], HIGH);
        }
      } else if ( !strcmp(kontrolka,testy[7]) ) { // przełączenie świateł Lw
        if ( digitalRead(l[0]) ) {
          digitalWrite(l[0], LOW);
          alarm((char*)"l1on");
        } else {
          digitalWrite(l[0], HIGH);
          alarm((char*)"l1off");
        } 
      } else if ( !strcmp(kontrolka,testy[8]) ) { // Le
        if ( digitalRead(l[1]) ) {
          digitalWrite(l[1], LOW);
          alarm((char*)"l2on");
        } else {
          digitalWrite(l[1], HIGH);
          alarm((char*)"l2off");
        }
      } else if ( !strcmp(kontrolka,testy[9]) ) { // Lr
        if ( digitalRead(l[2]) ) {
          digitalWrite(l[2], LOW);
          alarm((char*)"l3off");
        } else {
          digitalWrite(l[2], HIGH);
          alarm((char*)"l3on");
        }
      }
    }
  }
}
