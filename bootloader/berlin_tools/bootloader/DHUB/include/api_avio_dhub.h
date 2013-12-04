#ifndef __API_AVIO_DHUB_H__
#define __API_AVIO_DHUB_H__
#include "dHub.h"
#include "avioDhub.h"
#include "api_dhub.h"

#if !BG2CD /* for BG2CD */
#define VPP_DHUB_BASE			(MEMMAP_VPP_DHUB_REG_BASE + RA_vppDhub_dHub0)
#define VPP_HBO_SRAM_BASE		(MEMMAP_VPP_DHUB_REG_BASE + RA_vppDhub_tcm0)
#if (BERLIN_CHIP_VERSION >= BERLIN_B_0)
#define VPP_NUM_OF_CHANNELS		(avioDhubChMap_vpp_TT_R+1)
#else
#define VPP_NUM_OF_CHANNELS		(avioDhubChMap_vpp_BG_R+1)
#endif
#define AG_DHUB_BASE			(MEMMAP_AG_DHUB_REG_BASE + RA_agDhub_dHub0)
#define AG_HBO_SRAM_BASE		(MEMMAP_AG_DHUB_REG_BASE + RA_agDhub_tcm0)
#define AG_NUM_OF_CHANNELS		(avioDhubChMap_ag_PG_ENG_W+1)

#if (BERLIN_CHIP_VERSION >= BERLIN_BG2)
#define VIP_DHUB_BASE			(MEMMAP_VIP_DHUB_REG_BASE + RA_vipDhub_dHub0)
#define VIP_HBO_SRAM_BASE		(MEMMAP_VIP_DHUB_REG_BASE + RA_vipDhub_tcm0)

#if (BERLIN_CHIP_VERSION >= BERLIN_BG2 && BERLIN_CHIP_VERSION < BERLIN_BG2_A0)
#define VIP_NUM_OF_CHANNELS		(avioDhubChMap_vip_VBI_WR+1)
#else
#define VIP_NUM_OF_CHANNELS             (avioDhubChMap_vip_MIC3_W+1)
#endif


#define VIP_DHUB_BANK0_START_ADDR        (0)
#define VIP_DHUB_BANK1_START_ADDR        (1024*12)
#define VIP_DHUB_BANK2_START_ADDR        (1024*24)
#define VIP_DHUB_BANK3_START_ADDR        (1024*36)
#endif

#define DHUB_BANK0_START_ADDR		(8192*0)
#define DHUB_BANK1_START_ADDR		(8192*1)
#define DHUB_BANK2_START_ADDR		(8192*2)
#define DHUB_BANK3_START_ADDR		(8192*3)
#define DHUB_BANK4_START_ADDR		(8192*4)
#define DHUB_BANK5_START_ADDR		(8192*5)

#else /* for BG2CD */

#define VPP_DHUB_BASE                   (MEMMAP_VPP_DHUB_REG_BASE + RA_vppDhub_dHub0)
#define VPP_HBO_SRAM_BASE               (MEMMAP_VPP_DHUB_REG_BASE + RA_vppDhub_tcm0)
#define VPP_NUM_OF_CHANNELS             (avioDhubChMap_vpp_SPDIF_W+1)

#define AG_DHUB_BASE                    (MEMMAP_AG_DHUB_REG_BASE + RA_agDhub_dHub0)
#define AG_HBO_SRAM_BASE                (MEMMAP_AG_DHUB_REG_BASE + RA_agDhub_tcm0)
#define AG_NUM_OF_CHANNELS              (avioDhubChMap_ag_GFX_R+1)

#define VIP_DHUB_BASE                   (MEMMAP_VIP_DHUB_REG_BASE + RA_vipDhub_dHub0)
#define VIP_HBO_SRAM_BASE               (MEMMAP_VIP_DHUB_REG_BASE + RA_vipDhub_tcm0)
#define VIP_NUM_OF_CHANNELS             (avioDhubChMap_vip_MIC3_W+1)

#define VIP_DHUB_BANK0_START_ADDR        (0)
#define VIP_DHUB_BANK1_START_ADDR        (1024*12)
#define VIP_DHUB_BANK2_START_ADDR        (1024*24)
#define VIP_DHUB_BANK3_START_ADDR        (1024*36)

#define VPP_DHUB_BANK0_START_ADDR        (8192*0)
#define VPP_DHUB_BANK1_START_ADDR        (8192*1)
#define VPP_DHUB_BANK2_START_ADDR        (8192*2)
#define VPP_DHUB_BANK3_START_ADDR        (8192*3)
#define VPP_DHUB_BANK4_START_ADDR        (8192*4)

#define AG_DHUB_BANK0_START_ADDR         (8192*0)
#define AG_DHUB_BANK1_START_ADDR         (8192*1)
#define AG_DHUB_BANK2_START_ADDR         (8192*2)

#endif /* for BG2CD */


typedef struct DHUB_channel_config {
	SIGN32 chanId;
	UNSG32 chanCmdBase;
	UNSG32 chanDataBase;
	SIGN32 chanCmdSize;
	SIGN32 chanDataSize;
	SIGN32 chanMtuSize;
	SIGN32 chanQos;
	SIGN32 chanSelfLoop;
	SIGN32 chanEnable;
} DHUB_channel_config;

extern HDL_dhub2d AG_dhubHandle;
extern HDL_dhub2d VPP_dhubHandle;
#if (BERLIN_CHIP_VERSION >= BERLIN_BG2)
extern HDL_dhub2d VIP_dhubHandle;
#endif
extern DHUB_channel_config  AG_config[];
extern DHUB_channel_config  VPP_config[];
#if (BERLIN_CHIP_VERSION >= BERLIN_BG2)
extern DHUB_channel_config  VIP_config[];
#endif

/******************************************************************************************************************
 *	Function: DhubInitialization
 *	Description: Initialize DHUB .
 *	Parameter : cpuId ------------- cpu ID
 *             dHubBaseAddr -------------  dHub Base address.
 *             hboSramAddr ----- Sram Address for HBO.
 *             pdhubHandle ----- pointer to 2D dhubHandle
 *             dhub_config ----- configuration of AG
 *             numOfChans	 ----- number of channels
 *	Return:		void
******************************************************************************************************************/
void DhubInitialization(SIGN32 cpuId, UNSG32 dHubBaseAddr, UNSG32 hboSramAddr, HDL_dhub2d *pdhubHandle, DHUB_channel_config *dhub_config,SIGN32 numOfChans);

void DhubChannelClear(void *hdl, SIGN32 id, T64b cfgQ[]);
#endif //__API_AVIO_DHUB_H__
