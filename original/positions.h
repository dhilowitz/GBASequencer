// These are the positions of the selection fields for
// all the different user interface screens.
int field_positions[5][3][4] = {
	{					//Screen 1
	{32,28,224,146},	//Field 1
	{4,6,106,22},		//Field 2
	{108,6,162,22}		//Field 3
	},
	{					//Screen 2
	{32,28,224,146},	//Field 1
	{2,4,120,26}		//Field 2
	},
	{					//Screen 3
	{32,28,224,146},
	{2,4,120,26}
	},
	{					//Screen 4
	{6,27,224,108},
	{50,14,166,28}
	},
	{					//Screen 5
	{30,32,120,44},
	{30,42,120,54}
	}
	};

// These are the positions of the background refresh regions
// each one corresponds to a (hopefully) useful are of the
// background. The three parameters are (screen, index number,  
// and x1, y1, x2, y2).
int background_regions[5][20][4] = {
	{					//Screen 1
	{0,0,240,160},			//Entire Screen
	{6,29,6+25,29+117},		//Piano Keyboard
	{31,29,31+13,29+117},	//Column 1
	{43,29,43+13,29+117},	//Column 2
	{55,29,55+13,29+117},	//Column 3
	{67,29,67+13,29+117},	//Column 4
	{79,29,79+13,29+117},	//Column 5
	{91,29,91+13,29+117},	//Column 6
	{103,29,103+13,29+117},	//Column 7
	{115,29,115+13,29+117},	//Column 8
	{127,29,127+13,29+117},	//Column 9
	{139,29,139+13,29+117},	//Column 10
	{151,29,151+13,29+117},	//Column 11
	{163,29,163+13,29+117},	//Column 12
	{175,29,175+13,29+117},	//Column 13
	{187,29,187+13,29+117},	//Column 14
	{199,29,199+13,29+117},	//Column 15
	{211,29,211+13,29+117},	//Column 16
	{31,29+117,211+113,29+117+10},	//The bottom scrollbar
	{211+13,29,211+13+10,29+117}, // The right scrollbar
	},
	{					//Screen 2
	{32,28,224,146},	//Field 1
	{2,4,120,26}		//Field 2
	},
	{					//Screen 3
	{32,28,224,146},
	{2,4,120,26}
	},
	{					//Screen 4
		{6,27,224,108},
		{50,14,166,28}
	},
	{					//Screen 5
		{30,32,120,44},
		{30,42,120,54}
	}
	};