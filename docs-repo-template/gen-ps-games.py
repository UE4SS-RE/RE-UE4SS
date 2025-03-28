# This python script takes the output from Patternsleuth, and generates a markdown
# file displaying which games are supported by our AOBs out of the box.
# This is the Patternsleuth command:
# cargo run --release report -r GUObjectArray -r FNameToString -r FNameCtorWchar -r GMalloc -r StaticConstructObjectInternal -r FTextFString -r EngineVersion
# The json file in /report/ should be given as the '-f' param to the python script.

import argparse
import json

parser = argparse.ArgumentParser()
parser.add_argument("-f", "--file", help = "JSON file generated from Patternsleuth", required = True)
args = parser.parse_args()

successful_games = ""
failed_games = ""

banned_games = [
    "Tekken",
    "BlankProject",
    "505S_Blank505",
    "ShooterGame",
    "Shooter Game",
    "Shooter game",
    "ShooterExample",
    "DragonBall",
    "DRAGON BALL",
    "AimGods",
    "Guilty_Gear_Strive",
    "MarvelMidnightSuns",
    "Assetto",
    "PayDay",
    "PAYDAY",
    "Texas_Chainsaw_Massacre",
    "MK12",
    "Deceit",
    "Unfortunate Spacemen",
    "Rogue Operatives",
    "StateOfDecay",
    "VRMultigames",
    "ZomDay",
    "Murnatan",
    "Spellsworn",
    "BattleRush",
    "PWND",
    "Islands of Nyne Battle Royale",
    "Kabounce",
    "Battle Siege Royale",
    "Z Escape",
    "Predator Hunting Grounds",
    "Drifters Loot the Galaxy",
    "XERA Survival",
    "Aliens Fireteam Elite",
    "Back4Blood",
    "Rogue Company",
    "BATTALION Legacy",
    "Hypercharge Unboxed",
    "Grapple Tournament",
    "StarshipTroopersExtermination",
    "Vail VR",
    "PaxDei_nonencrypted",
    "Off The Grid",
    "Dreadnought",
    "RoboSports VR",
    "Fog Of War - Free Edition",
    "Hide vs. Seek",
    "BATTALION Legacy",
    "Into The Core",
    "Not My Car Battle Royale",
    "TSA Frisky",
    "Darwin Project",
    "Last Year",
    "Tower Unite Dedicated Server",
    "Capsular",
    "PROZE Prologue",
    "PROP AND SEEK",
    "Valgrave Immortal Plains",
    "Refight Burning Engine",
    "VEILED EXPERTS",
    "AXIOM SOCCER",
    "Light Bearers",
    "Roboquest",
    "Rumbleverse",
    "Warhaven Demo",
    "POLYGON",
    "Team Rise",
    "GrayZoneWarfare",
    "Vindictus",
    "Shootout",
    "Dead_By_Daylight",
    "Ship Ahoy Open BETA",
    "The Cleansing - Versus",
    "Laser League",
    "Major League Gladiators",
    "Outbreak in Space VR - Free",
    "Pew Dew Redemption",
    "RUCKBALL",
    "Destroy All Humans Clone Carnage",
    "Rock of Ages 3 Make and Break Hot Potato",
    "Divine Knockout",
    "Singulive",
    "Survive The Island",
    "Phoenix Squadron Azure Stars",
    "Paragon Overprime",
    "Predecessor",
    "Battle Life Online",
    "Brimstone Brawlers",
    "Seekers of Skyveil Playtest",
]

with open(args.file, 'r') as file_input:
    json_data = json.load(file_input)
    for game in json_data:
        should_skip = False
        for banned_game in banned_games:
            if banned_game in game:
                should_skip = True
                break
        if should_skip:
            continue
        is_supported = True
        why_not_supported = []
        for aob in json_data[game]:
            for result in json_data[game][aob]:
                if result == "Err":
                    is_supported = False
                    why_not_supported.append("`" + aob + "`: " + json_data[game][aob][result]["Msg"])
        if not is_supported:
            failed_games += f'''<details>
<summary>{game}</summary>\n\n'''

            for r in why_not_supported:
                failed_games += f'{r}  \n'
            failed_games += '\n</details>\n\n'
        else:
            successful_games += f"{game}  \n"

template = f'''
# UE4SS Game Support (AOB only)

This is a list of games that UE4SS will find AOBs for out of the box.  

> [!IMPORTANT]
> Full support is **not** implied just because all AOBs are found for a game.  
> There can be various other problems preventing UE4SS from working out of the box. 

Inclusion of a game in this list does not guarantee support or imply endorsement of UE4SS usage on that game.  
Users are advised to review and comply with the game's Terms of Service and any applicable agreements before using mods or third-party software.

There's a second list at the bottom that contains games that Patternsleuth is testing, but can't find AOBs for.  
These lists are powered by [Patternsleuth](https://github.com/trumank/patternsleuth).

## Games tested with all AOBs found by Patternsleuth

{successful_games}

## Games tested but with AOBs not found by Patternsleuth

{failed_games}

'''
print(template)
