// mcp2515.c - MCP2515 CAN Controller — Driver Implementation

#include "mcp2515.h"
#include "mcp2515_regs.h"
#include "project_config.h"
#include <string.h>

#define MCP_MUTEX_TIMEOUT_MS    100U

#define MCP_MODE_RETRY_COUNT    10U

#define MCP_RESET_DELAY_MS      2U

// Yazılım busy-wait delay (HAL_Delay'den bağımsız)
static void mcp_sw_delay_ms(uint32_t ms)
{
    volatile uint32_t count = ms * 25000UL;
    while (count--) { __NOP(); }
}

// CS pin LOW (aktif)
static inline void mcp_cs_low(const MCP2515_Handle_t *hmcp)
{
    HAL_GPIO_WritePin(hmcp->cs_port, hmcp->cs_pin, GPIO_PIN_RESET);
}

// CS pin HIGH (pasif)
static inline void mcp_cs_high(const MCP2515_Handle_t *hmcp)
{
    HAL_GPIO_WritePin(hmcp->cs_port, hmcp->cs_pin, GPIO_PIN_SET);
}

// Mutex al — task context'te çağrılmalı
static bool mcp_mutex_take(MCP2515_Handle_t *hmcp)
{
    if (hmcp->spi_mutex == NULL) {
        return true;    
    }
    return (xSemaphoreTake(hmcp->spi_mutex, pdMS_TO_TICKS(MCP_MUTEX_TIMEOUT_MS)) == pdTRUE);
}

// Mutex bırak
static void mcp_mutex_give(MCP2515_Handle_t *hmcp)
{
    if (hmcp->spi_mutex != NULL) {
        xSemaphoreGive(hmcp->spi_mutex);
    }
}

// Tek register oku (mutex almadan — dahili kullanım)
static MCP2515_Status_t mcp_read_reg_nolock(MCP2515_Handle_t *hmcp, uint8_t addr, uint8_t *value)
{
    uint8_t tx_buf[3] = { MCP2515_CMD_READ, addr, 0x00 };
    uint8_t rx_buf[3] = { 0 };

    mcp_cs_low(hmcp);
    HAL_StatusTypeDef hal_status = HAL_SPI_TransmitReceive(hmcp->hspi, tx_buf, rx_buf, 3, 100);
    mcp_cs_high(hmcp);

    if (hal_status != HAL_OK) {
        return MCP2515_ERR_SPI;
    }

    *value = rx_buf[2];
    return MCP2515_OK;
}

// Tek register yaz (mutex almadan — dahili kullanım)
static MCP2515_Status_t mcp_write_reg_nolock(MCP2515_Handle_t *hmcp, uint8_t addr, uint8_t value)
{
    uint8_t tx_buf[3] = { MCP2515_CMD_WRITE, addr, value };

    mcp_cs_low(hmcp);
    HAL_StatusTypeDef hal_status = HAL_SPI_Transmit(hmcp->hspi, tx_buf, 3, 100);
    mcp_cs_high(hmcp);

    return (hal_status == HAL_OK) ? MCP2515_OK : MCP2515_ERR_SPI;
}

// Bit modify (mutex almadan)
static MCP2515_Status_t mcp_bit_modify_nolock(MCP2515_Handle_t *hmcp,
                                                uint8_t addr, uint8_t mask, uint8_t data)
{
    uint8_t tx_buf[4] = { MCP2515_CMD_BIT_MODIFY, addr, mask, data };

    mcp_cs_low(hmcp);
    HAL_StatusTypeDef hal_status = HAL_SPI_Transmit(hmcp->hspi, tx_buf, 4, 100);
    mcp_cs_high(hmcp);

    return (hal_status == HAL_OK) ? MCP2515_OK : MCP2515_ERR_SPI;
}

// Ardışık register oku (mutex almadan)
static MCP2515_Status_t mcp_read_regs_nolock(MCP2515_Handle_t *hmcp,
                                               uint8_t start_addr,
                                               uint8_t *buf, uint8_t len)
{
    uint8_t cmd[2] = { MCP2515_CMD_READ, start_addr };

    mcp_cs_low(hmcp);

    HAL_StatusTypeDef status;
    status = HAL_SPI_Transmit(hmcp->hspi, cmd, 2, 100);
    if (status == HAL_OK) {
        status = HAL_SPI_Receive(hmcp->hspi, buf, len, 100);
    }

    mcp_cs_high(hmcp);
    return (status == HAL_OK) ? MCP2515_OK : MCP2515_ERR_SPI;
}

// Reset komutu gönder (mutex almadan)
static MCP2515_Status_t mcp_reset_nolock(MCP2515_Handle_t *hmcp)
{
    uint8_t cmd = MCP2515_CMD_RESET;

    mcp_cs_low(hmcp);
    HAL_StatusTypeDef status = HAL_SPI_Transmit(hmcp->hspi, &cmd, 1, 100);
    mcp_cs_high(hmcp);

    return (status == HAL_OK) ? MCP2515_OK : MCP2515_ERR_SPI;
}

MCP2515_Status_t MCP2515_Init(MCP2515_Handle_t *hmcp)
{
    if (hmcp == NULL || hmcp->hspi == NULL) {
        return MCP2515_ERR_PARAM;
    }

    hmcp->initialized = false;

    mcp_cs_high(hmcp);

    if (!mcp_mutex_take(hmcp)) {
        return MCP2515_ERR_MUTEX;
    }

    MCP2515_Status_t status = mcp_reset_nolock(hmcp);
    if (status != MCP2515_OK) {
        mcp_mutex_give(hmcp);
        return status;
    }

    mcp_sw_delay_ms(MCP_RESET_DELAY_MS);

    uint8_t canstat;
    status = mcp_read_reg_nolock(hmcp, MCP2515_REG_CANSTAT, &canstat);
    if (status != MCP2515_OK) {
        mcp_mutex_give(hmcp);
        return status;
    }

    if ((canstat & MCP2515_CANSTAT_OPMOD_MASK) != MCP2515_MODE_CONFIG) {
        mcp_mutex_give(hmcp);
        DEBUG_LOG(DEBUG_LEVEL_ERROR, "MCP2515: Config mode verify failed, CANSTAT=0x%02X\r\n", canstat);
        return MCP2515_ERR_MODE;
    }

    hmcp->initialized = true;
    mcp_mutex_give(hmcp);

    DEBUG_LOG(DEBUG_LEVEL_INFO, "MCP2515: Init OK, config mode active\r\n");
    return MCP2515_OK;
}

MCP2515_Status_t MCP2515_ConfigBitTiming(MCP2515_Handle_t *hmcp,
                                          uint8_t cnf1, uint8_t cnf2, uint8_t cnf3)
{
    if (hmcp == NULL || !hmcp->initialized) {
        return MCP2515_ERR_PARAM;
    }

    if (!mcp_mutex_take(hmcp)) {
        return MCP2515_ERR_MUTEX;
    }

    MCP2515_Status_t status;

    status = mcp_write_reg_nolock(hmcp, MCP2515_REG_CNF1, cnf1);
    if (status != MCP2515_OK) goto cleanup;

    status = mcp_write_reg_nolock(hmcp, MCP2515_REG_CNF2, cnf2);
    if (status != MCP2515_OK) goto cleanup;

    status = mcp_write_reg_nolock(hmcp, MCP2515_REG_CNF3, cnf3);
    if (status != MCP2515_OK) goto cleanup;

    uint8_t verify;
    status = mcp_read_reg_nolock(hmcp, MCP2515_REG_CNF1, &verify);
    if (status != MCP2515_OK || verify != cnf1) {
        status = MCP2515_ERR_SPI;
        DEBUG_LOG(DEBUG_LEVEL_ERROR, "MCP2515: CNF1 verify failed (wrote 0x%02X, read 0x%02X)\r\n", cnf1, verify);
        goto cleanup;
    }

    DEBUG_LOG(DEBUG_LEVEL_INFO, "MCP2515: Bit timing set CNF=[0x%02X, 0x%02X, 0x%02X]\r\n",
              cnf1, cnf2, cnf3);

cleanup:
    mcp_mutex_give(hmcp);
    return status;
}

MCP2515_Status_t MCP2515_ConfigFilter(MCP2515_Handle_t *hmcp,
                                       uint16_t mask0, uint16_t mask1,
                                       uint16_t filter0, uint16_t filter1)
{
    if (hmcp == NULL || !hmcp->initialized) {
        return MCP2515_ERR_PARAM;
    }

    if (!mcp_mutex_take(hmcp)) {
        return MCP2515_ERR_MUTEX;
    }

    MCP2515_Status_t status;

    status = mcp_write_reg_nolock(hmcp, MCP2515_REG_RXM0SIDH, (uint8_t)(mask0 >> 3));
    if (status != MCP2515_OK) goto cleanup;
    status = mcp_write_reg_nolock(hmcp, MCP2515_REG_RXM0SIDL, (uint8_t)((mask0 & 0x07) << 5));
    if (status != MCP2515_OK) goto cleanup;

    status = mcp_write_reg_nolock(hmcp, MCP2515_REG_RXM1SIDH, (uint8_t)(mask1 >> 3));
    if (status != MCP2515_OK) goto cleanup;
    status = mcp_write_reg_nolock(hmcp, MCP2515_REG_RXM1SIDL, (uint8_t)((mask1 & 0x07) << 5));
    if (status != MCP2515_OK) goto cleanup;

    status = mcp_write_reg_nolock(hmcp, MCP2515_REG_RXF0SIDH, (uint8_t)(filter0 >> 3));
    if (status != MCP2515_OK) goto cleanup;
    status = mcp_write_reg_nolock(hmcp, MCP2515_REG_RXF0SIDL, (uint8_t)((filter0 & 0x07) << 5));
    if (status != MCP2515_OK) goto cleanup;

    status = mcp_write_reg_nolock(hmcp, MCP2515_REG_RXF1SIDH, (uint8_t)(filter1 >> 3));
    if (status != MCP2515_OK) goto cleanup;
    status = mcp_write_reg_nolock(hmcp, MCP2515_REG_RXF1SIDL, (uint8_t)((filter1 & 0x07) << 5));
    if (status != MCP2515_OK) goto cleanup;

    status = mcp_write_reg_nolock(hmcp, MCP2515_REG_RXB0CTRL,
                                   MCP2515_RXBnCTRL_RXM_STD | MCP2515_RXB0CTRL_BUKT);
    if (status != MCP2515_OK) goto cleanup;

    status = mcp_write_reg_nolock(hmcp, MCP2515_REG_RXB1CTRL,
                                   MCP2515_RXBnCTRL_RXM_ANY);

    DEBUG_LOG(DEBUG_LEVEL_INFO, "MCP2515: Filters set M0=0x%03X M1=0x%03X F0=0x%03X F1=0x%03X\r\n",
              mask0, mask1, filter0, filter1);

cleanup:
    mcp_mutex_give(hmcp);
    return status;
}

MCP2515_Status_t MCP2515_SetMode(MCP2515_Handle_t *hmcp, MCP2515_OpMode_t mode)
{
    if (hmcp == NULL || !hmcp->initialized) {
        return MCP2515_ERR_PARAM;
    }

    if (!mcp_mutex_take(hmcp)) {
        return MCP2515_ERR_MUTEX;
    }

    MCP2515_Status_t status;
    status = mcp_bit_modify_nolock(hmcp, MCP2515_REG_CANCTRL,
                                    MCP2515_MODE_MASK, (uint8_t)mode);
    if (status != MCP2515_OK) {
        mcp_mutex_give(hmcp);
        return status;
    }

    uint8_t canstat;
    for (uint8_t retry = 0; retry < MCP_MODE_RETRY_COUNT; retry++) {
        
        mcp_sw_delay_ms(1);
        status = mcp_read_reg_nolock(hmcp, MCP2515_REG_CANSTAT, &canstat);
        if (status != MCP2515_OK) {
            mcp_mutex_give(hmcp);
            return status;
        }

        if ((canstat & MCP2515_CANSTAT_OPMOD_MASK) == (uint8_t)mode) {
            mcp_mutex_give(hmcp);
            DEBUG_LOG(DEBUG_LEVEL_INFO, "MCP2515: Mode changed to 0x%02X\r\n", mode);
            return MCP2515_OK;
        }
    }

    mcp_mutex_give(hmcp);
    DEBUG_LOG(DEBUG_LEVEL_ERROR, "MCP2515: Mode change to 0x%02X FAILED, CANSTAT=0x%02X\r\n",
              mode, canstat);
    return MCP2515_ERR_MODE;
}

MCP2515_Status_t MCP2515_SendFrame(MCP2515_Handle_t *hmcp, const CAN_Frame_t *frame)
{
    if (hmcp == NULL || frame == NULL || !hmcp->initialized) {
        return MCP2515_ERR_PARAM;
    }

    if (frame->dlc > 8U) {
        return MCP2515_ERR_PARAM;
    }

    if (!mcp_mutex_take(hmcp)) {
        return MCP2515_ERR_MUTEX;
    }

    MCP2515_Status_t status;

    uint8_t ctrl_addr = 0;
    uint8_t sidh_addr = 0;
    uint8_t load_cmd  = 0;
    uint8_t rts_cmd   = 0;
    uint8_t ctrl_val;
    bool    found = false;

    static const struct {
        uint8_t ctrl;
        uint8_t sidh;
        uint8_t load;
        uint8_t rts;
    } tx_bufs[3] = {
        { MCP2515_REG_TXB0CTRL, MCP2515_REG_TXB0SIDH, MCP2515_CMD_LOAD_TX_B0SIDH, MCP2515_CMD_RTS_TX0 },
        { MCP2515_REG_TXB1CTRL, MCP2515_REG_TXB1SIDH, MCP2515_CMD_LOAD_TX_B1SIDH, MCP2515_CMD_RTS_TX1 },
        { MCP2515_REG_TXB2CTRL, MCP2515_REG_TXB2SIDH, MCP2515_CMD_LOAD_TX_B2SIDH, MCP2515_CMD_RTS_TX2 },
    };

    for (uint8_t i = 0; i < 3; i++) {
        status = mcp_read_reg_nolock(hmcp, tx_bufs[i].ctrl, &ctrl_val);
        if (status != MCP2515_OK) {
            mcp_mutex_give(hmcp);
            return status;
        }
        if ((ctrl_val & MCP2515_TXBnCTRL_TXREQ) == 0) {
            ctrl_addr = tx_bufs[i].ctrl;
            sidh_addr = tx_bufs[i].sidh;
            load_cmd  = tx_bufs[i].load;
            rts_cmd   = tx_bufs[i].rts;
            found = true;
            break;
        }
    }

    if (!found) {
        mcp_mutex_give(hmcp);
        return MCP2515_ERR_TX_BUSY;
    }

    uint8_t tx_data[13];    

    if (!frame->is_extended) {
        
        tx_data[0] = (uint8_t)(frame->id >> 3);                
        tx_data[1] = (uint8_t)((frame->id & 0x07U) << 5);     
    } else {
        
        tx_data[0] = (uint8_t)(frame->id >> 21);
        tx_data[1] = (uint8_t)(((frame->id >> 18) & 0x07U) << 5) | 0x08U |
                     (uint8_t)((frame->id >> 16) & 0x03U);
    }
    tx_data[2] = 0x00;  
    tx_data[3] = 0x00;  
    tx_data[4] = frame->dlc & 0x0FU;

    if (frame->is_rtr) {
        tx_data[4] |= 0x40U;   
    }

    memcpy(&tx_data[5], frame->data, frame->dlc);

    uint8_t total_len = 5U + frame->dlc;

    mcp_cs_low(hmcp);
    uint8_t cmd = load_cmd;
    HAL_StatusTypeDef hal_s;
    hal_s = HAL_SPI_Transmit(hmcp->hspi, &cmd, 1, 100);
    if (hal_s == HAL_OK) {
        hal_s = HAL_SPI_Transmit(hmcp->hspi, tx_data, total_len, 100);
    }
    mcp_cs_high(hmcp);

    if (hal_s != HAL_OK) {
        mcp_mutex_give(hmcp);
        return MCP2515_ERR_SPI;
    }

    mcp_cs_low(hmcp);
    hal_s = HAL_SPI_Transmit(hmcp->hspi, &rts_cmd, 1, 100);
    mcp_cs_high(hmcp);

    mcp_mutex_give(hmcp);

    if (hal_s != HAL_OK) {
        return MCP2515_ERR_SPI;
    }

    return MCP2515_OK;
}

MCP2515_Status_t MCP2515_ReadFrame(MCP2515_Handle_t *hmcp, CAN_Frame_t *frame)
{
    if (hmcp == NULL || frame == NULL || !hmcp->initialized) {
        return MCP2515_ERR_PARAM;
    }

    if (!mcp_mutex_take(hmcp)) {
        return MCP2515_ERR_MUTEX;
    }

    MCP2515_Status_t status;
    uint8_t intf;

    status = mcp_read_reg_nolock(hmcp, MCP2515_REG_CANINTF, &intf);
    if (status != MCP2515_OK) {
        mcp_mutex_give(hmcp);
        return status;
    }

    uint8_t read_cmd;
    uint8_t clear_flag;

    if (intf & MCP2515_CANINTF_RX0IF) {
        read_cmd   = MCP2515_CMD_READ_RX_B0SIDH;
        clear_flag = MCP2515_CANINTF_RX0IF;
    } else if (intf & MCP2515_CANINTF_RX1IF) {
        read_cmd   = MCP2515_CMD_READ_RX_B1SIDH;
        clear_flag = MCP2515_CANINTF_RX1IF;
    } else {
        mcp_mutex_give(hmcp);
        return MCP2515_ERR_NO_MSG;
    }

    uint8_t rx_data[13];
    memset(rx_data, 0, sizeof(rx_data));

    mcp_cs_low(hmcp);
    HAL_StatusTypeDef hal_s;
    hal_s = HAL_SPI_Transmit(hmcp->hspi, &read_cmd, 1, 100);
    if (hal_s == HAL_OK) {
        hal_s = HAL_SPI_Receive(hmcp->hspi, rx_data, 13, 100);
    }
    mcp_cs_high(hmcp);

    if (hal_s != HAL_OK) {
        mcp_mutex_give(hmcp);
        return MCP2515_ERR_SPI;
    }

    mcp_bit_modify_nolock(hmcp, MCP2515_REG_CANINTF, clear_flag, 0x00);

    mcp_mutex_give(hmcp);

    frame->is_extended = (rx_data[1] & 0x08U) ? true : false;

    if (!frame->is_extended) {
        frame->id = ((uint32_t)rx_data[0] << 3) | ((uint32_t)(rx_data[1] >> 5) & 0x07U);
    } else {
        frame->id = ((uint32_t)rx_data[0] << 21) |
                    ((uint32_t)(rx_data[1] & 0xE0U) << 13) |
                    ((uint32_t)(rx_data[1] & 0x03U) << 16) |
                    ((uint32_t)rx_data[2] << 8) |
                    ((uint32_t)rx_data[3]);
    }

    frame->dlc = rx_data[4] & 0x0FU;
    frame->is_rtr = (rx_data[4] & 0x40U) ? true : false;

    if (frame->dlc > 8U) {
        frame->dlc = 8U;   
    }

    memcpy(frame->data, &rx_data[5], frame->dlc);

    return MCP2515_OK;
}

MCP2515_Status_t MCP2515_GetInterruptFlags(MCP2515_Handle_t *hmcp, uint8_t *flags)
{
    if (hmcp == NULL || flags == NULL) {
        return MCP2515_ERR_PARAM;
    }

    if (!mcp_mutex_take(hmcp)) {
        return MCP2515_ERR_MUTEX;
    }

    MCP2515_Status_t status = mcp_read_reg_nolock(hmcp, MCP2515_REG_CANINTF, flags);
    mcp_mutex_give(hmcp);
    return status;
}

MCP2515_Status_t MCP2515_ClearInterruptFlags(MCP2515_Handle_t *hmcp, uint8_t flags)
{
    if (hmcp == NULL) {
        return MCP2515_ERR_PARAM;
    }

    if (!mcp_mutex_take(hmcp)) {
        return MCP2515_ERR_MUTEX;
    }

    MCP2515_Status_t status = mcp_bit_modify_nolock(hmcp, MCP2515_REG_CANINTF, flags, 0x00);
    mcp_mutex_give(hmcp);
    return status;
}

MCP2515_Status_t MCP2515_GetErrorFlags(MCP2515_Handle_t *hmcp, uint8_t *eflg)
{
    if (hmcp == NULL || eflg == NULL) {
        return MCP2515_ERR_PARAM;
    }

    if (!mcp_mutex_take(hmcp)) {
        return MCP2515_ERR_MUTEX;
    }

    MCP2515_Status_t status = mcp_read_reg_nolock(hmcp, MCP2515_REG_EFLG, eflg);
    mcp_mutex_give(hmcp);
    return status;
}

MCP2515_Status_t MCP2515_GetCurrentMode(MCP2515_Handle_t *hmcp, MCP2515_OpMode_t *mode)
{
    if (hmcp == NULL || mode == NULL) {
        return MCP2515_ERR_PARAM;
    }

    if (!mcp_mutex_take(hmcp)) {
        return MCP2515_ERR_MUTEX;
    }

    uint8_t canstat;
    MCP2515_Status_t status = mcp_read_reg_nolock(hmcp, MCP2515_REG_CANSTAT, &canstat);
    if (status == MCP2515_OK) {
        *mode = (MCP2515_OpMode_t)(canstat & MCP2515_CANSTAT_OPMOD_MASK);
    }

    mcp_mutex_give(hmcp);
    return status;
}

MCP2515_Status_t MCP2515_EnableInterrupts(MCP2515_Handle_t *hmcp, uint8_t int_mask)
{
    if (hmcp == NULL || !hmcp->initialized) {
        return MCP2515_ERR_PARAM;
    }

    if (!mcp_mutex_take(hmcp)) {
        return MCP2515_ERR_MUTEX;
    }

    MCP2515_Status_t status = mcp_write_reg_nolock(hmcp, MCP2515_REG_CANINTE, int_mask);

    if (status == MCP2515_OK) {
        
        uint8_t verify;
        status = mcp_read_reg_nolock(hmcp, MCP2515_REG_CANINTE, &verify);
        if (status == MCP2515_OK && verify != int_mask) {
            status = MCP2515_ERR_SPI;
            DEBUG_LOG(DEBUG_LEVEL_ERROR, "MCP2515: CANINTE verify failed (wrote 0x%02X, read 0x%02X)\r\n",
                      int_mask, verify);
        }
    }

    mcp_mutex_give(hmcp);

    if (status == MCP2515_OK) {
        DEBUG_LOG(DEBUG_LEVEL_INFO, "MCP2515: Interrupts enabled, CANINTE=0x%02X\r\n", int_mask);
    }

    return status;
}

