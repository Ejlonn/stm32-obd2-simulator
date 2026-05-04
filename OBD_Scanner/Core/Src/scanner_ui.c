// scanner_ui.c - OBD Scanner — OLED UI Yöneticisi Implementasyonu (Paket 5)

#include "scanner_ui.h"
#include "scanner_comm.h"
#include "obd_pid_decode.h"
#include "ssd1306.h"
#include "project_config.h"

#include "FreeRTOS.h"
#include "task.h"
#include <string.h>
#include <stdio.h>

static I2C_HandleTypeDef *s_hi2c    = NULL;
static bool               s_oled_ok = false;

static ScannerCmd_t        s_mode      = SCANNER_CMD_NONE;
static bool                s_has_data  = false;
static ScannerComm_Status_t s_last_err = SCOMM_STATUS_NONE;

static OBD_LiveData_t      s_live;
static OBD_DtcList_t       s_dtc;
static OBD_Vin_t           s_vin;
static bool                s_clear_ok = false;

static char s_line[22];

static void draw_header(const char *title)
{
    SSD1306_Clear();
    SSD1306_SetCursor(0, 0);
    SSD1306_WriteString(title, SSD1306_WHITE);
    SSD1306_DrawHLine(0, 10, 128, SSD1306_WHITE);
}

static void draw_menu(void)
{
    draw_header("=== OBD Scanner ===");
    SSD1306_SetCursor(0, 14);
    SSD1306_WriteString("K1: Live Data", SSD1306_WHITE);
    SSD1306_SetCursor(0, 26);
    SSD1306_WriteString("K2: Read DTC", SSD1306_WHITE);
    SSD1306_SetCursor(0, 38);
    SSD1306_WriteString("K3: Clear DTC", SSD1306_WHITE);
    SSD1306_SetCursor(0, 50);
    SSD1306_WriteString("K4: Read VIN", SSD1306_WHITE);
    SSD1306_UpdateScreen();
}

static void draw_requesting(void)
{
    const char *name = "...";
    switch (s_mode) {
        case SCANNER_CMD_LIVE_DATA: name = "Live Data";  break;
        case SCANNER_CMD_READ_DTC:  name = "Read DTC";   break;
        case SCANNER_CMD_CLEAR_DTC: name = "Clear DTC";  break;
        case SCANNER_CMD_READ_VIN:  name = "Read VIN";   break;
        default: break;
    }
    draw_header("Requesting...");
    SSD1306_SetCursor(0, 24);
    SSD1306_WriteString(name, SSD1306_WHITE);
    SSD1306_SetCursor(0, 40);
    SSD1306_WriteString("Please wait...", SSD1306_WHITE);
    SSD1306_UpdateScreen();
}

static void draw_dashboard(void)
{
    draw_header("Live Data");

    snprintf(s_line, sizeof(s_line), "RPM: %u", s_live.rpm);
    SSD1306_SetCursor(0, 14);
    SSD1306_WriteString(s_line, SSD1306_WHITE);

    SSD1306_SetCursor(90, 14);
    SSD1306_WriteString(s_live.mil_on ? "MIL:ON" : "MIL:--", SSD1306_WHITE);

    snprintf(s_line, sizeof(s_line), "Speed: %u km/h", s_live.speed_kmh);
    SSD1306_SetCursor(0, 26);
    SSD1306_WriteString(s_line, SSD1306_WHITE);

    snprintf(s_line, sizeof(s_line), "Coolant: %d C", s_live.coolant_temp);
    SSD1306_SetCursor(0, 38);
    SSD1306_WriteString(s_line, SSD1306_WHITE);

    snprintf(s_line, sizeof(s_line), "Oil: %d C", s_live.oil_temp);
    SSD1306_SetCursor(0, 50);
    SSD1306_WriteString(s_line, SSD1306_WHITE);

    SSD1306_UpdateScreen();
}

static void draw_dtc_list(void)
{
    snprintf(s_line, sizeof(s_line), "DTC List (%u)", s_dtc.count);
    draw_header(s_line);

    if (s_dtc.count == 0) {
        SSD1306_SetCursor(0, 28);
        SSD1306_WriteString("No DTCs found", SSD1306_WHITE);
    } else {
        uint8_t max_show = (s_dtc.count > 4) ? 4 : s_dtc.count;
        for (uint8_t i = 0; i < max_show; i++) {
            snprintf(s_line, sizeof(s_line), "%u. %s", i + 1, s_dtc.strings[i]);
            SSD1306_SetCursor(0, 14 + i * 12);
            SSD1306_WriteString(s_line, SSD1306_WHITE);
        }
    }
    SSD1306_UpdateScreen();
}

static void draw_vin(void)
{
    draw_header("Vehicle Info");
    SSD1306_SetCursor(0, 20);
    SSD1306_WriteString("VIN:", SSD1306_WHITE);
    SSD1306_SetCursor(0, 34);
    SSD1306_WriteString(s_vin.vin, SSD1306_WHITE);
    SSD1306_UpdateScreen();
}

static void draw_clear_result(void)
{
    draw_header("Clear DTC");
    SSD1306_SetCursor(0, 28);
    SSD1306_WriteString(s_clear_ok ? "DTC Cleared!" : "Clear FAILED",
                        SSD1306_WHITE);
    SSD1306_UpdateScreen();
}

static void draw_error(void)
{
    draw_header("ERROR");
    SSD1306_SetCursor(0, 24);
    SSD1306_WriteString(
        (s_last_err == SCOMM_STATUS_TIMEOUT) ? "NO DATA" : "CAN ERROR",
        SSD1306_WHITE);
    SSD1306_SetCursor(0, 40);
    SSD1306_WriteString("Check connection", SSD1306_WHITE);
    SSD1306_UpdateScreen();
}

static void show_splash(void)
{
    SSD1306_Clear();
    SSD1306_SetCursor(8, 10);
    SSD1306_WriteString("IntelliOBD", SSD1306_WHITE);
    SSD1306_SetCursor(20, 28);
    SSD1306_WriteString("Scanner", SSD1306_WHITE);
    SSD1306_SetCursor(16, 48);
    SSD1306_WriteString("Starting...", SSD1306_WHITE);
    SSD1306_UpdateScreen();
    vTaskDelay(pdMS_TO_TICKS(2000));
}

void ScannerUI_Init(I2C_HandleTypeDef *hi2c)
{
    s_hi2c = hi2c;
    memset(&s_live, 0, sizeof(s_live));
    memset(&s_dtc,  0, sizeof(s_dtc));
    memset(&s_vin,  0, sizeof(s_vin));
}

void ScannerUI_Task(void *argument)
{
    (void)argument;

    TickType_t last_wake = xTaskGetTickCount();

    DEBUG_LOG(DEBUG_LEVEL_INFO, "ScannerUI: Task started\r\n");

    if (s_hi2c != NULL) {
        s_oled_ok = SSD1306_Init(s_hi2c);
        if (s_oled_ok) {
            DEBUG_LOG(DEBUG_LEVEL_INFO, "ScannerUI: OLED init OK\r\n");
            show_splash();
            draw_menu();
        } else {
            DEBUG_LOG(DEBUG_LEVEL_WARN, "ScannerUI: OLED not found\r\n");
        }
    }

    for (;;)
    {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(200));

        if (!s_oled_ok) {
            continue;
        }

        const ScannerComm_Result_t *r = ScannerComm_GetResult();

        if (r->cmd != s_mode) {
            s_mode     = r->cmd;
            s_has_data = false;
            s_last_err = SCOMM_STATUS_NONE;
        }

        if (r->ready && s_mode != SCANNER_CMD_NONE)
        {
            if (r->status == SCOMM_STATUS_OK) {
                switch (r->cmd) {
                    case SCANNER_CMD_LIVE_DATA:
                        OBD_DecodeLiveData(r, &s_live);
                        break;
                    case SCANNER_CMD_READ_DTC:
                        OBD_DecodeDtcList(&r->single, &s_dtc);
                        break;
                    case SCANNER_CMD_CLEAR_DTC:
                        s_clear_ok = OBD_DecodeClearDtc(&r->single);
                        break;
                    case SCANNER_CMD_READ_VIN:
                        OBD_DecodeVin(&r->single, &s_vin);
                        break;
                    default:
                        break;
                }
                s_has_data = true;
            } else {
                s_last_err = r->status;
                
                if (s_mode != SCANNER_CMD_LIVE_DATA || !s_has_data) {
                    s_has_data = false;
                }
            }
        }

        if (s_mode == SCANNER_CMD_NONE) {
            draw_menu();
        } else if (!s_has_data && s_last_err != SCOMM_STATUS_NONE) {
            draw_error();
        } else if (!s_has_data) {
            draw_requesting();
        } else {
            switch (s_mode) {
                case SCANNER_CMD_LIVE_DATA: draw_dashboard();    break;
                case SCANNER_CMD_READ_DTC:  draw_dtc_list();     break;
                case SCANNER_CMD_CLEAR_DTC: draw_clear_result(); break;
                case SCANNER_CMD_READ_VIN:  draw_vin();          break;
                default:                    draw_menu();         break;
            }
        }
    }
}
