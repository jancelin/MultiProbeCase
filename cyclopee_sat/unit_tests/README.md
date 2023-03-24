---
layout: default
title: Satellite Cyclopée
nav_order: 1
has_children: True
---

Tests unitaires
==================

Chaque test unitaire des capteurs ultrasonore et de température a été effectué sur deux types de boards, un Teensy 3.5 et une carte ESP32 de chez Joy It. Tous les programmes fonctionnent sur ces deux boards moyennant quelques légères modifications des `#define`. Les commentaires permmetent de comprendre le fonctionnement du programme.

Les librairies utilisées pour communiquer avec les différents composants du sytème se situent dans le dossier `libraries`.

## Les tests
- `temperature_test` permet de tester le fonctionnement de la sonde de température DS18B20.
- `distance_test` permet de tester le fonctionnement du capteur ultrasonore URM14.
- `ext_temp_comp_dist` permet de tester la mesure de distance avec l'URM14, compensée avec la température ambiante mesurée par la sonde DS18B20.

