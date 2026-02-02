# TLC Super Solvers Mission Structure

This document details the mission progression, objectives, and game flow for each supported game.

---

## Operation Neptune (1991)

### Overview
The player pilots a submarine to recover canisters containing a critical scientific formula from a sunken research ship. The underwater environment is a scrolling maze filled with sea creatures and obstacles.

### Main Objective
Recover all formula canisters from the depths of the ocean while managing submarine resources.

### Game Structure

#### Hub: Submarine Base
- Starting location between missions
- View collected canisters
- Review mission briefing
- Select difficulty level

#### Mission Flow
```
1. Launch submarine from base
2. Navigate underwater labyrinth
3. Encounter sea creatures (educational challenges)
4. Collect formula canisters
5. Manage oxygen and fuel levels
6. Return to base or continue deeper
7. Complete all canister collections
```

### Labyrinth Navigation
- **Scrolling maze** - Multi-screen underwater environment
- **Depth levels** - Multiple vertical layers to explore
- **Hazards** - Jellyfish, eels, sharks, sea plants
- **Collectibles** - Oxygen tanks, fuel cells, treasure

### Educational Challenges
When encountering sea creatures, player must solve:

#### Word Problems (Reading Comprehension)
- Read a passage about marine life
- Answer multiple-choice questions
- Topics: Ocean science, marine biology, ecology

#### Number Problems (Math)
- Arithmetic operations
- Word problems with numerical answers
- Difficulty scales with game level

#### Sorting Puzzles
- Categorize items by properties
- Match related concepts
- Pattern recognition

### Difficulty Levels
| Level | Challenge | Creatures |
|-------|-----------|-----------|
| Easy | Basic math, simple reading | Friendly fish |
| Medium | Multi-step problems | Moderate threats |
| Hard | Complex word problems | Aggressive creatures |

### Win Condition
Collect all formula canisters and return safely to the submarine base.

### Asset Mapping (Expected)
| Game Element | Resource File | Type |
|--------------|---------------|------|
| Labyrinth maps | LABRNTH1.RSC, LABRNTH2.RSC | Tilemaps |
| Sorting puzzles | SORTER.RSC | Sprites + logic |
| Reading puzzles | READER1.RSC, READER2.RSC | Text + UI |
| Common sprites | COMMON.RSC | Submarine, creatures |
| Audio | WSCOMMON.GRP, WS*.GRP | WAV files |

---

## Gizmos & Gadgets (1993)

### Overview
The player (a Super Solver) must beat Morty Maxwell in building vehicles by collecting parts from a warehouse filled with science puzzles.

### Main Objective
Collect vehicle parts by solving science puzzles, build working vehicles, and win races against Morty Maxwell.

### Game Structure

#### Hub: Main Building Select
Three buildings, each with a different vehicle type:
- **Building 1**: Cars
- **Building 2**: Airplanes
- **Building 3**: Alternative vehicles (hovercraft, etc.)

#### Building Structure
Each building has 5 floors:
```
Floor 5 (Top)    - Advanced puzzles, rare parts
Floor 4          - Harder puzzles
Floor 3 (Middle) - Medium difficulty
Floor 2          - Easier puzzles
Floor 1 (Ground) - Starting area, vehicle assembly
```

#### Mission Flow
```
1. Enter building
2. Explore floors using ladders/elevators
3. Find locked rooms with parts
4. Solve science puzzles to unlock rooms
5. Collect vehicle parts
6. Avoid/outrun Morty Maxwell
7. Return to Floor 1 to assemble vehicle
8. Race against Morty's vehicle
9. Win race to complete building
10. Repeat for all 3 buildings
```

### Puzzle Categories (8 Types)

#### 1. Balance Puzzles
- Balance scales with weighted objects
- Understand mass and equilibrium
- Resource ID range: 42xxx

#### 2. Electricity Puzzles
- Complete electrical circuits
- Connect batteries, wires, bulbs
- Resource ID range: 43xxx

#### 3. Energy Puzzles
- Transform energy types
- Potential/kinetic energy
- Resource ID range: 44xxx

#### 4. Force Puzzles
- Apply forces to move objects
- Friction, gravity, momentum
- Resource ID range: 45xxx

#### 5. Gear Puzzles
- Arrange gears to transfer motion
- Gear ratios and direction
- Resource ID range: 46xxx

#### 6. Jigsaw Puzzles
- Assemble machine diagrams
- Visual-spatial reasoning
- Resource ID range: 47xxx

#### 7. Magnet Puzzles
- Use magnetic fields
- Attract/repel objects
- Resource ID range: 48xxx

#### 8. Simple Machine Puzzles
- Levers, pulleys, inclined planes
- Mechanical advantage
- Resource ID range: 49xxx

### Vehicle Assembly
- Collect all required parts for vehicle type
- Parts have different quality levels (affects race performance)
- Better parts = faster/better handling vehicle

### Racing Minigame
- Side-scrolling race track
- Player vehicle vs Morty's vehicle
- Performance based on parts quality
- Must cross finish line first

### Morty Maxwell (Antagonist)
- Roams building floors
- Steals parts from player if caught
- Builds his own vehicle simultaneously
- Must outrun or avoid

### Difficulty Levels
| Level | Puzzles | Morty | Racing |
|-------|---------|-------|--------|
| Easy | Simpler solutions | Slower, less aggressive | Forgiving |
| Medium | Standard difficulty | Moderate speed | Competitive |
| Hard | Complex solutions | Fast, aggressive | Challenging |

### Win Condition
Complete all three buildings by winning all three races.

### Asset Mapping
| Game Element | Resource File | Type |
|--------------|---------------|------|
| Main sprites | GIZMO256.DAT | 256-color sprites |
| Puzzle graphics | PUZ256.DAT | Puzzle sprites |
| Room layouts | GIZMO.DAT | Room definitions |
| Racing | AUTO*.DAT, PLANE*.DAT | Race sprites |
| Fonts | FONT.DAT | Text rendering |
| Audio | GIZMO.DAT (CUSTOM_32519) | WAV audio |
| Music | MIDI/*.MID | MIDI files |

---

## OutNumbered! (1990)

### Overview
The player explores a TV studio to stop the Master of Mischief from causing chaos. Math problems unlock rooms and defeat enemies.

### Main Objective
Navigate the TV station, solve math challenges to collect clues, and stop the Master of Mischief.

### Game Structure

#### Hub: TV Station Lobby
- Central navigation point
- Access to different studio areas
- View collected clues

#### Studio Areas
- **Newsroom** - Current events themed puzzles
- **Game Show Set** - Competition-style math
- **Control Room** - Technical puzzles
- **Storage/Backstage** - Hidden clues

### Mission Flow
```
1. Enter TV station
2. Explore studio areas
3. Encounter Telly (robot antagonist)
4. Solve math problems to:
   - Open locked doors
   - Defeat Telly
   - Collect clues
5. Gather all clues
6. Confront Master of Mischief
7. Final math challenge
```

### Educational Challenges
- **Addition/Subtraction** - Basic to multi-digit
- **Multiplication/Division** - Times tables to long division
- **Word Problems** - Real-world math scenarios
- **Number Patterns** - Sequences and relationships
- **Estimation** - Approximate calculations

### Telly (Antagonist Robot)
- Patrols studio areas
- Challenges player to math duels
- Can be defeated with correct answers
- Incorrect answers cost "zaps" (health)

### Win Condition
Collect all clues and defeat the Master of Mischief in the final confrontation.

---

## Spellbound! (1991)

### Overview
The player explores a haunted house to rescue a wizard's apprentice by solving word puzzles and spelling challenges.

### Main Objective
Navigate the haunted mansion, solve word puzzles to collect spell ingredients, and cast the rescue spell.

### Game Structure

#### Hub: Mansion Entrance
- Map of explored rooms
- Spell book (tracks progress)
- Ingredient inventory

#### Mansion Layout
- **Ground Floor** - Entry rooms, library
- **Upper Floors** - Bedrooms, attic
- **Basement** - Laboratory, dungeon
- **Tower** - Final challenge location

### Mission Flow
```
1. Enter haunted mansion
2. Explore rooms for clues
3. Encounter word challenges
4. Collect spell ingredients
5. Avoid/defeat ghostly enemies
6. Assemble complete spell
7. Reach tower for final challenge
8. Cast rescue spell
```

### Educational Challenges
- **Spelling** - Spell words correctly
- **Vocabulary** - Word definitions
- **Word Building** - Prefixes, suffixes, roots
- **Analogies** - Word relationships
- **Context Clues** - Figure out meaning from usage

### Enemies/Obstacles
- **Ghosts** - Word challenges to defeat
- **Locked Doors** - Spelling keys
- **Traps** - Quick word puzzles

### Win Condition
Collect all spell ingredients and successfully cast the rescue spell.

---

## Treasure Mountain! (1990)

### Overview
The player climbs a mountain to catch elves and collect treasures by solving reading and logic puzzles.

### Main Objective
Ascend the mountain, capture elves by answering their riddles, and collect the crown jewels at the summit.

### Game Structure

#### Hub: Mountain Base
- Trail map
- Treasure inventory
- Hint system

#### Mountain Levels
```
Level 1 (Base)    - Forest trails, easy puzzles
Level 2           - Rocky paths, medium puzzles
Level 3           - Alpine meadows, harder puzzles
Level 4 (Summit)  - Peak challenges, crown jewels
```

### Mission Flow
```
1. Start at mountain base
2. Climb trails (side-scrolling exploration)
3. Find and catch elves
4. Answer riddles correctly to earn:
   - Coins (for hints)
   - Keys (to unlock paths)
   - Treasures (goal items)
5. Reach each level's treasure
6. Ascend to summit
7. Collect crown jewels
```

### Educational Challenges
- **Reading Comprehension** - Story-based riddles
- **Logic Puzzles** - Deductive reasoning
- **Classification** - Sorting and categories
- **Sequencing** - Order and patterns
- **Inference** - Reading between the lines

### Elves
- Hide in bushes and trees
- Must be found and caught
- Each has a riddle to solve
- Correct answer = reward

### Treasures
- Gems and coins scattered
- Special treasures at level milestones
- Crown jewels at summit = victory

### Win Condition
Collect the crown jewels at the mountain summit.

---

## Treasure MathStorm! (1992)

### Overview
The player climbs an enchanted mountain to rescue the crown from an evil Ice Queen by solving math puzzles.

### Main Objective
Ascend the snow-covered mountain, solve math challenges from elves, and reach the Ice Queen's castle.

### Game Structure

#### Hub: Mountain Village
- Equipment shop (spend coins)
- Progress tracking
- Difficulty selection

#### Mountain Zones
```
Zone 1: Forest Base     - Basic math
Zone 2: Snowy Slopes    - Intermediate problems
Zone 3: Ice Caves       - Advanced challenges
Zone 4: Castle Summit   - Final confrontation
```

### Mission Flow
```
1. Start in mountain village
2. Collect equipment (sled, snowshoes)
3. Explore mountain paths
4. Find treasure chests (math puzzles)
5. Catch elves for more puzzles
6. Collect crystals and gems
7. Navigate ice obstacles
8. Reach Ice Queen's castle
9. Final math challenge
10. Rescue the crown
```

### Educational Challenges
- **Arithmetic** - All four operations
- **Fractions** - Basic to complex
- **Measurement** - Length, weight, volume
- **Money** - Counting, making change
- **Time** - Clocks, elapsed time
- **Geometry** - Shapes, spatial reasoning

### Ice Queen (Antagonist)
- Creates storms and obstacles
- Sends ice creatures
- Final boss with hardest puzzles

### Equipment System
- Buy items with collected coins
- Items help navigate obstacles
- Better equipment = easier traversal

### Win Condition
Defeat the Ice Queen's challenges and recover the stolen crown.

---

## Treasure Cove! (1992)

### Overview
Underwater adventure collecting treasures by solving reading puzzles. Similar structure to Treasure Mountain but ocean-themed.

### Main Objective
Explore the underwater cove, solve reading puzzles to collect treasures, and find the legendary pearl.

### Game Structure
- **Coral Reef Zones** - Progressively deeper areas
- **Sea Creatures** - Give puzzles for rewards
- **Sunken Ship** - Major treasure location
- **Pearl's Location** - Final challenge area

### Educational Challenges
- **Phonics** - Sound-letter relationships
- **Vocabulary** - Word meanings
- **Reading Comprehension** - Story understanding
- **Word Families** - Related words

### Win Condition
Collect all treasures and find the legendary pearl.

---

## Common Design Patterns

### Across All Games
1. **Hub and Spoke** - Central location with areas to explore
2. **Progressive Difficulty** - Easier content at start, harder deeper in
3. **Collect and Return** - Gather items to achieve goal
4. **Antagonist Presence** - Enemy that adds pressure/challenge
5. **Educational Integration** - Puzzles tied to curriculum goals
6. **Multiple Difficulty Levels** - Adapt to player skill

### UI Elements (Expected)
- Health/resource meter
- Inventory display
- Score/progress counter
- Hint system
- Pause/menu access

### Audio Cues
- Correct answer sounds
- Wrong answer sounds
- Discovery/collection jingles
- Background music per area
- Character voice clips (some games)
