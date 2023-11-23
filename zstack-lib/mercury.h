#ifndef MERCURY_H
#define MERCURY_H

#include "zcl_app.h"

#define MERCURY_INVALID_RESPONSE 0xFFFF



typedef void (*request_measure_t)(uint32 serial_num, uint8 cmd);
typedef current_values_t (*read_curr_values_t)(void);
typedef energy_t (*read_energy_t)(void);

typedef struct {
  request_measure_t RequestMeasure;
  read_curr_values_t ReadCurrentValues;
  read_energy_t ReadEnergy;
} zclMercury_t;

#endif //MERCURY_H