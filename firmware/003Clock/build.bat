set base=..\..\N76E003_BSP

if not exist .\Objects\ mkdir .\Objects\
cd .\Objects\
sdcc -c ..\pt6961.c -I ..\. -I %base%\Library\Device\Include -I %base%\Library\StdDriver\inc -D __SDCC__
sdcc -c ..\clock.c -I ..\. -I %base%\Library\Device\Include -I %base%\Library\StdDriver\inc -D __SDCC__
sdcc ..\main.c --code-size 16384 --iram-size 256 --xram-size 768 pt6961.rel clock.rel -I ..\. -I %base%\Library\Device\Include -I %base%\Library\StdDriver\inc -D __SDCC__
sdobjcopy -Iihex -Obinary main.ihx main.bin