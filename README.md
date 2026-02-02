# ZMK Inertia

Physics-based mouse cursor movement with smooth acceleration and glide effects for ZMK.

## Installation

Add to your `config/west.yml`:

```yaml
manifest:
  remotes:
    - name: zmkfirmware
      url-base: https://github.com/zmkfirmware
    - name: robertstojs
      url-base: https://github.com/robertstojs
  projects:
    - name: zmk
      remote: zmkfirmware
      revision: main
      import: app/west.yml
    - name: zmk-inertia
      remote: robertstojs
      revision: main
  self:
    path: config
```

## Usage

```c
#include <behaviors/inertia.dtsi>

/ {
    keymap {
        compatible = "zmk,keymap";
        default_layer {
            bindings = <
                &mmi_up    &mmi_down
                &mmi_left  &mmi_right
            >;
        };
    };
};
```

Available behaviors: `&mmi_up`, `&mmi_down`, `&mmi_left`, `&mmi_right`, `&mmi_up_left`, `&mmi_up_right`, `&mmi_down_left`, `&mmi_down_right`

## Configuration

| Property | Default | Description |
|----------|---------|-------------|
| `delay-ms` | 150 | Time after key press before acceleration begins |
| `interval-ms` | 16 | Time between cursor updates. Lower = smoother. |
| `max-speed` | 16 | Maximum pixels moved per update |
| `time-to-max` | 32 | Updates to reach max speed. Higher = slower ramp. |
| `friction` | 24 | How fast cursor stops after release (0-255). Higher = quicker stop. |

Override defaults:

```c
&mmi_up {
    max-speed = <32>;
    friction = <16>;
};
```

## License

MIT
