# Trials
Trials C++ Plugin for Unreal Tournament.

# Installing
The plugin is currently a very early work-in-progress, and is thus not yet available as a packaged .pak file.

If you really want to try out this plugin, then you'll have to acquire and compile Unreal Tournament (++UT+Main-CL-3228288) yourself, see: https://github.com/EpicGames/UnrealTournament

When is all done and working, proceed by cloning this project and saving it to /UnrealTournament/Plugins/. Compile the project as "Development-Editor", and open STR-Example, and hit play!

# Game Modes
This plugin aims to implement two Trial game modes, STR(Solo Trials) and GTR(Group Trials).

## STR
Solo Trials, or as some of you might call Speed Trials, is a game mode for maps that are designed to be completed by a single player, although many players will be present, the maps are designed in such a way that other players cannot help anyone else but themselves.

A STR map consists of one or multiple objectives, these objectives are very similar to how objectives are represented in Assault of UT99 and UT2004, but will mostly consist of proximity objectives.

An objective will not be disabled upon completion, but can be completed an infinite amount of times, as the goal of a STR map is not to win by completing all objectives, but to achieve the best completion time. 

Each player can achieve a personal best time and thus can compete with anyone, not just the top record holder. A ghost is recorded for each objective completion, but only the top and personal records will keep their ghost. This ghost will always be spawned for every player that is attempting to complete the objective, if the player has no personal best record, then the top's record ghost will be spawned for the player, the player can then race against this ghost.

## GTR
Group Trials, is in most regards identical to STR, but differs in that a group is required to complete an objective.

A GTR map usually consists of triggers, and shooting puzzles that can only completed by a group of people.
Another difference in this game is that the game requires a minimum and maximum amount of players and can only be started when this requirement is fullfilled.


# Making a map
See STR-Example.

TODO.
