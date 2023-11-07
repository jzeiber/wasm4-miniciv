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
Units in cities get a 50% defense bonus (additive)

Veteran units receive a +1 attack and +1 defense modifier (before any bonus is applied)

Multiple land units may occupy the same space

Only a single water unit may occupy a water space at a time

Land units may embark on water units if there is enough space available by moving on them from land  (Currently there is capacity 1 for Trireme and 2 for Sail)

Land units may disembark by moving from the sea unit to a land space

Water units that dock at a city will disembark their embarked units at that city (Land units in this situation maintain their movement points)

Selecting building "None" in a city production will convert 50% of excess resources into gold  (It will still create the necessary resources for the upkeep of units from this city)

Cities will auto select which surrounding tiles to gather resources from.  Food production is prioritized first.

## City Improvements
Granary - Doubles storage space of food in a city

Barracks - New units created in this city will receive veteran status

Market - Increases food production by 50% (additive)

Bank - Increases gold production by 50% (additive)

Factory - Increases resource production by 50% (additive)

City Walls - Increases defense of units in city by 50% (additive)

Aqueduct - Allows city expanding beyond a population of 10

## Limitations
- 4 Civilizations
- 8 Cities per Civilization (You can capture other cities, but can't build/rebuild more than 8 of your own civs cities)
- Max of 5 units per City
- No fog of war
- No Science/Tech tree
- No government
- No sentry/fotifications/goto
- Taxes are fixed at 7%
- No population happiness
- No air units
- Currently no AI (The computer will just skip their turn)
