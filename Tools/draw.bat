@echo off
..\Tools\Graphviz\bin\dot.exe -Tpng %1 >%1.png 2>%1.log && start %1.png
