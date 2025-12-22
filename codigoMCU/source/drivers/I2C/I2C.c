#include "I2C.h"
#include "fsl_i2c.h" // Biblioteca de I2C del SDK lamentablemente

static void soft_reset_I2C(void);

static I2C_Type* I2C_ptr = I2C0;

void init_I2C(void)
{
    i2c_master_config_t masterConfig;

    SIM->SCGC5 |= SIM_SCGC5_PORTE_MASK;  // Para los puertos
    SIM->SCGC4 |= SIM_SCGC4_I2C0_MASK;
    PORTE->PCR[24] = PORT_PCR_MUX(5) | PORT_PCR_ODE_MASK;
    PORTE->PCR[25] = PORT_PCR_MUX(5) | PORT_PCR_ODE_MASK;

    I2C_MasterGetDefaultConfig(&masterConfig);
    masterConfig.baudRate_Bps = 100000U;   // 100kHz

    soft_reset_I2C();
    I2C_MasterInit(I2C_ptr, &masterConfig, (50E6));
}

bool write_I2C(uint8_t address, char *payload, uint8_t size)
{
    i2c_master_transfer_t transfer = {0};

    // Guard: por si alguien pisa el pinmux, no encuentro donde mas los estamos usando asi que por las dudas
    PORTE->PCR[24] = PORT_PCR_MUX(5) | PORT_PCR_ODE_MASK;
    PORTE->PCR[25] = PORT_PCR_MUX(5) | PORT_PCR_ODE_MASK;

    transfer.slaveAddress = address;
    transfer.direction = kI2C_Write;
    transfer.subaddress = 0; 
    transfer.subaddressSize = 0;
    transfer.data = (uint8_t *)payload;
    transfer.dataSize = size;
    transfer.flags = kI2C_TransferDefaultFlag;

    status_t status = I2C_MasterTransferBlocking(I2C_ptr, &transfer);
    return (status == kStatus_Success);
}

static void soft_reset_I2C(void)
{
    I2C0->C1 = 0;                    
    I2C0->S  = I2C_S_IICIF_MASK | I2C_S_ARBL_MASK;
    I2C0->C2 = 0;
}

