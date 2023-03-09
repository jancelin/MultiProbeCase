Valise multicapteurs
=======================
Synthèse des projets existants
---------------------------------

### SensOcean

#### Fonctionnement
SensOcean est un boîtier autonome de mesure de température et salinité de l'eau de mer ainsi que de la température et pression ambiante. Il fonctionne par cycles de mesures dont la période de déclanchement peut être configurée. Le reste du temps, le boîtier est en veille. Après chaque cycle de mesures, les données sont affichées sur un écran et enregistrées sur une carte SD. Il fonctionne sur batterie et se recharge seul grâce à un panneau solaire.

#### Liste grossière du matériel
- ESP32;
- Ecran ePaper (bien de jour, pas terrible de nuit);
- Horloge temps réel (pour la date et l'heure);
- Sondes avec ou sans circuit de conditionnement;
- Batterie + panneau solaire;
- Slot SD (stockage de la configuration et des données);
- Recepteur GPS (uniquement pour la position).

#### Software
La station majoritairement en veille, elle se réveille pour effectuer les mesures. Utilisation du mode *deepsleep* de l'ESP32, qui permet d'uniquement utiliser la fonction *setup()* d'Arduino.
La configuration du boîtier (intervalles de mesures, etc.) est à renseigner dans le fichier de configuration sur la carte SD.

L'interface utilisateur se compose d'une signalisation LED et de l'écran. Ce dernier affiche l'étape en cours pendant le boot et les mesures après un cycle.

La calibration requiert le téléversement de programmes d'aide à la calibration vers l'ESP32, un moniteur série pour récupérer les valeurs des sondes et des tableaux Excel pour le caclul de la conductivité des solutions étalons. Le système prévient également l'utilisateur si les sondes doivent être recalibrées.

#### Détail des sondes
Ce projet est équipé de 4 capteurs :
- Un capteur de température DS18B20;
- Un capteur de pression BMP280;
- Un kit Atlas Scientific K 0.1 Conductivity;
- Un kit Gravity Analog Temperature.
 
Seuls les kits ici nous intéressent car leurs sondes résistent à l'eau salée.

|Kit Atlas Scientific K 0.1 Conductivity|Graphite|
|---------------------------------------|-|
|Etendue de mesure|0.07 à 50 000 µS/cm|
|Précision|+/-2%|
|Temps de réponse|90% en 1s|
|Températre d'utilisation|1 à 110°C|
|Durée de vies estimée|10 ans|
|Longueur de câble|1m|
|Prix|219.99€|


|Kit Gravity Analog Temperature|Sonde Température PT-1000|
|---------------------------------|-|
|Type de sonde|Platine Classe B, RTD|
|Etendue de mesure|-50 à 200°C|
|Précision|+/-(0,3 + 0,005.T)|
|Temps de réponse|90% en 8s|
|Durée de vie estimée|15 ans|
|Longueur de câble|1m|
|Prix|29.99€|

### SETIER Datalogger

#### Fonctionnement
Son focntionnement est proche de celui de *SensOcean*. Le datalogger effectue des cycles de mesures à une fréquence configurable (choix limités). Il présente par contre une plus grande variété de sondes. Il permet également de déclencher et visulaiser des mesures sur une page HTML, de télécharger les données mesurées et de rentrer les coefficients de calibration via son point d'accès WiFi. Il est alimenté sur secteur.

#### Liste grossière du matériel
- Arduino MKR WiFi;
- Shield de connecteur pour capteurs;
- Shield SD;
- Sondes avec ou sans circuits de conditionnement;
- Boîtes à relais I²C pour le pilotage de l'alimentation des capteurs;
- Hub I²C pour le contrôle des relais;
- Horloge temps réel (pour la date et l'heure);
- Alimentation secteur.

#### Software
Le datalogger est majoritairement en veille. Il se réveille pour effectuer un cycle de mesure capteur par capteur, c.à.d. que tous les capteurs sont mis sous tension, acquierent une donnée et mis hors tension les uns après les autres. La période de mesure est configurable parmi les choix proposés.

L'interface utilisateur est une page HTML accèssible sur le réseau WiFi du datalogger. Elle permet de lancer une mesure pour un capteur en particulier, visualiser la valeur mesurée, télécharger les données acquises et stockées sur la carte SD, entrer les coefficients de calibration. 

Le processus de calibration est identique au projet *SensOcean* mais il n'y a pas besoin de téléverser de nouveau programme à l'ESP32 et les valeurs des sondes sont retournéew sur la page HTML, non pas sur un port série.

#### Détail des sondes
Les capteurs utilisés pour ce projets ont été approuvés pour la mesure d'eaux usées par les chercheurs ayant développé le datalogger.
**/!\ Ils ne semblent néanmoins pas tous totalement étanches /!\ .**

Fournisseurs :

- Farnell
- DFROBOT

|SEN0169|pH|
|---------------------------------|-|
|Tension d'entrée|3.3 à 5.5V|
|Etendue de mesure|0 à 14|
|Température d'utilsation|0 à 60°C|
|Résolution|0.1|
|Temps de réponse|<1min|
|Durée de vie estimée|6 mois|
|Longueur de câble|5m|
|Prix|US$65|

|DFR0300|Conductivité|
|---------------------------------|-|
|Tension d'entrée|3 à 5V|
|Etendue de mesure|0 à 20µS/cm|
|Température d'utilsation|0 à 40°C|
|Durée de vie estimée|6 mois|
|Longueur de câble|1m|
|Prix|65€|

|SEN0464|Potentiel Redox|
|---------------------------------|-|
|Tension d'entrée|5V|
|Etendue de mesure|-2000 à 2000mV|
|Température d'utilsation|5 à 70°C|
|Précision à 25°C|10mV|
|Prix|US$129|

|SEN0237|Oxigène Dissous|
|---------------------------------|-|
|Tension d'entrée|3.3 à 5.5V|
|Etendue de mesure|0 à 20mg/L|
|Température d'utilsation|0 à 60°C|
|Durée de vie estimée|12 mois|
|Durée de vie de la membrane|1-2 mois|
|Durée de vie de la soude|1 mois|
|Longueur de câble|2m|
|Prix|155€|


### OceanIsOpen

#### Fonctionnement
Le projet OceanIsOpen est le plus complet des trois. En plus d'une station de mesure, il compte aussi un système de stockage et mise à disposition des données sophistiqué.

La station de mesure effectue des acquisitions périodiques, de fréquence paramétrable, pour stockage temporaire local sans jamais entrer en veille.

Le système de stockage peut être sépraré en deux parties, le stockage local (sur la station) et distant (sur un serveur distant). Sur la station, les acquisitions sont envoyées dans une base de données PostgreSQL qui les stocke avec leurs métadonnées (date, heure et position GNSS). Ces données sont accessibles via le point d'accès WiFi embarqué de la station. L'application web et un serveur Grafana permettent de gérer et visualiser les données en local.

Les données locales sont envoyées pour stockage vers un serveur PostgreSQL distant grâce à un accès à internet fourni par une carte SIM. De ce serveur accessible depuis internet, elles peuvent être extraites et utilisées.

#### Liste grossière du matériel
- Raspberry Pi;
- Teensy (ESP32);
- Sondes avec ou sans circuits de conditionnement;
- Module GPS;
- Serveur distant.

#### Software
Le Teensy effectue les acquisitions périodiquement (si les logs sont activés) et les envoie aux Raspberry à travers un port série au format JSON. Il scrute également les ordres envoyés par le Rapberry Pi sur ce port.

Le Raspberry joue plusieurs rôles dans le système. Il héberge :

- Un serveur multiusages;
- une application web (interface utilisateur);
- Une base de données PostgreSQL;
- Un serveur Grafana;
- Un service d'injection auto.replay des données au serveur distant.

Le seveur multiusages (serveur.js) est chargé : d'intéragir avec la base de données (insertions, extractions, etc.), de gérer la communication avec le Teensy (données entrantes et ordres sortants), de récupérer l'heure et la position GNSS, et la communication avec l'application web ReactApp (page web sortante et ordres Teensy entrants). L'application web et le serveur Grafana proposent une interface utilisateur permettant de respectivement contrôler paramétrer le Teensy et calibrer les capteurs; et visualiser les données en base de données.
Le script auto.replay permet d'injecter les données sur le serveur distant.

#### Détail des sondes
Fournisseur DFROBOT.

|SEN0161|pH|
|----------------------------------|-|
|Tension d'entrée|5V|
|Etendue de mesure|0 à 14|
|Température d'utilsation|0 à 60°C|
|Précision (25°C)|+/-0.1|
|Temps de réponse|<=1 min|
|Prix|US$29.5|

|DFR0300|Conductivité|
|----------------------------------|-|
|Tension d'entrée|3.0 à 5.0V|
|Etendue de mesure (recomandée)|0 à (15) 20mS/cm|
|Température d'utilsation|0 à 40°C|
|Précision|+/-5% F.S.|
|Durée de vie|6 mois|
|Longueur de câble|1m|
|Prix|US$69.9|

|SEN0189|Turbidité|
|----------------------------------|-|
|Tension d'entrée|5.0V|
|Temps de réponse|<500ms|
|Température d'utilsation|5 à 90°C|
|Prix|US$9.9|

|SEN0244|TDS|
|----------	-----------------------|-|
|Tension d'entrée|3.3 à 5.50V|
|Etendue de mesure|0 à 1000ppm|
|Précision (25°C)|+/-10% F.S.|
|Prix|US$11.80|

|SEN0237-A|Oxygène dissous|
|----------------------------------|-|
|Tension d'entrée|3.3 à 5.5V|
|Etendue de mesure|0 à 20mg/L|
|Temps de réponse (25°C)|98% en 90s|
|Durée de vie de l'électrode|1 an|
|Rempalcement membrane|1-2 mois|
|Longueur de câble|2m|
|Prix|US$169|

|SEN0165|ORP|
|----------------------------------|-|
|Tension d'entrée|3.0 à 5.5V|
|Température d'utilisation|-55 à 125°C|
|Précision (-10 à 85°C)|+/-0.5°C|
|Longueur câble|95cm|
|Prix|US$6.90|


### Tableau récapitulatif des fonctionalités

| |SensOcean|SETIER Datalogger|OceanIsOpen|
|-|---------|-----------------|-----------|
|**Alimentation**|Batterie|Secteur|?|
|**Recharge**|Secteur & solaire||?|
|**Mise en veille**|Oui |Oui |Non|
|**Date/heure**|Horloge|Horloge|GNSS|
|**Accès aux données**|Carte SD|SD & WiFi|Internet, WiFi & SD|
|**Visualisation de données**|Ecran (temps réel) & post traitement| Post traitement|Grafana (local ou distant)|
|**Signalisation LED**|Oui|Non|?|
|**Ecran**|Oui|Non|Oui|
|**Aide à la calibration**|+|++|+++|
