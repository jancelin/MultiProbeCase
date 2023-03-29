---
layout: default
title: Satellite Cyclopée
nav_order: 4
has_children: True
---


Satellite Cyclopée
==================

## Description
Le satellite Cyclopée est destiné a être embarqué à bord de vehicules terrestres ou acquatiques pour réaliser des mesures GNSS. 
Il permet d'enregistrer les données GNSS brutes fournies par un récpeteur GNSS ZED-F9P sur une carte SD pour post traitement.
Il permet également d'enregistrer sa position GNSS associée à la hauteur mesurée, compensée par rapport à la température ambiante, entre lui-même et le sol / la surface de l'eau.

![Diagramme des flux](assets/schemas/flux_diagram_latest.png)

Le satellite est en cours de développement, pour l'instant seuls les capteurs de distance et de température ont été testés et le montage suivant de mesure de distance compensée a été réalisé :

![Photo du montage actuel](unit_tests/assets/set_up_img/ext_temp_comp_dist.jpg)

## Matériel
- Teensy 3.5 (ou ESP32 Joy It);
- Antenne GNSS multi-bandes IP66;
- Récpeteur GNSS Ublox ZED-F9P;
- Mikroe RS485 click 2;
- Capteur ultrasonore DFRobot URM14;
- Sonde DallasTemperature DS18B20;
- Résistance pullup 4.7kOhms;
- Alimentation (URM14) 7-15V;
- Alimentation (Board) 5 or 3.3V.

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

## Tests unitaires
Le dossier `unit_tests` de ce répertoire propose des exemples de programmes permettant de tester le matériel, notamment le capteur ultrasonore, la sonde de température et la carte SD.