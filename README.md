# WASM-4 Mini Civ
Multiplayer Civilization type game for WASM-4 fantasy game console

![Gameplay](https://github.com/jzeiber/wasm4-miniciv/raw/main/images/miniciv01.png "Gameplay")

# Play
https://jzeiber.github.io/wasm4-miniciv/bundle/

## Controls
Use button 1 on the controller to perform an action

Use button 2 on the controller to cycle through actions

Use direction buttons to move around or access sub menus/scroll on some screens

## General Notes
When it's your turn you will see an hour glass and time left in the lower right of the screen

You can only move units and perform most actions that affect the world when it's your turn

When it's not your turn you can scroll around the map and see details about your cities

Units in cities get a 50% defense bonus (additive)

Veteran units receive a +1 attack and +1 defense modifier (before any bonus is applied)

Multiple land units may occupy the same space

Only a single water unit may occupy a water space at a time

Land units may embark on water units if there is enough space available by moving on them from land  (Currently there is capacity 1 for Trireme and 3 for Sail)

Land units may disembark by moving from the sea unit to a land space

Water units that dock at a city will disembark their embarked units at that city (Land units in this situation maintain their movement points)

You may sentry a unit to skip them when cycling to the next unit in the menu.  To select them while in sentry status you must scroll to the unit's position on the map and select next unit at loc.  If an enemy moves within 4 spaces of unit it will automatically remove sentry status.

Selecting building "None" in a city production will convert 50% of excess resources into gold  (It will still create the necessary resources for the upkeep of units from this city)

Cities will auto select which surrounding tiles to gather resources from.  Food production is prioritized first.

The game is auto saved after every complete game turn (after the last player ends their turn)

Only player 1 can start a new game / load a game

## City Improvements
Granary - Doubles storage space of food in a city and allows city expanding beyond a population of 8

Barracks - New units created in this city will receive veteran status

Market - Increases food production by 50% (additive)

Bank - Increases gold production by 50% (additive)

Factory - Increases resource production by 50% (additive)

City Walls - Increases defense of units in city by 50% (additive)

Aqueduct - Allows city expanding beyond a population of 15 (must have granary as well for population growth)

## Limitations
- 4 Civilizations
- 8 Cities per Civilization (You can capture other cities, but can't build/rebuild more than 8 of your own civs cities)
- Max of 5 units per City
- No fog of war
- No Science/Tech tree
- No government
- No fotifications/goto for units
- No diplomacy
- No trade routes
- No roads/land improvements
- No pollution
- Taxes are fixed at 7%
- No population happiness
- No air units
- Very basic AI
