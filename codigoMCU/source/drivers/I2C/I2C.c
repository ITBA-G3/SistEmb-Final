#include "I2C.h"
#include "fsl_i2c.h" // Biblioteca de soporte de hardware

static void soft_reset_I2C(void);

static I2C_Type* I2C_ptr = I2C0;

void init_I2C(void)
{
    i2c_master_config_t masterConfig;

    // Activación del clock
    SIM->SCGC5 |= SIM_SCGC5_PORTE_MASK;  // Para los puertos
	// Para el I2C1
    SIM->SCGC4 |= SIM_SCGC4_I2C0_MASK;
	//Configuración de puertos
    PORTE->PCR[24] = PORT_PCR_MUX(5) | PORT_PCR_ODE_MASK;
    PORTE->PCR[25] = PORT_PCR_MUX(5) | PORT_PCR_ODE_MASK;

    // Obtener configuración predeterminada del maestro
    I2C_MasterGetDefaultConfig(&masterConfig);
    masterConfig.baudRate_Bps = 100000U;   // 100kHz

    soft_reset_I2C();
    // Inicializar el módulo I2C con la configuración proporcionada
    I2C_MasterInit(I2C_ptr, &masterConfig, (50E6));
}

bool write_I2C(uint8_t address, char *payload, uint8_t size)
{
    i2c_master_transfer_t transfer = {0};

    // Guard: por si alguien pisa el pinmux
    PORTE->PCR[24] = PORT_PCR_MUX(5) | PORT_PCR_ODE_MASK;
    PORTE->PCR[25] = PORT_PCR_MUX(5) | PORT_PCR_ODE_MASK;

    // Configurar la estructura de transferencia
    transfer.slaveAddress = address;
    transfer.direction = kI2C_Write;
    transfer.subaddress = 0; // Sin subdirección
    transfer.subaddressSize = 0;
    transfer.data = (uint8_t *)payload;
    transfer.dataSize = size;
    transfer.flags = kI2C_TransferDefaultFlag;

    // Enviar datos y verificar el estado
    status_t status = I2C_MasterTransferBlocking(I2C_ptr, &transfer);
    return (status == kStatus_Success);
}

static void soft_reset_I2C(void)
{
    I2C0->C1 = 0;                    // disable
    I2C0->S  = I2C_S_IICIF_MASK | I2C_S_ARBL_MASK; // clear flags (write 1)
    I2C0->C2 = 0;
}

