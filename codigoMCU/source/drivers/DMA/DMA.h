/*
 *      Author: Group 3
 */

#ifndef DMA_H_
#define DMA_H_

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include "../../../SDK/startup/hardware.h"

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/
//Struct for DMA channel configuration
// Size of the bits transferred in a brust
typedef enum{
  DMA_TransSize_8Bit = 0x0,
  DMA_TransSize_16Bit = 0x01,
  DMA_TransSize_32Bit = 0x02,
  DMA_TransSize_16BitBurst = 0x04,
  DMA_TransSize_32BitBurst = 0x05
} DMATranfSize_t;

typedef enum{
  DMA_CH0,
  DMA_CH1,
  DMA_CH2,
  DMA_CH3,
  DMA_CH4,
  DMA_CH5,
  DMA_CH6,
  DMA_CH7,
  DMA_CH8,
  DMA_CH9,
  DMA_CH10,
  DMA_CH11,
  DMA_CH12,
  DMA_CH13,
  DMA_CH14,
  DMA_CH15,
} DMAChannel_t;

//typedef DMA_Type *DMA_t;

typedef void (*callback_t)(void);


/*******************************************************************************
 * FUNCTION PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/


void DMA_Init(void);

void DMA_StartTransfer(DMAChannel_t channel);

/**
 * @brief Configura el DMAMUX para un canal específico
 *
 * @param channel Canal DMA a configurar
 * @param enable Habilitar o deshabilitar el canal
 * @param trigger Habilitar o deshabilitar trigger
 * @param source Solicitud DMA
 */
void DMAMUX_ConfigChannel(DMAChannel_t channel, bool enable, bool trigger, dma_request_source_t source);

/**
 * @brief Configura el modo de interrupción y la función de callback para un canal DMA
 *
 * @param channel Canal DMA
 * @param mode Modo de interrupción
 * @param interrupt_callback Función de callback a llamar en la interrupción
 */
void DMA_SetChannelInterrupt(DMAChannel_t channel, bool mode, callback_t interrupt_callback);

/**
 * @brief Habilita o deshabilita la solicitud DMA para un canal
 *
 * @param channel Canal DMA
 * @param enable Habilitar o deshabilitar
 */
void DMA_SetEnableRequest(DMAChannel_t channel, bool enable);

/**
 * @brief Obtiene el estado de la solicitud de habilitación de un canal DMA
 *
 * @param channel Canal DMA
 * @return true si la solicitud está habilitada, false en caso contrario
 */
bool DMA_GetEnableRequest(DMAChannel_t channel);

/**
 * @brief Configura el módulo de la dirección de origen para un canal DMA
 *
 * @param channel Canal DMA
 * @param mod Valor del módulo
 */
void DMA_SetSourceModulo(DMAChannel_t channel, uint16_t mod);

/**
 * @brief Get the source address modulo for a DMA channel
 *
 * @param channel DMA channel to query
 * @return Modulo value
 */
uint16_t DMA_GetSourceModulo(DMAChannel_t channel);

/**
 * @brief Obtiene el módulo de la dirección de origen para un canal DMA
 *
 * @param channel Canal DMA
 * @return Valor del módulo
 */
uint16_t DMA_GetSourceModulo(DMAChannel_t channel);

/**
 * @brief Obtiene la dirección de origen para un canal DMA
 *
 * @param channel Canal DMA
 * @return Dirección de origen
 */
uint32_t DMA_GetSourceAddr(DMAChannel_t channel);

/**
 * @brief Configura el offset de la dirección de origen para un canal DMA
 *
 * @param channel Canal DMA
 * @param offset Offset de la dirección de origen
 */
void DMA_SetSourceAddrOffset(DMAChannel_t channel, int32_t offset);

/**
 * @brief Obtiene el offset de la dirección de origen para un canal DMA
 *
 * @param channel Canal DMA
 * @return Offset de la dirección de origen
 */
int32_t DMA_GetSourceAddrOffset(DMAChannel_t channel);

/**
 * @brief Configura el tamaño de transferencia de origen para un canal DMA
 *
 * @param channel Canal DMA
 * @param size Tamaño de transferencia
 */
void DMA_SetSourceTransfSize(DMAChannel_t channel, DMATranfSize_t size);

/**
 * @brief Obtiene el tamaño de transferencia de origen para un canal DMA
 *
 * @param channel Canal DMA
 * @return Tamaño de transferencia
 */
DMATranfSize_t DMA_GetSourceTransfSize(DMAChannel_t channel);

/**
 * @brief Configura el offset de la última dirección de origen para un canal DMA
 *
 * @param channel Canal DMA
 * @param offset Offset de la última dirección
 */
void DMA_SetSourceLastAddrOffset(DMAChannel_t channel, int32_t offset);

void DMA_SetSourceAddr(DMAChannel_t channel, uint32_t address);

/**
 * @brief Obtiene el offset de la última dirección de origen para un canal DMA
 *
 * @param channel Canal DMA
 * @return Offset de la última dirección
 */
int32_t DMA_GetSourceLastAddrOffset(DMAChannel_t channel);

/**
 * @brief Configura el módulo de la dirección de destino para un canal DMA
 *
 * @param channel Canal DMA
 * @param mod Valor del módulo
 */
void DMA_SetDestModulo(DMAChannel_t channel, uint16_t mod);

/**
 * @brief Obtiene el módulo de la dirección de destino para un canal DMA
 *
 * @param channel Canal DMA
 * @return Valor del módulo
 */
uint16_t DMA_GetDestModulo(DMAChannel_t channel);

/**
 * @brief Configura la dirección de destino para un canal DMA
 *
 * @param channel Canal DMA
 * @param address Dirección de destino
 */
void DMA_SetDestAddr(DMAChannel_t channel, uint32_t address);

/**
 * @brief Obtiene la dirección de destino para un canal DMA
 *
 * @param channel Canal DMA
 * @return Dirección de destino
 */
uint32_t DMA_GetDestAddr(DMAChannel_t channel);

/**
 * @brief Configura el offset de la dirección de destino para un canal DMA
 *
 * @param channel Canal DMA
 * @param offset Offset de la dirección de destino
 */
void DMA_SetDestAddrOffset(DMAChannel_t channel, int32_t offset);

/**
 * @brief Obtiene el offset de la dirección de destino para un canal DMA
 *
 * @param channel Canal DMA
 * @return Offset de la dirección de destino
 */
int32_t DMA_GetDestAddrOffset(DMAChannel_t channel);

/**
 * @brief Setea el tamaño de transferencia de destino para un canal DMA
 *
 * @param channel Canal DMA
 * @param size Tamaño
 */
void DMA_SetDestTransfSize(DMAChannel_t channel, DMATranfSize_t size);

/**
 * @brief Obtiene el tamaño de transferencia de destino para un canal DMA
 *
 * @param channel Canal DMA
 * @return Tamaño
 */
DMATranfSize_t DMA_GetDestTransfSize(DMAChannel_t channel);

/**
 * @brief Configura el offset de la última dirección de destino para un canal DMA
 *
 * @param channel Canal DMA
 * @param offset Offset de la última dirección
 */
void DMA_SetDestLastAddrOffset(DMAChannel_t channel, int32_t offset);

/**
 * @brief Obtiene el offset de la última dirección de destino para un canal DMA
 *
 * @param channel Canal DMA
 * @return Offset de la última dirección
 */
int32_t DMA_GetDetLastAddrOffset(DMAChannel_t channel);

/**
 * @brief Configura el contador de transferencia de minor loop para un canal DMA
 *
 * @param channel Canal DMA
 * @param MLsize Tamaño del minor loop
 */
void DMA_SetMinorLoopTransCount(DMAChannel_t channel, uint32_t MLsize);

/**
 * @brief Obtiene el contador de transferencia de minor loop para un canal DMA
 *
 * @param channel Canal DMA
 * @return Tamaño del minor loop
 */
uint32_t DMA_GetMinorLoopTransCount(DMAChannel_t channel);

/**
 * @brief Configura el contador de major loop actual para un canal DMA
 *
 * @param channel Canal DMA a configurar
 * @param count Contador de major loop
 */
void DMA_SetCurrMajorLoopCount(DMAChannel_t channel, uint16_t count);

/**
 * @brief Obtiene el contador de major loop actual para un canal DMA
 *
 * @param channel Canal DMA a consultar
 * @return Contador de major loop
 */
uint16_t DMA_GetCurrMajorLoopCount(DMAChannel_t channel);

/**
 * @brief Configura el contador de inicio de major loop para un canal DMA
 *
 * @param channel Canal DMA a configurar
 * @param count Contador de major loop
 */
void DMA_SetStartMajorLoopCount(DMAChannel_t channel, uint16_t count);

/**
 * @brief Obtiene el contador de inicio de major loop para un canal DMA
 *
 * @param channel Canal DMA
 * @return Contador de major loop
 */
uint16_t DMA_GetStartMajorLoopCount(DMAChannel_t channel);

/**
 * @brief Limpia la bandera DONE de un canal DMA
 * 
 * @param channel Canal DMA
 */
void DMA_ClearChannelDoneFlag(DMAChannel_t channel);

/**
 * @brief Limpia la bandera de interrupción de un canal DMA
 * 
 * @param channel Canal DMA
 */
void DMA_ClearChannelIntFlag(DMAChannel_t channel);

#endif /* DMA_H_ */
