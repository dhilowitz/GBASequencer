// gba.h by eloist

#ifndef GBA_HEADER
#define GBA_HEADER

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;

typedef signed char s8;
typedef signed short s16;
typedef signed long s32;

typedef volatile unsigned char vu8;
typedef volatile unsigned short vu16;
typedef volatile unsigned long vu32;

typedef unsigned char byte;
typedef unsigned short hword;
typedef unsigned long word;

#define OAMmem         (u32*)0x7000000
#define VideoBuffer    (u16*)0x6000000
#define OAMdata		   (u16*)0x6100000
#define BGPaletteMem   (u16*)0x5000000
#define OBJPaletteMem  (u16*)0x5000200
		
#define REG_INTERUPT   *(u32*)0x3007FFC
#define REG_DISPCNT    *(u32*)0x4000000
#define REG_DISPCNT_L  *(u16*)0x4000000
#define REG_DISPCNT_H  *(u16*)0x4000002
#define REG_DISPSTAT   *(u16*)0x4000004
#define REG_VCOUNT     *(u16*)0x4000006
#define REG_BG0CNT     *(u16*)0x4000008
#define REG_BG1CNT     *(u16*)0x400000A
#define REG_BG2CNT     *(u16*)0x400000C
#define REG_BG3CNT     *(u16*)0x400000E
#define REG_BG0HOFS    *(u16*)0x4000010
#define REG_BG0VOFS    *(u16*)0x4000012
#define REG_BG1HOFS    *(u16*)0x4000014
#define REG_BG1VOFS    *(u16*)0x4000016
#define REG_BG2HOFS    *(u16*)0x4000018
#define REG_BG2VOFS    *(u16*)0x400001A
#define REG_BG3HOFS    *(u16*)0x400001C
#define REG_BG3VOFS    *(u16*)0x400001E
#define REG_BG2PA      *(u16*)0x4000020
#define REG_BG2PB      *(u16*)0x4000022
#define REG_BG2PC      *(u16*)0x4000024
#define REG_BG2PD      *(u16*)0x4000026
#define REG_BG2X       *(u32*)0x4000028
#define REG_BG2X_L     *(u16*)0x4000028
#define REG_BG2X_H     *(u16*)0x400002A
#define REG_BG2Y       *(u32*)0x400002C
#define REG_BG2Y_L     *(u16*)0x400002C
#define REG_BG2Y_H     *(u16*)0x400002E
#define REG_BG3PA      *(u16*)0x4000030
#define REG_BG3PB      *(u16*)0x4000032
#define REG_BG3PC      *(u16*)0x4000034
#define REG_BG3PD      *(u16*)0x4000036
#define REG_BG3X       *(u32*)0x4000038
#define REG_BG3X_L     *(u16*)0x4000038
#define REG_BG3X_H     *(u16*)0x400003A
#define REG_BG3Y       *(u32*)0x400003C
#define REG_BG3Y_L     *(u16*)0x400003C
#define REG_BG3Y_H     *(u16*)0x400003E
#define REG_WIN0H      *(u16*)0x4000040
#define REG_WIN1H      *(u16*)0x4000042
#define REG_WIN0V      *(u16*)0x4000044
#define REG_WIN1V      *(u16*)0x4000046
#define REG_WININ      *(u16*)0x4000048
#define REG_WINOUT     *(u16*)0x400004A
#define REG_MOSAIC     *(u32*)0x400004C
#define REG_MOSAIC_L   *(u32*)0x400004C
#define REG_MOSAIC_H   *(u32*)0x400004E
#define REG_BLDMOD     *(u16*)0x4000050
#define REG_COLEV      *(u16*)0x4000052
#define REG_COLEY      *(u16*)0x4000054
#define REG_SG10       *(u32*)0x4000060
#define REG_SG10_L     *(u16*)0x4000060
#define REG_SG10_H     *(u16*)0x4000062
#define REG_SG11       *(u16*)0x4000064
#define REG_SG20       *(u16*)0x4000068
#define REG_SG21       *(u16*)0x400006C
#define REG_SG30       *(u32*)0x4000070
#define REG_SG30_L     *(u16*)0x4000070
#define REG_SG30_H     *(u16*)0x4000072
#define REG_SG31       *(u16*)0x4000074
#define REG_SG40       *(u16*)0x4000078
#define REG_SG41       *(u16*)0x400007C
#define REG_SGCNT0     *(u32*)0x4000080
#define REG_SGCNT0_L   *(u16*)0x4000080
#define REG_SGCNT0_H   *(u16*)0x4000082
#define REG_SGCNT1     *(u16*)0x4000084
#define REG_SGBIAS     *(u16*)0x4000088
#define REG_SGWR0      *(u32*)0x4000090
#define REG_SGWR0_L    *(u16*)0x4000090
#define REG_SGWR0_H    *(u16*)0x4000092
#define REG_SGWR1      *(u32*)0x4000094
#define REG_SGWR1_L    *(u16*)0x4000094
#define REG_SGWR1_H    *(u16*)0x4000096
#define REG_SGWR2      *(u32*)0x4000098
#define REG_SGWR2_L    *(u16*)0x4000098
#define REG_SGWR2_H    *(u16*)0x400009A
#define REG_SGWR3      *(u32*)0x400009C
#define REG_SGWR3_L    *(u16*)0x400009C
#define REG_SGWR3_H    *(u16*)0x400009E
#define REG_SGFIF0A    *(u32*)0x40000A0
#define REG_SGFIFOA_L  *(u16*)0x40000A0
#define REG_SGFIFOA_H  *(u16*)0x40000A2
#define REG_SGFIFOB    *(u32*)0x40000A4
#define REG_SGFIFOB_L  *(u16*)0x40000A4
#define REG_SGFIFOB_H  *(u16*)0x40000A6
#define REG_DM0SAD     *(u32*)0x40000B0
#define REG_DM0SAD_L   *(u16*)0x40000B0
#define REG_DM0SAD_H   *(u16*)0x40000B2
#define REG_DM0DAD     *(u32*)0x40000B4
#define REG_DM0DAD_L   *(u16*)0x40000B4
#define REG_DM0DAD_H   *(u16*)0x40000B6
#define REG_DM0CNT     *(u32*)0x40000B8
#define REG_DM0CNT_L   *(u16*)0x40000B8
#define REG_DM0CNT_H   *(u16*)0x40000BA
#define REG_DM1SAD     *(u32*)0x40000BC
#define REG_DM1SAD_L   *(u16*)0x40000BC
#define REG_DM1SAD_H   *(u16*)0x40000BE
#define REG_DM1DAD     *(u32*)0x40000C0
#define REG_DM1DAD_L   *(u16*)0x40000C0
#define REG_DM1DAD_H   *(u16*)0x40000C2
#define REG_DM1CNT     *(u32*)0x40000C4
#define REG_DM1CNT_L   *(u16*)0x40000C4
#define REG_DM1CNT_H   *(u16*)0x40000C6
#define REG_DM2SAD     *(u32*)0x40000C8
#define REG_DM2SAD_L   *(u16*)0x40000C8
#define REG_DM2SAD_H   *(u16*)0x40000CA
#define REG_DM2DAD     *(u32*)0x40000CC
#define REG_DM2DAD_L   *(u16*)0x40000CC
#define REG_DM2DAD_H   *(u16*)0x40000CE
#define REG_DM2CNT     *(u32*)0x40000D0
#define REG_DM2CNT_L   *(u16*)0x40000D0
#define REG_DM2CNT_H   *(u16*)0x40000D2
#define REG_DM3SAD     *(u32*)0x40000D4
#define REG_DM3SAD_L   *(u16*)0x40000D4
#define REG_DM3SAD_H   *(u16*)0x40000D6
#define REG_DM3DAD     *(u32*)0x40000D8
#define REG_DM3DAD_L   *(u16*)0x40000D8
#define REG_DM3DAD_H   *(u16*)0x40000DA
#define REG_DM3CNT     *(u32*)0x40000DC
#define REG_DM3CNT_L   *(u16*)0x40000DC
#define REG_DM3CNT_H   *(u16*)0x40000DE
#define REG_TM0D       *(u16*)0x4000100
#define REG_TM0CNT     *(u16*)0x4000102
#define REG_TM1D       *(u16*)0x4000104
#define REG_TM1CNT     *(u16*)0x4000106
#define REG_TM2D       *(u16*)0x4000108
#define REG_TM2CNT     *(u16*)0x400010A
#define REG_TM3D       *(u16*)0x400010C
#define REG_TM3CNT     *(u16*)0x400010E
#define REG_SCD0       *(u16*)0x4000120
#define REG_SCD1       *(u16*)0x4000122
#define REG_SCD2       *(u16*)0x4000124
#define REG_SCD3       *(u16*)0x4000126
#define REG_SCCNT      *(u32*)0x4000128
#define REG_SCCNT_L    *(u16*)0x4000128
#define REG_SCCNT_H    *(u16*)0x400012A
#define REG_P1         *(u16*)0x4000130
#define REG_P1CNT      *(u16*)0x4000132
#define REG_R          *(u16*)0x4000134
#define REG_HS_CTRL    *(u16*)0x4000140
#define REG_JOYRE      *(u32*)0x4000150
#define REG_JOYRE_L    *(u16*)0x4000150
#define REG_JOYRE_H    *(u16*)0x4000152
#define REG_JOYTR      *(u32*)0x4000154
#define REG_JOYTR_L    *(u16*)0x4000154
#define REG_JOYTR_H    *(u16*)0x4000156
#define REG_JSTAT      *(u32*)0x4000158
#define REG_JSTAT_L    *(u16*)0x4000158
#define REG_JSTAT_H    *(u16*)0x400015A
#define REG_IE         *(u16*)0x4000200
#define REG_IF         *(u16*)0x4000202
#define REG_WSCNT      *(u16*)0x4000204
#define REG_IME        *(u16*)0x4000208
#define REG_PAUSE      *(u16*)0x4000300

////////////////sound channels/////////////
#define SOUNDINIT			0x8000	// makes the sound restart
#define SOUNDDUTY87			0x0000	//87.5% wave duty
#define SOUNDDUTY75			0x0040	//75% wave duty
#define SOUNDDUTY50			0x0080	//50% wave duty
#define SOUNDDUTY25			0x00C0	//25% wave duty

#define SOUND1PLAYONCE		0x4000	// play sound once
#define SOUND1PLAYLOOP		0x0000	// play sound looped
#define SOUND1INIT			0x8000	// makes the sound restart
#define SOUND1SWEEPSHIFTS(n)	n	// number of sweep shifts (0-7)
#define SOUND1SWEEPINC		0x0000	// sweep add (freq increase)
#define SOUND1SWEEPDEC		0x0008	// sweep dec (freq decrese)
#define SOUND1SWEEPTIME(n)	(n<<4)	// time of sweep (0-7)
#define SOUND1ENVSTEPS(n)	(n<<8)	// envelope steps (0-7)
#define SOUND1ENVINC		0x0800	// envellope increase
#define SOUND1ENVDEC		0x0000	// envellope decrease
#define SOUND1ENVINIT(n)	(n<<12) // initial envelope volume (0-15)


#define SOUND2PLAYONCE		0x4000	// play sound once
#define SOUND2PLAYLOOP		0x0000	// play sound looped
#define SOUND2INIT			0x8000	// makes the sound restart
#define SOUND2ENVSTEPS(n)	(n<<8)	// envelope steps (0-7)
#define SOUND2ENVINC		0x0800	// envellope increase
#define SOUND2ENVDEC		0x0000	// envellope decrease
#define SOUND2ENVINIT(n)	(n<<12) // initial envelope volume (0-15)



#define SOUND3BANK32		0x0000	// Use two banks of 32 steps each
#define SOUND3BANK64		0x0020	// Use one bank of 64 steps
#define SOUND3SETBANK0		0x0000	// Bank to play 0 or 1 (non set bank is written to)
#define SOUND3SETBANK1		0x0040
#define SOUND3PLAY			0x0080	// Output sound

#define SOUND3OUTPUT0		0x0000	// Mute output
#define SOUND3OUTPUT1		0x2000	// Output unmodified
#define SOUND3OUTPUT12		0x4000	// Output 1/2 
#define SOUND3OUTPUT14		0x6000	// Output 1/4 
#define SOUND3OUTPUT34		0x8000  // Output 3/4

#define SOUND3PLAYONCE		0x4000	// Play sound once
#define SOUND3PLAYLOOP		0x0000	// Play sound looped
#define SOUND3INIT			0x8000	// Makes the sound restart


#define SOUND4PLAYONCE		0x4000	// play sound once
#define SOUND4PLAYLOOP		0x0000	// play sound looped
#define SOUND4INIT			0x8000	// makes the sound restart
#define SOUND4ENVSTEPS(n)	(n<<8)	// envelope steps (0-7)
#define SOUND4ENVINC		0x0800	// envellope increase
#define SOUND4ENVDEC		0x0000	// envellope decrease
#define SOUND4ENVINIT(n)	(n<<12) // initial envelope volume (0-15)


#define SOUND4STEPS7		0x0004
#define SOUND4STEPS15		0x0000
#define SOUND4PLAYONCE		0x4000
#define SOUND4PLAYLOOP		0x0000
#define SOUND4INIT			0x8000

      #define REG_SOUND1CNT   *(vu32*)0x4000060	//sound 1
#define REG_SOUND1CNT_L *(vu16*)0x4000060	//
#define REG_SOUND1CNT_H *(vu16*)0x4000062	//
#define REG_SOUND1CNT_X *(vu16*)0x4000064	//

#define REG_SOUND2CNT_L *(vu16*)0x4000068		//sound 2 lenght & wave duty
#define REG_SOUND2CNT_H *(vu16*)0x400006C		//sound 2 frequency+loop+reset

#define REG_SG30       *(vu32*)0x4000070		//???
#define REG_SOUND3CNT  *(vu32*)0x4000070		//???
#define REG_SG30_L     *(vu16*)0x4000070		//???
#define REG_SOUND3CNT_L *(vu16*)0x4000070	//???
#define REG_SG30_H     *(vu16*)0x4000072		//???
#define REG_SOUND3CNT_H *(vu16*)0x4000072	//???
#define REG_SG31       *(vu16*)0x4000074		//???
#define REG_SOUND3CNT_X *(vu16*)0x4000074	//???

#define REG_SOUND4CNT_L *(vu16*)0x4000078		//???
#define REG_SOUND4CNT_H *(vu16*)0x400007C		//???

#define REG_SGCNT0     *(vu32*)0x4000080		
#define REG_SGCNT0_L   *(vu16*)0x4000080		
#define REG_SOUNDCNT   *(vu32*)0x4000080
#define REG_SOUNDCNT_L *(vu16*)0x4000080		//DMG sound control

#define REG_SGCNT0_H   *(vu16*)0x4000082		
#define REG_SOUNDCNT_H *(vu16*)0x4000082		//Direct sound control

#define REG_SGCNT1     *(vu16*)0x4000084		
#define REG_SOUNDCNT_X *(vu16*)0x4000084	    //Extended sound control

#define REG_SGBIAS     *(vu16*)0x4000088		
#define REG_SOUNDBIAS  *(vu16*)0x4000088		//bit rate+sound bias

#define REG_WAVE_RAM0  *(vu32*)0x4000090		//???
#define REG_SGWR0_L    *(vu16*)0x4000090		//???
#define REG_SGWR0_H    *(vu16*)0x4000092		//???
#define REG_WAVE_RAM1  *(vu32*)0x4000094		//???
#define REG_SGWR1_L    *(vu16*)0x4000094		//???
#define REG_SGWR1_H    *(vu16*)0x4000096		//???
#define REG_WAVE_RAM2  *(vu32*)0x4000098		//???
#define REG_SGWR2_L    *(vu16*)0x4000098		//???
#define REG_SGWR2_H    *(vu16*)0x400009A		//???
#define REG_WAVE_RAM3  *(vu32*)0x400009C		//???
#define REG_SGWR3_L    *(vu16*)0x400009C		//???
#define REG_SGWR3_H    *(vu16*)0x400009E		//???


#define REG_DMA1SAD     *(u32*)0x40000BC	//DMA1 Source Address
#define REG_DMA1SAD_L   *(u16*)0x40000BC	//DMA1 Source Address Low Value
#define REG_DMA1SAD_H   *(u16*)0x40000BE	//DMA1 Source Address High Value
#define REG_DMA1DAD     *(u32*)0x40000C0	//DMA1 Desination Address
#define REG_DMA1DAD_L   *(u16*)0x40000C0	//DMA1 Destination Address Low Value
#define REG_DMA1DAD_H   *(u16*)0x40000C2	//DMA1 Destination Address High Value
#define REG_DMA1CNT     *(u32*)0x40000C4	//DMA1 Control (Amount)
#define REG_DMA1CNT_L   *(u16*)0x40000C4	//DMA1 Control Low Value
#define REG_DMA1CNT_H   *(u16*)0x40000C6	//DMA1 Control High Value

#define REG_DMA2SAD     *(u32*)0x40000C8	//DMA2 Source Address
#define REG_DMA2SAD_L   *(u16*)0x40000C8	//DMA2 Source Address Low Value
#define REG_DMA2SAD_H   *(u16*)0x40000CA	//DMA2 Source Address High Value
#define REG_DMA2DAD     *(u32*)0x40000CC	//DMA2 Destination Address
#define REG_DMA2DAD_L   *(u16*)0x40000CC	//DMA2 Destination Address Low Value
#define REG_DMA2DAD_H   *(u16*)0x40000CE	//DMA2 Destination Address High Value
#define REG_DMA2CNT     *(u32*)0x40000D0	//DMA2 Control (Amount)
#define REG_DMA2CNT_L   *(u16*)0x40000D0	//DMA2 Control Low Value
#define REG_DMA2CNT_H   *(u16*)0x40000D2	//DMA2 Control High Value

#define REG_DMA3SAD     *(u32*)0x40000D4	//DMA3 Source Address
#define REG_DMA3SAD_L   *(u16*)0x40000D4	//DMA3 Source Address Low Value
#define REG_DMA3SAD_H   *(u16*)0x40000D6	//DMA3 Source Address High Value
#define REG_DMA3DAD     *(u32*)0x40000D8	//DMA3 Destination Address
#define REG_DMA3DAD_L   *(u16*)0x40000D8	//DMA3 Destination Address Low Value
#define REG_DMA3DAD_H   *(u16*)0x40000DA	//DMA3 Destination Address High Value
#define REG_DMA3CNT     *(u32*)0x40000DC	//DMA3 Control (Amount)
#define REG_DMA3CNT_L   *(u16*)0x40000DC	//DMA3 Control Low Value
#define REG_DMA3CNT_H   *(u16*)0x40000DE	//DMA3 Control High Value

#define REG_TM0CNT      *(u32*)0x4000100	//Timer 0
#define REG_TM0CNT_L	*(u16*)0x4000100	//Timer 0 count value
#define REG_TM0CNT_H    *(u16*)0x4000102	//Timer 0 Control

#define REG_TM1CNT     *(u32*)0x4000104		//Timer 2
#define REG_TM1CNT_L   *(u16*)0x4000104		//Timer 2 count value
#define REG_TM1CNT_H   *(u16*)0x4000106		//Timer 2 control


#endif
