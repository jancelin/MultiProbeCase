---
layout: default
parent: Satellite Cyclopée
title: Logger utilisant le temps GNSS
nav_order: 1
has_children: False
---

Logger utilisant le temps GNSS
==================================

## En bref
Ce programme enregistre les logs des mesures de température et de distance horodatées dans un fichier `.csv` sur la carte SD. Le temps utilisé pour l'horodatage est celui transmis par un module GNSS via les trames NMEA `$GPGGA` et `$GPRMC`.

## Inspiration
Ce programme est inspiré des tests unitaires précédents : 

- `clock_logger`;
- `GNSS_test`.


## Fonctionnement
#### Configuration
Les définitions en début de fichier (section `GLOBAL DEFINITIONS`) permettent de configurer le logger (fréquence de mesure, segmentation des fichiers de logs, etc.).
#### Setup
Au setup, le programme initilise la carte SD, le module GNSS, et les capteurs avec la configuration reneignée. Si une dépendance physique du système n'est pas satisfaite, il attendra que le problème soit résolu et d'être redémarré (cf. Debug).
#### Logs
La partie log du programme s'éxécute en permanence dans la fonction `loop()`. Cette fonction scanne l'état du bouton pour activer/désativer les logs. S'il sont activés, alors elle ouvre et gère un fichier de logs (ségmentation, passage au jour suivant) sur la carte SD, et enregistre les logs dans le fichier. Les fichiers de logs sont nommés avec l'heure de leur création et stockés dans un dossier journalier.
#### Mesures
La fonction `loop()` est interrompue pour effectuer la lecture des capteurs. Ceci permet d'assurer la périodicité des mesures, même pour des fréquences élevées. Les valeurs lues sont enregistrées dans des buffers permettant de stocker les données à logger. Quand le système ne mesure pas, il vide les buffers dans le fichier de logs.
#### Debug
En cas de problème, ou simplement pour monitorer le fonctionnement, le système présente trois modes de debug.<br>
La première ne nécéssite pas de moniteur série puisqu'elle utilise la LED déjà présente sur le Teensy 3.5. Celle-ci clignote différemment en foncion de l'état du système :

- **Eteinte** : Le système est hors tension;
- **Allumée** : Le système est en cours de démarrage (```setup()```). Si la LED reste indéfiniement allumée, l'une des conditions de démarrage n'est pas satisfaite. Il suffit régler le problème et redémarrer;
- **Clignotement lent (2s)** : Tout roule ! Le système enregistre les données sur la carte SD;
- **Clignotement moyennement rapide (600ms)** : Le système n'enregistre pas car le bouton n'a pas été appuyé;
- **Clignotement rapide (150ms)** : Le système à rencontré un erreur pendant la phase d'enregistrement. Un des éléments à probablement été déconnecté (DP0601, URM14, DS18B20, module GNSS, carte SD);

Les deux dernières sont moins archaïques mais nécéssitent de monitorer le port série USB auquel est connecté le Teensy :

- L'une renseigne simplement l'utilisateur sur l'**état du système pendant son initiaisation et son fonctionnement**.
- L'autre **affiche le contenu du fichier de logs ouvert**. Si un des capteurs est déconnecté, `NaN` remplacera alors la valeur lue de celui-ci.


## Matériel
- Teensy 3.5;
- Module GNSS (ici un Drotek DP0601);
- Capteur ultrasonore URM14;
- Interface Mikroe RS485 click 2;
- Alimentation 7-15V (URM14);
- Sonde de température DS18B20;
- Résistance pull up 4.7kOhms.
- Carte SD;
- Bouton poussoir (ou simplement un fil électrique très court).

## Bibliothèques
- `TinyGPSPlus`;
- `ModbusMaster`;
- `OneWire`;
- `DallasTemperature`;
- `Metro`;
- `TimeLib`;
- `SD`.

## Ports
Le port de communication `Serial` est utilisé pour le debug via USB (moniteur série de l'IDE Arduino) à 115200 baud. <br>
Le Teensy utilisera le port `Serial4` pour communiquer sur le bus de données Modbus à la vitesse de communication du capteur URM14. Il utilisera l'entrée digitale **30** pour communiquer en OneWire avec la sonde DS18B20.<br>
Le récepteur Drotek DP0601 a été configuré pour diffuser les trames NMEA `$GPGGA` et `$GPRMC` sur son port `UART1`.<br>
Le Teensy utilisera son port `Serial5` pour recevoir les trames NMEA du récepteur Drotek.


## Branchements

Le capteur URM14 doit être alimenté entre 7 et 15V !

|URM14|Interface RS485|Alimentation 7-15V|
|-----|---------------|------------------|
|Fil Blanc|A||
|Fil Bleu|B|
|Fil marron||Borne +|
|Fil noir||Borne -|

|Teensy|DP0601|
|------|------|
|RX5|UART1 B3 (TX)|
|Vin (5V)|UART1 B1 (5V)|
|GND|UART1 B6 (Gnd)|

|Teensy|Interface RS485|
|------|---------------|
|TX4|RX|
|RX4|TX|
|30|DE & RE|
|3V3|3V3|
|GND|GND|

|Teensy|ESP32|DS18B20|
|------|-----|-------|
|3V3|3V3|Fil Orange|
|GND|GND|Fil Blanc|
|30|D18|Fil Jaune|

|Teensy|Bouton|
|------|-------|
|0|Borne +|
|GND|Borne -|

**Pour imiter un bouton poussoir, un fil électrique très court à été utilisé. Il sert simplement à relier l'entrée digitale associée au bouton (pin 0 du Teensy) à la masse (GND). Le bouton est enfoncé quand le fil relie la masse au Teensy et relaché lorsqu'il ne la relie pas.**

![Montage](../assets/GNSS_logger.jpg)

