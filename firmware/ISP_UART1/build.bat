set base=..\..\N76E003_BSP

if not exist .\Objects\ mkdir .\Objects\
cd .\Objects\
sdcc -c ..\isp_uart1.c -I ..\. -I %base%\Library\Device\Include -I %base%\Library\StdDriver\inc -D __SDCC__
sdcc ..\main.c --code-size 2048 --iram-size 256 --xram-size 768 isp_uart1.rel -I ..\. -I %base%\Library\Device\Include -I %base%\Library\StdDriver\inc -D __SDCC__
sdobjcopy -Iihex -Obinary main.ihx main.bin