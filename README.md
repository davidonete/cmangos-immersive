# Immersive
Immersive mod for cmangos core which will increases roleplay aspect of the game.
The mod comes with the following features:
- Manual attributes: Set up the attributes of your character (stamina, strenght, intellect, spirit and agility) manually on a class trainer instead of automatically when leveling up
- Attribute loss on death: Set up attribute loss when your character dies (you will be able to pick them up again on the class trainer)
- Share xp, reputation, money and quests: Share experience, reputation and money with your alt characters (must have alt characters logged in as bots)
- Fishing Baubles: Require a fishing bauble in order to be able to fish
- Fall damage: Increase the fall damage at will
- Disable respawn: Mobs and gameobjects of an instance won't respawn until the instance is reset

This mod was ported from https://github.com/ike3/mangosbot-immersive and modified to become a independent module as well as adding extra features.

# Available Cores
Classic, TBC and WoTLK

# How to install
1. Follow the instructions in https://github.com/davidonete/cmangos-modules?tab=readme-ov-file#how-to-install
2. Enable the `BUILD_MODULE_IMMERSIVE` flag in cmake and run cmake. The module should be installed in `src/modules/immersive`
3. Copy the configuration file from `src/immersive.conf.dist.in` and place it where your mangosd executable is. Also rename it to `immersive.conf`.
4. Remember to edit the config file and modify the options you want to use.
5. Lastly you will have to install the database changes located in the `src/modules/immersive/sql/install` folder, each folder inside represents where you should execute the queries. E.g. The queries inside of `src/modules/immersive/sql/install/world` will need to be executed in the world/mangosd database, the ones in `src/modules/immersive/sql/install/characters` in the characters database, etc...

# How to uninstall
To remove immersive from your server you have multiple options, the first and easiest is to disable it from the `immersive.conf` file. The second option is to completely remove it from the server and db:

1. Remove the `BUILD_MODULE_IMMERSIVE` flag from your cmake configuration and recompile the game
2. Execute the sql queries located in the `src/modules/immersive/sql/uninstall` folder. Each folder inside represents where you should execute the queries. E.g. The queries inside of `src/modules/immersive/sql/install/world` will need to be executed in the world/mangosd database, the ones in `src/modules/immersive/sql/install//characters` in the characters database, etc...
