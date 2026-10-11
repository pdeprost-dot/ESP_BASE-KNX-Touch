# Matériel

## Identification réelle

- Produit : Waveshare ESP32-S3-Touch-LCD-1.47 (identification cohérente entre
  esptool, mémoire détectée et documentation constructeur).
- SoC : ESP32-S3 QFN56, révision 0.2, double cœur Xtensa LX7.
- Flash : 16 Mo, mode quad, 3,3 V.
- PSRAM : 8 Mo intégrés à l'ESP32-S3R8.
- USB : USB Serial/JTAG natif, VID:PID `303A:1001`.
- Écran : 1,47 pouce IPS, 172 × 320, contrôleur JD9853.
- Tactile : capacitif, contrôleur AXS5106L.

## Broches constructeur

| Fonction | GPIO |
|---|---:|
| LCD CS | 21 |
| LCD SCLK | 38 |
| LCD MOSI | 39 |
| LCD RST | 40 |
| LCD DC | 45 |
| LCD BL | 46 |
| Touch SCL | 41 |
| Touch SDA | 42 |
| Touch RST | 47 |
| Touch INT | 48 |
| Batterie ADC | 12 |
| SD D2/D3/CMD/CLK/D0/D1 | 13/14/15/16/17/18 |

La révision du PCB imprimée sur la carte n'a pas pu être lue par USB. Elle doit
être confirmée visuellement avant la phase écran.

## Diagnostic tactile préalable

Le schéma constructeur et les exemples Arduino concordent sur :

- AXS5106L à l'adresse I²C 7 bits `0x63`;
- SDA GPIO42 et SCL GPIO41;
- RST GPIO47 et INT GPIO48.

Les exemples ESP-IDF officiels inversent toutefois les deux dernières broches :
`INT=47`, `RST=48`. Le firmware usine `factory` est issu de ces sources ESP-IDF
et ses traces confirment GPIO47 configuré en entrée/interrupt et GPIO48 en
sortie/reset. Ses délais I²C sont donc très probablement provoqués par cette
inversion : le contrôleur n'est pas correctement réinitialisé et sa vraie ligne
d'interruption est pilotée en sortie. Ce résultat désigne d'abord une anomalie
logicielle du BSP constructeur, pas une panne matérielle.

La vérification finale devra employer le brochage Arduino/schéma, appliquer une
impulsion reset bas 200 ms puis haut 300 ms, sonder `0x63` et lire le registre
d'identification `0x08`, avant toute intégration LVGL.

## Validation tactile — phase 2A

Le diagnostic matériel du 10 octobre 2026 confirme le brochage suivant :

| Signal AXS5106L | GPIO | Résultat |
|---|---:|---|
| SDA | 42 | communication stable |
| SCL | 41 | communication stable à 400 kHz |
| RST | 47 | reset bas 200 ms puis haut 300 ms validé |
| INT | 48 | interruptions observées pendant les contacts |

Après ce reset, l'adresse `0x63` acquitte et le registre d'identification `0x08`
renvoie `51 06 01`. La lecture des données à partir du registre `0x01` produit
des coordonnées cohérentes. Avec la rotation employée par l'exemple Waveshare,
les coordonnées écran sont `X=rawY`, `Y=rawX`.

Cinq gestes réels ont été observés : quatre appuis courts et un appui prolongé.
Les positions écran mesurées incluent `(292,145)`, `(213,104)`, `(135,104)`,
`(19,98)` et environ `(187,29–38)`. Les échantillons restent constants ou ne
varient que de quelques unités pendant un maintien. Chaque appui possède un
relâchement correspondant. Aucun événement parasite n'a été observé au repos.

Cette validation confirme que les erreurs du firmware constructeur provenaient
du brochage RST/INT inversé dans son BSP ESP-IDF. Le contrôleur tactile n'est pas
défectueux. Elle ne constitue pas encore une calibration graphique définitive :
orientation, bornes et éventuelle transformation devront être confirmées avec
l'affichage pendant la phase d'interface autorisée ultérieurement.

