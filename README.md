# Audio Plugin (w/ JUCE Framework)
## Learn Modern C++ by Building an Audio Plugin (w/ JUCE Framework) - Full Course

Tutorial from `freeCodeCamp.org`` https://www.youtube.com/watch?v=i_Iq4_Kd7Rc  
There is another video from developer `Matkat Music` https://www.youtube.com/watch?v=H1IvfOfBsVQ  

### _Ubuntu_

To install all **JUCE** examples and extras, need some dependencies. For just the **JUCE**, just run cmake without any parameters.  
1. If you want the examples and extras:  

Some JUCE example needs `ladspa`, which needs `libsndfile`.  
Install https://github.com/libsndfile/libsndfile , just follow instructions, I used these config in VSCode
```json
    "cmake.configureArgs": [
        "-DBUILD_TESTING=OFF",
        "-DENABLE_PACKAGE_CONFIG=ON",
        "-DINSTALL_PKGCONFIG_MODULE=ON",
        "-DBUILD_SHARED_LIBS=ON"
    ],
    "cmake.installPrefix": "/usr/local",
```
`ladspa` is available as apt package 
```sh
sudo apt install ladspa-sdk
```
Or install from source https://www.ladspa.org/ladspa_sdk/overview.html . Download the `.tgz` file, extract, go to `src/` folder, edit the `Makefile` to change the `INSTALL_` location. I set them to `/usr/local` instead of defaul `/usr`.
```make
INSTALL_PLUGINS_DIR	=	/usr/local/lib/ladspa/
INSTALL_INCLUDE_DIR	=	/usr/local/include/
INSTALL_BINARY_DIR	=	/usr/local/bin/
```
Then run `sudo make -j10 install`.

Finally, download **JUCE** https://github.com/juce-framework/JUCE, 
build and install **JUCE** with these config in VSCode.
```json
    "cmake.configureArgs": [
        "-DJUCE_BUILD_EXAMPLES=ON",
        "-DJUCE_BUILD_EXTRAS=ON",
    ],
    "cmake.installPrefix": "/usr/local",
```
2. If you build **JUCE** using `cmake` and with `extras`, go to `$JUCE/build_dir/extras/Projucer/Projucer_artefacts/Release` and run `Projucer`
3. If not, go to `$JUCE/extras/Projucer/Builds/LinuxMakefile` and run `make -j10`. Once it is build, go to `$JUCE/extras/Projucer/Builds/LinuxMakefile/build` and run `Projucer`
4. Open `File`, then `Global Paths...`, set the paths to where you extracted/cloned **JUCE**.
5. COnfigure the project in `Projucer` to select plugins `VST3` and `Standalone`.

Follow the rest of the tutorial.

To build, go to `$Project/Builds/LinuxMakefile` and run `LDFLAGS=-march=native CONFIG=Release make -j10`.  
To run, go to `$Project/Builds/LinuxMakefile/build` and run `./$Project`.

To configure your IDE, check `JUCE_CPPFLAGS` from the `Makefile`.