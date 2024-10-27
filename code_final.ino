#include <WiFi.h>         // Cette bibliothèque permet à l'ESP32 de se connecter au réseau WiFi.
#include <PubSubClient.h> // Cette bibliothèque vous permet d'envoyer et de recevoir des messages MQTT.
#include <DHT.h>
#include <SPI.h>
#include <Wire.h> //BH1750 IIC Mode pour SEN0097
#include <math.h>
#include <BH1750.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Adafruit_CCS811.h"



// Capteurs
  const int HumidPin = 34;
  const int BoutonPin = 36;
  const int PresenceEauPin = 33;
  // const int LightSensorPin = I2C
  // const int AirPin = I2C
  const int TemperPin = 14;

// Actionneurs
  const int PompePin = 18;
  const int BuzzerPin = 32;
  const int LedPin = 25;
  const int Motor = 27; // broche compatible PWM
  // const int Ecran = I2C

// Declarations pour l'ecran
  #define SCREEN_WIDTH 128 // OLED display width, in pixels
  #define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for SSD1306 display connected using I2C
  #define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
  #define SCREEN_ADDRESS 0x3C
  Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Declaration pour les capteurs
  //Capteur Luxmetre
  BH1750 lightMeter;  
  //Capteur Temperature
  #define DHpin 14  
  #define DHTTYPE DHT11 
  DHT dht(DHpin, DHTTYPE); 
  // Capteur Qualite Air = CO2
  Adafruit_CCS811 sensor;

// Variables
  unsigned long lastRead = 0UL; //Capteur Qualite air = CO2
  int CO2value=0;                  //Capteur Qualite air = CO2
  int TVOCvalue=0;                 //Capteur Qualite air = CO2
  int HumiditeTerre=0;            //Capteur Humidite Terre
  int PresenceEau=0;              //Capteur Prescence Eau
  int demarrage=0;                //Capteur Bouton
  float Luminosite=0;                    //Capteur Luxmetre
  float Temperature=0;            //Capteur Temperature
  const char *fonctionnement = "Auto"; //De NodeRed
  int BoutonValeur;
  //variables de seuil
  const char SeuilTemperature = 28;
  const char SeuilHumidite = 28;
  const char Jour = 50;
  const char Nuit = 100;
  const char SeuilCo2 = 1000;



// Réseau WiFi auquel vous voulez vous connecter.
  const char *ssid = "MT";
  const char *password = "Anatomye";

// Ce champ contient l'adresse IP ou le nom d'hôte du serveur MQTT.
  const char *mqtt_server = "mqtt.ci-ciad.utbm.fr";

  WiFiClient espclientCBMK;               // Initialiser la bibliothèque client wifi (connexion WiFi)
  PubSubClient clientCBMK(espclientCBMK); // Créer un objet client MQTT (connexion MQTT)
// Maintenant, notre client MQTT s'appelle "espclientCBMK". Nous devons maintenant créer une connexion à un broker.
  long lastMsg = 0;
  const char *topic_hydro = "esp32/Hydroponique_CBMK/";
  char topic_actuel[100]; // Variable pour construire les topics MQTT

  // Liste des topics
  const char *topicList[] = {
      "Allumage", "ventilateur", "pompe", "mode",
      "ValeurHumidite", "ValeurLuminosite", "ValeurNiveauEau", "ValeurTemperature", "ValeurQualiteAir"};
  const int numTopics = 9;





void setup(){
    
    
  dht.begin();
  
  if(!sensor.begin())
  {
	Serial.println("Failed to start the CCS811 sensor! Please check your wiring.");
	while(1);
  }

  // Wait for the sensor to be ready
  while(!sensor.available());
  
  pinMode(PresenceEauPin, INPUT);
  pinMode(Motor,OUTPUT);
  pinMode(BuzzerPin,OUTPUT);
  pinMode(LedPin,OUTPUT);
  pinMode(BoutonPin,INPUT_PULLUP);
  pinMode(HumidPin, INPUT);
  pinMode(PompePin, OUTPUT);
  pinMode(TemperPin, INPUT);
  
  Serial.begin(9600);


// Demarrage des elements I2C dont l'affichage de l'ecran
  // Initialize the I2C bus (BH1750 library doesn't do this automatically)
  Wire.begin();
  // On esp8266 you can select SCL and SDA pins using Wire.begin(D4, D3);
  // For Wemos / Lolin D1 Mini Pro and the Ambient Light shield use Wire.begin(D2, D1);

  lightMeter.begin();

  Serial.println(F("Test begin"));

  // initialize the OLED object
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Uncomment this if you are using SPI
  //if(!display.begin(SSD1306_SWITCHCAPVCC)) {
  //  Serial.println(F("SSD1306 allocation failed"));
  //  for(;;); // Don't proceed, loop forever
  //}

  // Clear the buffer.
  display.clearDisplay();

  // Scroll full screen
  display.setCursor(0,28);
  display.setTextSize(1.5);
  display.setTextColor(WHITE);
  display.println("Demarrage");
  display.println("du");
  display.println("systeme!");
  display.display();
  display.startscrollright(0x00, 0x07);
  delay(2000);
  display.stopscroll();
  delay(1000);
  display.startscrollleft(0x00, 0x07);
  delay(2000);
  display.stopscroll();
  delay(1000);    
  display.startscrolldiagright(0x00, 0x07);
  delay(2000);
  display.startscrolldiagleft(0x00, 0x07);
  delay(2000);
  display.stopscroll();
  display.clearDisplay();

// Configuration WiFi et MQTT
  setup_wifi();
  clientCBMK.setServer(mqtt_server, 1883);
  clientCBMK.setCallback(callback);
  playMarioIntro();
}





//------Cette fonction se connecte au réseau WiFi en utilisant les paramètres de connexion fournis
// dans les variables ssid et password.
void setup_wifi()
{
  delay(10); // Cette instruction pause l'exécution du programme pendant 10 millisecondes.
  // We start by connecting to a WiFi network
  Serial.println(); // Imprime une ligne vide/saut de ligne dans la console série.
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password); // //démarre la connexion Wi-Fi avec les informations de connexion (SSID et mot de passe) fournies.

  // Cette boucle effectue une pause de 500 millisecondes jusqu'à ce que l'ESP32 soit
  // connecté au réseau Wi-Fi. Le statut de la connexion est obtenu en appelant "WiFi.status()".
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

//-----La fonction callback est utilisée pour traiter les messages MQTT reçus par l'ESP32 et de prévoir une action.
// Elle est appelée chaque fois qu'un nouveau message est reçu sur un Topic auquel l'ESP32 est abonné.
void callback(char *topic, byte *message, unsigned int length)
{

  sprintf(topic_actuel, "%sventilateur", topic_hydro);
  if (strcmp(topic, topic_actuel) == 0)
  {
     Serial.print("Message arrived on topic: ");
  Serial.print(topic); // imprime le nom du Topic sur lequel le message a été publié.
  Serial.print(". Message: ");

  // Le message reçu est transmis à la fonction en tant que tableau de bytes "message" avec une longueur "length".
  // Pour pouvoir travailler avec ce message dans le reste du code, nous devons d'abord le convertir
  // en chaîne de caractères.
  String messageTemp; // déclare une variable de chaîne temporaire pour stocker le message reçu.

  // boucle sur chaque élement dans le tableau de bytes "message" et les ajoute à la chaîne "messageTemp".
  for (int i = 0; i < length; i++)
  {
    messageTemp += (char)message[i];
    // Pour chaque itération, l'élément actuel du tableau de bytes "message" est converti en char avec
    //"(char)message[i]" et affiché sur la console série avec "Serial.print((char)message[i])".
    // Ensuite, ce même élément est ajouté à la fin de la chaîne "messageTemp" avec
    //"messageTemp += (char)message[i];
  }
  Serial.println(messageTemp);

  tone(BuzzerPin, 1000, 100);
  delay(100);        // Attendre que le bip se termine
  noTone(BuzzerPin); // Arrêter le bip

  // Afficher le message reçu pour un actionneur spécifique
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.println("Received:");
  display.print("Topic: ");
  display.println(topic);
  display.print("Message: ");
  for (int i = 0; i < length; i++)
  {
    display.print((char)message[i]);
  }
  display.display();
  delay(2000); // Afficher pendant un certain temps

  // Revenir à l'affichage des données des capteurs
  displaySensorData();

    //// Code à rajouter
    if (messageTemp == "True")
    {
      digitalWrite(Motor, HIGH);
    }
    else
    {
      digitalWrite(Motor, LOW);
    }
  }

  sprintf(topic_actuel, "%spompe", topic_hydro);
  if (strcmp(topic, topic_actuel) == 0)
  {
     Serial.print("Message arrived on topic: ");
  Serial.print(topic); // imprime le nom du Topic sur lequel le message a été publié.
  Serial.print(". Message: ");

  // Le message reçu est transmis à la fonction en tant que tableau de bytes "message" avec une longueur "length".
  // Pour pouvoir travailler avec ce message dans le reste du code, nous devons d'abord le convertir
  // en chaîne de caractères.
  String messageTemp; // déclare une variable de chaîne temporaire pour stocker le message reçu.

  // boucle sur chaque élement dans le tableau de bytes "message" et les ajoute à la chaîne "messageTemp".
  for (int i = 0; i < length; i++)
  {
    messageTemp += (char)message[i];
    // Pour chaque itération, l'élément actuel du tableau de bytes "message" est converti en char avec
    //"(char)message[i]" et affiché sur la console série avec "Serial.print((char)message[i])".
    // Ensuite, ce même élément est ajouté à la fin de la chaîne "messageTemp" avec
    //"messageTemp += (char)message[i];
  }
  Serial.println(messageTemp);

  tone(BuzzerPin, 1000, 100);
  delay(100);        // Attendre que le bip se termine
  noTone(BuzzerPin); // Arrêter le bip

  // Afficher le message reçu pour un actionneur spécifique
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.println("Received:");
  display.print("Topic: ");
  display.println(topic);
  display.print("Message: ");
  for (int i = 0; i < length; i++)
  {
    display.print((char)message[i]);
  }
  display.display();
  delay(2000); // Afficher pendant un certain temps

  // Revenir à l'affichage des données des capteurs
  displaySensorData();

    //// Code à rajouter
    if (messageTemp == "True")
    {
      digitalWrite(PompePin, HIGH);
    }
    else
    {
      digitalWrite(PompePin, LOW);
    }
  }

  sprintf(topic_actuel, "%smode", topic_hydro);
  if (strcmp(topic, topic_actuel) == 0)
  {
     Serial.print("Message arrived on topic: ");
  Serial.print(topic); // imprime le nom du Topic sur lequel le message a été publié.
  Serial.print(". Message: ");

  // Le message reçu est transmis à la fonction en tant que tableau de bytes "message" avec une longueur "length".
  // Pour pouvoir travailler avec ce message dans le reste du code, nous devons d'abord le convertir
  // en chaîne de caractères.
  String messageTemp; // déclare une variable de chaîne temporaire pour stocker le message reçu.

  // boucle sur chaque élement dans le tableau de bytes "message" et les ajoute à la chaîne "messageTemp".
  for (int i = 0; i < length; i++)
  {
    messageTemp += (char)message[i];
    // Pour chaque itération, l'élément actuel du tableau de bytes "message" est converti en char avec
    //"(char)message[i]" et affiché sur la console série avec "Serial.print((char)message[i])".
    // Ensuite, ce même élément est ajouté à la fin de la chaîne "messageTemp" avec
    //"messageTemp += (char)message[i];
  }
  Serial.println(messageTemp);

  tone(BuzzerPin, 1000, 100);
  delay(100);        // Attendre que le bip se termine
  noTone(BuzzerPin); // Arrêter le bip

  // Afficher le message reçu pour un actionneur spécifique
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.println("Received:");
  display.print("Topic: ");
  display.println(topic);
  display.print("Message: ");
  for (int i = 0; i < length; i++)
  {
    display.print((char)message[i]);
  }
  display.display();
  delay(2000); // Afficher pendant un certain temps

  // Revenir à l'affichage des données des capteurs
  displaySensorData();

    //// Code à rajouter
    if (messageTemp == "True")
    {
      fonctionnement = "Auto";
    }
    else
    {
      fonctionnement = "Manuel";
    }
  }
}

// La fonction "reconnect()" est utilisée pour garantir la connexion MQTT entre l'ESP32 et le broker MQTT.
// Elle est appelée dans la boucle principale et elle se répète jusqu'à ce que la connexion soit établie avec succès.
void reconnect()
{
  // Loop until we're reconnected
  while (!clientCBMK.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    // Si la connexion est déjà établie, la fonction ne fait rien.
    // Si la connexion n'est pas établie, le code essaie de se connecter à l'aide de "clientCBMK.connect("espclientCBMK")".
    // Si la connexion est réussie, le code imprime "connected" sur la console série et
    // s'abonne au topic "esp32/led" avec "clientCBMK.subscribe("esp32/led")".
    if (clientCBMK.connect("espclientCBMK"))
    {
      Serial.println("connected");
      // Subscribe
      for (int i = 0; i < numTopics; i++)
      {
        char fullTopic[100];
        sprintf(fullTopic, "%s%s", topic_hydro, topicList[i]);
        clientCBMK.subscribe(fullTopic);
      }
    }
    // Si la connexion échoue, le code imprime "failed, rc=" suivi de l'état de la connexion avec "clientCBMK.state()" sur la console série
    // La fonction se répète jusqu'à ce que la connexion soit établie avec succès.
    else
    {
      Serial.print("failed, rc=");
      Serial.print(clientCBMK.state());
      Serial.println(" try again in 5 seconds");
      // On attend 5 secondes avant de retenter la connexion
      delay(5000);
    }
  }
}

void loop() {

 // La première tâche de la fonction principale est de vérifier si le clientCBMK MQTT est connecté.
  // Si ce n'est pas le cas, la fonction reconnect() est appelée pour reconnecter le clientCBMK.
  if (!clientCBMK.connected())
  {
    reconnect();
  }
  clientCBMK.loop(); // La méthode clientCBMK.loop() est appelée pour traiter les messages MQTT entrants.
                     // Maintient la connexion avec le serveur MQTT en vérifiant si de nouveaux messages sont arrivés et en envoyant les messages en attente.

  // CAPTEUR BOUTON
  BoutonValeur = analogRead(BoutonPin);
  Serial.print(BoutonValeur);
  if (BoutonValeur == 4095){
    if (demarrage == 0)
    {
      demarrage = 1;
      Serial.println("Systeme allume");
        // ACTIONNEUR BUZZER
      tone(BuzzerPin, 1000, 100);
      delay(100);        // Attendre que le bip se termine
      noTone(BuzzerPin); // Arrêter le bip

      // ACTIONNEUR led
      digitalWrite(LedPin, HIGH);
      delay(500);
      digitalWrite(LedPin, LOW);

      digitalWrite(LedPin, HIGH);
      sprintf(topic_actuel, "%sAllumage", topic_hydro);
      clientCBMK.publish(topic_actuel, "On");
    }
    else if (demarrage == 1)
    {
      demarrage = 0;
      Serial.println("Systeme eteint");
      digitalWrite(LedPin, LOW);
      sprintf(topic_actuel, "%sAllumage", topic_hydro);
      clientCBMK.publish(topic_actuel, "Off");
    }
    delay(2000);
  }

// Demarrage du systeme
  if (demarrage==1)
  {
 // Demarrage du systeme en mode auto
    if (strcmp(fonctionnement, "Auto") == 0)
    {
   // CAPTEUR LUXMETRE
  Luminosite = lightMeter.readLightLevel();
  Serial.print("Light: ");
  Serial.print(Luminosite);
  Serial.println(" lx");
  sprintf(topic_actuel, "%sValeurLuminosite", topic_hydro);
  clientCBMK.publish(topic_actuel, String(Luminosite).c_str());

  // CAPTEUR HUMIDITE TERRE
  HumiditeTerre = analogRead(HumidPin);
  Serial.print("Humidite terre: " );
  Serial.println(HumiditeTerre);
  sprintf(topic_actuel, "%sValeurHumidite", topic_hydro);
  clientCBMK.publish(topic_actuel, String(HumiditeTerre).c_str());


  // CAPTEUR PRESCENCE EAU
  PresenceEau = digitalRead(PresenceEauPin);
  Serial.print("Prescence Eau: ");
  Serial.print(PresenceEau);
  if (PresenceEau == 1){
    Serial.println(", Pas d'eau");
  } else if(PresenceEau == 0) {
    Serial.println(", Eau detecte");
  } else {
    Serial.println(", Valeur du digitalread du capteur d'eau non valide");
  }
  sprintf(topic_actuel, "%sValeurNiveauEau", topic_hydro);
  clientCBMK.publish(topic_actuel, digitalRead(PresenceEau) ? "Plein" : "Vide");


  // CAPTEUR QUALITE D'AIR = CO2
  // Read the sensor every 500 milliseconds
    // Check, whether the sensor is available and ready
    if(sensor.available())
    {
      // Try to read some data. The if checks whether that
      // process was successful
      if(!sensor.readData())
      {
        // If the sensor succeeded, output the measured values
        CO2value = sensor.geteCO2();
        TVOCvalue = sensor.getTVOC();

        Serial.print("CO2: ");
        Serial.print(CO2value);
        Serial.print("ppm");

        Serial.print(", TVOC: ");
        Serial.print(TVOCvalue);
        Serial.println("ppb.");
        sprintf(topic_actuel, "%sValeurQualiteAir", topic_hydro);
        clientCBMK.publish(topic_actuel, String(CO2value).c_str());

      }
      else
      {
        // Otherwise, display that an error occured
        Serial.println("Error!");
      }
    
  }

  // CAPTEUR TEMPERATURE
  Temperature = dht.readTemperature();
  Serial.print("Temperature: ");
  Serial.print(Temperature);
  Serial.println("°C");
  sprintf(topic_actuel, "%sValeurTemperature", topic_hydro);
  clientCBMK.publish(topic_actuel, String(Temperature).c_str());



      // La dernière partie vérifie le temps écoulé depuis le dernier message publié et n'envoie le prochain message que toutes les 2 secondes (2000 millisecondes).
      long now = millis(); // Crée une variable "now" pour stocker le nombre de millisecondes écoulées depuis le démarrage du programme.
      if (now - lastMsg > 60000)
      {                // Vérifie si le temps écoulé depuis le dernier message publié est supérieur à 1mn.
        lastMsg = now; // Si oui, met à jour la variable "lastMsg" avec la valeur actuelle de "now".

        // tester chaque capteur et agir en conséquence sur les actionneurs
        if (HumiditeTerre < SeuilHumidite && Luminosite < Jour)
        {
          digitalWrite(PompePin, HIGH);
          Serial.println("!Activation de la pompe!");
          sprintf(topic_actuel, "%spompe", topic_hydro);
          clientCBMK.publish(topic_actuel, "On");
          delay(3000); //         6L/min donc 3s pour 0.3l
          digitalWrite(PompePin, LOW);
          clientCBMK.publish(topic_actuel, "Off");
        }

        if (digitalRead(PresenceEau) == 0)
        {
          digitalWrite(PompePin, LOW);
          displayWaterLevelAlert();
          tone(BuzzerPin, 262, 1000 / 4);
          delay(100);
          tone(BuzzerPin, 282, 1000 / 4);
          delay(100);
          tone(BuzzerPin, 302, 1000 / 4);
          delay(1000);
        }

        // Verifie si temp est pas trop chaude et active ventilateur sinon
        if (Temperature > SeuilTemperature)
        {
          analogWrite(Motor, 255); // PWM
          sprintf(topic_actuel, "%sventilateur", topic_hydro);
          clientCBMK.publish(topic_actuel, "On");
        }
        else
        {
          analogWrite(Motor, 0); // PWM
          sprintf(topic_actuel, "%sventilateur", topic_hydro);
          clientCBMK.publish(topic_actuel, "Off");
        }

        // Verifier si 'jour et active ventilateur si pas assez de co2) ou (si nuit et active ventilateur si trop de co2)
        if ((CO2value < SeuilCo2 && Luminosite < Jour) || (CO2value > SeuilCo2 && Luminosite > Nuit))
        {
          analogWrite(Motor, 255);
          sprintf(topic_actuel, "%sventilateur", topic_hydro);
          clientCBMK.publish(topic_actuel, "On");
        }
        else
        {
          analogWrite(Motor, 0); // PWM
          sprintf(topic_actuel, "%sventilateur", topic_hydro);
          clientCBMK.publish(topic_actuel, "Off");
        }
      }
    }

  }else
  {
    Serial.println("Systeme eteint");
  }
  

  
  // fin du loop
  displaySensorData();
  delay(3000);
  Serial.print("\n");
}





void playMarioIntro()
{
  // Notes de la mélodie de Mario
  int melody[] = {
      660, 660, 660, 510, 660, 770, 380};

  // Durées des notes: 1 pour une note courte, 2 pour une note plus longue
  int noteDurations[] = {
      10, 10, 10, 10, 10, 10, 20};

  for (int thisNote = 0; thisNote < 7; thisNote++)
  {
    int noteDuration = 1000 / noteDurations[thisNote];
    tone(BuzzerPin, melody[thisNote], noteDuration);
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    noTone(BuzzerPin);
  }
}



void displaySensorData()
{
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.print("Moisture: ");
  display.print(HumiditeTerre);
  display.println("%");

  display.print("Temperature: ");
  display.print(Temperature);
  display.println(" C");

  // Ajouter plus de capteurs ici
  display.print("Air quality: ");
  display.print(CO2value);
  display.println(" Ppm");

  display.print("Light: ");
  display.print(Luminosite);
  display.println(" Lux");

  display.print("Mod: ");
  display.print(fonctionnement);

  display.display();
}

void displayWaterLevelAlert()
{
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.setTextColor(WHITE);

  // Afficher l'alerte
  display.println("ALERT!");
  display.println("Water level low");
  display.println("Please refill");
  display.display();
}
