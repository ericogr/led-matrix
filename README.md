# Led Matrix (MAX7219)
A MongooseOS ledMatrix based on Arduino library

## Use

```
#include "LedMatrix.h"

using namespace lm;

LedMatrix ledMatrix;
ledMatrix.init();
//ledMatrix.setRotation(true);
ledMatrix.setText("My IOT Device...");
ledMatrix.setTextAlignment(TEXT_ALIGN_LEFT_END);
ledMatrix.startAnimatedText(50);
```

## Config Schema

```
config_schema:
  - ["ledm.number_of_displays", 4]
  - ["ledm.slave_select_pin", 5]

  - ["spi.enable", true]
  - ["spi.miso_gpio", -1]
  - ["spi.mosi_gpio", 23]
  - ["spi.sclk_gpio", 18]
  - ["spi.cs0_gpio", -1]
  - ["spi.cs1_gpio", -1]
  - ["spi.cs2_gpio", -1]
  ```
  * Change spi pins to your device. We don't use miso, cs0, cs1 and cs2

  ## Libs

  ```
  libs:
  - origin: https://github.com/ericogr/led-matrix
  ```
