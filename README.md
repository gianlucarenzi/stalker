STm32-to-Amiga-Local-Keyboard adaptER
=====================================

The STALKER adapter board enables connecting USB HID keyboards to your Amiga 1000, Amiga 2000/3000, Amiga 4000/CD32, Amiga 500, Amiga 1200 and Amiga 600 and it is based upon 32-bit STM32 microcontroller.

The STALKER provides built-in USB stack which enables USB HID keyboards support without the need for installing additional AmigaOS software drivers. The whole installation process comes down to simply connecting a STALKER adapter and plug to the keyboard port of your Amiga 1000, 2000 or 4000 externally, or internally in any other Amiga computers.

Additional feature of the adapter is its „Bootloader” function allowing future firmware upgrades.
The STALKER comes with dedicated firmware, but it can be easily installed any firmware version as it's an open source project.

Features
========

* supports for both USB HID, wireless adapters and USB-PS/2 keyboards,
* Amiga 1000, 2000/3000, Amiga 4000/CD32, Amiga 1200, Amiga 600 and Amiga 500 compatibility,
* built-in USB stack with HID Keyboard firmware driver
* "Bootloader" function allowing future firmware upgrades,
* "Status" indicator LED providing information about device's state,
* built around 32-bit STM32 microcontroller utilizing latest technology,
* non-blocking keyboard.

Why so a bad name for a so good product?

As stated in the title, the STALKER is an acronym of:

STm32-to-Amiga-Local-Keyboard adaptER
-------------------------------------

But if you think a little more, it is catching all the keystrokes you do,

so basically it is stalking you! ;-)

# Rendered Images
# [![](hw/AmigaKeyboardAdapters/StandAlone-Adapter/images/StandAlone-Adapter-f.png "Board Front")](#features)
# [![](hw/AmigaKeyboardAdapters/StandAlone-Adapter/images/StandAlone-Adapter-b.png "Board Back")](#features)
# [![](hw/AmigaKeyboardAdapters/StandAlone-Adapter/images/StandAlone-Adapter.png "Board Overall")](#features)

Upgrading with Linux and OpenOCD and STLinkV2 Programmer/Debugger
-----------------------------------------------------------------
Starting from this project folder, there are some shell scripts
that can be used to program the board if you unluckly brick it or simply to upgrade
some features from bootloader and application without using the STM32CubeProgrammer software.

Connect the [STLinkV2 jtag/swd programmer](https://it.aliexpress.com/item/1005005293861493.html?src=google&src=google&albch=shopping&acnt=494-037-6276&slnk=&plac=&mtctp=&albbt=Google_7_shopping&albagn=888888&isSmbAutoCall=false&needSmbHouyi=false&albcp=19215268799&albag=&trgt=&crea=it1005005293861493&netw=x&device=c&albpg=&albpd=it1005005293861493&gad_source=1&gclid=CjwKCAiA75itBhA6EiwAkho9e9Ok9PTFKaCFdsAMpPSbBLCq1OQtwuj0-TzUiPkTw3kzGwD3cOH9dRoCA5QQAvD_BwE&gclsrc=aw.ds&aff_fcid=d493d20850c64605be1501f0d54beda6-1705426166888-01099-UneMJZVf&aff_fsk=UneMJZVf&aff_platform=aaf&sk=UneMJZVf&aff_trace_key=d493d20850c64605be1501f0d54beda6-1705426166888-01099-UneMJZVf&terminal_id=d32dafb4e34148a48d22c2edf9d0f94b&afSmartRedirect=y&gatewayAdapt=glo2ita)
to the STALKER Board in J4 connector (STLink V2 in the silkscreen) using a standard 10 pin IDC female/female [cable 2x5 10P](https://it.aliexpress.com/item/1005003161799870.html?spm=a2g0o.productlist.main.3.4e647234kvSKE1&algo_pvid=74dab80a-ec8c-4818-bb77-7391a24769ef&aem_p4p_detail=202401160933194312512568752080000676956&algo_exp_id=74dab80a-ec8c-4818-bb77-7391a24769ef-1&pdp_npi=4%40dis%21EUR%214.74%211.61%21%21%215.06%211.72%21%402103956b17054263991973081e262b%2112000024427578441%21sea%21IT%210%21AB&curPageLogUid=Utv522cxXgsn&utparam-url=scene%3Asearch%7Cquery_from%3A&search_p4p_id=202401160933194312512568752080000676956_2).
Opening the project folder, you can compile all projects (application and bootloader) running:
```
make clean
make
```
and then launch the corresponding script:

* bootloader.sh (to erase all the flash memory and program the bootloader)

* flash.sh (to program the application, after having erased all needed flash sectors)


Upgrading quick guide
---------------------
First, get your own copy of STM32CubeProgrammer software.
It is free to use, please checkout the STMicroElectronics web site and install it into your PC.
Please have a look at the following web link: https://www.st.com/en/development-tools/stm32cubeprog.html

After launched it will be shown as the following image.

![pic1](https://user-images.githubusercontent.com/22798919/219906340-83e0abf5-7d11-4da2-b882-8896653abf63.png)

Connect the <b>special upgrade cable</b> to the right lower opening of <b>The STALKER</b>, then connect the board with the provided USB cable to the one of the USB ports available in your computer. The red LED will light steady.

Then click on the connect button on the upper right position of the STM32Cube Programmer's window as shown in the picture below:

![pic1-connect](https://user-images.githubusercontent.com/22798919/219906316-8d8b55ae-dc68-4bae-b0f4-8fe2d6cec071.png)

Some information will be shown into the right bottom side of the same window, plus the memory locations will be filled up by the programmed values.

![pic2-connected](https://user-images.githubusercontent.com/22798919/219906386-4c811574-6689-47c4-8744-b4ac94b1f7e0.png)

After, you will need to select the correct <b>FIRMWARE</b> file (<b>.hex file format</b>) clicking into the <b>Open File</b> tab button like in the following example:

![pic3-select-hex](https://user-images.githubusercontent.com/22798919/219906407-b783084b-6df9-425e-9266-d689db07113c.png)

If the file is loaded correctly, the <b>Download button</b> needs to be pressed to start the programming/flashing procedure. It will takes few seconds to be completed.

![pic4-download-flash](https://user-images.githubusercontent.com/22798919/219906426-8261f1b7-b1df-463f-b3c8-6042e0b08809.png)

The firmware is correctly installed into the board, and it can be used as usual.

![pic5-complete](https://user-images.githubusercontent.com/22798919/219906448-3876d894-80b7-4723-b005-8a08f07171c3.png)

Now you can:

* Disconnect the board from USB from your PC
* Remove the special ugrade cable
* Re-install the board into your Amiga Computer's setup.
* Connect a USB Keyboard, and turn on the Amiga Computer.

That's all folks!


Software & Hardware License
---------------------------
Copyright (C) 2018/2019/2022/2023 Gianluca Renzi <gianlucarenzi@eurek.it> <icjtqr@gmail.com>

The hardware of this project is released as free/open hardware under the
GNU GPL v3 License terms. See licence.txt for details.

