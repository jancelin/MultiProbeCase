---
layout: default
title: Utilisation
parent: MultiProbeCase
nav_order: 2
has_children: false
---


###### Utilisation (Nouveau .md)

Concernant les sorties, une IHM rustique, mais fonctionnelle, a été implémentée à partir d’une
LED. Elle permet le monitoring du fonctionnement du satellite. Celle-ci consiste en une LED restant allumée pendant le démarrage, et clignotant à différentes fréquences pendant le fonctionnement. Au démarrage, si une condition nécéssaire au fonctionnement n’est pas satisfaite, cette LED reste indéfiniment allumée. Pendant le fonctionnement, un clignotement lent signifie que le satellite enregistre des données ; un clignotement intermédiare, que l’enregistrement est désactivé ; et un clignotement rapide, qu’un problème est apparu. A noter que certains problèmes, comme la déconnexion d’un capteur, ne sont détectés que si l’appareil enregistre. Une sortie
textuelle de débogage est également disponible sur le port série USB, et permet d’identifier les problèmes survenus. En entrée, seul un bouton permet d’activer/désactiver l’enregistrement. Ces états (LED et enregistrement) sont gérés par l’interruption de lecture/écriture des entrées/sorties.
La configuration des satellites passe, pour le moment, par la modification du code source. Pour
faciliter son utilisation, et minimiser les changements à effectuer, le programme a été conçu pour
être modulaire. La configuration du système (fréquence d’acquisition, segmentation des données,
etc.) s’effectue en tête du fichier, et le choix des capteurs, par l’inclusion des modules développés
pour ceux-ci. La Figure 5.5 illustre l’organisation du fichier source.
Des améliorations sont à prévoir comme l’endormissement du microcontrôleur, la modularisa-
tion des autres périphériques, ou le rassemblement des buffers en une seule classe. Des efforts
seront également menés pour affranchir l’utilisateur de la modification du code source. Nous
avons donc pensé à un fichier de configuration sur la carte SD, qui sera lu à chaque démarrage
du système, plutôt que de devoir téléverser un nouveau programme à chaque modification. L’ob-
jectif final est d’éliminer toute différence logicielle entre les satellites, et d’obtenir un kit flexible,
capable d’héberger n’importe lequel d’entre eux, voire y connecter un jeu de sondes personnalisé.
La réception d’ordres de la passerelle doit également encore être implémentée pour permettre le
contrôle du système à distance.

