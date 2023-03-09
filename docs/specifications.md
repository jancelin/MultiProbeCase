---
layout: default
title: Spécifications
nav_order: 2
has_children: false
---

Valise Multicapteurs
====================
Cahier des charges
------------------

### Fonctionnalités :

- Différents modes d'envoi des données (serveur central, WiFi local) fonctions de l'utilisation.
- Envoi des données au serveur central (via 4G et/ou WiFi).
- Stockage des données en local si pas d'accès internet.
- Calcul de position, date, heure (Décodeur GNSS RTK + réseau Cenipède).
- Capteurs low cost, résistants (interchangeables ?).
- Mesures exploitables :
	- Température.
	- Conductivité (salinité).
	- O2 dissous.
	- Turbidité.
	- Chlorophylle.
	- pH.
- Autonomie maximale.
- User friendly (Interface, calibration, ...)
	
### Idées :

- Raspberrry Pi comme centrale on board et ESP32 comme station de mesure.

![Diagramme des flux du système](assets/schema/flux_diagram_v0.png "Diagramme des flux du système")
