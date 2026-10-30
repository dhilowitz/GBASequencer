/*****************************\
* 	saving.h
*	By staringmonkey
*	Last modified on 9/23/01
\*****************************/

#ifndef SAVING_H
#define SAVING_H

///////////////////Defines////////////////////
#define SRAM 0xe000000	//32k SRAM = 0x0e000000 - 0x0e00ffff

/////////////////Function Prototypes///////////
void SaveByte(u16 offset, u8 value);
void SaveString(u16 offset, char *string);
void SaveInt(u16 offset, int value);
u8 LoadByte(u16 offset);
int LoadInt(u16 offset);
void LoadString(u16 offset, int length, char *string);

///////////////////SaveByte////////////////////
void SaveByte(u16 offset, u8 value)
{
	//Save the value
	*(u8 *)(SRAM + offset) = value;
}

///////////////////SaveString////////////////////
void SaveString(u16 offset, char *string)
{
	//Looping variable
	int i;

	//Loop through until hit string end (NULL)
	for(i=0; i < 32768; i++)
	{
		if(string[i] == '\0')
		{
			
			*(u8 *)(SRAM + offset + i) = string[i];			
			break;
		}

		*(u8 *)(SRAM + offset + i) = string[i];
	}
}

///////////////////SaveInt////////////////////
void SaveInt(u16 offset, int value)
{
	//Looping variable
	int i;
	
	//Temp string
	char string[7];

	sprintf(string, "%d", value);

	for(i=0; i < 32768; i++)
	{
		if(string[i] == '\0')
		{
		*(u8 *)(SRAM + offset + i) = string[i];
			break;
		}

		*(u8 *)(SRAM + offset + i) = string[i];
	}
}

///////////////////LoadByte////////////////////
u8 LoadByte(u16 offset)
{
	//Read the value
	return *(u8 *)(SRAM + offset);
}

///////////////////LoadInt////////////////////
int LoadInt(u16 offset)
{
	//Looping variable
	int  i;
	char string[7];

	for(i = 0; i != 7; i++)
	{
		string[i] = *(u8 *)(SRAM + offset + i);
	}

	return atoi(string);
}

///////////////////LoadString////////////////////
void LoadString(u16 offset, int length, char *string)
{
	//Looping variable
	int i;

	for(i = 0; i != length; i++)
	{
		string[i] = *(u8 *)(SRAM + offset + i);
	}
}

#endif
