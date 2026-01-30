input_file = "lighting_model.tflite"

# -------- HEADER FILE --------
with open(input_file, "rb") as f:
    data = f.read()

hex_data = ", ".join(f"0x{b:02x}" for b in data)

# lighting_model.h
header_h = f"""#ifndef LIGHTING_MODEL_H
#define LIGHTING_MODEL_H

#include <stdint.h>

extern const unsigned char lighting_model_tflite[];
extern const unsigned int lighting_model_tflite_len;

#endif
"""

with open("lighting_model.h", "w") as f:
    f.write(header_h)

# -------- SOURCE FILE --------
# lighting_model.cpp
header_cpp = f"""#include "lighting_model.h"
#include <Arduino.h>

const unsigned char lighting_model_tflite[] PROGMEM = {{
{hex_data}
}};

const unsigned int lighting_model_tflite_len = {len(data)};
"""

with open("lighting_model.cpp", "w") as f:
    f.write(header_cpp)

print("lighting_model.h and lighting_model.cpp created successfully!")
