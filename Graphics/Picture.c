/*****************************************************************************
 *  Module for Microchip Graphics Library 
 *  GOL Layer 
 *  Picture
 *****************************************************************************
 * FileName:        Picture.c
 * Dependencies:    None 
 * Processor:       PIC24, PIC32
 * Compiler:       	MPLAB C30 V3.00, MPLAB C32
 * Linker:          MPLAB LINK30, MPLAB LINK32
 * Company:         Microchip Technology Incorporated
 *
 * Software License Agreement
 *
 * Copyright � 2007 Microchip Technology Inc.  All rights reserved.
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
 * Anton Alkhimenok 	11/12/07	Version 1.0 release
 *****************************************************************************/

#include "Graphics\Graphics.h"

#ifdef USE_PICTURE

/*********************************************************************
* Function: PICTURE  *PictCreate(WORD ID, SHORT left, SHORT top, SHORT right, 
*                              SHORT bottom, WORD state, char scale, void *pBitmap, 
*                              GOL_SCHEME *pScheme)
*
* Overview: creates the picture control
*
********************************************************************/
PICTURE *PictCreate(WORD ID, SHORT left, SHORT top, SHORT right, SHORT bottom, 
			       WORD state, char scale, void *pBitmap, GOL_SCHEME *pScheme)
{
	PICTURE *pPict = NULL;
	
	pPict = malloc(sizeof(PICTURE));
	if (pPict == NULL)
		return pPict;

	pPict->ID      	= ID;
	pPict->pNxtObj 	= NULL;
	pPict->type    	= OBJ_PICTURE;
	pPict->left    	= left;
	pPict->top     	= top;
	pPict->right   	= right;
	pPict->bottom  	= bottom;
	pPict->pBitmap 	= pBitmap;
	pPict->state   	= state;
	pPict->scale   	= scale;

	// Set the style scheme to be used
	if (pScheme == NULL)
		pPict->pGolScheme = _pDefaultGolScheme; 
	else 	
		pPict->pGolScheme = (GOL_SCHEME *)pScheme; 	

    GOLAddObject((OBJ_HEADER*) pPict);
	
	return pPict;
}

/*********************************************************************
* Function: WORD PictTranslateMsg(PICTURE *pPict, GOL_MSG *pMsg)
*
* Overview: translates the GOL message for the picture control
*
********************************************************************/
WORD PictTranslateMsg(PICTURE *pPict, GOL_MSG *pMsg)
{
	// Evaluate if the message is for the picture
    // Check if disabled first
	if ( GetState(pPict,PICT_DISABLED) )
		return OBJ_MSG_INVALID;

#ifdef USE_TOUCHSCREEN
    if(pMsg->type == TYPE_TOUCHSCREEN){
    	// Check if it falls in the picture area
	    if( (pPict->left   > pMsg->param1) &&
   	        (pPict->right  < pMsg->param1) &&
            (pPict->top    > pMsg->param2) &&
            (pPict->bottom < pMsg->param2) ){

            return PICT_MSG_SELECTED;
        }
    }
#endif

    return OBJ_MSG_INVALID;	
}

/*********************************************************************
* Function: WORD PictDraw(PICTURE *pPict)
*
* Output: returns the status of the drawing
*		  0 - not completed
*         1 - done
*
* Overview: draws picture
*
********************************************************************/
WORD PictDraw(PICTURE *pPict)
{
typedef enum {
	REMOVE,
	DRAW_IMAGE,
	DRAW_BACKGROUND1,
	DRAW_BACKGROUND2,
	DRAW_BACKGROUND3,
	DRAW_BACKGROUND4,
	DRAW_FRAME
} PICT_DRAW_STATES;

static PICT_DRAW_STATES state = REMOVE;
static SHORT posleft;
static SHORT postop;
static SHORT posright;
static SHORT posbottom;

    if(IsDeviceBusy())
        return 0;

    switch(state){

        case REMOVE:
            if(GetState(pPict,PICT_HIDE)){
                if(IsDeviceBusy())
                    return 0;
                SetColor(pPict->pGolScheme->CommonBkColor);
                Bar(pPict->left,pPict->top,pPict->right,pPict->bottom);
                return 1;
            }
            posleft = (pPict->left+pPict->right-pPict->scale*GetImageWidth(pPict->pBitmap))>>1;
            postop = (pPict->top+pPict->bottom-pPict->scale*GetImageHeight(pPict->pBitmap))>>1;
            posright = (pPict->right+pPict->left+pPict->scale*GetImageWidth(pPict->pBitmap))>>1;
            posbottom = (pPict->bottom+pPict->top+pPict->scale*GetImageHeight(pPict->pBitmap))>>1;
            state = DRAW_IMAGE;

        case DRAW_IMAGE:
            if(pPict->pBitmap != NULL){
                if(IsDeviceBusy())
                    return 0;
                PutImage( posleft, postop,pPict->pBitmap, pPict->scale); 
            }
            SetColor(pPict->pGolScheme->CommonBkColor);
            state = DRAW_BACKGROUND1;

        case DRAW_BACKGROUND1:
            if(IsDeviceBusy())
                return 0;
            Bar(pPict->left+1, pPict->top+1, pPict->right-1, postop-1);
            state = DRAW_BACKGROUND2;

        case DRAW_BACKGROUND2:
            if(IsDeviceBusy())
                return 0;
            Bar(pPict->left+1, posbottom, pPict->right-1, pPict->bottom-1);
            state = DRAW_BACKGROUND3;

        case DRAW_BACKGROUND3:
            if(IsDeviceBusy())
                return 0;
            Bar(pPict->left+1, postop, posleft-1, posbottom);
            state = DRAW_BACKGROUND4;

        case DRAW_BACKGROUND4:
            if(IsDeviceBusy())
                return 0;
            Bar(posright, postop, pPict->right-1, posbottom);
            state = DRAW_FRAME;

        case DRAW_FRAME:
            if(GetState(pPict,PICT_FRAME)){
                if(IsDeviceBusy())
                    return 0; 
		        SetLineType(SOLID_LINE);
		        SetColor(pPict->pGolScheme->TextColor0);
		        Rectangle(pPict->left, pPict->top,
                          pPict->right, pPict->bottom);
		        
            }
            state = REMOVE;
            return 1;
    }
    return 1;
}

#endif // USE_PICTURE
