/*****************************\
* 	text.h
*	By staringmonkey
*	Last modified on 9/06/01
\*****************************/

#ifndef TEXT_H
#define TEXT_H

///////////////////Necessary Includes//////////////////
#include <string.h>		//String functions

///////////////////Memory Divisions/////////////////////
u16* BG0 = (u16*)0x6000000;		//Memory for BG0's map
u16* BG1 = (u16*)0x6004000;		//Memory for BG1's map
u16* BG2 = (u16*)0x6008000;		//Memory for BG2's map
u16* BG3 = (u16*)0x600C000;		//Memory for BG3's map

//////////////////////Globals////////////////////////
#define SPACE 0x00

///////////////////Function Prototypes/////////
void WriteText(int textXLeft, int textXRight, int textYTop, int textYBottom, char *message, int layer);
void WriteNum(int textXLeft, int textXRight, int textYTop, int textYBottom, int number, int layer);
void ClearText(int layer);

///////////////////WriteText/////////////////
void WriteText(int textXLeft, int textXRight, int textYTop, int textYBottom, char *message, int layer)
{
	int currentLetter;				//Current letter
	int i,j = 0;					//Looping variables
	int xOff = 0;					//X Column
	int yOff = 0;					//Y Row
	int length = 0;					//Array length
	int findSpace;					//Used to find last space before line-end
	int newString[600];				//Hex version of the string
	int test;
	u16 *destination;				//What layer to draw to

	switch(layer)
	{
		case 0: destination = BG0; break;	//Draw to BG0
		case 1: destination = BG1; break;	//Draw to BG1???
		case 2: destination = BG2; break;	//Draw to BG2???
		case 3: destination = BG3; break;	//Draw to BG3???
	}

	while(message[j] != '_')
	{
		switch(message[j])
		{
			case 'A': newString[j] = 0x01; break;
			case 'B': newString[j] = 0x02; break;
			case 'C': newString[j] = 0x03; break;
			case 'D': newString[j] = 0x04; break;
			case 'E': newString[j] = 0x05; break;
			case 'F': newString[j] = 0x06; break;
			case 'G': newString[j] = 0x07; break;
			case 'H': newString[j] = 0x08; break;
			case 'I': newString[j] = 0x09; break;
			case 'J': newString[j] = 0x0A; break;
			case 'K': newString[j] = 0x0B; break;
			case 'L': newString[j] = 0x0C; break;
			case 'M': newString[j] = 0x0D; break;
			case 'N': newString[j] = 0x0E; break;
			case 'O': newString[j] = 0x0F; break;
			case 'P': newString[j] = 0x10; break;
			case 'Q': newString[j] = 0x11; break;
			case 'R': newString[j] = 0x12; break;
			case 'S': newString[j] = 0x13; break;
			case 'T': newString[j] = 0x14; break;
			case 'U': newString[j] = 0x15; break;
			case 'V': newString[j] = 0x16; break;
			case 'W': newString[j] = 0x17; break;
			case 'X': newString[j] = 0x18; break;
			case 'Y': newString[j] = 0x19; break;
			case 'Z': newString[j] = 0x1A; break;
			case 'a': newString[j] = 0x1B; break;
			case 'b': newString[j] = 0x1C; break;
			case 'c': newString[j] = 0x1D; break;
			case 'd': newString[j] = 0x1E; break;
			case 'e': newString[j] = 0x1F; break;
			case 'f': newString[j] = 0x20; break;
			case 'g': newString[j] = 0x21; break;
			case 'h': newString[j] = 0x22; break;
			case 'i': newString[j] = 0x23; break;
			case 'j': newString[j] = 0x24; break;
			case 'k': newString[j] = 0x25; break;
			case 'l': newString[j] = 0x26; break;
			case 'm': newString[j] = 0x27; break;
			case 'n': newString[j] = 0x28; break;
			case 'o': newString[j] = 0x29; break;
			case 'p': newString[j] = 0x2A; break;
			case 'q': newString[j] = 0x2B; break;
			case 'r': newString[j] = 0x2C; break;
			case 's': newString[j] = 0x2D; break;
			case 't': newString[j] = 0x2E; break;
			case 'u': newString[j] = 0x2F; break;
			case 'v': newString[j] = 0x30; break;
			case 'w': newString[j] = 0x31; break;
			case 'x': newString[j] = 0x32; break;
			case 'y': newString[j] = 0x33; break;
			case 'z': newString[j] = 0x34; break;
			case '0': newString[j] = 0x35; break;
			case '1': newString[j] = 0x36; break;
			case '2': newString[j] = 0x37; break;
			case '3': newString[j] = 0x38; break;
			case '4': newString[j] = 0x39; break;
			case '5': newString[j] = 0x3A; break;
			case '6': newString[j] = 0x3B; break;
			case '7': newString[j] = 0x3C; break;
			case '8': newString[j] = 0x3D; break;
			case '9': newString[j] = 0x3E; break;
			case '!': newString[j] = 0x3F; break;
			//case '"': newString[j] = 0x40; break;
			case '#': newString[j] = 0x41; break;
			case '$': newString[j] = 0x42; break;
			case '%': newString[j] = 0x43; break;
			case '&': newString[j] = 0x44; break;
			//case ''': newString[j] = 0x45; break;
			case '(': newString[j] = 0x46; break;
			case ')': newString[j] = 0x47; break;
			case '*': newString[j] = 0x48; break;
			case '+': newString[j] = 0x49; break;
			case ',': newString[j] = 0x4A; break;
			case '-': newString[j] = 0x4B; break;
			case '.': newString[j] = 0x4C; break;
			case '/': newString[j] = 0x4D; break;
			case ':': newString[j] = 0x4E; break;
			case ';': newString[j] = 0x4F; break;
			case '<': newString[j] = 0x50; break;
			case '=': newString[j] = 0x51; break;
			case '>': newString[j] = 0x52; break;
			case '?': newString[j] = 0x53; break;


			default:  newString[j] = SPACE; break;
		}

		j++;
	}

	length = j;

	for(currentLetter = 0; currentLetter < length; currentLetter++)
	{
		if(xOff >= textXRight - textXLeft)
		{
			if(newString[currentLetter - 1] == SPACE)
			{
				yOff++;					//Increase Y Line
				xOff = 0;				//Set x back to zero
			}
			else if(newString[currentLetter] == SPACE)
			{
				yOff++;					//Increase Y Line
				xOff = -1;				//Set x to -1
			}
			else
			{
				findSpace = currentLetter;	//Start at currentLetter

				while(newString[findSpace] != SPACE)
				{
					findSpace--;			//Search backwords
				}
				for(i = findSpace + 1; i < currentLetter; i++)
				{
					test = xOff - (currentLetter - i);		//Get current x location
					destination[(textYTop+yOff)*32 + textXLeft + test] = SPACE;
				}
				
				yOff++;
				xOff = 0;
				currentLetter = findSpace + 1;
			}
		}
				
		destination[(textYTop+yOff)*32 + textXLeft + xOff] = newString[currentLetter];
		xOff++;
	}
}

//////////////////WriteNum//////////////////
void WriteNum(int textXLeft, int textXRight, int textYTop, int textYBottom, int number, int layer)
{
	char numString[17] = "";			//Create a string to hold the conversion

	sprintf(numString, "%d", number);	//Convert
	numString[16] = '_';				//Add END character

	WriteText(textXLeft, textXRight, textYTop, textYBottom, numString, layer);
}

///////////////////ClearText///////////////////
void ClearText(int layer)
{
	//Looping variables
	int x,y;

	//What layer to clear
	u16 *destination;

	switch(layer)
	{
		case 0: destination = BG0; break;	//Draw to BG0
		case 1: destination = BG1; break;	//Draw to BG1
		case 2: destination = BG2; break;	//Draw to BG2
		case 3: destination = BG3; break;	//Draw to BG3
	}

	for(y=0;y<32;y++)
	{
		for(x=0;x<32;x++)
		{
			destination[y*30 + x] = SPACE;
		}
	}
}

#endif
