# AutoFlush

# Matériel : 

Arduino Nano ou compatible

* MAX7219 8 x 8 Dot Matrix 
(https://www.amazon.fr/gp/product/B07L2PDL7H/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&psc=1)

* Rotary Encoder KY-040
(https://www.amazon.fr/gp/product/B06XT58ZW9/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&psc=1)

* Servo Moteur Bluebird BMS-620MG
(https://www.conrad.fr/p/servomoteur-standard-analogique-bluebird-bms-620mg-bms-620mg-1-pcs-275462)

* Alimentation USB
* Câble USB A à sacrifier 

* Sonar : Module ultrasons HC-SR04
(https://www.amazon.fr/gp/product/B01C6O9AH6/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&psc=1)

# Utilisation du boitier

L'appareil a deux modes : un mode qui détecte le passage de l'animal, puis tire la chasse, l'autre mode ne fait rien, et attends les instructions. 

Pour passer d'un mode à l'autre, il faut trouner le bouton vers la gauche. 

Quelque soit le mode, tourner le bouton vers la droite permet de tirer la chasse. 

Chaque cran tourné vers la droite permet d'augmenter la durée de levage du bras. Cela permet de faire une chasse avec plus ou moins d'eau. 

Le temps de levage est représenté sur la matrice de LED, et s'ajuste en fonction de l'ordre demandé avec le bouton rotatif. Plus le temps est important, plus de LEDs sont allumées. 

## Logique générale de l'appareil, et utilisation 

Pour que l'appareil puisse estimer la présence d'un chat, il doit d'abord passer par une phase d'apprentissage.
Durant la phase d'apprentissage, l'appareil détermine la distance qui sépare le capteur ultrason du point visé sur la cuvette, ou le rebord des WC. 

Cette phase d'apprentissage est signalée par des flèches <-- --> sur la matrice LED. 

Une fois cet apprentissage terminé (environ 2 minutes), l'appareil est prêt à détecter le passage du chat. 

A partir de maintenant, la matrice LED représente l'activité de détection. 
Tant que l'appareil est en détection, les diodes s'allument en alternance, et cyclent chaque seconde. 
Dès que l'appareil aura déterminer qu'il faut tirrer la chasse d'eau, la grille de LED deviendra fixe, jusqu'au moment de tirer la chasse d'eau. 

Tant que l'appareil ne détecte rien, il n'y a que 2 points qui s'allument. 
Plus l'écart de mesure faite par le sonar est important, plus la grille de la matrice est remplie. 

L'appareil effectue la chasse d'eau automatique quelques minutes après que la valeur du sonar soit revenue à sa valeur normale, celle apprise durant la phase d'apprentissage. 

Par mesure de précaution, l'apprentissage est refait après chaque chasse d'eau automatique. 


