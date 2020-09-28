/**
 * Driver for SX1272 LoRa nodes using STM34F446RE micro-controllers. This is a 2018-19 summer research project under the
 * supervision of Dr. Chandima Ekanyake and Dr. Hui Ma, School of ITEE, The University of Queensland, Brisbane, QLD, Australia.
 * The project aims to create a LPWAN network for stand-alone, pole-mounted transformers within Queensland teritory.
 *
 * Created on: 23 November, 2018
 * Author: Trung Q. Cao - 44592394
 */

#include "../Drivers/Inc/sx1272.h"
#include <math.h>
#include "main.h"
#include "cmsis_os.h"
#include "../Utils/Inc/utils_crc.h"
#define SX1272_SET_SLEEP_MODE()	sx1272_set_op_mode((LORA << LRANGE_MODE) | (LORA_ACCESS << REG_SHARE) | (SLEEP << MODE))
#define SX1272_SET_STDBY_MODE()	sx1272_set_op_mode((LORA << LRANGE_MODE) | (LORA_ACCESS << REG_SHARE) | (STDBY << MODE))
static SX1272 *sx1272;
u8 rxBuf[PAYLOAD_LENGTH + CRC_LENGTH + HEADER_LENGTH];
extern CRC_HandleTypeDef hcrc;
extern osMutexId pckgBlthMutexHandle;
//extern osSemaphoreId semIRQLoraHandle;
extern LoraPckg loraPckgTx;

extern u8 isRxLora;
char* waitStrLora = "wait Rx LORA";
u8 bufTx[PAYLOAD_LENGTH + CRC_LENGTH + HEADER_LENGTH + 1];
u8 numsNoConnect = 0;

PckgRssi pckgRssi;

/**
 * Explicitly selects the device to start SPI communication protocol.
 */

void spiTx(u8 *txData, u8 sz) {
	spiLoraInfo.irqFlags.regIrq = 0;
	SPI1_LORA_CS_SEL();
	HAL_SPI_Transmit_IT(sx1272->hspi, txData, sz);
	waitTx("", &spiLoraInfo.irqFlags, 50, SPI_LORA_TIMEOUT);
	SPI1_LORA_CS_DESEL();
}

void spiTxRx(u8* pData, u8 sz){
	spiLoraInfo.irqFlags.regIrq = 0;
	SPI1_LORA_CS_SEL();
	HAL_SPI_TransmitReceive_IT(sx1272->hspi, pData, pData, sz);
	waitRx("",  &spiLoraInfo.irqFlags, 50, SPI_LORA_TIMEOUT);
	SPI1_LORA_CS_DESEL();
}

void spiRx(u8 cmd, u8 *pData, u8 sz){
	spiLoraInfo.irqFlags.regIrq = 0;
	SPI1_LORA_CS_SEL();
	HAL_SPI_Transmit_IT(sx1272->hspi, &cmd, 1);
	waitTx("",  &spiLoraInfo.irqFlags, 50, SPI_LORA_TIMEOUT);
	HAL_SPI_Receive_IT(sx1272->hspi, pData, sz);
	waitRx("",  &spiLoraInfo.irqFlags, 100, SPI_LORA_TIMEOUT);
	SPI1_LORA_CS_DESEL();
}

void sx1272_lora_init(SX1272 *node) {
	sx1272 = node;

	/* Reset pulse for programming mode */
//	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);

	/* Set current supply option */
	sx1272_set_ocp(0x2B);
	/* Set operating mode. Registers are only to be modified in either LORA_SLEEP or LORA_STDBY mode and
	 * FSK_SLEEP mode must be entered to initialize mode change */
	sx1272_set_op_mode(0); //FSK SLEEP MODE
	SX1272_SET_SLEEP_MODE();
	SX1272_SET_STDBY_MODE();

    /* Set modem configurations. To achieve high immunity to EMI caused by near by transformer, maximum coding rate,
     * spreading factor and minimum bandwidth are selected */
	sx1272_set_modem_config(
		(BW_500K << BAND_WIDTH) | (CR_4_8 << CODING_RATE) | (EXPLICIT_HEADER << HEADER_MODE) | (CRC_DISABLED << RX_CRC),
		(SF_7 << SPREADING_FACTOR) | (INTERNAL << AGC_MODE));

	/* Set max pay load length */
	sx1272_set_max_payload(MAX_PACKET_LENGTH);
	sx1272_set_payload_length(PAYLOAD_LENGTH + HEADER_LENGTH + CRC_LENGTH);

	/* Set base frequency */
	sx1272_set_freq(CH_8675);

	/* Set power output option */
	sx1272_set_pa_config(0x07 | (PA_BOOST << PA_SELECT)); //High output power. Double check threshold and supply.

	/* Clear flags */
	sx1272_clear_irq_flags();

	/* Reset FIFO pointer */
	sx1272_set_fifo_addr_ptr(0);
}


/**
 * Sets the operation mode of the device. This register can only be written in SLEEP/STDBY mode.
 *
 * @param op The operation mode to be set
 */
void sx1272_set_op_mode(uint8_t op) {
	uint8_t cmd[2] = {REG_LR_OP_MODE | WRITE, op};

	spiTx(cmd, 2);
}

/**
 * Sets the frequency channel of the device. This register can only be written in SLEEP/STDBY mode. The frequency is a 24-bit value,
 * being stored in 3 registers respectively.
 *
 * @param freq The frequency to be set
 */
void sx1272_set_freq(uint32_t freq) {
	uint8_t cmd[2];

	/* Set most significant bits */
	cmd[0] = REG_LR_FREQ_MSB | WRITE;
	cmd[1] = (uint8_t) (freq >> 16);

	spiTx(cmd, 2);

	/* Set mid bits */
	cmd[0] = REG_LR_FREQ_MIB | WRITE;
	cmd[1] = (uint8_t) (freq >> 8);

	spiTx(cmd, 2);

	/* Set least significant bits */
	cmd[0] = REG_LR_FREQ_LSB | WRITE;
	cmd[1] = (uint8_t) (freq);

	spiTx(cmd, 2);
}

void sx1272_set_pa_config(uint8_t pa) {
	uint8_t cmd[2];

	cmd[0] = REG_LR_PA_CONFIG | WRITE;
	cmd[1] = pa;

	spiTx(cmd, 2);
}

void sx1272_set_pa_ramp(uint8_t ramp) {
	u8 data[2] = {REG_LR_PA_RAMP | WRITE, ramp};

	spiTx(data, 2);
}

void sx1272_set_ocp(uint8_t ocp) {
	u8 cmd[2];

	cmd[0] = REG_LR_OCP | WRITE;
	cmd[1] = ocp;

	spiTx(cmd, 2);
}

void sx1272_set_lna(uint8_t lna) {
	u8 data[2] = {REG_LR_LNA | WRITE, lna};
	spiTx(data, 2);
}

void sx1272_set_fifo_addr_ptr(uint8_t addr) {
	uint8_t cmd[2] = {REG_LR_FIFOADDRPTR | WRITE, addr};

	spiTx(cmd, 2);
}

void sx1272_set_tx_base(uint8_t addr) {
	uint8_t cmd[2] = {REG_LR_FIFOTXBASEADDR | WRITE, addr};

	spiTx(cmd, 2);
}

void sx1272_set_rx_base(uint8_t addr) {
	uint8_t cmd[2] = {REG_LR_FIFORXBASEADDR | WRITE, addr};

	spiTx(cmd, 2);
}

void sx1272_set_modem_config(uint8_t config1, uint8_t config2) {
	u8 cmd[2];

	cmd[0] = REG_LR_MODEMCONFIG1 | WRITE;
	cmd[1] = config1;
	spiTx(cmd, 2);

	cmd[0] = REG_LR_MODEMCONFIG2 | WRITE;
	cmd[1] = config2;
	spiTx(cmd, 2);
}


void sx1272_set_payload_length(uint8_t length) {
    uint8_t cmd[2] = {REG_LR_PAYLOADLENGTH  | WRITE, length};

    spiTx(cmd, 2);
}

void sx1272_set_max_payload(uint8_t max) {
	uint8_t cmd[2];

	cmd[0] = REG_LR_RX_MAX_PAYLOADLENGTH | WRITE;
	cmd[1] = max;

	spiTx(cmd, 2);
}

void sx1272_set_detect_opt(uint8_t detect) {
	u8 data[2] = {REG_LR_DECTECT_OPTIMIZE | WRITE, detect};

	spiTx(data, 2);
}

void sx1272_set_sync_word(uint8_t sync) {
	uint8_t cmd[2] = {REG_LR_SYNC_WORD | WRITE, sync};

	spiTx(cmd, 2);
}

void sx1272_set_dio_mapping(uint8_t map) {

	u8 data[2] = {REG_IO_MAPPING_1 | WRITE, map};
	spiTx(data, 2);
}

void sx1272_clear_irq_flags() {
	uint8_t cmd[2] = {REG_LR_IRQFLAGS | WRITE, 0xFF};

	spiTx(cmd, 2);
}

void sx1272_write_fifo(u8* data) {
	bufTx[0] = REG_LR_FIFO | WRITE;
	bufTx[1] = LR_HEADER1;
	bufTx[2] = LR_HEADER2;

	memcpy(bufTx + 3, data, PAYLOAD_LENGTH + CRC_LENGTH);
	spiTx(bufTx, PAYLOAD_LENGTH + 1 + HEADER_LENGTH + CRC_LENGTH);
}

inline void sx1272_clear_fifo() {
    SX1272_SET_SLEEP_MODE();
}

void sx1272_send(u8 *data){
    uint8_t op = 0;

    /* Save the current mode */
    op = sx1272_get_op_mode();
    SX1272_SET_STDBY_MODE();
    sx1272_set_sync_word(0x34); //LoRa MAC preamble
//    sx1272_set_dio_mapping(MAP_DIO0_LORA_TXDONE);
//    /* GPIO-based interrupt has been set, mask out this bit */
//    sx1272_set_irq_mask(~(TX_DONE));

    /* Clears flags */
    sx1272_clear_irq_flags();

    /* Sets TX pointer to bottom of FIFO page */
    sx1272_set_tx_base(0x00);
    sx1272_set_fifo_addr_ptr(0x00);

    /* Writes pay load */
    sx1272_write_fifo(data);
    /* 2 bytes CRC for pay load */
    //  sx1272_set_fifo_addr_ptr(0x00);
	//  uint8_t buf[10];
	//  sx1272_read_fifo(buf, 6);


    /* Initializes TX mode to send the packet in FIFO */
    sx1272_set_op_mode((LORA << LRANGE_MODE) | (LORA_ACCESS << REG_SHARE) | (TX << MODE));
    sx1272_get_op_mode();
//    xSemaphoreTake(IRQLORASemaphoreHandle,  portMAX_DELAY);

    uint8_t flags = sx1272_get_irq_flags();

    /* Polls IRQ register for TxDone flag */
    while(!((flags & TX_DONE) >> TX_DONE_MASK)) {
        flags = sx1272_get_irq_flags();
        osDelay(10); // Might use threads + semaphore in FreeRTOS
    }

//    printf("OK: sx1272_send()\r\n");


    /* Clears FIFO */
    SX1272_SET_SLEEP_MODE();
    /* Restores previous mode */
    sx1272_set_op_mode(op);
    /* Clears flags */
    sx1272_clear_irq_flags();
}

u8 sx1272_receive(LoraPckg* rxPckg){
    u8 flags = 0, op = 0, prev;
    /* Saves the current mode */
    op = sx1272_get_op_mode();

    /* Configures receiver mode */
    sx1272_set_detect_opt(0x3);
    sx1272_set_pa_ramp(0x09);
    sx1272_set_lna(0x23);
	//sx1272_set_lna(BOOST_150 | (GAIN_MAX << 5));
    sx1272_set_sync_word(0x34); //LoRa MAC preamble

    /* Enables IO interrupts */
    sx1272_set_dio_mapping(MAP_DIO0_LORA_RXDONE);
    sx1272_clear_irq_flags();

    /* Sets RX base current to the bottom of FIFO page */
    sx1272_set_rx_base(0x00);
    /* Saves current FIFO RX pointer */
    prev = sx1272_get_rx_current_ptr();
    /* Points FIFO pointer to the last packet received */
    sx1272_set_fifo_addr_ptr(prev);

    /* Registers the packet length */
    sx1272_set_payload_length(PAYLOAD_LENGTH + CRC_LENGTH + HEADER_LENGTH);

    /* Initializes RxContinuous mode to receive packets */
    sx1272_set_op_mode((LORA << LRANGE_MODE) | (LORA_ACCESS << REG_SHARE) | (RX_CONT << MODE));

    flags = sx1272_get_irq_flags();
    /* Polls for ValidHeader flag */


    waitLora();
#if(LR_IS_TRANSMITTER)
    if(isRxLora){
		isRxLora = 0;
        flags = sx1272_get_irq_flags();
        /* Polls for ValidHeader flag */
        if(!(flags & VALID_HDR) >> VALID_HDR_MASK) {
        	sx1272_clear_irq_flags();
        	return LR_STAT_ERROR;
        }
        /* Polls for RxDone flag */
        if (!((flags & RXDONE) >> RXDONE_MASK)) {
        	sx1272_clear_irq_flags();
        	return LR_STAT_ERROR;
        }
        /* Checks for valid header CRC */
        if ((flags & CRC_ERR) >> CRC_ERR_MASK) {
            sx1272_clear_irq_flags();
            return LR_STAT_ERROR;
        }
        /* Reads and copies received data in FIFO to buffer */
        sx1272_read_fifo(rxBuf, PAYLOAD_LENGTH + CRC_LENGTH + HEADER_LENGTH);
        memcpy(rxPckg, rxBuf + 2, PAYLOAD_LENGTH + CRC_LENGTH);

        numsNoConnect = 0;

//        xSemaphoreTake(pckgBlthMutexHandle, portMAX_DELAY);
        if(rxPckg->crc == calcCrc16((u8*)(&(rxPckg->data)), PAYLOAD_LENGTH + CRC_LENGTH)){
        	++pckgRssi.rxNumPckg;
        	pckgRssi.rssi.r2 = rxPckg->data.rssiFromRemoteLora;
        	pckgRssi.rssi.statePckg = LR_STAT_OK;
        } else{
        	pckgRssi.rssi.statePckg = LR_STAT_BAD_CRC;
        }
        ++pckgRssi.allNumPckg;
        pckgRssi.rssi.r1 = sx1272_get_rssi();
        D(printf("OK RSSI: %d, %d\r\n", (int)pckgRssi.rssi.r1, (int)pckgRssi.rssi.r2));
//        xQueueSendToBack(blthPckgQueueHandle, &pckgRssi, 100);
//        xSemaphoreGive(pckgBlthMutexHandle);
    } else{
//    	++numsNoConnect;
//        xSemaphoreTake(pckgBlthMutexHandle, portMAX_DELAY);
        ++pckgRssi.allNumPckg;
        pckgRssi.rssi.r1 = 0;
        pckgRssi.rssi.r2 = 0;
        pckgRssi.rssi.statePckg = LR_STAT_NO_PCKG;
		rxPckg->data.rssiFromRemoteLora = 0;
        D(printf("ERROR: no rssi\r\n"));

//        if(numsNoConnect == LIMIT_NO_CON){
//        	pckgBlth.dataBlth.rssi.regRssi = 0;
//        	numsNoConnect = 1;
//        }
//        xQueueSendToBack(blthPckgQueueHandle, &pckgRssi, 100);
//        xSemaphoreGive(pckgBlthMutexHandle);
    }
#else
    if(isRxLora){
			isRxLora = 0;
            flags = sx1272_get_irq_flags();
            /* Polls for ValidHeader flag */
            if(!(flags & VALID_HDR) >> VALID_HDR_MASK) {
            	sx1272_clear_irq_flags();
            	return LR_STAT_ERROR;
            }
            /* Polls for RxDone flag */
            if (!((flags & RXDONE) >> RXDONE_MASK)) {
            	sx1272_clear_irq_flags();
            	return LR_STAT_ERROR;
            }
            /* Checks for valid header CRC */
            if ((flags & CRC_ERR) >> CRC_ERR_MASK) {
                sx1272_clear_irq_flags();
                return LR_STAT_ERROR;
            }
            /* Reads and copies received data in FIFO to buffer */
            sx1272_read_fifo(rxBuf, PAYLOAD_LENGTH + CRC_LENGTH + HEADER_LENGTH);
            memcpy(rxPckg, rxBuf + 2, PAYLOAD_LENGTH + CRC_LENGTH);

            numsNoConnect = 0;

    //        xSemaphoreTake(pckgBlthMutexHandle, portMAX_DELAY);
            if(rxPckg->crc == calcCrc16((u8*)(&(rxPckg->data)), PAYLOAD_LENGTH))
            	loraPckgTx.data.rssiFromRemoteLora = sx1272_get_rssi();
            else
            	return LR_STAT_ERROR;
        }
#endif

    SX1272_SET_SLEEP_MODE();
    sx1272_set_op_mode(op);
    sx1272_clear_irq_flags();
    return LR_STAT_OK;
}

uint8_t sx1272_get_rx_current_ptr() {
	u8 data[2] = {REG_LR_FIFORXCURRENTADDR | READ, DUMMY_BYTE};

	spiTxRx(data, 2);

    return data[1];
}

uint8_t sx1272_get_irq_flags() {
	u8 data[2] = {REG_LR_IRQFLAGS | READ, DUMMY_BYTE};

	spiTxRx(data, 2);

	return data[1];
}

uint8_t sx1272_get_op_mode() {
	u8 data[2] = {REG_LR_OP_MODE | READ, DUMMY_BYTE};
	spiTxRx(data, 2);

	return data[1];
}

/**
 * CRC algorithm implementation. See SX1272/73 Semtech datasheet page 123 for further details.
 *
 * @param crc Previous CRC value
 * @param data New data to be added to the CRC
 * @param pol CRC polynomial selection
 *
 * @retval Newly computed CRC
 */
uint16_t compute_crc(uint16_t crc, uint8_t data, uint16_t pol) {
	for (int i = 0; i < 8; i++) {
		if ((((crc & 0x8000) >> 8) ^ (data & 0x80)) != 0) {
			crc <<= 1;
			crc ^= pol;
		} else {
			crc <<= 1;
		}

		data <<= 1;
	}

	return crc;
}

/**
 * CRC algorithm implementation. See SX1272/73 Semtech datasheet page 123 for further details.
 *
 * @param buffer Array containing the data
 * @param size Data buffer length
 * @param crc_type Select the CRC polynomial and seed
 *
 * @retval Buffer computed CRC
 */

float calculate_time_on_air() {
    float toa = 11;

    return toa;
}

void sx1272_read_fifo(uint8_t* data, uint8_t sz){
	uint8_t cmd = REG_LR_FIFO | READ;
	spiRx(cmd, data, sz);

//	spi_select();
//	HAL_SPI_Transmit(sx1272->hspi, &cmd, 1, SPI_TIMEOUT);
//	HAL_SPI_Receive(sx1272->hspi, data, sz, SPI_TIMEOUT);
//	spi_deselect();
}

u8 sx1272_get_rssi(void){
	u8 data[2] = {REG_LR_PKTSNRVALUE | READ, DUMMY_BYTE};
	spiTxRx(data, 2);

	u8 rssi = 137 - data[1];
//	if(pktsnr < 0)
//		rssi += ((pktsnr-2)/4);			//note: correct rounding for negative numbers

	return rssi;
}

void waitLora(){
	u16 timeout = 0;

#if(LR_IS_TRANSMITTER)
	while(!isRxLora && timeout < LR_TIME_OUT_MS){
		osDelay(500);
		timeout += 500;
//		printf("%s timeout: %d\r\n", waitStrLora, timeout);
	}
#else
	while(!isRxLora){
		osDelay(500);
//		printf("%s\r\n", waitStrLora);
	}
#endif
}
