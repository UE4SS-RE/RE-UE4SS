# Live Viewer

The Live Viewer is a tool that allows you to search, view, edit & watch the properties of every object making it very powerful for debugging mods or figuring out how values are changed during runtime. Note however that it cannot show unreflected data.

In order to see it, you must make sure that the following configuration settings are set to 1:
- `GuiConsoleEnabled`
- `GuiConsoleVisible`

If you are having any issues seeing the window, for example if it opens as blanked out/white, you can change the `GraphicsAPI` setting, for example to `dx11`.

Sometimes the font is too small to easily see (particularly on larger resolutions). You can edit the `GuiConsoleFontScaling` setting to change this.

![live-viewer](https://github.com/UE4SS-RE/RE-UE4SS/assets/84156063/221ccc8c-b49f-47a5-81b5-59dbbe85cbd1)

You can right-click the search bar to show options.

![live-viewer search options](https://github.com/UE4SS-RE/RE-UE4SS/assets/84156063/94c603df-bb18-4bb7-8fd7-70d50c628b85)

Explanations of the settings:
- `Apply when not searching` will change the search results automatically, otherwise, you need to press enter on the search bar to apply settings changes.
- `Include inheritance` will include any child objects of any search results.
- `Instances only` will only include object instances. These are objects that are present in the level as part of actors (actors themselves, widgets, components etc.), and their properties are a reflection of the real-time values. These generally look like `<package name>_C` or `<package name>_C_<some instance id>` 
- `Non-instances only` will only include the default state of object packages, i.e. ones that are loaded in memory but or not present in the level. You cannot change the values of these properties.
- `CDOs only` will only include the class default objects which are basically the reflected properties inherited by non-instances from a UClass object. You can read more about CDOs [here](https://forums.unrealengine.com/t/what-is-cdo/310820/2). These typically look like `<full path>_GEN_VARIABLE` or `<package path>.Default__<package name>`. 
- `Include CDOs` will include CDOs in any of your search criteria.
- `Use Regex for search` allows you to use regex to make more specific searches.
- `Exclude class name` allows you to exclude specific class names, for example `CanvasPanelSlot` or `StaticMeshComponent`. You can find the class name as the first part of each line. Currently, you can only enter one class name at a time, there is no comma seperate lists or similar.
- `Has property` will only find objects with a property of a specific name, for example `Player Name`. This setting is only applied if you have entered any value in the search bar.
- `Has property type` will only find objects with a property of a specific type, such as `BoolProperty` or `MulticastInlineDelegateProperty`.

In the property viewer pane at the bottom, there are three sub-controls:
- `<<` which goes backwards through your history.
- `>>` which goes forwards through your history.
- `Find functions` which opens a window to search for and call any functions associated with this object and any of its children. You can use this to test calling functions in-game without having to write any code.

![property view controls](https://github.com/UE4SS-RE/RE-UE4SS/assets/84156063/551f9efd-5a68-4e7b-b11c-5fa72955768f)
![find functions](https://github.com/UE4SS-RE/RE-UE4SS/assets/84156063/58ec56ac-d4b9-402b-9f64-9e41c8270bb5)

## Watches

You can right click most property types, and functions, to add a watch.

![add watch](https://github.com/UE4SS-RE/RE-UE4SS/assets/84156063/ea7d8a0f-4966-44bc-a08d-7a197e4e6195)

To view your watches, go into the watches tab. 

Click the plus buttons on the left of each watch to expand the values box.

![expand watch values](https://github.com/UE4SS-RE/RE-UE4SS/assets/84156063/992eb214-304c-428a-b173-920511faf4e0)

On the left side of the watch, there are two checkboxes:
- Enable/disable watch
- Write the watch to a file. If this is enabled, when you close the game, the values from your watche will be saved as a text file in `Binaries/Win64/watches/`.

On the right side of the watch, there is an option to save the watch which will be automatically re-added when you restart the game. This data is stored inside of `Binaries/Win64/watches/watches.meta.json`

![save watch](https://github.com/UE4SS-RE/RE-UE4SS/assets/84156063/8b223d1d-a77f-4222-b287-89da12529c89)
![method of watch re-aquisition](https://github.com/UE4SS-RE/RE-UE4SS/assets/84156063/6cea33d9-73ae-416c-9943-e71553a6433d)
