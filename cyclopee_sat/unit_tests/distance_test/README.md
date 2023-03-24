---
layout: default
title: Test de lecture de la distance
nav_order: 1
has_children: False
---

Test de lecture de la distance
==============================

## En bref
Ce programme permet de tester la mesure de distance du capteur ultrasonore URM14 de DFRobot, sur un bus de données Modbus RTU.

## Matériel
- Teensy 3.5 ou carte ESP32;
- Capteur ultrasonore URM14;
- Interface Mikroe RS485 click 2;
- Alimentation 7-15V (URM14).

## Bibliothèque
- `ModbusMaster`.

## Inspiration
Ce programme s'inspire de l'exemple de communication Modbus half-duplex `RS485_HalfDuplex.pde` fourni avec la bibliothèque `ModbusMaster`.

## Montage
Deux montages utilisant un Teensy 3.5 et une carte ESP32 Joy-It ont été testés.

### Ports
Pour ces deux montages, le port de communication `Serial` est utilisé pour le debug via USB (moniteur série de l'IDE Arduino) à 115200 baud. 

Pour faciliter le montage, les ports de communication sur le bus de données sont différents :

- Le Teensy utilisera le port `Serial4` pour communiquer sur le bus de données à la vitesse de communication du capteur URM14.
- L'ESP32 utilisera le port `Serial1` pour communiquer sur le bus de données à la vitesse de communication du capteur URM14. les entrées digitales **D2** (TX) et **D4** (RX) sont définies dans le programme pour ce port.

### Branchements

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

![Montage](../assets/set_up_img/dist_test.jpg)

