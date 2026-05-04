// isotp.c - ISO-TP (ISO 15765-2) Transport Protocol — Implementation

#include "isotp.h"
#include "can_interface.h"
#include "project_config.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>

// CAN frame'i pad'le ve gönder
static ISOTP_Status_t isotp_send_can_frame(const ISOTP_Session_t *session,
                                             const uint8_t *data, uint8_t len)
{
    CAN_Frame_t frame;
    frame.id          = session->tx_can_id;
    frame.dlc         = ISOTP_CAN_DL;
    frame.is_extended = false;
    frame.is_rtr      = false;

    memcpy(frame.data, data, len);
    if (len < ISOTP_CAN_DL) {
        memset(&frame.data[len], ISOTP_PAD_BYTE, ISOTP_CAN_DL - len);
    }

    CAN_IF_Status_t can_status = CAN_IF_SendFrame(&frame);
    return (can_status == CAN_IF_OK) ? ISOTP_OK : ISOTP_ERR_INVALID_FRAME;
}

// Flow Control frame gönder
static ISOTP_Status_t isotp_send_fc(ISOTP_Session_t *session, uint8_t flow_status)
{
    uint8_t fc_data[3];
    fc_data[0] = ISOTP_PCI_FC | (flow_status & 0x0FU);
    fc_data[1] = ISOTP_FC_BLOCK_SIZE;   
    fc_data[2] = ISOTP_FC_STMIN_MS;     

    return isotp_send_can_frame(session, fc_data, 3);
}

// Tek bir Consecutive Frame gönder (non-blocking)
static ISOTP_Status_t isotp_send_one_cf(ISOTP_Session_t *session)
{
    if (session->tx_offset >= session->tx_total_length) {
        session->state = ISOTP_STATE_IDLE;
        return ISOTP_OK;  
    }

    if (session->tx_block_size > 0 &&
        session->tx_block_count >= session->tx_block_size)
    {
        session->state = ISOTP_STATE_TX_WAIT_FC;
        session->last_activity_tick = HAL_GetTick();
        return ISOTP_IN_PROGRESS;
    }

    if (session->tx_stmin > 0) {
        uint32_t elapsed = HAL_GetTick() - session->last_activity_tick;
        if (elapsed < session->tx_stmin) {
            return ISOTP_IN_PROGRESS;  
        }
    }

    uint8_t cf_data[ISOTP_CAN_DL];
    cf_data[0] = ISOTP_PCI_CF | (session->tx_seq_number & 0x0FU);

    uint16_t remaining = session->tx_total_length - session->tx_offset;
    uint8_t  copy_len  = (remaining > ISOTP_CF_DATA) ? ISOTP_CF_DATA : (uint8_t)remaining;

    memcpy(&cf_data[1], &session->tx_buffer[session->tx_offset], copy_len);

    ISOTP_Status_t status = isotp_send_can_frame(session, cf_data, 1U + copy_len);
    if (status != ISOTP_OK) {
        return status;
    }

    session->tx_offset     += copy_len;
    session->tx_seq_number  = (session->tx_seq_number + 1U) & 0x0FU;
    session->tx_block_count++;
    session->last_activity_tick = HAL_GetTick();

    if (session->tx_offset >= session->tx_total_length) {
        session->state = ISOTP_STATE_IDLE;
        return ISOTP_OK;
    }

    return ISOTP_IN_PROGRESS;
}

ISOTP_Status_t ISOTP_Init(ISOTP_Session_t *session, uint16_t tx_id, uint16_t rx_id)
{
    if (session == NULL) {
        return ISOTP_ERR_PARAM;
    }

    memset(session, 0, sizeof(ISOTP_Session_t));
    session->state     = ISOTP_STATE_IDLE;
    session->tx_can_id = tx_id;
    session->rx_can_id = rx_id;

    return ISOTP_OK;
}

ISOTP_Status_t ISOTP_Send(ISOTP_Session_t *session, const uint8_t *data, uint16_t length)
{
    if (session == NULL || data == NULL || length == 0) {
        return ISOTP_ERR_PARAM;
    }

    if (length > ISOTP_MAX_PAYLOAD_SIZE) {
        return ISOTP_ERR_OVERFLOW;
    }

    if (session->state != ISOTP_STATE_IDLE) {
        return ISOTP_ERR_BUSY;
    }

    if (length <= ISOTP_SF_MAX_DATA)
    {
        
        uint8_t sf_data[ISOTP_CAN_DL];
        sf_data[0] = ISOTP_PCI_SF | (length & 0x0FU);
        memcpy(&sf_data[1], data, length);

        return isotp_send_can_frame(session, sf_data, 1U + (uint8_t)length);
    }
    else
    {

        memcpy(session->tx_buffer, data, length);
        session->tx_total_length = length;
        session->tx_offset       = 0;
        session->tx_seq_number   = 1;   
        session->tx_block_count  = 0;

        uint8_t ff_data[ISOTP_CAN_DL];
        ff_data[0] = ISOTP_PCI_FF | (uint8_t)((length >> 8) & 0x0FU);  
        ff_data[1] = (uint8_t)(length & 0xFFU);                         

        uint8_t first_copy = ISOTP_FF_FIRST_DATA;
        if (length < first_copy) {
            first_copy = (uint8_t)length;
        }
        memcpy(&ff_data[2], data, first_copy);
        session->tx_offset = first_copy;

        ISOTP_Status_t status = isotp_send_can_frame(session, ff_data, 2U + first_copy);
        if (status != ISOTP_OK) {
            return status;
        }

        session->state              = ISOTP_STATE_TX_WAIT_FC;
        session->last_activity_tick = HAL_GetTick();

        return ISOTP_IN_PROGRESS;
    }
}

ISOTP_Status_t ISOTP_ProcessRxFrame(ISOTP_Session_t *session, const CAN_Frame_t *frame)
{
    if (session == NULL || frame == NULL || frame->dlc == 0) {
        return ISOTP_ERR_PARAM;
    }

    uint8_t pci_type = frame->data[0] & 0xF0U;

    session->last_activity_tick = HAL_GetTick();

    switch (pci_type)
    {
        
        case ISOTP_PCI_SF:
        {
            uint8_t sf_len = frame->data[0] & 0x0FU;
            if (sf_len == 0 || sf_len > ISOTP_SF_MAX_DATA) {
                return ISOTP_ERR_INVALID_FRAME;
            }

            memcpy(session->rx_buffer, &frame->data[1], sf_len);
            session->rx_total_length = sf_len;
            session->rx_offset       = sf_len;
            session->state           = ISOTP_STATE_IDLE;

            return ISOTP_COMPLETE;
        }

        case ISOTP_PCI_FF:
        {
            if (session->state != ISOTP_STATE_IDLE) {
                
                DEBUG_LOG(DEBUG_LEVEL_WARN, "ISOTP: FF received while state=%d, resetting\r\n",
                          session->state);
            }

            uint16_t total_len = ((uint16_t)(frame->data[0] & 0x0FU) << 8) | frame->data[1];

            if (total_len == 0 || total_len > ISOTP_MAX_PAYLOAD_SIZE) {
                return ISOTP_ERR_OVERFLOW;
            }

            session->rx_total_length = total_len;
            session->rx_offset       = 0;

            uint8_t first_data_len = ISOTP_FF_FIRST_DATA;
            if (total_len < first_data_len) {
                first_data_len = (uint8_t)total_len;
            }
            memcpy(session->rx_buffer, &frame->data[2], first_data_len);
            session->rx_offset     = first_data_len;
            session->rx_seq_number = 1;     

            session->state = ISOTP_STATE_RX_RECEIVING_CF;

            ISOTP_Status_t fc_status = isotp_send_fc(session, ISOTP_FC_FS_CTS);
            if (fc_status != ISOTP_OK) {
                session->state = ISOTP_STATE_IDLE;
                return fc_status;
            }

            return ISOTP_IN_PROGRESS;
        }

        case ISOTP_PCI_CF:
        {
            if (session->state != ISOTP_STATE_RX_RECEIVING_CF) {
                return ISOTP_ERR_INVALID_FRAME;   
            }

            uint8_t sn = frame->data[0] & 0x0FU;

            if (sn != session->rx_seq_number) {
                DEBUG_LOG(DEBUG_LEVEL_ERROR, "ISOTP: SN mismatch expected=%u got=%u\r\n",
                          session->rx_seq_number, sn);
                session->state = ISOTP_STATE_IDLE;
                return ISOTP_ERR_SEQUENCE;
            }

            uint16_t remaining = session->rx_total_length - session->rx_offset;
            uint8_t  copy_len  = (remaining > ISOTP_CF_DATA) ? ISOTP_CF_DATA : (uint8_t)remaining;

            memcpy(&session->rx_buffer[session->rx_offset], &frame->data[1], copy_len);
            session->rx_offset += copy_len;

            session->rx_seq_number = (session->rx_seq_number + 1U) & 0x0FU;

            if (session->rx_offset >= session->rx_total_length) {
                session->state = ISOTP_STATE_IDLE;
                return ISOTP_COMPLETE;
            }

            return ISOTP_IN_PROGRESS;
        }

        case ISOTP_PCI_FC:
        {
            if (session->state != ISOTP_STATE_TX_WAIT_FC) {
                return ISOTP_ERR_INVALID_FRAME;
            }

            uint8_t fs = frame->data[0] & 0x0FU;

            if (fs == ISOTP_FC_FS_OVFLW) {
                session->state = ISOTP_STATE_IDLE;
                return ISOTP_ERR_FC_OVERFLOW;
            }

            if (fs == ISOTP_FC_FS_WAIT) {
                
                session->last_activity_tick = HAL_GetTick();
                return ISOTP_IN_PROGRESS;
            }

            session->tx_block_size  = frame->data[1];
            session->tx_stmin       = frame->data[2];
            session->tx_block_count = 0;

            session->state = ISOTP_STATE_TX_SENDING_CF;
            session->last_activity_tick = HAL_GetTick();

            return ISOTP_IN_PROGRESS;
        }

        default:
            return ISOTP_ERR_INVALID_FRAME;
    }
}

ISOTP_Status_t ISOTP_GetReceivedMessage(ISOTP_Session_t *session,
                                         uint8_t *data, uint16_t *length)
{
    if (session == NULL || data == NULL || length == NULL) {
        return ISOTP_ERR_PARAM;
    }

    if (session->rx_offset == 0 || session->rx_offset < session->rx_total_length) {
        return ISOTP_ERR_PARAM;     
    }

    memcpy(data, session->rx_buffer, session->rx_total_length);
    *length = session->rx_total_length;

    session->rx_offset       = 0;
    session->rx_total_length = 0;

    return ISOTP_OK;
}

ISOTP_Status_t ISOTP_Tick(ISOTP_Session_t *session)
{
    if (session == NULL || session->state == ISOTP_STATE_IDLE) {
        return ISOTP_OK;
    }

    uint32_t now     = HAL_GetTick();
    uint32_t elapsed = now - session->last_activity_tick;

    if (elapsed > ISOTP_TIMEOUT_MS) {
        DEBUG_LOG(DEBUG_LEVEL_ERROR, "ISOTP: Timeout in state %d after %lu ms\r\n",
                  session->state, elapsed);
        ISOTP_Reset(session);
        return ISOTP_ERR_TIMEOUT;
    }

    return ISOTP_OK;
}

void ISOTP_Reset(ISOTP_Session_t *session)
{
    if (session != NULL) {
        uint16_t save_tx_id = session->tx_can_id;
        uint16_t save_rx_id = session->rx_can_id;

        session->state           = ISOTP_STATE_IDLE;
        session->rx_total_length = 0;
        session->rx_offset       = 0;
        session->tx_total_length = 0;
        session->tx_offset       = 0;

        session->tx_can_id = save_tx_id;
        session->rx_can_id = save_rx_id;
    }
}

ISOTP_State_t ISOTP_GetState(const ISOTP_Session_t *session)
{
    if (session == NULL) {
        return ISOTP_STATE_IDLE;
    }
    return session->state;
}

ISOTP_Status_t ISOTP_ProcessTx(ISOTP_Session_t *session)
{
    if (session == NULL || session->state != ISOTP_STATE_TX_SENDING_CF) {
        return ISOTP_OK;  
    }

    return isotp_send_one_cf(session);
}
