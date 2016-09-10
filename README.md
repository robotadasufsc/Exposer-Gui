Exposer-Gui
---
Exposer-Gui is a graphical interface for communication with [Exposer](https://github.com/Williangalvani/Exposer).

# Install

## Ubuntu

1. [download Qt5](https://wiki.qt.io/Install_Qt_5_on_Ubuntu) (Make sure you ticked "QtCharts" on the installation);

2.  Clone repo:

`git clone [repourl.git]`

3.  Build it (change the path if you installed Qt on another path)

```
cd Exposer-gui
mkdir build
cd build
cmake .. -DCMAKE_PREFIX_PATH=/opt/Qt5.7.0/5.7/gcc_64
```