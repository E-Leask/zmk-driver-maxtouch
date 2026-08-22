# ZMK Microchip maXTouch Driver Module (`zmk-driver-maxtouch`)

This repository is an isolated **ZMK Driver Module** for Microchip maXTouch touch controllers / trackpads (such as mXT1066T2, mXT640, mXT144, etc.).

## Usage

### 1. Include the Module in Your `zmk-config`

Add this module to your `config/west.yml` in your personal ZMK config repository:

```yaml
manifest:
  remotes:
    - name: zmkfirmware
      url-base: https://github.com/zmkfirmware
    - name: your-remote-name
      url-base: https://github.com/<your-username>
  projects:
    - name: zmk
      remote: zmkfirmware
      revision: main
      import: app/west.yml
    - name: zmk-driver-maxtouch
      remote: your-remote-name
      revision: main
  self:
    path: config
```

### 2. Devicetree Node Configuration

In your shield/board overlay file (e.g. `your_shield.overlay`), declare the maXTouch device under your I2C bus:

```dts
#include <dt-bindings/zmk/input_transform.h>

&i2c0 {
    status = "okay";

    trackpad: maxtouch@4a {
        compatible = "microchip,maxtouch";
        reg = <0x4a>;
        chg-gpios = <&gpio0 6 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>;
        sensor-width = <156>;
        sensor-height = <91>;
        swap-xy;
        repeat-each-cycle;
    };
};

/ {
    trackpad_listener {
        compatible = "zmk,input-listener";
        device = <&trackpad>;
    };
};
```

### 3. Kconfig

Enable trackpad / pointing device support in your `your_shield.conf` or `prj.conf`:

```kconfig
CONFIG_INPUT=y
CONFIG_ZMK_POINTING=y
CONFIG_INPUT_MICROCHIP_MAXTOUCH=y
```
