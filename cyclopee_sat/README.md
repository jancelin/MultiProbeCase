---
layout: default
title: Satellite Cyclopée
nav_order: 1
has_children: True
---


Satellite Cyclopée
==================

## Description
Le satellite Cyclopée est destiné a être embarqué à bord de vehicules terrestres ou acquatiques pour réaliser des mesures GNSS. 
Il permet d'enregistrer les données GNSS brutes fournies par un recpeteur GNSS ZED-F9P sur une carte SD pour post traitement.
Il permet également d'enregistrer sa position GNSS associée à la hauteur mesurée, compensée par rapport à la température ambiante, entre lui-même et le sol / la surface de l'eau.

## Matériel
- Teensy 3.5 ou ESP32 Joy It;
- Mikroe RS485 click 2;
- DFRobot URM14 ultrasonic sensor;
- DallasTemperture DS18B20 probe;
- 4.7kOhms pullup resistor;
- 7-15V power supply;
- 5 or 3.3V power supply.

## Tests unitaires
Le dossier `unit_tests` de ce répertoire propse des exemples de programmes permettant de tester le matériel, notamment le capteur ultrasonore et la sonde de température.