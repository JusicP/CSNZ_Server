Note: the glossary is not complete

**Server** - an application that processes client requests.

**Master server** - a server that processes the actions of users such as creating/joining a game server, storing game statistics, communication between users, chatting, inventory i.e. weapons and other user items.

**Client** - the one who connected to the server (master server).

**Packet** - a unit of data sent over network. It consists of header and user data (payload). Header contains information about the user data length, sequence number to ensure that packets arrive in the correct sequence, magic number to identify a valid packet.

**User** - a client logged in to the master server.

**Login** - the procedure of client authentication using a login and password or other data.

**Player** - a user who connected to the game server.

**Game server (game match server)** - a server that is located on the user's side or a master server (dedicated server) at the start of a game match, which provides communication between players within the game.

**Game match** - a structure that is created after the game server starts in the room and stores data of the game state (game time, some settings, game statistics for each player, which is then used to generate game results. These results affect the crediting of rewards for the game).

**Room** - a game room created by a user and belonging to a specific channel in which the user is joined. It can be accessed by other users who are in the same channel. Room has a host - a user who manages the room (kicking users, changing room settings)

**Channel** - the structure to which users belong. Channels can have certain restrictions on the creation of game rooms (for example, their number) or the connection of certain users to it (restrictions on the level of the user).

**Channel server** - the same as the master server, but in the context of channels.