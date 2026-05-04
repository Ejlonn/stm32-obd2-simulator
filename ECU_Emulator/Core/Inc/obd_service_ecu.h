// obd_service_ecu.h - ECU Emulator — OBD-II Service Handler (Responder)

#ifndef OBD_SERVICE_ECU_H
#define OBD_SERVICE_ECU_H

#ifdef __cplusplus
extern "C" {
#endif

#include "obd_types.h"

// OBD service dispatcher — gelen isteği ilgili handler'a yönlendirir
bool OBD_ECU_HandleRequest(const OBD_Message_t *request, OBD_Message_t *response);

#ifdef __cplusplus
}
#endif

#endif 
