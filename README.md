# Jeu de la vie configurable en C

Projet de Conway conforme a une soutenance de C structuree:

- code decoupe en plusieurs modules `.c/.h`
- allocation dynamique de grilles 2D
- chargement d'un monde initial depuis un fichier texte
- sauvegarde de l'etat a chaque tour ou a la demande
- fichier de configuration externe
- interface SDL legere et mode headless pour les tests
- build portable via CMake sous Windows et Linux

## Organisation du code

- `main.c` point d'entree
- `config.c/.h` lecture du fichier de configuration et des options CLI
- `world.c/.h` gestion de la grille dynamique
- `simulation.c/.h` regles du Jeu de la vie
- `io.c/.h` lecture/criture des mondes texte et sauvegardes
- `app.c/.h` boucle SDL et mode headless

## Prerequis

- CMake 3.24 ou plus recent
- un compilateur C compatible C11
- SDL3 disponible systeme ou telecharge automatiquement par CMake

Sous Linux, installer au minimum les dependances de build usuelles et SDL3 si vous ne laissez pas CMake le telecharger.

### Installer CMake

Windows (avec winget) :

```powershell
winget install Kitware.CMake
```

Linux Debian/Ubuntu :

```bash
sudo apt update
sudo apt install cmake ninja-build
```

Si vous utilisez Ninja, vous pouvez configurer le projet avec le générateur Ninja :

```bash
cmake -S . -B build -G "Ninja"
cmake --build build
```

## Compilation

```powershell
cmake --preset default
cmake --build --preset default
```

Pour reconstruire proprement après une modification du code source :

```powershell
cmake --build --preset default --clean-first
```

Si vous préférez nettoyer puis reconstruire manuellement :

```powershell
cmake --build --preset default --target clean
cmake --build --preset default
```

L'executable est genere dans `output/`.

## Configuration

Le programme peut lire `game_of_life.cfg` automatiquement ou un autre fichier avec `--config`.

Le fichier de configuration pilote la taille du monde, la vitesse d'affichage, la generation aleatoire et les sauvegardes.

Exemple fourni:

```ini
; Exemple de configuration pour la soutenance
columns = 120
rows = 80
cell_size = 8
step_ms = 120
density = 25
seed = 42
wrap_edges = true
save_every_step = false
save_dir = states
initial_world = examples/glider.txt
headless = false
max_steps = 10
```

### Reference des cles

- `columns` nombre de colonnes du monde si aucun fichier initial n'est charge
- `rows` nombre de lignes du monde si aucun fichier initial n'est charge
- `cell_size` taille en pixels d'une cellule dans la fenetre SDL
- `step_ms` delai entre deux generations en mode SDL
- `density` pourcentage de cellules vivantes lors d'une generation aleatoire
- `seed` seed pseudo-aleatoire pour reproduire exactement le meme monde
- `wrap_edges` `true` ou `false`, active ou non l'effet tore sur les bords
- `save_every_step` `true` ou `false`, sauvegarde automatique a chaque generation
- `save_dir` dossier de sortie pour les snapshots texte
- `initial_world` chemin d'un fichier texte contenant un motif initial
- `headless` `true` ou `false`, lance sans interface graphique
- `max_steps` nombre maximal de generations en mode headless

### Priorite des valeurs

L'ordre de priorite est le suivant:

1. valeurs par defaut compilees dans le programme
2. valeurs lues dans `game_of_life.cfg` ou dans le fichier passe a `--config`
3. options de ligne de commande, qui ecrasent la configuration precedente

Exemple:

```powershell
.\output\game_of_life.exe --config game_of_life.cfg --seed 123 --density 40
```

Ici, le fichier de configuration est charge, puis `seed` et `density` sont remplaces par les valeurs passees en ligne de commande.

## Comment generer des Game of Life

Le projet peut etre utilise de deux manieres principales:

- generer un monde aleatoire a partir d'un seed et d'une densite
- charger un motif initial puis produire une suite de generations sauvegardees

### 1. Generer un monde aleatoire en SDL

Si aucun fichier `initial_world` n'est fourni, le programme cree un monde aleatoire a partir de `seed`, `density`, `columns` et `rows`.

Windows:

```powershell
.\output\game_of_life.exe --seed 42 --cols 120 --rows 80 --density 25
```

Linux:

```bash
./output/game_of_life --seed 42 --cols 120 --rows 80 --density 25
```

## Lancement SDL

Le mode SDL sert a observer le monde en direct, ralentir ou accelerer la simulation, modifier la grille a la souris et sauvegarder a la demande.

Windows:

```powershell
.\output\game_of_life.exe --config game_of_life.cfg
```

Linux:

```bash
./output/game_of_life --config game_of_life.cfg
```

## Lancement headless

Pratique pour la soutenance, les tests ou une machine sans affichage:

```powershell
.\output\game_of_life.exe --headless --load examples\glider.txt --max-steps 8 --save-every-step --save-dir states
```

```bash
./output/game_of_life --headless --load examples/glider.txt --max-steps 8 --save-every-step --save-dir states
```

### Monde aleatoire avec sauvegarde de chaque etape

```powershell
.\output\game_of_life.exe --headless --seed 2026 --cols 100 --rows 70 --density 30 --max-steps 15 --save-every-step --save-dir states
```

Chaque generation est alors ecrite dans le dossier choisi.

Le mode headless est aussi le meilleur choix pour:

- generer rapidement une serie de mondes textes
- produire des donnees sans dependre d'un affichage graphique
- demonstrer que le programme reste utilisable sous Linux sur une machine distante

## Format des mondes texte

Le chargeur accepte notamment:

- `#`, `X`, `O`, `1`, `*` pour une cellule vivante
- `.`, `0`, `-`, espace pour une cellule morte

Un exemple de monde initial est fourni dans `examples/glider.txt`.

Exemple de fichier valide:

```text
.#.
..#
###
```

Les sauvegardes produites par le programme utilisent le meme principe, avec en plus un petit en-tete indiquant la generation et la taille du monde.

### Monde aleatoire reproductible

```powershell
.\output\game_of_life.exe --seed 2026 --cols 100 --rows 70 --density 30
```

### Monde charge depuis un motif texte

```powershell
.\output\game_of_life.exe --load examples\glider.txt
```

### Charger une sauvegarde avec affichage

```powershell
.\output\game_of_life.exe --load states\state_exemple.txt
```

Cette commande charge l'état sauvegardé depuis `states\state_exemple.txt` et ouvre la fenêtre SDL pour continuer la simulation.

## Sorties generees

Quand `save_every_step = true` ou quand vous appuyez sur `S` en mode SDL:

- un dossier de sortie est cree si necessaire
- un fichier texte est genere pour chaque generation sauvegardee
- les noms suivent le format `state_000000.txt`, `state_000001.txt`, etc.

Cela permet de rejouer l'evolution, de comparer deux configurations ou de fournir des traces pendant la soutenance.

## Controles SDL

- `Espace` pause / reprise
- `N` une generation en pause
- `G` monde aleatoire avec le seed suivant
- `R` rechargement du monde initial ou regeneration identique
- `C` efface la grille
- `S` sauvegarde l'etat courant dans `save_dir`
- `Fleche haut / bas` accelere ou ralentit
- `Clic gauche / droit` active ou desactive une cellule
- `Echap` quitte

## Points a presenter a l'oral

- pourquoi la grille est geree avec deux tableaux dynamiques
- comment le fichier de configuration surcharge les valeurs par défaut
- comment un monde texte est charge puis sauve a chaque generation
- difference entre mode SDL interactif et mode headless demonstrable sous Linux