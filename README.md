

# USB-2-232-KBD
 Converts USB mice and USB keyboards to serial mice and XT-AT-PS/2 keyboards.
  <img align="center" alt="ASSEMBLED_ADAPTER" width="800px" src="https://raw.githubusercontent.com/LimeProgramming/USB-2-232-KBD/dev/images/built.webp"/>

# Development
In progress, not ready for production

## About
This project is an extension of a previous project that you can find here  [previous project that you can find here.](https://github.com/LimeProgramming/USB-serial-mouse-adapter)
My previous project just converted USB mice to Serial mice, this project aims to also convert USB keyboards to XT-AT-PS/2 keyboards. 

## Mouse Features
With this adapter you can convert a modern native USB mouse to a classic serial mouse. 
Want to use an optical mouse on a 386 running Wolfenstein 3D? Well now you can! Want to use a wireless optical mouse on a 486 running Doom? Well you might be able to! (depending on the mouse)

**Supported Protocols:**
- Microsoft Two Button
- Logitech Three Button
- Microsoft Wheel


## Hardware
[<img align="center" alt="Back_PCB" width="380px" src="https://raw.githubusercontent.com/LimeProgramming/USB-2-232-KBD/dev/images/front_nobg.webp"/>](https://raw.githubusercontent.com/LimeProgramming/USB-2-232-KBD/dev/images/front_bg.jpg)[<img align="center" alt="Back_PCB" width="380px" src="https://raw.githubusercontent.com/LimeProgramming/USB-2-232-KBD/dev/images/back_nobg.webp"/>](https://raw.githubusercontent.com/LimeProgramming/USB-2-232-KBD/dev/images/back_bg.jpg)

For the PCB I tried to mimic real keyboard hardware where possible. My old AT keyboard uses 2.2k pull-ups, ferrites, clamp caps and so this PCB does as well for the keyboard interface. Interfacing directly with the GPIO via level shifters uses much higher pullup values, no clamp caps to mitigate signal bounce, no ferrites to mitigate signal noise and importantly, no fuse on the power in line from the host PC. Technically, using a level shifters should work just fine but it is a bit out of spec so early AT and XT machines *may* have issues with level shifters.

I mentioned a fuse, there is a 750ma poly fuse on the PCB as standard. It seems that most AT motherboards have a 1 amp fuse for their keyboard, however that will need to be changed if it pops. While RGB keyboards and mice use more power than the old AT keyboards, there is still more than enough headroom for powering them. Issues can pop up if you're pulling too much power from the keyboard port or back feeding power into the motherboard. For example, my KVM will pull power from the USB port if it is not being powered by it's power adapter, it could potentially pull more than 1 amp, so hopefully the self resetting poly fuse will pop before the 1amp fuse on your motherboard. 

If you want to power the adapter from the keyboard port, put a jumper on pin header J5, label KBDPWR. 
**Important:** If you're powering the adapter using any other methods (eg, built in mini USB, picos micro usb, J4 pin header or a KVM) you must take the jumper off of J5. You risk back feeding a connected computer. 

### Din or PS/2 Port
The PCB can use either a din or a PS/2 port in the same footprint. You can choose which you want to use based on your use case. Recommended use case would be to use the PS/2 port because 6 pin mini din cables are cheap and easy to find.
[<img align="center" alt="Back_PCB" width="380px" src="https://raw.githubusercontent.com/LimeProgramming/USB-2-232-KBD/dev/images/rear_nobg.webp"/>](https://raw.githubusercontent.com/LimeProgramming/USB-2-232-KBD/dev/images/rear_bg.jpg)[<img align="center" alt="Back_PCB" width="380px" src="https://raw.githubusercontent.com/LimeProgramming/USB-2-232-KBD/dev/images/rear_ps2_nobg.webp"/>](https://raw.githubusercontent.com/LimeProgramming/USB-2-232-KBD/dev/images/rear_ps2_bg.jpg)




