------------
# jzIntvImGui (Nintendo Switch)
------------
This is the Work In Progress branch for the Nintendo Switch port of [jzIntvImGui](https://github.com/jenergy/jzIntvImGui). 
Emulation is already perfect, while interface has little issues to be fixed:

- Switch between Screenshots / Boxes with L / R buttons, instead of + and -
- Use left analog stick for navigate roms list
- Use right analog stick to scroll description of games
- Map 'B' button as Escape in "Options" screen
- Fix ImguiFileDialog when navigating between folders in "Options" screen
- Fix soft keyboard management: keyboard appears but input is not recognized
- Fix home button management in emulation, in order to automatically pause game in progress
- Make physical keyboards work
-------------------------
# Execution notes

#### Nintendo Switch
- Copy "bin" folder and all its content in a subdirectory of directory "switch" in the root of SD 
- Copy all your roms and bioses in the "resources/Roms" subfolder
- If you wish to have your app in home, you can install provided nsp, it points to sdmc:/switch/jzIntvImGui/jzIntvImGui.nro

 

Provided package is automatically configured to use the hackfile named "hackfile_nintendo_switch.cfg". You can find it into subfolder "resources/Configs".
Here is the buttons map:

```
X+Y (combo)       --> 1
X                 --> 2
A+X (combo)       --> 3
Y                 --> 4
L3                --> 5
A                 --> 6
B+Y (combo)       --> 7
B                 --> 8
A+B (combo)       --> 9
Right Stick Left  --> Clear
R3                --> 0
Right Stick Right --> Enter
-                 --> Reset
+                 --> Pause
+ + - (combo)     --> Quit game
Left Stick        --> Disc (16 directions)
Left Buttons      --> Disc (8 directions)
```

Other hackfile are provided, you can choose them in "Options" mask (use touchPad)

<br/><br/>
Have fun! <br/>
###### Daniele Moglia (jenergy@tiscali.it)
 









