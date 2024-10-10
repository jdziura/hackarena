# C++ WebSocket Client for Hackathon 2024

This C++ based WebSocket client was developed for the Hackathon 2024, organized
by WULS-SGGW. It serves as a framework for participants to create AI agents that
can play the game.

To fully test and run the game, you will also need the game server and GUI
client, as the GUI provides a visual representation of gameplay. You can find
more information about the server and GUI client in the following repository:

- [Server and GUI Client Repository](https://github.com/INIT-SGGW/HackArena2024H2-Game)

## Development

The agent logic you are going to implement is located in `src/agent/agent.cpp` and declarations in `src/agent/agent.h`

The `Agent` class implements methods, which define the agent's
behavior. The constructor is called when the agent is created, and the
`NextMove` function is called every game tick to determine the agent's next
move. The `OnGameEnded` function is called when the game ends to provide the
final game state.

`NextMove` returns a `ResponseVariant` (`std::variant`), which can be one of the following:

- `Rotate`: This variant indicates a rotation action, allowing the player to rotate their tank and/or turret.
    - `tankRotation` (`RotationDirection`): Specifies the rotation direction of the tank (left or right).
    - `turretRotation` (`RotationDirection`): Specifies the rotation direction of the turret (left or right).

- `Move`: This variant indicates a movement action, allowing the player to move the tank.
    - `direction` (`MoveDirection`): Specifies the direction of movement (forward or backward).

- `Shoot`: This variant represents a shooting action, where the tank fires a bullet in the direction the turret is pointing.

- `Wait`: This variant indicates that the player chooses to wait, doing nothing during the current tick.

`ResponseVariant` allows the game to handle different player actions in a flexible manner, responding to their decisions at each game tick.

### Enum Definitions:
- **`RotationDirection`**:
    - `left`: Rotate the tank or turret counterclockwise.
    - `right`: Rotate the tank or turret clockwise.

- **`MoveDirection`**:
    - `forward`: Move the tank forward.
    - `backward`: Move the tank backward.

### Warning System

There is also a `WarningType` enum that defines various types of warnings the system may issue in response to certain player actions:
- `CustomWarning`: A general warning that could be used for any custom messages.
- `PlayerAlreadyMadeActionWarning`: A warning that indicates a player has already made their action for this tick.
- `ActionIgnoredDueToDeadWarning`: A warning that the player's action was ignored because their tank is dead.
- `SlowResponseWarning`: A warning that indicates the player's response to the game server was slow, potentially affecting their action's timing.

### Map and Related Structs

The game map is represented by several structs that define its layout, tiles, zones, and the status of various objects like tanks, bullets, walls, and more. Item with index \[0]\[0] represents top-left corner of the map. Here's a breakdown of these components:

#### **Map**
The `Map` struct represents the game world and is made up of tiles and zones:
- **tiles**: A 2D vector of `TileVariant`, where each tile can be one of the following:
    - `Wall`: Represents an indestructible barrier.
        - `zoneName`: The name of the zone where the wall exists (or '?' if no zone).
    - `Tank`: Represents a tank on the map.
        - `ownerId`: The player owning the tank.
        - `direction`: The direction the tank is facing.
        - `turret`: A `Turret` struct containing the direction of the turret and, if available, bullet information.
        - `health` (optional): Health points of the tank (absent for enemies).
        - `isVisible`: Whether the tank is visible to the player.
        - `zoneName`: The name of the zone where the tank is located (or '?' if no zone).
    - `Bullet`: Represents a bullet on the map.
        - `id`: A unique identifier for the bullet.
        - `speed`: The speed at which the bullet moves.
        - `direction`: The direction the bullet is traveling (up, down, left, right).
        - `isVisible`: Whether the bullet is visible to the player.
        - `zoneName`: The name of the zone where the bullet is located (or '?' if no zone).
    - `None`: Represents an empty or non-interactable tile.
        - `isVisible`: Whether the tile is visible to the player.
        - `zoneName`: The name of the zone for this tile (or '?' if no zone).

- **zones**: A vector of `Zone` structs that represent specific regions on the map.
    - **Zone**
        - `x`, `y`: The coordinates of the zone's position on the map.
        - `width`, `height`: The dimensions of the zone.
        - `name`: The character representing the zone (e.g., A, B, C).
        - `status`: A `ZoneStatus` struct representing the current state of the zone.

- **visibility**: A 2D vector of chars representing visibility for each tile ('0' for invisible, '1' for visible).

#### **ZoneStatus**
This struct tracks the current state of a zone, such as whether it's being captured or contested:
- **type**: A string representing the type of status (e.g., "beingCaptured", "captured", "contested").
- **remainingTicks** (optional): If the zone is being captured or retaken, this field shows how many ticks are left.
- **playerId** (optional): The ID of the player capturing the zone.
- **capturedById** (optional): The ID of the player who originally captured the zone.
- **retakenById** (optional): The ID of the player retaking the zone, if applicable.

---

### Player, Lobby, and End Game Structs

#### **LobbyPlayer**
Represents a player in the game lobby before the game starts:
- **id**: A unique identifier for the player.
- **nickname**: The player's chosen name.
- **color**: The player's assigned color, represented as a `uint32_t`.

#### **LobbyData**
This struct holds data related to the game lobby:
- **myId**: The ID of the current player.
- **players**: A vector of `LobbyPlayer` structs, representing all players in the lobby.
- **gridDimension**: The dimensions of the game grid.
- **numberOfPlayers**: The number of players in the game.
- **seed**: A seed value used for randomization.
- **broadcastInterval**: The interval (in milliseconds) between server broadcasts.
- **eagerBroadcast**: A boolean indicating if the game is using eager broadcasting (for faster updates).

#### **EndGamePlayer**
Similar to `LobbyPlayer`, but also includes the player's score:
- **id**: The player's ID.
- **nickname**: The player's name.
- **color**: The player's color.
- **score**: The player's final score at the end of the game.

#### **EndGameLobby**
Holds information about all players at the end of the game:
- **players**: A vector of `EndGamePlayer` structs, containing the end-game data for each player.

---

### Tank, Bullet, and Turret Structs

#### **Tank**
Represents a tank in the game, including its owner and status:
- **ownerId**: The ID of the player controlling the tank.
- **direction**: The direction the tank is facing (e.g., up, down, left, right).
- **turret**: A `Turret` struct representing the tank's turret.
- **health** (optional): The health points of the tank (absent for enemy tanks).
- **isVisible**: Whether the tank is visible to the player.
- **zoneName**: The name of the zone where the tank is located.

#### **Turret**
Represents the turret on a tank:
- **direction**: The direction the turret is facing.
- **bulletCount** (optional): The number of bullets left (absent for enemy turrets).
- **ticksToRegenBullet** (optional): The ticks remaining before a new bullet is regenerated (absent for enemies).

#### **Bullet**
Represents a bullet fired by a tank:
- **id**: A unique ID for the bullet.
- **speed**: The speed of the bullet.
- **direction**: The direction the bullet is traveling.
- **isVisible**: Whether the bullet is visible to the player.
- **zoneName**: The name of the zone where the bullet is located.

---

### GameState Struct

The `GameState` struct captures the state of the game at a specific point in time:
- **time**: The current tick number (a measure of game progress).
- **players**: A vector of `Player` structs, representing all players currently in the game.
    - **Player**
        - **id**: The player's unique ID.
        - **nickname**: The player's name.
        - **color**: The player's assigned color.
        - **ping**: The player's ping (latency).
        - **score** (optional): The player's score (absent for enemies).
        - **ticksToRegen** (optional): The number of ticks until the player's next action can occur (e.g., reloading).
- **map**: The `Map` struct, representing the current state of the game world, including tiles, zones, and visibility.


You can modify mentioned files and create more files in the `src/agent`
directory. Do not modify any other files, as this may prevent us from running
your agent during the competition.

Feel free to extend the functionality of the `GameState` struct or any other structs as you see fit. Just make sure it works :)

## Running the Client

You can run this client in two different ways: locally using CLion (recommended) or Visual Studio or using Docker.

### 1. Running Locally

Have Cmake installed or install it from [here](https://cmake.org/download/)

Install Vcpkg preferably on `C:\` but you can install it anywhere
```sh
   git clone https://github.com/microsoft/vcpkg.git
   ```
then run bootstrap-vcpkg.bat on windows or .sh on linux
```sh
   cd vcpkg; .\bootstrap-vcpkg.bat
   ```

Modify line 11 or 15 in `CMakeLists.txt` depending on vcpkg location

Clone repo in chosen IDE

Preferably add x64-Release profile if it does not exist

### 2. Running in a Docker Container (Manual Setup)

To run the client manually in a Docker container, ensure Docker is installed on
your system.

Steps:

1. Build the Docker image:
   ```sh
   docker build -t client .
   ```
2. Run the Docker container:
   ```sh
   docker run --rm client --nickname Cxx --host host.docker.internal
   ```

If the server is running on your local machine, use the
`--host host.docker.internal` flag to connect the Docker container to your local
host.