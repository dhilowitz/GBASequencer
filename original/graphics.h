/******************************\
* 	graphics.h
*	By staringmonkey
*	Last modified on 8/21/01
\******************************/

#ifndef GRAPHICS_H
#define GRAPHICS_H

//////////////////Includes///////////////////
#include <math.h>			//abs(), rand(), etc.

////////////////////Globals//////////////////
#define SCREENWIDTH 240		//Screen width in pixels
#define SCREENHEIGHT 160	//Screen height in pixels

////////////////////Macros//////////////////
#define RGB16(r,g,b)  ((r)+(g<<5)+(b<<10))  //this converts a color value to 15 bit BGR value used by GBA		
#define BGR(b,g,r) ((r)+((g)<<5)+((b)<<10)) //you can have 0-31 levels of each red,green,blue component.

///////////////////Function Prototypes/////////
void FillBoxRGB(int x_start, int y_start, int x_stop, int y_stop, int r, int g, int b);
void FillBox(int x_start, int y_start, int x_stop, int y_stop, u16 color);
void FillScreenRGB(int r, int g, int b);
void FillScreen(u16 color);
void PlotPixelRGB(int x, int y, int r, int g, int b);
void PlotPixel(int x, int y, u16 color);
//void DMAClearScreen(void);
void LineRGB(int x1, int y1, int x2, int y2, int r, int g, int b);
void Line(int x1, int y1, int x2, int y2, u16 color);
void CircleRGB(int xCenter, int yCenter, int radius, int r, int g, int b);
void Circle(int xCenter, int yCenter, int radius, int color);

//////////////////FillBox////////////////////
void FillBoxRGB(int x_start, int y_start, int x_stop, int y_stop, int r, int g, int b)
{
	int x_current;
	int	y_current;

	for(x_current = x_start; x_current<x_stop; x_current++)   //loop through all x
	{
		for(y_current = y_start; y_current<y_stop; y_current++)   //loop through all x
		{

			videoBuffer[x_current+y_current*SCREENWIDTH] = RGB16(r,g,b);

		}
	}

}

//////////////////FillBox////////////////////
void FillBox(int x_start, int y_start, int x_stop, int y_stop, u16 color)
{
	int x_current;
	int	y_current;

	for(x_current = x_start; x_current<x_stop; x_current++)   //loop through all x
	{
		for(y_current = y_start; y_current<y_stop; y_current++)   //loop through all x
		{

			videoBuffer[x_current+y_current*SCREENWIDTH] = color;

		}
	}

}

//////////////////FillScreen////////////////////
void FillScreenRGB(int r, int g, int b)
{
	int x_current;
	int	y_current;

	for(x_current = 0; x_current<SCREENWIDTH; x_current++)   //loop through all x
	{
		for(y_current = 0; y_current<SCREENHEIGHT; y_current++)   //loop through all y
		{

			videoBuffer[x_current+y_current*SCREENWIDTH] = RGB16(r,g,b);

		}
	}

}

//////////////////FillScreen////////////////////
void FillScreen(u16 color)
{
	int x_current;
	int	y_current;

	for(x_current = 0; x_current<SCREENWIDTH; x_current++)   //loop through all x
	{
		for(y_current = 0; y_current<SCREENHEIGHT; y_current++)   //loop through all y
		{

			videoBuffer[x_current+y_current*SCREENWIDTH] = color;

		}
	}

}

/////////////PlotPixel////////////////////
void PlotPixelRGB(int x, int y, int r, int g, int b)
{
	videoBuffer[x+y*SCREENWIDTH] = RGB16(r,g,b);
}

/////////////PlotPixel////////////////////
void PlotPixel(int x, int y, u16 color)
{
	videoBuffer[x+y*SCREENWIDTH] = color;
}

//////////////////////DMAClearScreen/////////////////
/*void DMAClearScreen(void)
{
	REG_DMA3SAD = 0x08010000;		//Source Address
	REG_DMA3DAD = 0x06000000;		//Destination Address
	REG_DMA3CNT = 0x84002580;		//9600 Bytes?
}
*/
/////////////////////Line//////////////////////////
//An implementation of the Bresenham Line algorithm
void LineRGB(int x1, int y1, int x2, int y2, int r, int g, int b)
{
	//Variables
	int i, deltax, deltay, numpixels;
	int d, dinc1, dinc2;
	int x, xinc1, xinc2;
	int y, yinc1, yinc2;

	//Calculate deltaX and deltaY
	deltax = abs(x2 - x1);
	deltay = abs(y2 - y1);

	//Init vars
	if(deltax >= deltay)
	{
		//If x is independent variable
		numpixels = deltax + 1;
		d = (2 * deltay) - deltax;
		dinc1 = deltay << 1;
		dinc2 = (deltay - deltax) << 1;
		xinc1 = 1;
		xinc2 = 1;
		yinc1 = 0;
		yinc2 = 1;
	}
	else
	{
		//If y is independant variable
		numpixels = deltay + 1;
		d = (2 * deltax) - deltay;
		dinc1 = deltax << 1;
		dinc2 = (deltax - deltay) << 1;
		xinc1 = 0;
		xinc2 = 1;
		yinc1 = 1;
		yinc2 = 1;
	}

	//Move the right direction
	if(x1 > x2)
	{
		xinc1 = -xinc1;
		xinc2 = -xinc2;
	}
	if(y1 > y2)
	{
		yinc1 = -yinc1;
		yinc2 = -yinc2;
	}

	x = x1;
	y = y1;

	//Draw the pixels
	for(i = 1; i < numpixels; i++)
	{
		PlotPixelRGB(x, y, r, g, b);

		if(d < 0)
		{
			d = d + dinc1;
			x = x + xinc1;
			y = y + yinc1;
		}
		else
		{
			d = d + dinc2;
			x = x + xinc2;
			y = y + yinc2;
		}
	}
}

/////////////////////Line//////////////////////////
//An implementation of the Bresenham Line algorithm
void Line(int x1, int y1, int x2, int y2, u16 color)
{
	//Variables
	int i, deltax, deltay, numpixels;
	int d, dinc1, dinc2;
	int x, xinc1, xinc2;
	int y, yinc1, yinc2;

	//Calculate deltaX and deltaY
	deltax = abs(x2 - x1);
	deltay = abs(y2 - y1);

	//Init vars
	if(deltax >= deltay)
	{
		//If x is independent variable
		numpixels = deltax + 1;
		d = (2 * deltay) - deltax;
		dinc1 = deltay << 1;
		dinc2 = (deltay - deltax) << 1;
		xinc1 = 1;
		xinc2 = 1;
		yinc1 = 0;
		yinc2 = 1;
	}
	else
	{
		//If y is independant variable
		numpixels = deltay + 1;
		d = (2 * deltax) - deltay;
		dinc1 = deltax << 1;
		dinc2 = (deltax - deltay) << 1;
		xinc1 = 0;
		xinc2 = 1;
		yinc1 = 1;
		yinc2 = 1;
	}

	//Move the right direction
	if(x1 > x2)
	{
		xinc1 = -xinc1;
		xinc2 = -xinc2;
	}
	if(y1 > y2)
	{
		yinc1 = -yinc1;
		yinc2 = -yinc2;
	}

	x = x1;
	y = y1;

	//Draw the pixels
	for(i = 1; i < numpixels; i++)
	{
		PlotPixel(x, y, color);

		if(d < 0)
		{
			d = d + dinc1;
			x = x + xinc1;
			y = y + yinc1;
		}
		else
		{
			d = d + dinc2;
			x = x + xinc2;
			y = y + yinc2;
		}
	}
}

///////////////////CircleRGB/////////////////////////
//An implementation of the Bresenham Circle algorithm
void CircleRGB(int xCenter, int yCenter, int radius, int r, int g, int b)
{
	int x = 0;
	int y = radius;
	int p = 3 - 2 * radius;
	while (x <= y)
	{ 
		PlotPixelRGB(xCenter + x, yCenter + y, r, g, b);
		PlotPixelRGB(xCenter - x, yCenter + y, r, g, b);
		PlotPixelRGB(xCenter + x, yCenter - y, r, g, b);
		PlotPixelRGB(xCenter - x, yCenter - y, r, g, b);
		PlotPixelRGB(xCenter + y, yCenter + x, r, g, b);
		PlotPixelRGB(xCenter - y, yCenter + x, r, g, b);
		PlotPixelRGB(xCenter + y, yCenter - x, r, g, b);
		PlotPixelRGB(xCenter - y, yCenter - x, r, g, b);

		if (p < 0)
		{
			p += 4 * x++ + 6;
		}
		else
		{
			p += 4 * (x++ - y--) + 10;
		}
	}
}

///////////////////CircleRGB/////////////////////////
//An implementation of the Bresenham Circle algorithm
void Circle(int xCenter, int yCenter, int radius, int color)
{
	int x = 0;
	int y = radius;
	int p = 3 - 2 * radius;
	while (x <= y)
	{ 
		PlotPixel(xCenter + x, yCenter + y, color);
		PlotPixel(xCenter - x, yCenter + y, color);
		PlotPixel(xCenter + x, yCenter - y, color);
		PlotPixel(xCenter - x, yCenter - y, color);
		PlotPixel(xCenter + y, yCenter + x, color);
		PlotPixel(xCenter - y, yCenter + x, color);
		PlotPixel(xCenter + y, yCenter - x, color);
		PlotPixel(xCenter - y, yCenter - x, color);

		if (p < 0)
		{
			p += 4 * x++ + 6;
		}
		else
		{
			p += 4 * (x++ - y--) + 10;
		}
	}
}

#endif
