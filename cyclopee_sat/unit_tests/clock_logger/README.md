---
layout: default
parent: Tests unitaires
grand_parent: Satellite Cyclopée
title: Logger utilisant l'horloge interne
nav_order: 5
has_children: False
---

Logger utilisant l'horloge interne
==================================

## En bref
Ce programme enregistre les logs de mesures de température et de distances horodatés dans un fichier `.csv` sur la carte SD. Le temps utilisé pour l'horodatage est celui généré par l'horloge interne du Teensy 3.5.<br>

## Inspiration
Ce programme est inspiré des tests unitaires précédents : 

- `ext_temp_comp_test`;
- `SD_test`.


## Fonctionnement
#### Configuration
Les définitions en début de fichier (section `GLOBAL DEFINITIONS`) permettent de configurer le logger (fréquence de mesure, segmentation des fichirs de log, etc.).
#### Setup
Au setup, le programme initilise la carte SD, la date et l'heure, et les capteurs avec la configuration reneignée. Si une dépendance physique du système n'est pas satisfaite, il attendra que le problème soit résolu et d'être redémarré.<br>
#### Logs
La partie log du programme s'éxécute en permanence dans la fonction `loop()`. Cette fonction scanne l'état du bouton pour activer/désativer les logs. S'ils sont activés, alors elle ouvre et gère un fichier de logs (ségmentation, passage au jour suivant) sur la carte SD, et enregistre les logs dans le fichier. Les fichiers de logs sont nommés avec l'heure de leur création et stockés dans un dossier journalier
#### Mesures
La fonction `loop()` est interrompue pour effectuer la lecture des capteurs. Ceci permet d'asurer la périodicité des mesures, même pour des fréquences élevées. Les valeurs lues sont enregistrées dans des buffers permettant de stocker les données à logger. Quand le système ne mesure pas, il vide les buffers dans le fichier de logs.
#### Debug
En cas de problème au démarrage ou pendant le fonctionnement, ou simplement pour monitorer son fonctionnement, le système présente deux fontionnalités de debug sur le port série USB. La première renseigne simplement l'utilisateur sur l'état du système pendant son initiaisation et son fonctionnement. La seconde affiche le contenu du fichier log ouvert.

## Matériel
- Teensy 3.5 ou carte ESP32;
- Capteur ultrasonore URM14;
- Interface Mikroe RS485 click 2;
- Alimentation 7-15V (URM14);
- Sonde de température DS18B20;
- Résistance pull up 4.7kOhms.
- Carte SD.

## Bibliothèques
- `ModbusMaster`;
- `OneWire`;
- `DallasTemperature`;
- `Metro`;
- `TimeLib`;
- `SD`.

## Ports
Le port de communication `Serial` est utilisé pour le debug via USB (moniteur série de l'IDE Arduino) à 115200 baud. <br>
Le Teensy utilisera le port `Serial4` pour communiquer sur le bus de données Modbus à la vitesse de communication du capteur URM14. Il utilisera l'entrée digitale **30** pour communiquer en OneWire avec la sonde DS18B20.

## Branchements

|Teensy|ESP32|Interface RS485|
|------|-----|---------------|
|TX4|D2|RX|
|RX4|D4|TX|
|30|D5|DE & RE|
|3V3|3V3|3V3|
|GND|GND|GND|

Le capteur URM14 doit être alimenté entre 7 et 15V !

|URM14|Interface RS485|Alimentation 7-15V|
|---------------|-----|------------------|
|Fil Blanc|A||
|Fil Bleu|B|
|Fil marron||Borne +|
|Fil noir||Borne -|

|Teensy|ESP32|DS18B20|
|------|-----|-------|
|3V3|3V3|Fil Orange|
|GND|GND|Fil Blanc|
|30|D18|Fil Jaune|


![Montage](../assets/set_up_img/ext_temp_comp_dist.jpg)

