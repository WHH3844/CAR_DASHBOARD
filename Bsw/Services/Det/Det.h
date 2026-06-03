#ifndef DET_H
#define DET_H

#include <stdint.h>

void Det_Init(void);
void Det_ReportError(uint16_t module_id, uint8_t instance_id, uint8_t api_id, uint8_t error_id);

#endif /* DET_H */
