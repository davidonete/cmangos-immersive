# Immersive Mod
Immersive mod for cmangos core which will increases roleplay aspect of the game. Currently only compatible with classic core.
The mod comes with the following features:
- Manual attributes: Set up the attributes of your character (stamina, strenght, intellect, spirit and agility) manually on a class trainer instead of automatically when leveling up
- Attribute loss on death: Set up attribute loss when your character dies (you will be able to pick them up again on the class trainer)
- Share xp, reputation, money and quests: Share experience, reputation and money with your alt characters (must have alt characters logged in as bots)
- Fishing Baubles: Require a fishing bauble in order to be able to fish
- Fall damage: Increase the fall damage at will
- Disable respawn: Mobs and gameobjects of an instance won't respawn until the instance is reset

# How to install
NOTE: This guide assumes that you have basic knowledge on how to generate c++ projects, compile code and modify a database

1. You should use a compatible core version for this to work: 
- Classic (Immersive only): https://github.com/davidonete/mangos-classic/tree/immersive-mod
- Classic (All mods): https://github.com/davidonete/mangos-classic/tree/mods
- TBC (Immersive only): To be done...
- TBC (All mods): To be done...
- WoTLK (Immersive only): To be done...
- WoTLK (All mods): To be done...

2. Clone the core desired and generate the solution using cmake. This mod requires you to enable the BUILD_IMMERSIVE flag for it to compile.

3. Build the project.

4. Copy the configuration file from src/immersive.conf.dist.in and place it where your mangosd executable is. Also rename it to immersive.conf.

5. Remember to edit the config file and modify the options you want to use.

6. Lastly you will have to install the database changes located in the sql folder, each folder inside represents where you should execute the queries. E.g. The queries inside of "world" will need to be executed in the world/mangosd database, the ones in characters in the characters database, etc...