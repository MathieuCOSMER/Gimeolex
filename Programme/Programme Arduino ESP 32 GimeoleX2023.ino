#define DEBUG  //mettre en commentaire pour supprimer les Serial.println de débuggage

#ifdef DEBUG
#define DEBUG_PRINTLN(x)  Serial.println(x)
#define DEBUG_PRINT(x)  Serial.print(x)
#else
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINT(x)
#endif





#include <Wire.h>
#include <Adafruit_ADS1X15.h>

#include "BluetoothSerial.h" //Header File for Serial Bluetooth, will be added by default into Arduino
BluetoothSerial ESP_BT;
#include <Preferences.h>
Preferences Energies;
int dureeIntegral=millis();
int temps = millis();
int intervalle = 500;
int tempsSauvegarde = millis();
int intervalleSauvegarde = 60000; //sauvergarde dans l'eeprom toutes les 60s
int EnergieEole = 0;
int EnergiePV = 0;
int EnergieHumain = 0;
float Ubatt = 0.01;
float Ieole = 0.01;
float Ihumain = 0.01;
float Ipv = 0.01;

Adafruit_ADS1115 ADS;


int Energie = 0;

bool ADSOK = false;
//variable pour le scan automatique


void setup() {

  Serial.begin(115200);
  ESP_BT.begin("CarteMesureGimeoleX2"); //Bluetooth device name

  DEBUG_PRINTLN("##########################Setup ###############################");

  delay(300);

  if (!ADS.begin(0x48)) {
    DEBUG_PRINTLN("ADS non initialisé.");
    ADSOK = false;
  } else {
    ADSOK = true;
    ADS.setDataRate(RATE_ADS1115_8SPS);
  }

  delay(300);
  
  RecuperationEnergies();
  

}


void loop()
{
  if (millis() - tempsSauvegarde > intervalleSauvegarde) {
    DEBUG_PRINTLN("Sauvegarde");
    tempsSauvegarde = millis();
    Sauvegarde();

  }//envoie des donnÃƒÂ©es tous les intervalles en ms



  if (millis() - temps > intervalle) {
    DEBUG_PRINTLN("****************************");

    Mesures_Prod();
    temps = millis();

    EnvoyerDonnees();
    //interupteur de reset sur GPIO18
    if(digitalRead(18)==HIGH){
        ResetSauvegarde();
        
    }


  }//envoie des donnÃƒÂ©es tous les intervalles en ms


}


void EnvoyerDonnees()
{
  //const int TailleString = 7;
  DEBUG_PRINTLN("Envoi des donnees");
  ESP_BT.print("Ubatt=");
  ESP_BT.print(Ubatt);
  ESP_BT.print(",");
  ESP_BT.print("   Ieole=");
  ESP_BT.print(Ieole);
  ESP_BT.print(",");
  ESP_BT.print("   Ipv=");
  ESP_BT.print(Ipv);
  ESP_BT.print(",");
  ESP_BT.print("   Ihumain=");
  ESP_BT.print(Ihumain);
  ESP_BT.print(",");
  ESP_BT.print("   PuissanceEole=");
  ESP_BT.print(Ieole*Ubatt);
  ESP_BT.print(",");
  ESP_BT.print("   PuissancePV=");
  ESP_BT.print(Ipv*Ubatt);
  ESP_BT.print(",");
  ESP_BT.print("   PuissanceHumain=");
  ESP_BT.print(Ihumain*Ubatt);
  ESP_BT.print(",");
  ESP_BT.print("   EnergieEole=");
  ESP_BT.print(EnergieEole);
  ESP_BT.print(",");
  ESP_BT.print("   EnergiePV=");
  ESP_BT.print(EnergiePV);
  ESP_BT.print(",");
  ESP_BT.print("   EnergieHumain=");
  ESP_BT.print(EnergieHumain);
  ESP_BT.print(";");
  
  
  
 
}


void Mesures_Prod()
{
  dureeIntegral=millis()-dureeIntegral;
  ESP_BT.print("   Duree Integration=");
  ESP_BT.print(dureeIntegral);
  ESP_BT.print(";");
  float ConversionU=4095.0/(3.2*2);//conversion si adsd HS
  float ConversionI=4095.0/(3.2*8.87);//conversion si ADS HS
  float offsetCourant = 13360;
  float conversionCourant = 12.0;
  float pontDivEntree = 8.87; //a calibrer
  float multiplier = 0.1875; //* ADS1115  @ +/- 6.144V gain (16-bit results)
  int16_t resultat_conv;
  if (ADSOK) {

    Ubatt = ADS.readADC_SingleEnded(0) * multiplier * pontDivEntree / 1000;
    Ieole = (ADS.readADC_SingleEnded(1) - offsetCourant) * multiplier * conversionCourant / 1000+0.06;
    Ihumain = (ADS.readADC_SingleEnded(2) - offsetCourant) * multiplier * conversionCourant / 1000+0.06;
    Ipv = (ADS.readADC_SingleEnded(3) - offsetCourant) * multiplier * conversionCourant / 1000+0.06;
    EnergieEole += Ubatt * Ieole * (dureeIntegral / 1000.0);
    EnergiePV += Ubatt * Ipv * (dureeIntegral / 1000.0);
    EnergieHumain += Ubatt * Ihumain * (dureeIntegral / 1000.0);
    DEBUG_PRINT("Ubatt=");
    DEBUG_PRINT(Ubatt);
    DEBUG_PRINT("   Ieole=");
    DEBUG_PRINT(Ieole);
    DEBUG_PRINT("   Ihumain=");
    DEBUG_PRINT(Ihumain);
    DEBUG_PRINT("   Ipv=");
    DEBUG_PRINTLN(Ipv);
    DEBUG_PRINT("   EnergieEole=");
    DEBUG_PRINT(EnergieEole);
    DEBUG_PRINT("   EnergiePV=");
    DEBUG_PRINT(EnergiePV);
    DEBUG_PRINT("   Ipv=");
    DEBUG_PRINTLN(EnergieHumain);
    DEBUG_PRINT("   PuissanceEole=");
    DEBUG_PRINT(Ieole*Ubatt);
    DEBUG_PRINT("   PuissancePV=");
    DEBUG_PRINT(Ipv*Ubatt);
    DEBUG_PRINT("   PuissanceHumain=");
    DEBUG_PRINTLN(Ihumain*Ubatt);

  } else {
    if (!ADS.begin())
    {
      DEBUG_PRINTLN("ADS non initialisé.");
      ADSOK = false;
    Ubatt = analogRead(39)*.008;
    Ieole =(analogRead(32)-1350)*0.0186;
    Ihumain = (analogRead(33)-1350)*0.0186;
    Ipv = (analogRead(36)-1350)*0.0186;
    EnergieEole += Ubatt * Ieole * (dureeIntegral / 1000.0);
    EnergiePV += Ubatt * Ipv * (dureeIntegral / 1000.0);
    EnergieHumain += Ubatt * Ihumain * (dureeIntegral / 1000.0);
    DEBUG_PRINT("Ubatt=");
    DEBUG_PRINT(Ubatt);
    DEBUG_PRINT("   Ieole=");
    DEBUG_PRINT(Ieole);
    DEBUG_PRINT("   Ihumain=");
    DEBUG_PRINT(Ihumain);
    DEBUG_PRINT("   Ipv=");
    DEBUG_PRINTLN(Ipv);
    DEBUG_PRINT("   EnergieEole=");
    DEBUG_PRINT(EnergieEole);
    DEBUG_PRINT("   EnergiePV=");
    DEBUG_PRINT(EnergiePV);
    DEBUG_PRINT("   Ipv=");
    DEBUG_PRINTLN(EnergieHumain);
    DEBUG_PRINT("   PuissanceEole=");
    DEBUG_PRINT(Ieole*Ubatt);
    DEBUG_PRINT("   PuissancePV=");
    DEBUG_PRINT(Ipv*Ubatt);
    DEBUG_PRINT("   PuissanceHumain=");
    DEBUG_PRINTLN(Ihumain*Ubatt);
    }  else {
      ADSOK = true;
      ADS.setDataRate(RATE_ADS1115_8SPS);
    }
  }

dureeIntegral=millis();


}


void Sauvegarde() {
  Energies.begin("GimeoleX", false); //sauvergade en mémoire non volatile
  Energies.putInt("Eole", EnergieEole);
  Energies.putInt("PV", EnergiePV);
  Energies.putInt("Humain", EnergieHumain);
  Energies.end();

  ESP_BT.print("Sauvegarde dans la mémoire interne de l'esp32");
  ESP_BT.print(";");

}

void RecuperationEnergies() {
  Energies.begin("GimeoleX", false);
  EnergieEole = Energies.getInt("Eole", 0);
  EnergiePV = Energies.getInt("PV", 0);
  EnergieHumain = Energies.getInt("Humain", 0);
  Energies.end();
}

void ResetSauvegarde(){
  Energies.begin("GimeoleX", false);
  Energies.clear();
  Energies.end();
  EnergieEole = 0;
  EnergiePV = 0;
  EnergieHumain = 0;
  DEBUG_PRINTLN("Reset energies");
  ESP_BT.print("Reset energies");
  ESP_BT.print(";");
}









