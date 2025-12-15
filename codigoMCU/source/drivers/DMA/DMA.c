/*
 *      Author: Group 3
 */

#include "DMA.h"

static callback_t callback[16] = {0};

void DMA_Init(){
    //Clock para eDMA y DMAMUX
    SIM->SCGC7 |= SIM_SCGC7_DMA_MASK;  // Clock para eDMA
    SIM->SCGC6 |= SIM_SCGC6_DMAMUX_MASK;  // Clock para DMAMUX

    // Limpiar todos los eventos pendientes
    NVIC_ClearPendingIRQ(DMA0_IRQn);  // Limpiar IRQ pendiente para DMA0

    // Interrupciones DMA
    NVIC_EnableIRQ(DMA0_IRQn);  // Habilitar interrupción para DMA0
    NVIC_EnableIRQ(DMA1_IRQn);
}

void DMA_StartTransfer(DMAChannel_t channel){
    DMA0->SSRT = channel;  // Establece el bit de inicio para la transferencia DMA en el canal
}

// habilita el DMAMUX, configura el modo de disparo y selecciona fuente de datos
void DMAMUX_ConfigChannel(DMAChannel_t channel, bool enable, bool trigger, dma_request_source_t source){
//    DMA0->TCD[channel].CSR = 0;  // Limpia registro de control y estado del TCD
	// CAPAZ DESCOMENTAR.
	 uint32_t src = ((uint32_t)source) & 0x3Fu; // SOURCE is 6 bits

	    DMAMUX0->CHCFG[channel] =
	        DMAMUX_CHCFG_SOURCE(src) |
	        DMAMUX_CHCFG_TRIG(trigger) |
	        DMAMUX_CHCFG_ENBL(enable);
}

// configura interrupción en el canal
void DMA_SetChannelInterrupt(DMAChannel_t channel, bool mode, callback_t interrupt_callback){
    callback[channel] = interrupt_callback;  //callback para la interrupción
    DMA0->TCD[channel].CSR = (DMA0->TCD[channel].CSR & ~DMA_CSR_INTMAJOR_MASK) + DMA_CSR_INTMAJOR(mode);  // habilita interrupción

}


void DMA_SetEnableRequest(DMAChannel_t channel, bool state){
    DMA0->ERQ = (DMA0->ERQ & ~(1 << channel)) + (state << channel); //habilita o deshabilita la solicitud DMA 
}

bool DMA_GetEnableRequest(DMAChannel_t channel){
	return (DMA0->ERQ >> channel) & 1;  // devuelve status
}

void DMA_SetSourceModulo(DMAChannel_t channel, uint16_t mod){
	// setea dirección y define tamaño para buffer circular
	DMA0->TCD[channel].ATTR = (DMA0->TCD[channel].ATTR & ~DMA_ATTR_SMOD_MASK) + DMA_ATTR_SMOD(mod);  
}

uint16_t DMA_GetSourceModulo(DMAChannel_t channel){
	//devuelve dirección
	return (DMA0->TCD[channel].ATTR & DMA_ATTR_SMOD_MASK) >> DMA_ATTR_SMOD_SHIFT; 
}

void DMA_SetDestModulo(DMAChannel_t channel, uint16_t mod){
    // setea dirección y define tamaño para buffer circular
    DMA0->TCD[channel].ATTR = (DMA0->TCD[channel].ATTR & ~DMA_ATTR_DMOD_MASK) + DMA_ATTR_DMOD(mod);
}

uint16_t DMA_GetDestModulo(DMAChannel_t channel){
	//devuelve dirección
	return (DMA0->TCD[channel].ATTR & DMA_ATTR_DMOD_MASK) >> DMA_ATTR_DMOD_SHIFT;
}

void DMA_SetSourceAddr(DMAChannel_t channel, uint32_t address){
	DMA0->TCD[channel].SADDR = address;  // setea dirección
}

uint32_t DMA_GetSourceAddr(DMAChannel_t channel){
	return DMA0->TCD[channel].SADDR;  // devuelve dirección
}

void DMA_SetSourceAddrOffset(DMAChannel_t channel, int32_t offset){
    DMA0->TCD[channel].SOFF = offset;  // setea el offset de la dirección de origen
}

int32_t DMA_GetSourceAddrOffset(DMAChannel_t channel){
	return DMA0->TCD[channel].SOFF;  // devuelve el offset de la dirección de origen
}


void DMA_SetSourceTransfSize(DMAChannel_t channel, DMATranfSize_t size){
    DMA0->TCD[channel].ATTR = (DMA0->TCD[channel].ATTR & ~DMA_ATTR_SSIZE_MASK) + DMA_ATTR_SSIZE(size);//setea tamaño para cada minor loop 
}

DMATranfSize_t DMA_GetSourceTransfSize(DMAChannel_t channel){
    return (DMA0->TCD[channel].ATTR & DMA_ATTR_SSIZE_MASK) >> DMA_ATTR_SSIZE_SHIFT; //devuelve el tamaño de cada minor loop 
}

// setea el offset de la última dirección después de un major loop
void DMA_SetSourceLastAddrOffset(DMAChannel_t channel, int32_t offset){
    DMA0->TCD[channel].SLAST = offset;
}

// busca el offset de la última dirección después de un major loop
int32_t DMA_GetSourceLastAddrOffset(DMAChannel_t channel){
    return DMA0->TCD[channel].SLAST;  
}

void DMA_SetDestAddr(DMAChannel_t channel, uint32_t address){
	DMA0->TCD[channel].DADDR = address;  // setea dirección de destino
}

uint32_t DMA_GetDestAddr(DMAChannel_t channel){
	return DMA0->TCD[channel].DADDR;  // devuelve dirección de destino
}

void DMA_SetDestAddrOffset(DMAChannel_t channel, int32_t offset){
	DMA0->TCD[channel].DOFF = offset;  // setea offset para la dirección de destino
}

int32_t DMA_GetDestAddrOffset(DMAChannel_t channel){
	return DMA0->TCD[channel].DOFF;  // devuelve el offset de la dirección de destino
}

void DMA_SetDestTransfSize(DMAChannel_t channel, DMATranfSize_t size){
	DMA0->TCD[channel].ATTR = (DMA0->TCD[channel].ATTR & ~DMA_ATTR_DSIZE_MASK) + DMA_ATTR_DSIZE(size);  // setea el tamaño de la transferencia de destino
}

DMATranfSize_t DMA_GetDestTransfSize(DMAChannel_t channel){
	return (DMA0->TCD[channel].ATTR & DMA_ATTR_DSIZE_MASK) >> DMA_ATTR_DSIZE_SHIFT;  // devuelve el tamaño de la transferencia de destino
}

// setea el offset de la última dirección de destino después de un loop grande
void DMA_SetDestLastAddrOffset(DMAChannel_t channel, int32_t offset){
    DMA0->TCD[channel].DLAST_SGA = offset;
}

void DMA_SetMinorLoopTransCount(DMAChannel_t channel, uint32_t MLsize){
	DMA0->TCD[channel].NBYTES_MLOFFNO = MLsize & DMA_NBYTES_MLOFFNO_NBYTES_MASK;  //tamaño para el loop petite
}

uint32_t DMA_GetMinorLoopTransCount(DMAChannel_t channel){
	return DMA0->TCD[channel].NBYTES_MLOFFNO;  // contador para loop chikito
}


void DMA_SetCurrMajorLoopCount(DMAChannel_t channel, uint16_t count){
	DMA0->TCD[channel].CITER_ELINKNO = count & DMA_CITER_ELINKNO_CITER_MASK; //setea loop grande
}

uint16_t DMA_GetCurrMajorLoopCount(DMAChannel_t channel){
	return DMA0->TCD[channel].CITER_ELINKNO & DMA_CITER_ELINKNO_CITER_MASK; //contador para el loop grande
}

void DMA_SetStartMajorLoopCount(DMAChannel_t channel, uint16_t count){
	DMA0->TCD[channel].BITER_ELINKNO = count & DMA_BITER_ELINKNO_BITER_MASK; //setea loop grande
}

uint16_t DMA_GetStartMajorLoopCount(DMAChannel_t channel){
	return DMA0->TCD[channel].BITER_ELINKNO & DMA_BITER_ELINKNO_BITER_MASK; //contador para el loop grande
}

void DMA_ClearChannelDoneFlag(DMAChannel_t channel) {
    DMA0->CDNE = DMA_CDNE_CDNE(channel);
}

void DMA_ClearChannelIntFlag(DMAChannel_t channel) {
    DMA0->CINT = DMA_CINT_CINT(channel);
}

void DMA0_IRQHandler(){
	// Check channel 0
	if (DMA0->INT & (1u << 0)) {
		DMA_ClearChannelIntFlag(DMA_CH0);
		if (callback[0]) callback[0]();
	}

	// Check channel 1
//	if (DMA0->INT & (1u << 1)) {
//		DMA_ClearChannelDoneFlag(DMA_CH1);
//		DMA_ClearChannelIntFlag(DMA_CH1);
//		if (callback[1]) callback[1]();
//	}
}

void DMA1_IRQHandler(){
	// Check channel 1
	if (DMA0->INT & (1u << 1)) {
		DMA_ClearChannelDoneFlag(DMA_CH1);
		DMA_ClearChannelIntFlag(DMA_CH1);
		if (callback[1]) callback[1]();
	}
}
