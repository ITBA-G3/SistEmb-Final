/***************************************************************************/ /**
   @file     main.c
   @brief    MP3 Player main
   @author   Grupo 3
  ******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/
#include "fsl_sdhc.h"
/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/

/*******************************************************************************
 *******************************************************************************
						GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

/* Función que se llama 1 vez, al comienzo del programa */
void App_Init(void)
{
	sdhc_config_t sdhc_config;
	sdhc_config.cardDetectDat3 = true;
	sdhc_config.endianMode = kSDHC_EndianModeLittle;
	sdhc_config.dmaMode = kSDHC_DmaModeNo;
	sdhc_config.readWatermarkLevel = 128U;
	sdhc_config.writeWatermarkLevel = 128U;
	SDHC_Init(SDHC, &sdhc_config);
	SDHC_SetDataBusWidth(SDHC, kSDHC_DataBusWidth4Bit);
	sdhc_transfer_config_t transferConfig;
	transferConfig.dataBlockSize = 512U;
	transferConfig.dataBlockCount = 2U;
	transferConfig.commandArgument = 0x01AAU;
	transferConfig.commandIndex = 8U;
	transferConfig.flags |= (kSDHC_EnableDmaFlag | kSDHC_EnableAutoCommand12Flag | kSDHC_MultipleBlockFlag);
	SDHC_SetTransferConfig(SDHC, &transferConfig);
}

/* Función que se llama constantemente en un ciclo infinito */
void App_Run(void)
{

}

/*******************************************************************************
 *******************************************************************************
						LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

/*******************************************************************************
 ******************************************************************************/
