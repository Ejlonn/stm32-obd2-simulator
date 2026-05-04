// mcp2515_regs.h - MCP2515 CAN Controller — Register adresleri ve bit tanımları

#ifndef MCP2515_REGS_H
#define MCP2515_REGS_H

#ifdef __cplusplus
extern "C" {
#endif

#define MCP2515_CMD_RESET           0xC0U   
#define MCP2515_CMD_READ            0x03U   
#define MCP2515_CMD_WRITE           0x02U   
#define MCP2515_CMD_READ_STATUS     0xA0U   
#define MCP2515_CMD_RX_STATUS       0xB0U   
#define MCP2515_CMD_BIT_MODIFY      0x05U   
#define MCP2515_CMD_READ_RX_B0SIDH  0x90U   
#define MCP2515_CMD_READ_RX_B0D0    0x92U   
#define MCP2515_CMD_READ_RX_B1SIDH  0x94U   
#define MCP2515_CMD_READ_RX_B1D0    0x96U   
#define MCP2515_CMD_LOAD_TX_B0SIDH  0x40U   
#define MCP2515_CMD_LOAD_TX_B0D0    0x41U   
#define MCP2515_CMD_LOAD_TX_B1SIDH  0x42U   
#define MCP2515_CMD_LOAD_TX_B1D0    0x43U   
#define MCP2515_CMD_LOAD_TX_B2SIDH  0x44U   
#define MCP2515_CMD_LOAD_TX_B2D0    0x45U   
#define MCP2515_CMD_RTS_TX0         0x81U   
#define MCP2515_CMD_RTS_TX1         0x82U   
#define MCP2515_CMD_RTS_TX2         0x84U   

#define MCP2515_REG_CANSTAT         0x0EU   
#define MCP2515_REG_CANCTRL         0x0FU   

#define MCP2515_REG_CNF3            0x28U   
#define MCP2515_REG_CNF2            0x29U   
#define MCP2515_REG_CNF1            0x2AU   
#define MCP2515_REG_CANINTE         0x2BU   
#define MCP2515_REG_CANINTF         0x2CU   
#define MCP2515_REG_EFLG            0x2DU   

#define MCP2515_REG_TXB0CTRL        0x30U
#define MCP2515_REG_TXB0SIDH        0x31U
#define MCP2515_REG_TXB0SIDL        0x32U
#define MCP2515_REG_TXB0EID8        0x33U
#define MCP2515_REG_TXB0EID0        0x34U
#define MCP2515_REG_TXB0DLC         0x35U
#define MCP2515_REG_TXB0D0          0x36U   

#define MCP2515_REG_TXB1CTRL        0x40U
#define MCP2515_REG_TXB1SIDH        0x41U
#define MCP2515_REG_TXB1SIDL        0x42U
#define MCP2515_REG_TXB1DLC         0x45U
#define MCP2515_REG_TXB1D0          0x46U

#define MCP2515_REG_TXB2CTRL        0x50U
#define MCP2515_REG_TXB2SIDH        0x51U
#define MCP2515_REG_TXB2SIDL        0x52U
#define MCP2515_REG_TXB2DLC         0x55U
#define MCP2515_REG_TXB2D0          0x56U

#define MCP2515_REG_RXB0CTRL        0x60U
#define MCP2515_REG_RXB0SIDH        0x61U
#define MCP2515_REG_RXB0SIDL        0x62U
#define MCP2515_REG_RXB0EID8        0x63U
#define MCP2515_REG_RXB0EID0        0x64U
#define MCP2515_REG_RXB0DLC         0x65U
#define MCP2515_REG_RXB0D0          0x66U   

#define MCP2515_REG_RXB1CTRL        0x70U
#define MCP2515_REG_RXB1SIDH        0x71U
#define MCP2515_REG_RXB1SIDL        0x72U
#define MCP2515_REG_RXB1DLC         0x75U
#define MCP2515_REG_RXB1D0          0x76U

#define MCP2515_REG_RXF0SIDH        0x00U
#define MCP2515_REG_RXF0SIDL        0x01U
#define MCP2515_REG_RXF1SIDH        0x04U
#define MCP2515_REG_RXF1SIDL        0x05U
#define MCP2515_REG_RXF2SIDH        0x08U
#define MCP2515_REG_RXF2SIDL        0x09U
#define MCP2515_REG_RXF3SIDH        0x10U
#define MCP2515_REG_RXF3SIDL        0x11U
#define MCP2515_REG_RXF4SIDH        0x14U
#define MCP2515_REG_RXF4SIDL        0x15U
#define MCP2515_REG_RXF5SIDH        0x18U
#define MCP2515_REG_RXF5SIDL        0x19U

#define MCP2515_REG_RXM0SIDH        0x20U
#define MCP2515_REG_RXM0SIDL        0x21U
#define MCP2515_REG_RXM1SIDH        0x24U
#define MCP2515_REG_RXM1SIDL        0x25U

#define MCP2515_REG_TEC             0x1CU   
#define MCP2515_REG_REC             0x1DU   

#define MCP2515_CANCTRL_REQOP2      (1U << 7)
#define MCP2515_CANCTRL_REQOP1      (1U << 6)
#define MCP2515_CANCTRL_REQOP0      (1U << 5)
#define MCP2515_CANCTRL_ABAT        (1U << 4)   
#define MCP2515_CANCTRL_OSM         (1U << 3)   
#define MCP2515_CANCTRL_CLKEN       (1U << 2)   
#define MCP2515_CANCTRL_CLKPRE1     (1U << 1)
#define MCP2515_CANCTRL_CLKPRE0     (1U << 0)

#define MCP2515_MODE_NORMAL         0x00U       
#define MCP2515_MODE_SLEEP          0x20U
#define MCP2515_MODE_LOOPBACK       0x40U       
#define MCP2515_MODE_LISTENONLY     0x60U
#define MCP2515_MODE_CONFIG         0x80U       
#define MCP2515_MODE_MASK           0xE0U

#define MCP2515_CANSTAT_OPMOD_MASK  0xE0U
#define MCP2515_CANSTAT_ICOD_MASK   0x0EU

#define MCP2515_CANINTF_RX0IF       (1U << 0)   
#define MCP2515_CANINTF_RX1IF       (1U << 1)   
#define MCP2515_CANINTF_TX0IF       (1U << 2)   
#define MCP2515_CANINTF_TX1IF       (1U << 3)   
#define MCP2515_CANINTF_TX2IF       (1U << 4)   
#define MCP2515_CANINTF_ERRIF       (1U << 5)   
#define MCP2515_CANINTF_WAKIF       (1U << 6)   
#define MCP2515_CANINTF_MERRF       (1U << 7)   

#define MCP2515_CANINTE_RX0IE       (1U << 0)
#define MCP2515_CANINTE_RX1IE       (1U << 1)
#define MCP2515_CANINTE_TX0IE       (1U << 2)
#define MCP2515_CANINTE_TX1IE       (1U << 3)
#define MCP2515_CANINTE_TX2IE       (1U << 4)
#define MCP2515_CANINTE_ERRIE       (1U << 5)
#define MCP2515_CANINTE_WAKIE       (1U << 6)
#define MCP2515_CANINTE_MERRE       (1U << 7)

#define MCP2515_TXBnCTRL_TXREQ      (1U << 3)   
#define MCP2515_TXBnCTRL_TXP1       (1U << 1)   
#define MCP2515_TXBnCTRL_TXP0       (1U << 0)   
#define MCP2515_TXBnCTRL_ABTF       (1U << 6)   
#define MCP2515_TXBnCTRL_MLOA       (1U << 5)   
#define MCP2515_TXBnCTRL_TXERR      (1U << 4)   

#define MCP2515_RXB0CTRL_RXM1       (1U << 6)   
#define MCP2515_RXB0CTRL_RXM0       (1U << 5)   
#define MCP2515_RXB0CTRL_BUKT       (1U << 2)   
#define MCP2515_RXB0CTRL_FILHIT0    (1U << 0)

#define MCP2515_RXBnCTRL_RXM_ANY    (0x03U << 5)

#define MCP2515_RXBnCTRL_RXM_STD    (0x00U << 5)

#define MCP2515_CNF2_BTLMODE        (1U << 7)   
#define MCP2515_CNF2_SAM            (1U << 6)   

#define MCP2515_EFLG_RX1OVR         (1U << 7)   
#define MCP2515_EFLG_RX0OVR         (1U << 6)   
#define MCP2515_EFLG_TXBO           (1U << 5)   
#define MCP2515_EFLG_TXEP           (1U << 4)   
#define MCP2515_EFLG_RXEP           (1U << 3)   
#define MCP2515_EFLG_TXWAR          (1U << 2)   
#define MCP2515_EFLG_RXWAR          (1U << 1)   
#define MCP2515_EFLG_EWARN          (1U << 0)   

#ifdef __cplusplus
}
#endif

#endif 
