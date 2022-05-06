------------
# jzIntvImGui
------------

Welcome to jzIntvImGui! It's an all-in-one powerful Dear ImGui interface which allows you to manage your collection of Intellivision games, offering an easy and immediate way to select and play a specific game through the magic of jzIntv. Actually you don't need to have jzIntv as external program, since it is integrated in the product.

A port of jzIntvImGui exists for:
- Windows
- Linux
- Android
- [Nintendo Switch](https://github.com/jenergy/jzIntvImGui/tree/Nintendo_Switch)

This is the main interface Window, showing the list of games.

![image](https://user-images.githubusercontent.com/57037540/140431278-7e9b8dd6-7624-4d98-9f5f-44df017e034a.png)

An item is GREEN when the program found the rom file related to a game configured in the internal database, by CRC32 match.

An item is RED when the program DIDN'T find the rom file related to a game configured in the internal database, by CRC32 match.

An item is YELLOW when the program found a rom file with a not configured CRC32.

The same view is available on android too when device orientation is landscape

![image](https://user-images.githubusercontent.com/57037540/140434140-7e401212-87a6-4f5f-92ef-747a3abfa361.png)

While in portrait mode you'll have only the list of games

![image](https://user-images.githubusercontent.com/57037540/140434219-e99a8593-47e6-497f-82e0-be5e0d2ce4bf.png)

Obviously, you have to choose the folder containing roms (and bioses) of your collection. By default, the program will search them in the "Roms" folder located in the resources folder, but you can change it by browsing your file system.
You can change this and other configurations in the "General options" section
![image](https://user-images.githubusercontent.com/57037540/167163743-79e2f43b-1768-48ec-b872-3dd370688c8e.png)

For Desktop ports (Windows and Linux), you can choose to use an external jzIntv instead of the embedded one. It must be located in the same path of the application, and executable must be called "jzintv.exe" for Windows (case insensitive) and "jzintv" for Linux (case sensitive)

You can also configure single-game Options:

![image](https://user-images.githubusercontent.com/57037540/140432073-f77d5a37-4b2e-493e-a055-d36330cad0ee.png)

If a game does not exist in the internal database, you can add it

![image](https://user-images.githubusercontent.com/57037540/140432423-a89759cd-8fac-45f3-9d41-c747803bc7e2.png)

After saving your data, you'll have your game into database

![image](https://user-images.githubusercontent.com/57037540/140432979-d19eadeb-18f6-4fce-ba6d-31e34014179f.png)

![image](https://user-images.githubusercontent.com/57037540/140433366-6522c3c0-9052-486b-8880-269b4d7797d3.png)

In order to play, on Android devices by default you'll have all the needed buttons on the screen

![image](https://user-images.githubusercontent.com/57037540/140434457-d56ea511-f4a7-48ac-8042-71b0b9815668.png)

And if needed, there's a realistic ECS Keyboard too

![image](https://user-images.githubusercontent.com/57037540/140434559-3776fad6-e428-409f-a9d9-4178b3d2f38d.png)


JzIntvImGui allows to change property of every control (size, position, transparency ecc..), and to save current configuration as default, or to save it only for the actual game

![image](https://user-images.githubusercontent.com/57037540/140434794-9205a5d5-d29e-48d9-aacd-62bf46430879.png)

The left section of the configuration bar contains these buttons:
  - Select previous control
  - Select next control
  - Reset selected control to default values
  - Hide/show selected control
  - Increase transparency
  - Decrease transparency
  - Save actual configuration as default
  - Save actual configuration just for selected game
  - X-Flip controls

The right section of the configuration bar contains a disc-style configuration switches:
  - Decrease width of selected control
  - Move up selected control
  - Increase width of selected control
  - Move left selected control
  - Move right selected control
  - Decrease height of selected control
  - Move down selected control
  - Increase height of selected control

-----------------
# Technical notes

### Resources Folder

Included in the package there's the "resources" folder, containing all the images and configuration files used (and usable) by the program.
It is composed by these subfolders:

- #### Configs 
Keyboard hack files and palette files to pass as parameters to jzIntv.
- #### Fonts
Ttf and otf fonts file you can use in the interface.
- #### Images
Images used by the interface. All image formats are supported.
- #### Images/Screenshots 
Screenshots of the games, shown in interface. When you get a screenshot during jzIntv emulation (by default by F11 key), it will be moved in this folder.
- #### Images/Boxes
Box images of the games, shown in interface.
- #### Images/Interface
Support images for the interface.
- #### Images/Controls
Images used in Android as buttons, in order to play with touch screen of the device.

Feel free to replace/add your custom files in any of those folders. :blush:
With the exception of Android, this "resources" folder must be located in the same folder of executable file. In Android devices instead, this folder will be written inside the folder "jzIntvImGui", automatically created in the root of the internal SD of the device once app is executed for the first time.

-----------------
# Hackish behaviours for Android

JzIntvImGui offers by default all the configuration you may need to play with your games on Android. You have all the controller numbered buttons, clear and enter buttons, side buttons, for both left and right controllers. The program also provides useful buttons to pause, reset and quit emulation, buttons for change player, get a screenshot and show Ecs keyboard.
All configuration options are read and wrote from/to the configuration file, called "jzIntvImGui.ini". Every parameter is managed directly in the interface, so you don't need to edit it manually. 
But, if you wish, you can override standard behaviour of every button, or you can also add custom buttons producing jzIntv events not 'included' by default.
For example, you can add a button generating the "AVI" event, in order to record a VIDEO of your game.
Or, you can decide that in a particular game, side buttons would behave as volume up and volume down.

Well, actually this can be done only by manual editing the configuration file. This file structured with the typical key-value pair management, and it is composed in three different sections:

- General section, starting with string "[General]"
- Controls section, starting with string "[Controls]". This is available only in Android.
  This section is composed by several sub-sections, one for every single control having at least one property not standard.
- Games section, starting with string "[Games]". 
  This section is composed by several sub-sections, one for every single game.

Example:

     [General]
     option1=value1
     option2=value2

     [Controls]
     [-- Named Control 1 -- ]
     option1=value1
     option2=value2

     [-- Named Control 2 -- ]
     option1=value1
     option3=value3

     [Games]
     [-- CRC32 GAME 1 -- ]
     option1=value1
     option2=value2

     [-- CRC32 GAME 2 -- ]
     option3=value3
     option2=value2

You can open ini file to see all standard options.

These are the override options instead
- ##### control_file_name_released
defines the name of the image to use for a control when it's not pressed
- ##### control_file_name_pressed
defines the name of the image to use for a control when it's pressed
- ##### control_override_event
defines the name of the jzIntv event to send when pressed

If you want to override / create new behaviours for all games, you have to edit Controls section.
For example, you want to use your own custom image for a control. You have to put your image in the Images/Controls folder, then in the Controls section you can add:

     [Controls]
     [PD0L_KP1__L]
     control_file_name_released = indy-style.png

Here you are:

![image](https://user-images.githubusercontent.com/57037540/140439159-81ec6afe-dee4-4bcc-813f-ccbd005ee876.png)

If you want to override / create new behaviours for a single specific game, you have to edit file in the specific game section which is a subsection of "Games"
For example, for a particular game you want to override the KP1 event in order to get a screenshot.<br/>
In this case, you'll have to change a little the name of the property:

     [Games]
     [0x604611C0]
     ....
     control_override_event_PD0L_KP1__L = SHOT

Here you are:

![image](https://user-images.githubusercontent.com/57037540/140439733-e52e18a8-752b-4958-9fb3-9802ded8d086.png)

The 'L' stands for left layout, and if you flip the screen, Indy vanishes

![image](https://user-images.githubusercontent.com/57037540/140439998-38777cc0-ad55-47ff-bb63-032158208030.png)

If you want to have the same behaviour also for the other layout, repeat with 'R' :smiley:

-------------------------
# Execution notes

-------------------
#### Windows
Provided "bin" folder contains all the needed files. The "Microsoft Visual C++ Redistributable per Visual Studio x86" package must be installed (https://docs.microsoft.com/it-it/cpp/windows/latest-supported-vc-redist?view=msvc-170)

-------------------
#### Linux
jzIntvImGui needs libSDL2 to be executed. In my Debian GNU/Linux distro, I need to install them (as superUser) with commands:

     sudo apt-get --assume-yes update 
     sudo apt-get --assume-yes install libsdl2-image-2.0-0

-------------------
#### Android
Just install the apk file in "bin" folder 

-------------------
#### Nintendo Switch
[See here](https://github.com/jenergy/jzIntvImGui/tree/Nintendo_Switch#execution-notes)

-------------------------
# Development notes
jzIntvImGui is a CMake project. For both Windows and Linux I use jetbrains CLion (https://www.jetbrains.com/clion/) to manage it

-------------------
#### Windows
- Install MINIMAL Mingw (https://sourceforge.net/projects/mingw/). Select and mark for installation mingw32-base and mingw32-gcc-g++. Then, by clicking on "All Packages" be sure that mingw32-make is selected. Then click on "Apply Changes" on "Installation" menu item.
- Install/Download CLion and open project by selecting "jzIntvImGui" folder
- To check that all is ok, when project is open click on File --> Settings --> Build, Execution, Deployment --> Toolchains and in Toolset be sure that your Mingw installation in selected
-------------------
#### Linux
- Install packages
     ```
     sudo apt-get --assume-yes update
     sudo apt-get --assume-yes install libsdl2-image-dev libglfw3-dev build-essential
     ```

- Install/Download CLion and open project by selecting "jzIntvImGui" folder
-------------------
####                          
Once project is open in Clion (Both Windows and Linux):
- Right click on app/src/main/cpp/CmakeLists.txt and choose "Load CMake project"
- Force build directory to app/bin (File --> Settings --> Build, Execution, Deployment --> Cmake --> Build Directory)
- Choose your build type (File --> Settings --> Build, Execution, Deployment --> Cmake --> Build type)
- Build in "Build" menu, or press the green "Play" button in the toolbar
- Recreate cache if requested (Tools -> CMake -> Reset Cache and Reload Project)

-------------------
#### Android (Apk created on Windows)
Project should be loaded directly by Android Studio. I personally use Intellij ULTIMATE (https://www.jetbrains.com/idea/) since it's a little smaller.
Before opening project, if you don't have Android sdk/ndk installed:
- With Android Studio you will be automatically prompted to install Android SDK components and SDK Manager
- With Intellij ULTIMATE, you can follow these three steps:
  - Manual create SDK folder (example: c:\androidSdk)
  - Open Intellij ULTIMATE.
  - Choose new project --> Android. You'll be asked to configure/install Android SDK into your folder (ex. C:\androidSdk). This step is a quick shortcut that allows you to use SDK Manager from intellij, you don't really need to
 create a new project, so when SDK Manager is installed you can click on cancel.

Now (Both Intellij ULTIMATE and Android Studio):
- Go to Tools --> Android --> SDK Manager
  - On SDK Platforms click "show package details" and install:
    -  Android 11.0 (R) SDK Platform 30.
    -  Accept all licenses.
  - On SDK Tools click "show package details" and install: 
    -  Google Usb driver
    -  Android SDK Platform-tools
    -  Android SDK Build-Tools 31 ->30.0.2
    -  ndkVersion "21.1.6352462"
    -  Cmake 3.10.2.4988404
    -  Android Emulator(if you need it, I don't)
    -  Accept all licenses. 
  - If requested by IDE, download pre-build shared indexes.

- Open project and wait until ALL is loaded (the first time will take SOME minutes)
- To check if all is ok:
  - File --> Project Structure:
    - Project:
      -  Project SDK: jbr-11 JetBrains Runtime version 11.0.11
      -  Project language level: SDK default (11)
  - File --> Settings --> Build, Execution, Deployment --> Android --> Android Project Structure
      - Project:
         - Android Gradle Plugin Version: 4.1.1
         - Gradle Version: 6.5
      - Modules:
        - Properties:
          - Compile Sdk Version 29
          - Build Tools Version 30.0.2
        - Default Config:
          - Target SDK Version 29
          - Min SDK Version 22
  - File --> Settings --> Build, Execution, Deployment --> Build Tools --> Gradle
      - Build and run using: Gradle
      - Run Tests using: Gradle
      - Use Gradle from: 'gradle-wrapper.properties' file
      - Gradle JVM: jbr-11 JetBrains Runtime version 11.0.11
									        
- To switch between debug/release --> Build->Select Build Variant...
									        
Once all is OK, AT YOUR OWN RISK you can try to update versions of plugins, ndk, sdk, cmake etc... 
...but I had a bad time on it :neutral_face:
 
-------------------
#### Nintendo Switch
[See here](https://github.com/jenergy/jzIntvImGui/tree/Nintendo_Switch#development-notes)

-------------------
# Next Steps (hope soon)
- Add Overlays support
- On Windows, use ImguiFileDialog to select folders/files instead of Microsoft dialogs

-------------------
# Next Steps (somewhere, maybe, in the future)
#### Nintendo Switch
 - Fix and merge [Nintendo Switch branch](https://github.com/jenergy/jzIntvImGui/tree/Nintendo_Switch) in main branch
#### All
 - Capture jzIntv output
 
-------------------
# Known issues

Dear ImGui actually does not support word wrap in multiline text fields. So I implemented my version (https://github.com/ocornut/imgui/issues/3237#issuecomment-835876480) 
<br/> But if you have font size too big, the computation in landscape mode is VERY slow. I don't think I will fix it, I hope that one day word wrap will be implemented in Dear Imgui :roll_eyes:

-------------------
# Thanks to:

###### Joe Zbiciak and his jzIntv http://spatula-city.org/~im14u2c/intv/
###### Dear ImGui https://github.com/ocornut/imgui
###### CLion project https://github.com/joelcancela/ImguiDemoCLion
###### Switch imgui glfw https://github.com/MstrVLT/switch_imgui_glfw
###### Android imgui https://github.com/sfalexrog/Imgui_Android
###### Lazyfoo native example https://lazyfoo.net/tutorials/SDL/52_hello_mobile/index.php
###### LibSDl2_Image with CMake https://trenki2.github.io/blog/2017/07/04/using-sdl2-image-with-cmake/
###### FileDialog c++ dll https://docs.microsoft.com/it-it/samples/microsoft/windows-classic-samples/open-dialog-box-sample/
###### FileDialog c++ dll https://www.daniweb.com/programming/software-development/threads/446920/setting-a-hook-for-getopenfilename
###### Nativefiledialog-extended https://github.com/btzy/nativefiledialog-extended
###### ImguiFileDialog https://github.com/aiekick/ImGuiFileDialog
###### Emanuele Zangara for creation of terrific configurations buttons and disc button too emanuele.zangara@yahoo.it
###### Zendocon for support, suggestions and tests https://atariage.com/forums/profile/31886-zendocon/
###### larryvgs for support, suggestions and tests https://atariage.com/forums/profile/79111-larryvgs/

-------------------
### ChangeLog:

###### 27/08/2020 1.0.0 - First release: compilation with jzIntv sources jzintv-20200712
###### 24/03/2021 2.0.0 - Improved interface
###### 29/05/2021 2.1.0 - Options screen
###### 05/10/2021 2.2.0 - First android release
###### 05/11/2021 2.2.6 - Stable version (Thanks Zendocon for your tests in intermediate releases)
###### 20/11/2021 2.3.2 - Android usb management + jzIntv fix crash on launching a non JLP game after a JLP game
###### 06/05/2022 2.3.7 - Bugfix for Pause button not working<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;- Added external jzIntv management<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;- Decreased minSDk Android version to 22, for Amazon Fire Stick devices<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;- Added icon to Windows executable<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;(Thanks larryvgs for motivating me to do everything, and for testing :blush:) 
<br/><br/>
Have fun! <br/>
###### Daniele Moglia (jenergy@tiscali.it)
 









