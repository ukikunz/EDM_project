/*****************************************************************************
 *  Module for Microchip Graphics Library
 *  GOL Layer 
 *  Button
 *****************************************************************************
 * FileName:        Chart.c
 * Dependencies:    Chart.h
 * Processor:       PIC24, PIC32
 * Compiler:       	MPLAB C30 Version 3.00, C32
 * Linker:          MPLAB LINK30, LINK32
 * Company:         Microchip Technology Incorporated
 *
 * Software License Agreement
 *
 * Copyright � 2008 Microchip Technology Inc.  All rights reserved.
 * Microchip licenses to you the right to use, modify, copy and distribute
 * Software only when embedded on a Microchip microcontroller or digital
 * signal controller, which is integrated into your product or third party
 * product (pursuant to the sublicense terms in the accompanying license
 * agreement).  
 *
 * You should refer to the license agreement accompanying this Software
 * for additional information regarding your rights and obligations.
 *
 * SOFTWARE AND DOCUMENTATION ARE PROVIDED �AS IS� WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY WARRANTY
 * OF MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR
 * PURPOSE. IN NO EVENT SHALL MICROCHIP OR ITS LICENSORS BE LIABLE OR
 * OBLIGATED UNDER CONTRACT, NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION,
 * BREACH OF WARRANTY, OR OTHER LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT
 * DAMAGES OR EXPENSES INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL,
 * INDIRECT, PUNITIVE OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA,
 * COST OF PROCUREMENT OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY
 * CLAIMS BY THIRD PARTIES (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF),
 * OR OTHER SIMILAR COSTS.
 *
 * Author               Date        Comment
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Paolo A. Tamayo
 * Anton Alkhimenok		4/8/07	Version 1.0 release
 *****************************************************************************/

#include "Graphics\Graphics.h"

#ifdef USE_CHART

// internal functions and macros
WORD word2xchar(WORD pSmple, XCHAR *xcharArray, WORD cnt); 
void GetCirclePoint(SHORT radius, SHORT angle, SHORT *x, SHORT *y);
void DrawSector(SHORT cx, SHORT cy, SHORT radius, SHORT angleFrom, SHORT angleTo, WORD outLineColor);
WORD GetColorShade(WORD color, BYTE shade); 
WORD ChParseShowData(DATASERIES *pData);
DATASERIES *ChGetNextShowData(DATASERIES *pData);

// array used to define the default colors used to draw the bars or sectors of the chart
const WORD ChartVarClr[16] = {	CH_CLR0, CH_CLR1, CH_CLR2, CH_CLR3,
								CH_CLR4, CH_CLR5, CH_CLR6, CH_CLR7,
								CH_CLR8, CH_CLR9, CH_CLR10,CH_CLR11,
								CH_CLR12,CH_CLR13,CH_CLR14,CH_CLR15};


/*********************************************************************
* Function: CHART  *ChCreate(WORD ID, SHORT left, SHORT top, SHORT right, 
*                              SHORT bottom, WORD state ,CHARTDATA *pData, 
*                              GOL_SCHEME *pScheme)
*
*
* Notes: Creates a CHART object and adds it to the current active list.
*        If the creation is successful, the pointer to the created Object 
*        is returned. If not successful, NULL is returned.
*
********************************************************************/
CHART  *ChCreate( WORD ID, SHORT left, SHORT top, SHORT right, SHORT bottom, 
				  WORD state, DATASERIES *pData, CHARTPARAM *pParam, GOL_SCHEME *pScheme)
{
	CHART *pCh = NULL;
	
	pCh = malloc(sizeof(CHART));
	
	if (pCh == NULL) 
		return NULL;
	
	pCh->hdr.ID         = ID;			// unique id assigned for referencing
	pCh->hdr.pNxtObj    = NULL;			// initialize pointer to NULL
	pCh->hdr.type       = OBJ_CHART;	// set object type
	pCh->hdr.left       = left;       	// left position
	pCh->hdr.top        = top;       	// top position
	pCh->hdr.right      = right;       	// right position
	pCh->hdr.bottom     = bottom;      	// bottom position
    pCh->hdr.state   	= state; 	    // state
	
	if (pParam != NULL) {
		pCh->prm.pTitle		= pParam->pTitle;
		pCh->prm.pSmplLabel	= pParam->pSmplLabel;
		pCh->prm.pValLabel	= pParam->pValLabel;
		pCh->prm.smplStart	= pParam->smplStart;
		pCh->prm.smplEnd	= pParam->smplEnd;
		pCh->prm.valMax		= pParam->valMax;
		pCh->prm.valMin		= pParam->valMin;
		pCh->prm.pColor		= pParam->pColor;
		pCh->prm.pTitleFont = pParam->pTitleFont;
	} else {
		pCh->prm.pTitle		= NULL;
		pCh->prm.pSmplLabel	= NULL;
		pCh->prm.pValLabel	= NULL;
		pCh->prm.smplStart	= 0;
		pCh->prm.smplEnd	= 0;
		pCh->prm.valMax		= 0;
		pCh->prm.valMin		= 0;
		// use the default color table 
		pCh->prm.pColor		= (WORD*)ChartVarClr;
		pCh->prm.pTitleFont = NULL;
	}

	pCh->pChData	    = pData;		// assign the chart data 

	// check if how variables have SHOW_DATA flag set
	pCh->prm.seriesCount = ChParseShowData(pData);

	// Set the color scheme to be used
	if (pScheme == NULL) {
		pCh->hdr.pGolScheme = _pDefaultGolScheme; 
		pCh->prm.pTitleFont = _pDefaultGolScheme; 
	} else {
		pCh->hdr.pGolScheme = (GOL_SCHEME *)pScheme; 	
		pCh->prm.pTitleFont = pCh->hdr.pGolScheme->pFont; 
	}	

    GOLAddObject((OBJ_HEADER*) pCh);
    
	return pCh;
}

/*********************************************************************
* Function: WORD ChTranslateMsg(CHART *pCh, GOL_MSG *pMsg)
*
* Notes: Evaluates the message if the object will be affected by the 
*		 message or not.
*
********************************************************************/
WORD ChTranslateMsg(CHART *pCh, GOL_MSG *pMsg)
{
	// Evaluate if the message is for the static text
    // Check if disabled first
	if (GetState(pCh, CH_DISABLED))
        return OBJ_MSG_INVALID;

#ifdef  USE_TOUCHSCREEN
    if(pMsg->type == TYPE_TOUCHSCREEN) {
		// Check if it falls in static text control borders
		if( (pCh->hdr.left     < pMsg->param1) &&
	  	    (pCh->hdr.right    > pMsg->param1) &&
	   	    (pCh->hdr.top      < pMsg->param2) &&
	   	    (pCh->hdr.bottom   > pMsg->param2) ) {
			       	
    		return CH_MSG_SELECTED;
        }
	}
#endif	

	return OBJ_MSG_INVALID;
}

////////////////////////////////////////////////
// internal functions
////////////////////////////////////////////////
DATASERIES *ChGetNextShowData(DATASERIES *pData)
{
	DATASERIES *pVar = pData;
	
	// find the next data series that will be shown
	while(pVar->show != SHOW_DATA) {
		if ((pVar = pVar->pNextData) == NULL)
		return NULL;
	}
	return pVar; 
}

WORD ChParseShowData(DATASERIES *pData)
{
	DATASERIES *pParse;
	WORD sCnt = 0;
	
	if (pData != NULL)
	{
		pParse = pData;
		while(pParse != NULL) {
			if (pParse->show == SHOW_DATA)
				sCnt++;
			pParse = pParse->pNextData;	
		}
	}
	return sCnt;
}	
	
		
WORD GetLongestNameLength(CHART *pCh) {

	WORD     temp = 0;
	DATASERIES *pVar;

   	if (!GetState(pCh, CH_LEGEND)) 
   		return 0;

	// find the data series with the longest name
    pVar = pCh->pChData;
	
   	while (pVar) {
		// check if the data series is to be shown
	   	if (pVar->show == SHOW_DATA) {
       		if (temp < GetTextWidth((XCHAR*)pVar->pSData, pCh->hdr.pGolScheme->pFont))
       			temp = GetTextWidth((XCHAR*)pVar->pSData, pCh->hdr.pGolScheme->pFont);
       	}
       	pVar = pVar->pNextData;
    }
   	return temp;
}   		

WORD word2xchar(WORD pSmple, XCHAR *xcharArray, WORD cnt) {
	
	WORD j, z;
	XCHAR xTemp;
	static XCHAR *pXchar;
	
	pXchar = xcharArray;

	// this implements sprintf(strVal, "%d", temp); faster
	// note that this is just for values >= 0, while sprintf covers negative values.
	j = 1;
	z = pSmple;

	pXchar = &(*xcharArray) + (cnt-j);
	do {
		*pXchar = (z%10) + '0';
		*pXchar--;
		if ((z/= 10) == 0)
			break;
		j++;
	} while(j <= cnt);
	return j;
}

WORD GetColorShade(WORD color, BYTE shade) {
	WORD newColor, ave;
	BYTE rgb[3];
	
	rgb[0] = ((color >> 11)<<3);   // red
	rgb[1] = ((color >>  5)<<2);   // green
	rgb[2] = ((color      )<<3);   // blue
	BYTE i;
	
	// get the average
	ave = (rgb[0] + rgb[1] + rgb[2])/3;
	
	for (i = 0; i < 3; i++) {
		if (rgb[i] > 128) 
			rgb[i] = rgb[i]-((rgb[i]-128)>>(shade)); 
		else 
			rgb[i] = rgb[i]+((128-rgb[i])>>(shade)); 
	}

	newColor = RGB565CONVERT(rgb[0], rgb[1], rgb[2]);
	return newColor;
}	

/*********************************************************************
* Function: WORD ChDraw(CHART *pCh)
*
*
* Notes: This is the state machine to draw the button.
*
********************************************************************/
#define STR_CHAR_CNT  			11
#define DCLR_STR_CHAR_CNT  		(STR_CHAR_CNT+1)

WORD ChDraw(CHART *pCh)
{
	
typedef enum {
	REMOVE,
	FRAME_DRAW_PREP,
	FRAME_DRAW,
	CHECK_CHART_TYPE,

// BAR type states
	GRID_PREP,
	SAMPLE_GRID_DRAW1,
	VALUE_GRID_DRAW1,
	SAMPLE_GRID_DRAW2,
	VALUE_GRID_DRAW2,
	
	VALUE_GRID_3D_DRAW,
	
	TITLE_LABEL_DRAW_SET,	
	TITLE_LABEL_DRAW_RUN,	
	
	SAMPLE_LABEL_DRAW_SET,
	SAMPLE_LABEL_DRAW_RUN,
	VALUE_LABEL_DRAW_INIT,
	VALUE_LABEL_DRAW_SET,
	VALUE_LABEL_DRAW_RUN,

	XAXIS_LABEL_DRAW_RUN,
	XAXIS_LABEL_DRAW_SET,
	YAXIS_LABEL_DRAW_RUN,
	YAXIS_LABEL_DRAW_SET,

	//LEGEND_DRAW_SET,
	LEGEND_DRAW_BOX,
	LEGEND_DRAW_RUN,
	LEGEND_DRAW_UPDATE_VAR,

	DATA_DRAW_INIT,
	DATA_DRAW_SET,
	BAR_DATA_DRAW,
	BAR_DATA_DRAW_CHECK,

	BAR_DATA_DRAW_3D_PREP,
	BAR_DATA_DRAW_3D_LOOP_1,
	BAR_DATA_DRAW_3D_LOOP_2,

	BAR_DATA_DRAW_3D_OUTLINE1,
	BAR_DATA_DRAW_3D_OUTLINE2,
	BAR_DATA_DRAW_3D_OUTLINE3,
	BAR_DATA_DRAW_3D_OUTLINE4,
	BAR_DATA_DRAW_3D_OUTLINE5,
	BAR_DATA_DRAW_3D_OUTLINE6,

	PIE_DONUT_HOLE_DRAW,

	BAR_DATA_DRAW_VALUE,
	BAR_DATA_DRAW_VALUE_RUN,
	
// PIE type states
	PIE_PREP,
	PIE_DRAW_OUTLINE1,
	PIE_DRAW_OUTLINE2,
	PIE_DRAW_SECTOR,
	PIE_DRAW_SECTOR_LOOP,
	PIE_DRAW_SECTOR_LOOP_CONTINUE,
	PIE_DRAW_SECTOR_LOOP_CREATE_STRINGS,
	PIE_DRAW_SECTOR_LOOP_STRINGS_RUN,
} CH_DRAW_STATES;

static XCHAR tempXchar[2] = {'B',0};
static XCHAR tempStr[DCLR_STR_CHAR_CNT] = {'0','0','0','0','0','0','0','0','0','0',0};

static CH_DRAW_STATES state = REMOVE;
static WORD x, y, z, xStart, yStart, ctr, ctry, samplesMax, temp;
static splDelta, valDelta;
static WORD barWidth, barDepth, chart3DDepth;

static DATASERIES *pVar;
static WORD *pSmple, datSmpl;
static XCHAR *pXcharTemp;
static DWORD dTemp;
static SHORT varCtr, pieX, pieY;
	   DWORD dPercent;
	   XCHAR xtemp;
	   WORD  j, k, h, i;
	   
    if(IsDeviceBusy())
        return 0;

    switch(state) {

        case REMOVE:

            if(IsDeviceBusy())
                return 0;  

	        if (GetState(pCh,CH_HIDE)) {  				      	// Hide the Chart (remove from screen)
       	        SetColor(pCh->hdr.pGolScheme->CommonBkColor);
       	        Bar(pCh->hdr.left, pCh->hdr.top, pCh->hdr.right, pCh->hdr.bottom);
		        return 1;
		    }

   			SetLineThickness(NORMAL_LINE);
			SetLineType(SOLID_LINE);			
			  			 
			// check if we only need to refresh the data on the chart
			if (GetState(pCh, CH_DRAW_DATA)) {
				// this is only performed when refreshing data in the chart
   	    	       	    	    
   		        // erase the current contents
   	    	    SetColor(pCh->hdr.pGolScheme->CommonBkColor);
   	    	    
				if (GetState(pCh, CH_BAR)) {
					if (GetState(pCh, CH_3D_ENABLE)) {
				
						if (GetState(pCh, CH_BAR_HOR) == CH_BAR_HOR) {
							if ((GetState(pCh, CH_LEGEND)) || (GetState(pCh, CH_VALUE))) {
				   		        Bar(xStart-GetTextWidth(tempXchar, pCh->hdr.pGolScheme->pFont)-
				   		        		  (GetTextWidth(tempXchar, pCh->hdr.pGolScheme->pFont)>>1), 
				   		        	yStart-((ChGetSampleRange(pCh)+1)*splDelta)-chart3DDepth, 
				   		        	xStart+((CH_YGRIDCOUNT-1)*valDelta)+chart3DDepth+
				   		        	(GetTextWidth(tempXchar, pCh->hdr.pGolScheme->pFont)*3),
				   		        	yStart);
			   		        } else {
				   		        Bar(xStart-GetTextWidth(tempXchar, pCh->hdr.pGolScheme->pFont)-
				   		        		  (GetTextWidth(tempXchar, pCh->hdr.pGolScheme->pFont)>>1), 
				   		        	yStart-((ChGetSampleRange(pCh)+1)*splDelta)-chart3DDepth, 
				   		        	xStart+((CH_YGRIDCOUNT-1)*valDelta)+chart3DDepth+CH_MARGIN,
				   		        	yStart);
			   		        }
						} else {
			   		        Bar(xStart, yStart-((CH_YGRIDCOUNT-1)*valDelta)-chart3DDepth-(GetTextHeight(pCh->hdr.pGolScheme->pFont)>>1), 
			   		        	xStart+splDelta*(ChGetSampleRange(pCh)+1)+chart3DDepth,
			   		        	yStart+GetTextHeight(pCh->hdr.pGolScheme->pFont));
			   		    }
			   		} else {
						if (GetState(pCh, CH_BAR_HOR) == CH_BAR_HOR) {
							if ((GetState(pCh, CH_LEGEND)) || (GetState(pCh, CH_VALUE))) {
				   		        Bar(xStart-GetTextWidth(tempXchar, pCh->hdr.pGolScheme->pFont)-
					   		              (GetTextWidth(tempXchar, pCh->hdr.pGolScheme->pFont)>>1),
				   		        	yStart-((ChGetSampleRange(pCh)+1)*splDelta), 
					   		        xStart+((CH_YGRIDCOUNT-1)*valDelta)+
					   		        	(GetTextWidth(tempXchar, pCh->hdr.pGolScheme->pFont)*4),
					   		        yStart);
							} else {
				   		        Bar(xStart-GetTextWidth(tempXchar, pCh->hdr.pGolScheme->pFont)-
					   		              (GetTextWidth(tempXchar, pCh->hdr.pGolScheme->pFont)>>1),
				   		        	yStart-((ChGetSampleRange(pCh)+1)*splDelta), 
					   		        xStart+((CH_YGRIDCOUNT-1)*valDelta)+CH_MARGIN,
					   		        yStart);
				   		    }
						} else {
			   		        Bar(xStart, yStart-((CH_YGRIDCOUNT-1)*valDelta)-(GetTextHeight(pCh->hdr.pGolScheme->pFont)), 
			   		        	xStart+splDelta*(ChGetSampleRange(pCh)+1),
			   		        	yStart+GetTextHeight(pCh->hdr.pGolScheme->pFont));
						}
					}
				} else {
					// erase the current pie chart drawn
					Bar(pCh->hdr.left+GOL_EMBOSS_SIZE, 
						ctry-z-GetTextHeight(pCh->hdr.pGolScheme->pFont), 
						ctr+z+(GetTextWidth(tempXchar, pCh->hdr.pGolScheme->pFont)*6), 
						pCh->hdr.bottom-CH_MARGIN);
						//ctry+z+GetTextHeight(pCh->hdr.pGolScheme->pFont));
				}

				// check the type of chart
				if (GetState(pCh, CH_BAR)) {
					state = GRID_PREP;
					goto chrt_grid_prep;
				} else {
					state = PIE_PREP;
					goto chrt_pie_prep;
				}
			}
           	state = FRAME_DRAW_PREP;
			
/*========================================================================*/
//					  Draw the frame 
/*========================================================================*/

        case FRAME_DRAW_PREP:

			// check how many data series do we need to display
			pCh->prm.seriesCount = ChParseShowData(pCh->pChData);
			
		    // set up the frame drawing
	    	GOLPanelDraw(pCh->hdr.left, pCh->hdr.top, pCh->hdr.right,  	
		   				 pCh->hdr.bottom, 0, pCh->hdr.pGolScheme->CommonBkColor, 
		   				 pCh->hdr.pGolScheme->EmbossLtColor, pCh->hdr.pGolScheme->EmbossDkColor,  
			  			 NULL, 1);


           	state = FRAME_DRAW;

        case FRAME_DRAW:
            if(!GOLPanelDrawTsk()){
                return 0;
            }
			
       		state = TITLE_LABEL_DRAW_SET;

/*========================================================================*/
//					  Draw the Chart Title
/*========================================================================*/
chrt_title_label_draw_set:
		case TITLE_LABEL_DRAW_SET:
		
			// find the location of the title
			MoveTo(	pCh->hdr.left+
						((pCh->hdr.right+pCh->hdr.left-GetTextWidth((XCHAR*)pCh->prm.pTitle, pCh->prm.pTitleFont))>>1), 
					pCh->hdr.top+CH_MARGIN);
        	state = TITLE_LABEL_DRAW_RUN;
        	
		case TITLE_LABEL_DRAW_RUN:
			// Set the font
			SetFont(pCh->prm.pTitleFont);

			// NOTE: we use the emboss dark color here to draw the chart title.
       		SetColor(pCh->hdr.pGolScheme->EmbossDkColor);

			if(!OutText(pCh->prm.pTitle))
            	return 0;			

			// check if legend will be drawn            
        	if(GetState(pCh, CH_LEGEND)) { 
	            // position the x and y points to the start of the first variable
				temp = GetLongestNameLength(pCh);
	            	
            	x = pCh->hdr.right-(CH_MARGIN<<1)-temp-GetTextHeight(pCh->hdr.pGolScheme->pFont);
				y = ((pCh->hdr.bottom+pCh->hdr.top)>>1) - ((pCh->prm.seriesCount*GetTextHeight(pCh->hdr.pGolScheme->pFont))>>1);
				
	            // initialize the variable counter for the legend drawing	
	            temp  = 0;
	            pVar = pCh->pChData;
				state = LEGEND_DRAW_BOX;
			} else {
				// legend will not be drawn, go to data drawing next
				state = CHECK_CHART_TYPE;
				goto chrt_check_chart_type;
			}

/*========================================================================*/
//					  Draw the Legend
/*========================================================================*/
chrt_draw_legend:
		case LEGEND_DRAW_BOX:
			// check if we will be showing this data series
			if (ChGetShowSeriesStatus(pVar) == SHOW_DATA) {

				SetColor(*(&(*pCh->prm.pColor)+temp));
				if ((ChGetShowSeriesCount(pCh)==1) && (GetState(pCh, CH_PIE))) {
				} else {
					Bar(x,y+(GetTextHeight(pCh->hdr.pGolScheme->pFont)>>2), 
						x+(GetTextHeight(pCh->hdr.pGolScheme->pFont)>>1), 
						y+(GetTextHeight(pCh->hdr.pGolScheme->pFont)-(GetTextHeight(pCh->hdr.pGolScheme->pFont)>>2)));
				}
				MoveTo(x+2+(GetTextHeight(pCh->hdr.pGolScheme->pFont)>>1), y); 
				state = LEGEND_DRAW_RUN;
			} else {
				state = LEGEND_DRAW_UPDATE_VAR;
				goto chrt_draw_legend_update;
			}
			
		case LEGEND_DRAW_RUN:

        	SetColor(pCh->hdr.pGolScheme->TextColor1);
        	SetFont(pCh->hdr.pGolScheme->pFont);

			if(!OutText(pVar->pSData))
            	return 0;	
            // increment the variable counter
   	        temp++;	
   	        if (temp == ChGetShowSeriesCount(pCh)) {
	        	state = CHECK_CHART_TYPE;
	   	        goto chrt_check_chart_type;
			} else 
	        	state = LEGEND_DRAW_UPDATE_VAR;
			   	        
chrt_draw_legend_update:			
		case LEGEND_DRAW_UPDATE_VAR:
			// update the data series pointer and y position 
			if (ChGetShowSeriesStatus(pVar) == SHOW_DATA) 
        		y += GetTextHeight(pCh->hdr.pGolScheme->pFont);
        	pVar = pVar->pNextData;
        	state = LEGEND_DRAW_BOX;
        	goto chrt_draw_legend;

chrt_check_chart_type:			
		case CHECK_CHART_TYPE:	
	
			if (GetState(pCh, CH_BAR)) {
	   			state = GRID_PREP;
			} 
			else if (GetState(pCh, CH_PIE)) {
				state = PIE_PREP;
				goto chrt_pie_prep;
			} else {
        		state = REMOVE;	
            	return 1;				
				
			}  
         
/**************************************************************************/
// 					BAR CHART states 
/**************************************************************************/

/*========================================================================*/
//					  Draw the grids 
/*========================================================================*/

chrt_grid_prep:
        case GRID_PREP:
        	/* NOTE: X or Y Grid Labels - label for each division in the x or y axis
        			 X or Y Axis Labels - label to name the x or y axis. Text is 
        			 					  user given in the CHART structure, 
        			 					  CHARTPARAM member.
        	*/
        
        	// count the number of characters needed for the axis label that 
        	// represents the value of the samples (or bars)
        	
        	// get the width of one character
        	temp = GetTextWidth((XCHAR*)tempXchar, pCh->hdr.pGolScheme->pFont); 

	        if (GetState(pCh, CH_BAR_HOR) == CH_BAR_HOR) {
	        	// if in the horizontal orientation width will only be 
	        	// max of 2 characters (1, 2, 3...10, 11... or A, B, C...)
	        	if ((ChGetSampleEnd(pCh)-ChGetSampleStart(pCh)) > 9)
	        		y = 2;
	        	else
	        		y = 1;	
	        } else {
				if (GetState(pCh, CH_PERCENT)) {
					// include in the computation the space that will be occupied by '%' sign
	        		y = 4;
				} else {
	        		x = ChGetValueRange(pCh);
		        	y = 1;
		        	// count the number of characters needed
	    	    	while(x/=10)
	        			++y;
	        	}
	        }
	        
        	// estimate the space that will be occupied by the y grid labels
        	temp = temp*y;	
        	
        	// get x starting position
        	xStart = CH_MARGIN + temp + pCh->hdr.left;
        	// adjust x start to accomodate Y axis label
        	xStart += (GetTextHeight(pCh->hdr.pGolScheme->pFont)+
        			   (GetTextWidth(tempXchar, pCh->hdr.pGolScheme->pFont)>>1));  

        	// now get y starting position
        	yStart = pCh->hdr.bottom - ((GetTextHeight(pCh->hdr.pGolScheme->pFont)<<1) + CH_MARGIN);

			// =======================================================================
			// Sample delta (splDelta) and Value delta (valDelta) will depend if the 
			// chart is drawn horizontally or vertically.
			// =======================================================================
        	
        	// find the variable with the longest name
        	// to add space for the names of variables in the legend
			temp = GetLongestNameLength(pCh);

        	// get sample delta (space between data) and value delta (defines the grid for the value) 
			// check first if we compute in the x-axis or y-axis
        	if (GetState(pCh, CH_BAR_HOR) == CH_BAR_HOR) {

	        	// if horizontally drawn sample delta is not affected by the legend
	        	splDelta = (yStart-(pCh->hdr.top+CH_MARGIN+GetTextHeight(pCh->prm.pTitleFont))) /   \
	        					   (ChGetSampleRange(pCh)+1);

	        	// adjust space for displayed values
		   		if (GetState(pCh, CH_VALUE)) {
		   			temp += (GetTextWidth(tempXchar, pCh->hdr.pGolScheme->pFont)*4);
		   		}
		   		
	        	// get the value delta 
	    	   	valDelta = (pCh->hdr.right-xStart-((CH_MARGIN*3)+temp+GetTextHeight(pCh->hdr.pGolScheme->pFont)))/(CH_YGRIDCOUNT-1);
			} else {
		   		if(GetState(pCh, CH_LEGEND)) { 
	    	    	splDelta = (pCh->hdr.right-xStart-((CH_MARGIN<<2)+temp+GetTextHeight(pCh->hdr.pGolScheme->pFont)))/	\
	        				   (ChGetSampleRange(pCh)+1);
				} else {
    		    	splDelta = (pCh->hdr.right-xStart-(CH_MARGIN<<2))/(ChGetSampleRange(pCh)+1);
    		    }
	        	// get the value delta 
	        	valDelta = (yStart-(pCh->hdr.top+CH_MARGIN+GetTextHeight(pCh->prm.pTitleFont)+ \
	        									           GetTextHeight(pCh->hdr.pGolScheme->pFont))) / (CH_YGRIDCOUNT-1);
    		}		    

			if (GetState(pCh, CH_3D_ENABLE)) {
				// adjust the splDelta due to 3D effect space
				splDelta -= ((splDelta/(2+ChGetShowSeriesCount(pCh)))>>1);
			}

       	
    	    // initilize the counter for the sample axis drawing
	       	temp = ChGetSampleRange(pCh)+2;
	       	x = xStart;
	       	y = yStart;
	      	state = SAMPLE_GRID_DRAW1;
	      	
        case SAMPLE_GRID_DRAW1:
        	// draw the small grids on the x-axis
			while (temp) {
				SetColor(pCh->hdr.pGolScheme->Color0);
        		if (GetState(pCh, CH_BAR_HOR) == CH_BAR_HOR) {
					Bar(x, y, x+3, y+1); 	
					y -= splDelta;
	        	} else {
					Bar(x, y, x+1, y+3); 	
					x += splDelta;
				}
				--temp; 
			}

			// get the bar width (bar here refers to the sample data represented as bars, where the height
			// of the bar represents the value of the sample)
			barWidth = splDelta/(2+ChGetShowSeriesCount(pCh));

        	temp  = CH_YGRIDCOUNT;
       		if (GetState(pCh, CH_BAR_HOR) == CH_BAR_HOR) 
        		y = yStart;
        	else	
        		x = xStart;

			if (GetState(pCh, CH_3D_ENABLE)) {
				// limit the 3-D depth to only 12 pixels.
				chart3DDepth = (barWidth > 12) ? 12:barWidth;
				// set the bar 3-D depth.
				barDepth = chart3DDepth >> 1;
				state = VALUE_GRID_3D_DRAW;
			} else { 
        		state = VALUE_GRID_DRAW1;
        		goto chrt_value_grid_draw1;
			}

        case VALUE_GRID_3D_DRAW:
        	// draw the 3D grids on the value-axis
			while(temp) {
				SetColor(pCh->hdr.pGolScheme->Color0);
	       		if (GetState(pCh, CH_BAR_HOR) == CH_BAR_HOR) {
					Bar(x+(chart3DDepth), y-(chart3DDepth)-(splDelta*(ChGetSampleRange(pCh)+1)), 
						x+(chart3DDepth), y-(chart3DDepth)); 
					x += valDelta;	
				} else {
					Bar(x+(chart3DDepth), y-(chart3DDepth), 
						x+(chart3DDepth)+(splDelta*(ChGetSampleRange(pCh)+1)), y-(chart3DDepth)); 
					y -= valDelta;	
				}
			 	--temp;
			}
        	temp  = CH_YGRIDCOUNT;
        	x = xStart;
        	y = yStart;
        	state = VALUE_GRID_DRAW1;
 
chrt_value_grid_draw1: 
        case VALUE_GRID_DRAW1:
        	// draw the grids on the y-axis
			if (GetState(pCh, CH_3D_ENABLE)) {
				// just draw the first one to define the x-axis
				SetColor(pCh->hdr.pGolScheme->Color0);
	       		if (GetState(pCh, CH_BAR_HOR) == CH_BAR_HOR) {
					Bar(x, y-(splDelta*(ChGetSampleRange(pCh)+1)), x, y); 
				} else {	
					Bar(x, y, x+(splDelta*(ChGetSampleRange(pCh)+1)), y); 
				}
			} else {
				while(temp) {
					SetColor(pCh->hdr.pGolScheme->Color0);
		       		if (GetState(pCh, CH_BAR_HOR) == CH_BAR_HOR) {
						Bar(x, y-(splDelta*(ChGetSampleRange(pCh)+1)), x, y); 
						x += valDelta;	
					} else {
						Bar(x, y, x+(splDelta*(ChGetSampleRange(pCh)+1)), y); 
						y -= valDelta;	
					}
				 	--temp;
				}
			}

       		if (GetState(pCh, CH_BAR_HOR) == CH_BAR_HOR) {
				x = xStart;
			} else {
				y = yStart;
			}

			if (GetState(pCh, CH_3D_ENABLE)) {
				temp = CH_YGRIDCOUNT + 1;
    	    	state = VALUE_GRID_DRAW2;
    	    } else {
				temp = 2;
	        	state = SAMPLE_GRID_DRAW2;
	        	goto chrt_xgrid_draw2;
        	}

        case VALUE_GRID_DRAW2:

        	// draw the 3-D effect on the y axis of the chart
			SetColor(pCh->hdr.pGolScheme->Color0);
			while(temp) {
				if (temp == (CH_YGRIDCOUNT + 1)) {
		       		if (GetState(pCh, CH_BAR_HOR) == CH_BAR_HOR) {
						Line(x,y-(splDelta*(ChGetSampleRange(pCh)+1)),
							 x+chart3DDepth,y-(splDelta*(ChGetSampleRange(pCh)+1))-chart3DDepth);
			       	} else {
						Line(x+(splDelta*(ChGetSampleRange(pCh)+1)),y,
							 x+(chart3DDepth)+(splDelta*(ChGetSampleRange(pCh)+1)),y-chart3DDepth);
			       	}
	
					--temp;
					continue;
				}
				else	
					Line(x, y, x+chart3DDepth, y-chart3DDepth); 

	       		if (GetState(pCh, CH_BAR_HOR) == CH_BAR_HOR) {
					x += valDelta;	
				} else {
					y -= valDelta;	
				}
					
				--temp;
			}
			temp = 3;
       		if (GetState(pCh, CH_BAR_HOR) == CH_BAR_HOR) {
				x = xStart;
			} else {
				y = yStart;
			}			
			
			state = SAMPLE_GRID_DRAW2;

chrt_xgrid_draw2:
        case SAMPLE_GRID_DRAW2:
        	// draw the left and right edges of the chart
			while(temp) {
				if (GetState(pCh, CH_3D_ENABLE)) {
	
					if (temp == 3) {
			       		if (GetState(pCh, CH_BAR_HOR) == CH_BAR_HOR) {
							Bar(x,y,x+((CH_YGRIDCOUNT-1)*valDelta),y);
						} else {
							Bar(x,y-((CH_YGRIDCOUNT-1)*valDelta),x,y);
						}
						--temp;
						continue;
					}
					else	
			       		if (GetState(pCh, CH_BAR_HOR) == CH_BAR_HOR) {
							Bar(x+chart3DDepth, y-chart3DDepth, x+chart3DDepth+((CH_YGRIDCOUNT-1)*valDelta),y-chart3DDepth);
						} else {
							Bar(x+chart3DDepth, y-((CH_YGRIDCOUNT-1)*valDelta)-chart3DDepth,x+chart3DDepth,y-chart3DDepth);
						}
				} else {
		       		if (GetState(pCh, CH_BAR_HOR) == CH_BAR_HOR) {
						Bar(x, y,x+((CH_YGRIDCOUNT-1)*valDelta),y);
					} else {
						Bar(x, y-((CH_YGRIDCOUNT-1)*valDelta),x,y);
					}
				}
	       		if (GetState(pCh, CH_BAR_HOR) == CH_BAR_HOR) {
					y -= (splDelta*(ChGetSampleRange(pCh)+1));
				} else {
					x += (splDelta*(ChGetSampleRange(pCh)+1));
				}
				--temp;
			}
       		if (GetState(pCh, CH_BAR_HOR) == CH_BAR_HOR) 
				y = yStart;
			else
				x = xStart;
			ctr = ChGetSampleStart(pCh);
			SetFont(pCh->hdr.pGolScheme->pFont);
        	state = SAMPLE_LABEL_DRAW_SET;
        	
/*========================================================================*/
//					  Draw the Sample Grid labels
/*========================================================================*/
chrt_sample_label_draw_set:
        case SAMPLE_LABEL_DRAW_SET:
			// for data only redraw we need to refresh the x-axis labels to indicate
			// the correct sample points being shown
			
			if(GetState(pCh, CH_NUMERIC)) {
				j = word2xchar(ctr, tempStr, STR_CHAR_CNT);
			} else {
				// note that we will only have A-Z labels.
				tempStr[STR_CHAR_CNT-1] = 'A' + (ctr-1);
				j = 1;
			}

			temp = GetTextWidth((&tempStr[STR_CHAR_CNT-j]), pCh->hdr.pGolScheme->pFont);
       		if (GetState(pCh, CH_BAR_HOR) == CH_BAR_HOR) { 
				MoveTo(x-temp, y-((splDelta+GetTextHeight(pCh->hdr.pGolScheme->pFont)>>1)));
			} else {
				MoveTo(x+((splDelta-temp)>>1), y);
			}
        	state = SAMPLE_LABEL_DRAW_RUN;
        	
        case SAMPLE_LABEL_DRAW_RUN:
        	// draw the x axis grid numbers
        	SetColor(pCh->hdr.pGolScheme->TextColor0);
			if(!OutText(&tempStr[STR_CHAR_CNT-j]))
            	return 0;	
       		if (GetState(pCh, CH_BAR_HOR) == CH_BAR_HOR) { 
	            y -= splDelta;
	        } else {
	            x += splDelta;
	        }
            ctr++;	
            if (ctr > ChGetSampleEnd(pCh)) {	
	            // check if we only need to redraw the data
				if (GetState(pCh, CH_DRAW_DATA)) {
    		    	state = DATA_DRAW_INIT;
					goto chrt_data_draw_init;
				} else {

		       		if (GetState(pCh, CH_BAR_HOR) == CH_BAR_HOR) { 
			       		state = YAXIS_LABEL_DRAW_SET;
			       		goto chrt_yaxis_label_draw_set;
		    	   	} else {
		       			state = XAXIS_LABEL_DRAW_SET;
		       		}
	        	}
			} else {	
				temp = x;
				goto chrt_sample_label_draw_set;
			}
			
/*========================================================================*/
//					  Draw the X - Axis labels
/*========================================================================*/
chrt_xaxis_label_draw_set:        
        case XAXIS_LABEL_DRAW_SET:

			// find the location of the label 
       		if (GetState(pCh, CH_BAR_HOR) == CH_BAR_HOR) { 
	       		pXcharTemp = pCh->prm.pValLabel;
				temp = ((valDelta*(CH_YGRIDCOUNT-1)) - GetTextWidth(pXcharTemp, pCh->hdr.pGolScheme->pFont))>>1;
			} else {
	       		pXcharTemp = pCh->prm.pSmplLabel;
				temp = ((splDelta*(ChGetSampleRange(pCh)+1))-GetTextWidth(pXcharTemp, pCh->hdr.pGolScheme->pFont))>>1;
			}
			MoveTo(	xStart+temp, yStart+GetTextHeight(pCh->hdr.pGolScheme->pFont)); 
			state = XAXIS_LABEL_DRAW_RUN;

        case XAXIS_LABEL_DRAW_RUN:

        	SetColor(pCh->hdr.pGolScheme->TextColor1);
			if(!OutText(pXcharTemp))
   	        	return 0;	

	   		if (GetState(pCh, CH_BAR_HOR) == CH_BAR_HOR) { 
	       		state = DATA_DRAW_INIT;
    	   		goto chrt_data_draw_init;
				
			} else {
				state = VALUE_LABEL_DRAW_INIT;
			}
			
chrt_value_label_draw_init:
        case VALUE_LABEL_DRAW_INIT:
        
			ctr = 0;
	        // x is used here to represent change in value grid labels
			// Note that we add a multiplier of 10 to compute for round off errors.
			// It will not be perfect but approximations is better unless you work with 
			// figures divisible by 5.
			if (GetState(pCh, CH_PERCENT)) {
				// Scaling of the labels is included here
   	    		x = ChGetPercentRange(pCh)*100/(CH_YGRIDCOUNT-1);
			} else {
				x = (ChGetValueRange(pCh)*100)/(CH_YGRIDCOUNT-1);
    	    }        	
    	    
			// compute for round off error, the adjustment for the factor 100 is done in 
			// conversion of the integer to string below.
			if ((x%10) < 5)
				x += 10;			

        	state = VALUE_LABEL_DRAW_SET;

/*========================================================================*/
//					  Draw the Value Grid labels
/*========================================================================*/
chrt_value_label_draw_set:
        case VALUE_LABEL_DRAW_SET:

			// note that the adjustment of the 100 factor is done here.
			if (GetState(pCh, CH_PERCENT)) {
				// add the percent sign on the label
				tempStr[STR_CHAR_CNT-1] = '%';
				// we have a plus 1 here since we add '%' sign already
				j = 1 + word2xchar((ctr*x/100)+ChGetPercentMin(pCh), tempStr, STR_CHAR_CNT-1);
			} else {
				j = word2xchar((ctr*x/100)+ChGetValueMin(pCh), tempStr, STR_CHAR_CNT);
			}
       		if (GetState(pCh, CH_BAR_HOR) == CH_BAR_HOR) { 
				temp = GetTextWidth((&tempStr[STR_CHAR_CNT-j]), pCh->hdr.pGolScheme->pFont);
				MoveTo(	xStart+((valDelta*ctr)-(temp>>1)),yStart);
			} else {
				temp = GetTextHeight(pCh->hdr.pGolScheme->pFont);
				MoveTo(	xStart-GetTextWidth((&tempStr[STR_CHAR_CNT-j]), pCh->hdr.pGolScheme->pFont), 
						yStart-((valDelta*ctr)+(temp>>1)));
			}
        	state = VALUE_LABEL_DRAW_RUN;
        	
        case VALUE_LABEL_DRAW_RUN:
        	// draw the y axis grid numbers

        	SetColor(pCh->hdr.pGolScheme->TextColor0);
			if(!OutText(&tempStr[STR_CHAR_CNT-j]))
            	return 0;	
            ctr++;	
            if (ctr >= CH_YGRIDCOUNT) {	
				state = XAXIS_LABEL_DRAW_SET;
			} else {	
				goto chrt_value_label_draw_set;
			}
			
		    if (GetState(pCh, CH_BAR_HOR) == CH_BAR_HOR) { 
				state = XAXIS_LABEL_DRAW_SET;
			    goto chrt_xaxis_label_draw_set;
			} else {
				state = YAXIS_LABEL_DRAW_SET;
			}

/*========================================================================*/
//					  Draw the Y - Axis labels
/*========================================================================*/
chrt_yaxis_label_draw_set:
        case YAXIS_LABEL_DRAW_SET:

			// find the location of the label 
       		if (GetState(pCh, CH_BAR_HOR) == CH_BAR_HOR) { 
	       		pXcharTemp = pCh->prm.pSmplLabel;
				temp = ((splDelta*(ChGetSampleRange(pCh)+1))-
							GetTextWidth(pXcharTemp, pCh->hdr.pGolScheme->pFont))>>1;
			} else {
	       		pXcharTemp = pCh->prm.pValLabel;
	       		temp = ((valDelta*CH_YGRIDCOUNT)-GetTextWidth(pXcharTemp, pCh->hdr.pGolScheme->pFont))>>1;
			}
			MoveTo( xStart - GetTextWidth((&tempStr[STR_CHAR_CNT-j]), pCh->hdr.pGolScheme->pFont)-
							 (GetTextWidth(tempXchar, pCh->hdr.pGolScheme->pFont)>>1)-
							 GetTextHeight(pCh->hdr.pGolScheme->pFont),
					yStart - temp);
					
			state = YAXIS_LABEL_DRAW_RUN;

        case YAXIS_LABEL_DRAW_RUN:

			// set the orientation of text to vertical 
			SetFontOrientation(ORIENT_VER);		
        	SetColor(pCh->hdr.pGolScheme->TextColor1);
			if(!OutText(pXcharTemp))
            	return 0;	
			// place back the orientation of text to horizontal
			SetFontOrientation(ORIENT_HOR);		

			if (GetState(pCh, CH_BAR_HOR) == CH_BAR_HOR) { 
				state = VALUE_LABEL_DRAW_INIT;	
				goto chrt_value_label_draw_init;
			} else {
	       		state = DATA_DRAW_INIT;
			}

/*========================================================================*/
//					  Draw the bars representing the data/samples
/*========================================================================*/
chrt_data_draw_init:
		case DATA_DRAW_INIT:
			/* pSmple - points to the sample data of the variables
			   ctr - the sample counter
			   varCtr - variable counter
			   temp - the width of the bars
			   dTemp - the height of the bars
			   x or y - the location of the first bar per sample. For single variables
			       there will be only one bar per sample. x for vertical bars and y for horizontal bars.
			*/			
			ctr = 0;
			temp = splDelta/(2+ChGetShowSeriesCount(pCh));			// <---- note this! this can be used to calculate the minimum size limit of the chart
			 
			state = DATA_DRAW_SET;

chrt_data_draw_set:
		case DATA_DRAW_SET:
		
			varCtr = 0;			
			
			pVar = ChGetNextShowData(pCh->pChData);
           
			// set the position to start bar drawing
			// x and y here are used in horizontal drawing and vertical drawing as the variable
			// that refers to the position of the bar being drawn. 
			if (GetState(pCh, CH_BAR_HOR) == CH_BAR_HOR) { 
				y = yStart-(ctr*splDelta)-temp;
			} else {
				x = xStart+(ctr*splDelta)+temp;
			}
			state = BAR_DATA_DRAW;

chrt_data_draw:
		case BAR_DATA_DRAW:

			// get sample data from current variable
			pSmple = (&(*pVar->pData) + (ctr+ChGetSampleStart(pCh)-1));

			// calculate the total value of the samples to compute the percentages
			if (GetState(pCh, CH_PERCENT)) {
				j = ChGetSampleStart(pCh);
				samplesMax = 0;

				while (j <= (ChGetSampleRange(pCh)+ChGetSampleStart(pCh))) {
					samplesMax += (*(&(*pVar->pData) + (j-1)));
					j++;
				}					

				// Get the percentage of the sample
				dTemp = ((DWORD)(*pSmple)*100)/samplesMax;
				
				// check if scaling is needed				
				if (ChGetPercentMax(pCh) <= dTemp)
					dTemp = ChGetPercentRange(pCh);
				else {
					if (dTemp < ChGetPercentMin(pCh))
						dTemp = 0;
					else 
						dTemp = dTemp - ChGetPercentMin(pCh);
				}						
				dTemp = ((DWORD)(dTemp)*(valDelta*(CH_YGRIDCOUNT-1)))/(ChGetPercentRange(pCh));
		
			} else {

				// get the height of the current bar to draw
				// this should be adjusted to the min and max set values 
				if (ChGetValueMax(pCh) <= (*pSmple)) {
					dTemp = ChGetValueRange(pCh);
				} else {
					if ((*pSmple) < ChGetValueMin(pCh))
						dTemp = 0;
					else 
						dTemp = (*pSmple) - ChGetValueMin(pCh);	
				}
				dTemp = ((DWORD)(dTemp)*(valDelta*(CH_YGRIDCOUNT-1))/ChGetValueRange(pCh));
			}		

			// draw the front side of the bar
			SetColor(*(&(*pCh->prm.pColor)+varCtr));
			
			
			if (GetState(pCh, CH_3D_ENABLE)) {
	
				if (GetState(pCh, CH_BAR_HOR) == CH_BAR_HOR) { 
		        	Bar(xStart+1, y-1-(temp*(varCtr+1)), xStart+1+dTemp, y-1-(temp*varCtr));
		        } else {
		        	Bar(x+1+1+(temp*varCtr), (yStart-1)+1-dTemp, x+(temp*(varCtr+1)), (yStart-1-1));
		        }
				state = BAR_DATA_DRAW_3D_PREP;
			} else {
				if (GetState(pCh, CH_BAR_HOR) == CH_BAR_HOR) { 
		        	Bar(xStart+1, y-(temp*(varCtr+1)), xStart+1+dTemp, y-1-(temp*varCtr));
		        } else {
		        	Bar(x+1+(temp*varCtr), (yStart-1)-dTemp, x+(temp*(varCtr+1)), (yStart-1));
		        }
	        	if((GetState(pCh, CH_VALUE)) || (GetState(pCh, CH_PERCENT))) {
					state = BAR_DATA_DRAW_VALUE;
					goto chrt_bar_data_draw_value;
				} else {	 
					state = BAR_DATA_DRAW_CHECK;
					goto chrt_bar_data_draw_check;
		        }
			}


chrt_bar_data_draw_3d_prep:
		case BAR_DATA_DRAW_3D_PREP:
       	   	// draw the 3-D component

        	// draw the 45 degree lines
        	// we will use y here as the variable to move the line drawn
			if (GetState(pCh, CH_BAR_HOR) == CH_BAR_HOR) { 
	        	z = y;
   		    	x = xStart+1+dTemp;
			} else {
	        	z = x+1;
   		    	y = yStart-1-dTemp;
			}
			state = BAR_DATA_DRAW_3D_LOOP_1;

chrt_bar_data_draw_3d_loop_1:
		case BAR_DATA_DRAW_3D_LOOP_1:

			if (GetState(pCh, CH_BAR_HOR) == CH_BAR_HOR) { 
	        	if (x <= xStart+1+dTemp+barDepth) {
		        	if ((x == (xStart+1+dTemp))||(x == (xStart+1+dTemp+barDepth))) {
		        		SetColor(BLACK);	
		        	} else {
						SetColor(GetColorShade(*(&(*pCh->prm.pColor)+varCtr), 2)); 
		        	}
					Bar(x, z-1-(temp*(varCtr+1)),
							x, z-1-(temp*varCtr));	
				
					state = BAR_DATA_DRAW_3D_LOOP_2;
				} else {
					state = BAR_DATA_DRAW_3D_OUTLINE1;
					goto chrt_bar_data_draw_3d_outline_1;
				}
			} else {
	        	if (y >= yStart-1-dTemp-barDepth) {
		        	if ((y == (yStart-1-dTemp))||(y == (yStart-1-dTemp-barDepth))) {
		        		SetColor(BLACK);	
		        	} else {
						SetColor(GetColorShade(*(&(*pCh->prm.pColor)+varCtr), 2)); 
		        	}
					Bar(z+1+(temp*varCtr), y,
							z-1+(temp*(varCtr+1)), y);
						
					state = BAR_DATA_DRAW_3D_LOOP_2;
				} else {
					state = BAR_DATA_DRAW_3D_OUTLINE1;
					goto chrt_bar_data_draw_3d_outline_1;
				}
			}
	        	
chrt_bar_data_draw_3d_loop_2:
		case BAR_DATA_DRAW_3D_LOOP_2:
			// check if we are going to draw the outline or the shade
			if (GetState(pCh, CH_BAR_HOR) == CH_BAR_HOR) { 
	        	if ((x == (xStart+1+dTemp))||(x == (xStart+1+dTemp+barDepth))) {
	        		SetColor(BLACK);	
	        	} else {
					SetColor(GetColorShade(*(&(*pCh->prm.pColor)+varCtr), 1)); 
	        	}
			} else {
	        	if ((y == (yStart-1-dTemp))||(y == (yStart-1-dTemp-barDepth))) {
	        		SetColor(BLACK);	
	        	} else {
					SetColor(GetColorShade(*(&(*pCh->prm.pColor)+varCtr), 1)); 
	        	}
			}
			// draw the outline or shade
			if (GetState(pCh, CH_BAR_HOR) == CH_BAR_HOR) { 
				Bar(x-dTemp, z-1-(temp*(varCtr+1)),
					  x, z-1-(temp*(varCtr+1)));
			} else {
				Bar(z+(temp*(varCtr+1)), y,
						z+(temp*(varCtr+1)), y+dTemp);
			}
		
			// update the loop variables
			if (GetState(pCh, CH_BAR_HOR) == CH_BAR_HOR) { 
				x++;
				z--;		
			} else {
				y--;
				z++;		
			}
			state = BAR_DATA_DRAW_3D_LOOP_1;
			goto chrt_bar_data_draw_3d_loop_1;		

chrt_bar_data_draw_3d_outline_1:
		case BAR_DATA_DRAW_3D_OUTLINE1:

        	SetColor(BLACK);
			if (GetState(pCh, CH_BAR_HOR) == CH_BAR_HOR) { 
        		Line(x-1, y-1-(temp*varCtr)-barDepth,
        				x-1-barDepth, y-1-(temp*varCtr));
			} else {
        		Line(x+1+(temp*varCtr), (yStart-1)-dTemp, 
        				x+1+(temp*varCtr)+barDepth, (yStart-1)-dTemp-barDepth);
			}
			state = BAR_DATA_DRAW_3D_OUTLINE2;

		case BAR_DATA_DRAW_3D_OUTLINE2:

			if (GetState(pCh, CH_BAR_HOR) == CH_BAR_HOR) { 
        		Line(x-1, y-1-(temp*(varCtr+1))-barDepth,
        				x-1-barDepth, y-1-(temp*(varCtr+1)));
			} else {
				Line(x+1+(temp*(varCtr+1)), (yStart-1-dTemp), 
        				x+1+(temp*(varCtr+1))+barDepth, (yStart-1-dTemp)-barDepth);
        	}
			state = BAR_DATA_DRAW_3D_OUTLINE3;
        			
		case BAR_DATA_DRAW_3D_OUTLINE3:
			if (GetState(pCh, CH_BAR_HOR) == CH_BAR_HOR) { 
        		Line(x-dTemp-1, y-1-(temp*(varCtr+1))-barDepth,
        				x-dTemp-barDepth-1, y-1-(temp*(varCtr+1)));
			} else {
	        	Line(x+1+(temp*(varCtr+1)), (yStart-1), 
    	    			x+1+(temp*(varCtr+1))+barDepth, (yStart-1)-barDepth);
    	    }
			state = BAR_DATA_DRAW_3D_OUTLINE4;

		case BAR_DATA_DRAW_3D_OUTLINE4:
			if (GetState(pCh, CH_BAR_HOR) == CH_BAR_HOR) { 
        		Bar(x-dTemp-barDepth-1, y-1-(temp*(varCtr+1)),
        				x-dTemp-barDepth-1, y-1-(temp*(varCtr+1))+temp);
			} else {
	        	Bar(x+1+(temp*varCtr), (yStart-1)-dTemp, 
	        			x+1+(temp*varCtr), (yStart-1));
			}
			state = BAR_DATA_DRAW_3D_OUTLINE5;

		case BAR_DATA_DRAW_3D_OUTLINE5:
			// draw the horizontal lines
			if (GetState(pCh, CH_BAR_HOR) == CH_BAR_HOR) { 
        		Bar(x-dTemp-barDepth-1, y-1-(temp*(varCtr+1))+temp,
        				x-barDepth-1, y-1-(temp*(varCtr+1))+temp);
			} else {
       			Bar(x+1+(temp*varCtr), (yStart-1), 
        				x+1+(temp*(varCtr+1)), (yStart-1));
    	    }
        	if((GetState(pCh, CH_VALUE)) || (GetState(pCh, CH_PERCENT)))
				state = BAR_DATA_DRAW_VALUE;
			else {	
	        	state = BAR_DATA_DRAW_CHECK;
	        	goto chrt_bar_data_draw_check;
	        }

chrt_bar_data_draw_value:
		case BAR_DATA_DRAW_VALUE:

			if (GetState(pCh, CH_VALUE)) {
				j = word2xchar(*pSmple, tempStr, STR_CHAR_CNT);
			} else {
				// add the percent sign on the label
				tempStr[STR_CHAR_CNT-1] = '%';

				// compute for the percentage
				if (ChGetValueMax(pCh) <= (*pSmple))
					dPercent = ChGetValueRange(pCh);
				else {
					if ((*pSmple) < ChGetValueMin(pCh))
						dPercent = 0;
					else 
						dPercent = (*pSmple) - ChGetValueMin(pCh);	
				}

				dPercent = ((DWORD)(dPercent*1000))/samplesMax;
			
				// check if we need to round up or not
				if ((dPercent%10) < 5)
					dPercent = (dPercent/10);						// do not round up to next number
				else	
					dPercent = (dPercent/10) + 1;					// round up the value

				// we have a plus 1 here since we add '%' sign already
				j = 1 + word2xchar(dPercent, tempStr, STR_CHAR_CNT-1);
			}
			
			if (GetState(pCh, CH_3D_ENABLE)) {
				if (GetState(pCh, CH_BAR_HOR) == CH_BAR_HOR) { 
					MoveTo(	xStart+dTemp+(barDepth<<1),
							y-(temp*varCtr)-(GetTextHeight(pCh->hdr.pGolScheme->pFont)>>1)-barDepth); 
				} else {
					MoveTo(	x+1+(temp*varCtr), 
							(yStart-1)-dTemp-GetTextHeight(pCh->hdr.pGolScheme->pFont)); 
				}
			} else {
				if (GetState(pCh, CH_BAR_HOR) == CH_BAR_HOR) { 
					MoveTo(	xStart+dTemp+3,
							y-(temp*varCtr)-(GetTextHeight(pCh->hdr.pGolScheme->pFont)>>1)); 
				} else {
					MoveTo(	x+1+(temp*varCtr), 
							(yStart-1)-dTemp-(GetTextHeight(pCh->hdr.pGolScheme->pFont))); 
							//(yStart-1)-dTemp-(GetTextHeight(pCh->hdr.pGolScheme->pFont)>>1)); 
				}
			}

        	state = BAR_DATA_DRAW_VALUE_RUN;
        	
        case BAR_DATA_DRAW_VALUE_RUN:
        	// draw the values on top of the bars
			// NOTE: we use the emboss light color here to draw the values.
       		SetColor(pCh->hdr.pGolScheme->EmbossLtColor);
			if(!OutText(&tempStr[STR_CHAR_CNT-j]))
    	       	return 0;	
	        state = BAR_DATA_DRAW_CHECK;


chrt_bar_data_draw_check:
		case BAR_DATA_DRAW_CHECK:

			// find the next data series that will be shown
			pVar = ChGetNextShowData(pVar);
			if (pVar == NULL) {
				state = REMOVE;
				return 1;
			}
			

	        // increment the variable counter
			varCtr++;
			if (varCtr < ChGetShowSeriesCount(pCh)) {
				pVar = pVar->pNextData;
				goto chrt_data_draw;
			}

			// increment the sample counter
            ctr++;
            if (ctr < ChGetSampleRange(pCh)+1) { 		
	            x += (splDelta+1);
	            goto chrt_data_draw_set;
	        } 
        	state = REMOVE;	
            return 1;
/**************************************************************************/
// 					PIE CHART states 
/**************************************************************************/
/*========================================================================*/
//					  Draw the pie
/*========================================================================*/
chrt_pie_prep:
        case PIE_PREP:
        
        	/* If more than two data series have their SHOW_DATA flag set
        	   the pie chart is drawn to represent the values of each variable 
        	   at Start sample point . End sample point is ignored in this case.
        	   If only one data series is set to be shown the pie chart is drawn
        	   from the sample start point and sample end point (inclusive). 

        	   Pie chart is drawn as a percentage of the data samples.
        	   Therefore: percentage is computed depending on the sample
        	   points used.
        	   Only 1 data series is set to be shown
        	   	 total = 	summation of the variable sample points from
        	   				sample start point to sample end point inclusive.
        	   More than 1 data series is set to be shown
        	   	 total = 	summation of the sample points for each variable
        	   				using the sample start point.
			*/
			
			/* For PIE chart variables used are the following
				ctr = x position of the center of the pie chart
				ctry = y position of the center of the pie chart
				z   = radius of the pie chart
				j	= start angle 
				k   = end angle
			*/
			// calculate the needed variables
			// radius z is affected by the following: CH_LEGEND, CH_VALUE and CH_PERCENT
			temp = CH_MARGIN<<1;

			// check the largest/longest sample
			// use x here as a counter
			varCtr = ChGetShowSeriesCount(pCh);
			if (varCtr == 1) {
				varCtr = ChGetSampleRange(pCh)+1;
			} 
			pVar = ChGetNextShowData(pCh->pChData);

			// y and z is used here as a temporary variable
			y = 0;
			// find the sample value that is largest (place in y)
			// also while doing this get the total of the selected data (put in dTemp)
			dTemp = 0;
			while (varCtr >= 0) {

				if (ChGetShowSeriesCount(pCh) > 1) {
					z = *(&(*pVar->pData) + ChGetSampleStart(pCh)-1); 
				} else {
					z = *(&(*pVar->pData) + ChGetSampleEnd(pCh)-varCtr);
				}
				dTemp += z;
				varCtr--;
				// check if we get a larger value
				if (z > y)
					y = z;
				if (varCtr == 0)
					break;

				if (ChGetShowSeriesCount(pCh) > 1) {
					pVar = ChGetNextShowData(pVar->pNextData);
				}
				// this is insurance (in case the link list is corrupted)
				if (pVar == NULL) {
					break;
				}
			}

			// get the space occupied by the value
			if (GetState(pCh, CH_VALUE)) {
				z = word2xchar(y, tempStr, STR_CHAR_CNT);
				x = (GetTextWidth(&tempStr[STR_CHAR_CNT-z], pCh->hdr.pGolScheme->pFont));
			} else
				x = 0;


			// get the space occupied by percent value
			if (GetState(pCh, CH_PERCENT)) {
				// 5 is derived from "100%," - five possible characters 
				y = (5*GetTextWidth((XCHAR*)tempXchar, pCh->hdr.pGolScheme->pFont));
			} else
				y = 0;

			// get the space occupied by the legend
			if (GetState(pCh, CH_LEGEND)) 
				temp += ((GetLongestNameLength(pCh)+(GetTextHeight(pCh->hdr.pGolScheme->pFont)>>1)));

			// calculate the center of the pie chart
			ctr  = (pCh->hdr.left+pCh->hdr.right-temp)>>1;
			ctry = ((pCh->hdr.bottom+(pCh->hdr.top+GetTextHeight(pCh->hdr.pGolScheme->pFont)))>>1)+CH_MARGIN;
			
			// radius size is checked against the x and y area
			if (((pCh->hdr.right-pCh->hdr.left)-temp-((x+y)<<1)) < 				\
				((pCh->hdr.bottom-pCh->hdr.top-GetTextHeight(pCh->prm.pTitleFont))-CH_MARGIN-(GetTextHeight(pCh->hdr.pGolScheme->pFont)<<1))) {

				// use dimension in the x axis
				if (x+y)
					z = ctr - (pCh->hdr.left+(x+y)+CH_MARGIN);
				else	
					z = ctr - pCh->hdr.left;
			} else {
				// use dimension in the y axis
				if (x+y)
					z = ctry - (pCh->hdr.top+(CH_MARGIN<<1)+GetTextHeight(pCh->prm.pTitleFont)+(GetTextHeight(pCh->hdr.pGolScheme->pFont)<<1));
				else	
					z = ctry - (pCh->hdr.top+(CH_MARGIN<<1)+(GetTextHeight(pCh->prm.pTitleFont)<<1));
					
			}
			state = PIE_DRAW_OUTLINE1;
			
chrt_pie_draw_outline1:
        case PIE_DRAW_OUTLINE1:
 
 			// Required items before the pie chart can be drawn
   			SetColor(LIGHTGRAY);
    		// Draw pie-chart outline
    		Circle(ctr,ctry, z);    
			state = PIE_DRAW_OUTLINE2;

chrt_pie_draw_outline2:
        case PIE_DRAW_OUTLINE2:

 			// Draw start sector line
    		// This line must between the pie-chart center and most right outline point (xend = center+radius)
    		Line(ctr,ctry, ctr+z,ctry); 
			state = PIE_DRAW_SECTOR;

/*========================================================================*/
//					  Draw the sectors of the pie
/*========================================================================*/

chrt_pie_draw_sector:
        case PIE_DRAW_SECTOR:

			// now we are ready to draw the sectors
			// calculate the sector that a value will need
			
			k = 0;
			// check if more than one data series set to be shown , draw the pie chart of 
			// the data series that are set to be shown.
			pVar = ChGetNextShowData(pCh->pChData);
			y = dTemp;
			varCtr = ChGetShowSeriesCount(pCh);
			if (varCtr == 1) {
				varCtr = ChGetSampleRange(pCh)+1;
			} else if (varCtr == 0) {
				pVar = NULL;
				y = 0;
			} 
			// we start at 0 degree.
			j = 0;

			state = PIE_DRAW_SECTOR_LOOP;

chrt_pie_draw_sector_loop:
        case PIE_DRAW_SECTOR_LOOP:

			if (varCtr >= 0) {
				// get the value to be computed
				if (ChGetShowSeriesCount(pCh) > 1) {
					temp = *(&(*pVar->pData) + ChGetSampleStart(pCh)-1); 
				} else {
					temp = *(&(*pVar->pData) + ChGetSampleEnd(pCh)-varCtr);
				}
				
				// calculate the sector that the value will occupy
				dTemp = ((DWORD)(temp)*(3600))/y;

				// check if we need to round up or not
				if ((dTemp%10) < 5)
					dTemp = (dTemp/10);						// do not round up to next number
				else	
					dTemp = (dTemp/10) + 1;					// round up the value
				
				
				// set the color to the color of the variable
				SetColor(*(&(*pCh->prm.pColor)+k));
				
				// check if the sector has zero angle if it is zero just draw the 
				// line.
				if (dTemp == 0) {
					goto chrt_pie_draw_sector_loop_continue;
        			state = PIE_DRAW_SECTOR_LOOP_CONTINUE;
				}

				// check if it is the last sector to be drawn
				if ((varCtr == 1) || ((j+dTemp) >= 358))
					DrawSector(ctr, ctry, z, j, 360, LIGHTGRAY);
				else 
					DrawSector(ctr, ctry, z, j, (j+dTemp), LIGHTGRAY);
				state = PIE_DRAW_SECTOR_LOOP_CREATE_STRINGS;

			} else {
				if(GetState(pCh, CH_DONUT) == CH_DONUT) {
					state = PIE_DONUT_HOLE_DRAW;
					goto chrt_pie_donut_hole_draw;
				} else {
	        		state = REMOVE;	
		          	return 1;	        	
		        }
			}

chrt_pie_draw_sector_loop_create_strings:
        case PIE_DRAW_SECTOR_LOOP_CREATE_STRINGS:


			// create the strings of the values if needed
			if (GetState(pCh, CH_VALUE) || GetState(pCh, CH_PERCENT)) {

				h = 0;
				GetCirclePoint(z, (j+(dTemp>>1)), &pieX, &pieY);
				pieX += ctr; 
				pieY += ctry;
			
				// do we need the show the values? create the strings here
				if (GetState(pCh, CH_VALUE)) {
					h = word2xchar(temp, tempStr, STR_CHAR_CNT);
				}
				
				// do we need to show the percentage? create the strings here
				if (GetState(pCh, CH_PERCENT)) {
					// add the % sign
					// check if we need to add comma
					if (GetState(pCh, CH_VALUE)) {
						h += 1;											// adjust h
						tempStr[STR_CHAR_CNT-h] = ',';
					}
					h += 1;												// adjust h
					tempStr[STR_CHAR_CNT-h] = '%';

					// now add the percentage
					dPercent = (DWORD)(dTemp*1000)/360;
					// check if we need to round up or not
					if ((dPercent%10) < 5)
						dPercent = (dPercent/10);						// do not round up to next number
					else	
						dPercent = (dPercent/10) + 1;					// round up the value
						
					i = word2xchar((WORD)dPercent, tempStr, STR_CHAR_CNT-h);
					
					// adjust the h position
					h += i;
				}

				// we have to relocate the text depending on the position
				if ((j+(dTemp>>1) > 0) && (j+(dTemp>>1) <= 90)) {
					MoveTo(pieX+GetTextWidth(tempXchar, pCh->hdr.pGolScheme->pFont), pieY);
				} else if ((j+(dTemp>>1) > 90) && (j+(dTemp>>1) <= 180)) {
					MoveTo(pieX-GetTextWidth(&tempStr[STR_CHAR_CNT-h], pCh->hdr.pGolScheme->pFont)-
								GetTextWidth(tempXchar, pCh->hdr.pGolScheme->pFont), 
						   pieY); 
							//pieY + (GetTextHeight(pCh->hdr.pGolScheme->pFont)>>2));
				} else 	if ((j+(dTemp>>1) > 180) && (j+(dTemp>>1) <= 270)) {
					MoveTo(pieX-GetTextWidth(&tempStr[STR_CHAR_CNT-h], pCh->hdr.pGolScheme->pFont)-
								GetTextWidth(tempXchar, pCh->hdr.pGolScheme->pFont),  
						   pieY-GetTextHeight(pCh->hdr.pGolScheme->pFont));
				} else 	if ((j+(dTemp>>1) > 270) && (j+(dTemp>>1) <= 360)) {
					MoveTo(pieX+5, 
							pieY-GetTextHeight(pCh->hdr.pGolScheme->pFont));
				} else {
					MoveTo(pieX,pieY);
				}

				state = PIE_DRAW_SECTOR_LOOP_STRINGS_RUN;
			} else {
				state = PIE_DRAW_SECTOR_LOOP_CONTINUE;
				goto chrt_pie_draw_sector_loop_continue;
			}
			
chrt_pie_draw_sector_loop_strings_run:
        case PIE_DRAW_SECTOR_LOOP_STRINGS_RUN:

			// now draw the strings of the values and/or percentages
			SetColor(BLACK);
			SetFont(pCh->hdr.pGolScheme->pFont);

			if(!OutText(&tempStr[STR_CHAR_CNT-h]))
    	       	return 0;	
 	    
            state = PIE_DRAW_SECTOR_LOOP_CONTINUE;

chrt_pie_draw_sector_loop_continue:
        case PIE_DRAW_SECTOR_LOOP_CONTINUE:
			j += dTemp;
			varCtr--;
			if (varCtr == 0) {
				if (GetState(pCh, CH_DONUT) == CH_DONUT) {
					state = PIE_DONUT_HOLE_DRAW;
					goto chrt_pie_donut_hole_draw;
				} else {
        			state = REMOVE;	
            		return 1;	        	
            	}
			}

			// check if more than one data series to be shown
			if (ChGetShowSeriesCount(pCh) > 1) {
				pVar = ChGetNextShowData(pVar->pNextData);
				if (pVar == NULL) {
					break;
				}
			}
			k++;
            state = PIE_DRAW_SECTOR_LOOP;
            goto chrt_pie_draw_sector_loop;

chrt_pie_donut_hole_draw:
		case PIE_DONUT_HOLE_DRAW:
			SetColor(LIGHTGRAY);
			Circle(ctr, ctry, (z>>1)-(z>>3));
			SetColor(pCh->hdr.pGolScheme->CommonBkColor);
			FillCircle(ctr,ctry, ((z>>1)-(z>>3))-1); 

			state = REMOVE;
			return 1;
    }
    return 1;
}

/*********************************************************************
* Function: ChAddDataSeries(CHART *pCh, WORD nSamples, WORD *pData, XCHAR *pName)
*
*
* Notes: Adds a new variable data structure in the linked list 
*		 of variable datas. Number of samples is given with the 
*		 array of the samples. If there is only one data 
* 		 nSamples is set to 1 with the address of the variable data.
*		 
*
********************************************************************/
DATASERIES *ChAddDataSeries(CHART *pCh, WORD nSamples, WORD *pData, XCHAR *pName)
{
	DATASERIES *pVar = NULL, *pListVar;
	
	pVar = malloc(sizeof(DATASERIES));

	if (pVar == NULL) 
		return NULL;

	// add the other parameters of the variable data
	pVar->pSData    = (XCHAR*)pName;
	pVar->samples   = nSamples;
	pVar->pData     = (WORD*)pData;
	pVar->show		= SHOW_DATA;
	pVar->pNextData = NULL;
	
	pListVar = pCh->pChData;
	if (pCh->pChData == NULL)
		pCh->pChData = pVar;
	else {	
		// search the end of the list and append the new data
		while (pListVar->pNextData != NULL)
			pListVar = pListVar->pNextData; 
		pListVar->pNextData = pVar;			 			
	}
	// update the variable count before exiting
	pCh->prm.seriesCount++;
	return pVar;
}

/*********************************************************************
* Function: ChRemoveDataSeries(CHART *pCh, WORD number)
*
*
* Notes: Removes a data series structure in the linked list 
*		 of data series. 
*
********************************************************************/
void ChRemoveDataSeries(CHART *pCh, WORD number)
{
	DATASERIES *pVar = NULL, *pPrevVar;
	WORD ctr = 1;
	
	pVar = pCh->pChData;

	// check if the list is empty
	if (pVar == NULL)
		return;
		
	// check if there is only one entry
	if (pVar->pNextData == NULL) {
		free(pVar);
		pCh->pChData = NULL;
		return;
	}

	// there are more than one entry, remove the entry specified
	while (ctr < number) {
		pPrevVar = pVar;
		pVar = pVar->pNextData;
		ctr++;
	}
	// remove the item from the list
	pPrevVar->pNextData = pVar->pNextData;
	
	// free the memory used by the item
	free(pVar);

}

/*********************************************************************
* Function: ChSetDataSeries(CHART *pCh, WORD seriesNum, BYTE status)
*
*
* Notes: Sets the specified data series number show flag to be set to 
*		 SHOW_DATA or HIDE_DATA depending on the status. 
*    	 If the seriesNum is 0, it sets all the data series 
*		 entries in the data series linked list. Returns the same passed
*		 number if successful otherwise -1 if unsuccesful.
*
********************************************************************/
SHORT ChSetDataSeries(CHART *pCh, WORD seriesNum, BYTE status)
{
	DATASERIES *pListSer;
	WORD ctr = 1;
	
	pListSer = pCh->pChData;

	// check if the list is empty
	if (pListSer == NULL)
		return -1;
	
	while (pListSer != NULL) {
		// check if we need to show all
		if (seriesNum == 0)
			pListSer->show = status;
		else if (seriesNum == ctr) {
			pListSer->show = status;
			break;
		}
		ctr++;
		pListSer = pListSer->pNextData; 
	}
	if (seriesNum == ctr)
		return seriesNum;
	else
		return -1;	
}	

/*********************************************************************
* Function: ChSetSampleRange(CHART *pCh, WORD start, WORD end) 
*
*
* Notes: Sets the sampling start and end points when drawing the chart.
*		 Depending on the number of data series with SHOW_DATA flag
*		 set and the values of end and start samples a single
*		 data series is drawn or multiple data series are drawn.
*
********************************************************************/
void ChSetSampleRange(CHART *pCh, WORD start, WORD end) 
{
	pCh->prm.smplStart = start;
	if (end < start)
		pCh->prm.smplEnd = start;
	else	
		pCh->prm.smplEnd = end;
}	

/*********************************************************************
* Function: ChSetValRange(CHART *pCh, WORD min, WORD max) 
*
*
* Notes: Sets the sampling start and end points when drawing the chart.
*		 Depending on the number of data series with SHOW_DATA flag
*		 set and the values of end and start samples a single
*		 data series is drawn or multiple data series are drawn.
*
********************************************************************/
void ChSetValRange(CHART *pCh, WORD min, WORD max)
{
	pCh->prm.valMin = min;
	if (max < min)
		pCh->prm.valMax = min;
	else	
		pCh->prm.valMax = max;
}	 

/*********************************************************************
* Function: ChSetPerRange(CHART *pCh, WORD min, WORD max) 
*
*
* Notes: Sets the percentage range when drawing the chart. This affects
*		 bar charts only and CH_PERCENTAGE bit state is set.
*
********************************************************************/
void ChSetPerRange(CHART *pCh, WORD min, WORD max)
{
	pCh->prm.perMin = min;
	if (max < min)
		pCh->prm.perMax = min;
	else	
		pCh->prm.perMax = max;
}	 

///////////////////// SIN and COS Tables from 0 to 45 deg /////////////////////

const WORD sinTable[] __attribute__  ((aligned(2))) = {
       0, 1143,  2287, 3429, 4571,
    5711, 6850,  7986, 9120,10251,
    11380,12504,13625,14742,15854,
    16961,18063,19160,20251,21336,
    22414,23485,24549,25606,26655,
    27696,28728,29752,30766,31771,
    32767,33753,34728,35692,36646,
    37589,38520,39439,40347,41242,
    42125,42994,43851,44694,45524,
    46340
};

const WORD cosTable[] __attribute__  ((aligned(2))) = {
    65535,65525,65495,65445,65375,
    65285,65175,65046,64897,64728,
    64539,64330,64102,63855,63588,
    63301,62996,62671,62327,61964,
    61582,61182,60762,60325,59869,
    59394,58902,58392,57863,57318,
    56754,56174,55576,54962,54330,
    53683,53018,52338,51642,50930,
    50202,49459,48701,47929,47141,
    46340
};


void    FillSector(SHORT x, SHORT y, WORD outLineColor)
{
WORD    pixel;
SHORT   left, right;
SHORT   top, bottom;
SHORT   xc, yc;
SHORT   temp;  

    // scan down
    top = bottom = yc = y;
    left = right = xc = x;
    while(1){
        pixel = GetPixel(xc,yc);
        if(pixel == outLineColor){

            for(xc = left+1; xc<right; xc++){
                pixel = GetPixel(xc,yc);
                if(pixel != outLineColor){
                    break;
                }
            }

            if(xc == right)
                break;
            
        }
        // left scan
        left  = xc;
        do{
            PutPixel(left--,yc);
            pixel = GetPixel(left,yc);
        }while(pixel != outLineColor);

        // right scan
        right = xc;
        pixel = GetPixel(right,yc);
        do{
            PutPixel(right++,yc);
            pixel = GetPixel(right,yc);
        }while(pixel != outLineColor);
        
        xc = (left+right)>>1;
        yc++;
    }

    // scan up
    yc = y;
    xc = x;
    while(1){
        pixel = GetPixel(xc,yc);
        if(pixel == outLineColor){

            for(xc = left+1; xc<right; xc++){
                pixel = GetPixel(xc,yc);
                if(pixel != outLineColor){
                    break;
                }
            }

            if(xc == right)
                break;
        }
        // left scan
        left  = xc;
        do{
            PutPixel(left--,yc);
            pixel = GetPixel(left,yc);
        }while(pixel != outLineColor);

        // right scan
        right = xc;
        pixel = GetPixel(right,yc);
        do{
            PutPixel(right++,yc);
            pixel = GetPixel(right,yc);
        }while(pixel != outLineColor);
        
        xc = (left+right)>>1;
        yc--;
    }
}

void  GetCirclePoint(SHORT radius, SHORT angle, SHORT* x, SHORT* y){
DWORD rad;
SHORT ang;
SHORT temp;
    
    ang = angle%45;
    if((angle/45)&0x01)
        ang = 45 - ang;

    rad = radius;
    rad *= cosTable[ang];
    *x = ((DWORD_VAL)rad).w[1];
    rad = radius;
    rad *= sinTable[ang];
    *y = ((DWORD_VAL)rad).w[1];

    if( ((angle >  45) && (angle < 135)) ||
        ((angle > 225) && (angle < 315)) ){
        temp = *x; *x = *y; *y = temp;
    }

    if( (angle > 90) && (angle < 270) ){
        *x = -*x;
    }

    if( (angle > 180) && (angle < 360) ){
        *y = -*y;
    }
}

void DrawSector(SHORT cx, SHORT cy, SHORT outRadius,
                 SHORT angleFrom, SHORT angleTo, WORD outLineColor)
{
SHORT x1, y1, x2, y2, x3, y3;
LONG  temp;
WORD  tempColor;
SHORT angleMid;

    angleMid =(angleTo+angleFrom)>>1;
    GetCirclePoint(outRadius, angleFrom, &x1, &y1);
    GetCirclePoint(outRadius, angleTo, &x2, &y2);
    x1 += cx; y1 += cy; x2 += cx; y2 += cy;
    
	GetCirclePoint(outRadius-1, angleFrom+1, &x3, &y3);
    x3 += cx; y3 += cy;
    Line(x3, y3, cx, cy);
    GetCirclePoint(outRadius-1, angleTo-1, &x3, &y3);
    x3 += cx; y3 += cy;
    Line(x3, y3, cx, cy);
    GetCirclePoint(outRadius-1, angleMid, &x3, &y3);
    x3 += cx; y3 += cy;
	Line(x3, y3, cx, cy);

    tempColor = GetColor();
    SetColor(outLineColor);
    Line(x1, y1, cx, cy);
    Line(x2, y2, cx, cy);
    SetColor(tempColor);


    temp = ((x1-x2)*(x1-x2));
    temp += ((y1-y2)*(y1-y2));
    
    if( ((DWORD)temp <= (DWORD)16) &&
        ((angleTo-angleFrom)<90) ) 
        return;
    GetCirclePoint(outRadius-2, angleMid, &x3, &y3);
    x3 += cx; y3 += cy;
    FillSector(x3, y3, outLineColor);
}


#endif // USE_CHART
