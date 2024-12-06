cd .\Objects\
nulink_8051ot -e CFG0
nulink_8051ot -w CFG0 0x7F
nulink_8051ot -v CFG0 0x7F
nulink_8051ot -e CFG1
nulink_8051ot -w CFG1 0xFD
nulink_8051ot -v CFG1 0xFD
nulink_8051ot -e LDROM
nulink_8051ot -w LDROM main.bin
nulink_8051ot -v LDROM main.bin