# Modular MIDI-Controller for GrandMA2 Software
*** This project only allows you to controll the grandMA2 Software via a hardware controller. You sitll need to buy a henuine grandMA Interface to unlock the reqired amount of universes. Please also be aware, that this is just a side-project of mine and is not designed for production use. Use the real stuff for that.***

The diyMA is an open source MIDI controller, strongly mimicking the onPC command and fader wing layout. It communicates with the onPC Software via MIDI. This limits the resolution of the faders to 7-Bit (0-127). In my case this is still enough.
I've designed three modules to construct your own modular controller

## The Modules
### Module I : Fader-Module
The fader module contains:
* 5 Executor-Faders (+3 Buttons each)
* 5 Button Executor
* Controller-Header (to connect to Controller-Module)
* 2 PCBs (1x Component-PCB, 1x Panel-PCB)

### Module 3 : Controller-Module
The fader module contains:
* MIDI-In (Passed through for daisy chaining)
* MIDI-Out
* Atmega8-16P @ 16MHz
* 1 ICSP Header (programming)
* +5V/+12V Power Supply

### Module 3 : Fader-Module
The fader module contains:
* 1 Master-Fader
* 69 Buttons
* MIDI-In (Passed through for daisy chaining)
* MIDI-Out
* +5V/+12V Power Supply
* Atmega8-16P @ 16MHz
* 1 Expansion Header (e.g.: more Executors)
* 1 ICSP Header (programming)
* 2 PCBs (1x Component-PCB, 1x Panel-PCB)

## Special-Parts
* https://de.aliexpress.com/item/1005003752250659.html?spm=a2g0o.order_list.order_list_main.105.3a725c5fgOUOYA&gatewayAdapt=glo2deu
* https://de.aliexpress.com/item/1005003201365793.html?spm=a2g0o.order_list.order_list_main.336.3a725c5fgOUOYA&gatewayAdapt=glo2deu
* https://de.aliexpress.com/item/32913352658.html?spm=a2g0o.order_list.order_list_main.321.3a725c5fgOUOYA&gatewayAdapt=glo2deu