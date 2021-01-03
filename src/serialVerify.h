

inline static Boolean ValidateSerialNumber(unsigned char *regInfo, Boolean eitherBoxedOrShareware);

#ifndef gRegInfo
extern unsigned char	gRegInfo[64];
#endif

#ifndef gSerialFileName
extern Str255  gSerialFileName;
#endif


/****************** DO SERIAL DIALOG *************************/

inline static void DoSerialDialog(unsigned char *out)
{
DialogPtr 		myDialog;
Boolean			dialogDone = false, isValid;
short			itemType,itemHit;
ControlHandle	itemHandle;
Rect			itemRect;

	Enter2D();
	
	if (gShareware)
		myDialog = GetNewDialog(3000 + gGamePrefs.language,nil,MOVE_TO_FRONT);
	else
		myDialog = GetNewDialog(4000 + gGamePrefs.language,nil,MOVE_TO_FRONT);
			
			
				/* DO IT */
				
	while(dialogDone == false)
	{
		ModalDialog(nil, &itemHit);
		switch (itemHit)
		{
			case	1:									        // Register
					GetDialogItem(myDialog,3,&itemType,(Handle *)&itemHandle,&itemRect);
					GetDialogItemText((Handle)itemHandle,gRegInfo);
                    BlockMove(&gRegInfo[1], &gRegInfo[0], SERIAL_LENGTH);   // skip length byte from src string

                    isValid = ValidateSerialNumber(gRegInfo, false);    	// validate the number
    
                    if (isValid == true)
                    {
                        gGameIsRegistered = true;
                        dialogDone = true;
                        BlockMove(gRegInfo, out, SERIAL_LENGTH);		// copy to output
                    }
                    else
                    {
                    	switch(gGamePrefs.language)
                    	{
                    		case	LANGUAGE_SPANISH:
			                        DoAlert("Lo sentimos, el número de serie no es válido.  Por favor inténtelo nuevamente.");
                    				break;
                    				
                    		default:
			                        DoAlert("Sorry, that serial number is not valid.  Please try again.");
			            }
						InitCursor();                        
                    }
					break;

			case 	2:									// QUIT
                    ExitToShell();
					break;	
					
            case    4:                                  // URL
            		if (gShareware)
					{
						if (LaunchURL("http://www.pangeasoft.net/otto/serials.html") == noErr)
		                    ExitToShell();
					}
                    break;
		}
	}
	DisposeDialog(myDialog);

	Exit2D();
}






/********************* VALIDATE REGISTRATION NUMBER ******************/
//
// Return true if is valid
//
// INPUT:	eitherBoxedOrShareware = true if the serial number can be either the boxed-faux-code or
//									a legit shareware code (for reading Gamefiles which could have either)
//

inline static Boolean ValidateSerialNumber(unsigned char *regInfo, Boolean eitherBoxedOrShareware)
{
FSSpec	spec;
u_long	customerID, i;
u_long	seed0,seed1,seed2, validSerial, enteredSerial;
u_char	shift;
int		j,c;
Handle	hand;
const u_char pirateNumbers[11][SERIAL_LENGTH*2] =
{
	"H%J%K%W%P%E%S%K%N%T%G%N%",		// put "%" in there to confuse pirates scanning for this data
	"H%N%H%L%O%R%I%R%H%J%T%H%",
	"H%M%U%P%R%Q%H%O%F%T%I%N%",
	"H%R%U%U%G%I%K%J%M%Q%K%Q%",
	"H%R%U%V%N%T%I%P%P%O%H%I%",
	"H%R%U%W%G%R%J%Q%T%K%I%E%",
	"H%T%V%X%Q%F%F%S%G%E%K%M%",
	"M%N%Z%F%L%K%T%P%L%F%L%O%",
	"J%E%V%O%G%P%T%F%O%O%E%J%",
	"J%E%V%P%P%I%E%E%H%P%H%J%",
};


			/******************************************************/
			/* THIS IS THE BOXED VERSION, SO VERIFY THE FAUX CODE */
			/******************************************************/

	if ((!gShareware) || eitherBoxedOrShareware)
	{	    
		int	i;
		static unsigned char fauxCode[SERIAL_LENGTH] = "PANG00115272";

		for (i = 0 ; i < SERIAL_LENGTH; i++)
		{
			if ((regInfo[i] >= 'a') && (regInfo[i] <= 'z'))		// conver to upper-case
				regInfo[i] = 'A' + (regInfo[i] - 'a');
		
			if (regInfo[i] != fauxCode[i])						// see if doesn't match
			{
				if (eitherBoxedOrShareware)						// see if we should also try as shareware code
					goto try_shareware_code;
				else
					return(false);
			}
		}
		gSerialWasVerified = true;					// set this to verify that hackers didn't bypass this function
	    return(true);
	}



			/*******************************************************/
			/* THIS IS THE SHAREWARE VERSION, SO VALIDATE THE CODE */
			/*******************************************************/
	else
	{
try_shareware_code:
	
			/* CONVERT TO UPPER CASE */
				    
	    for (i = 0; i < SERIAL_LENGTH; i++)
	    {
			if ((regInfo[i] >= 'a') && (regInfo[i] <= 'z'))
				regInfo[i] = 'A' + (regInfo[i] - 'a');
		}
	    
	    customerID  = (regInfo[0] - 'H') * 0x1000;				// convert H,I,J,K,	L,M,N,O, P,Q,R,S, T,U,V,W to 0x1000...0xf000
	    customerID += (regInfo[1] - 'E') * 0x0100;				// convert E,F,G,H, I,J,K,L, M,N,O,P, Q,R,S,T to 0x0100...0x0f00
	    customerID += (regInfo[2] - 'G') * 0x0010;				// convert G,H,I,J, K,L,M,N, O,P,Q,R, S,T,U,V to 0x0001...0x00f0
	    customerID += (regInfo[3] - 'K') * 0x0001;				// convert K,L,M,N, O,P,Q,R, S,T,U,V, W,X,Y,Z to 0x0010...0x000f

		if (customerID < 200)									// there are no customers under 200 since we want to confuse pirates
			return(false);
		if (customerID > 20000)									// also assume not over 20,000
			return(false);

			/* NOW SEE WHAT THE SERIAL SHOULD BE */

		seed0 = 0xf08cc1d5;										// init random seed
		seed1 = 0x0699cab3;
		seed2 = 0x4545faa2;
			
		for (i = 0; i < customerID; i++)						// calculate the random serial
  			seed2 ^= (((seed1 ^= (seed2>>5)*1568397607UL)>>7)+(seed0 = (seed0+1)*3141592621UL))*2435386481UL;

		validSerial = seed2;


			/* CONVERT ENTERED SERIAL STRING TO NUMBER */
			
		shift = 0;
		enteredSerial = 0;
		for (i = SERIAL_LENGTH-1; i >= 4; i--)						// start @ last digit
		{
			u_long 	digit = regInfo[i] - 'E';					// convert E,F,G,H, I,J,K,L, M,N,O,P, Q,R,S,T to 0x0..0xf		
			enteredSerial += digit << shift;					// shift digit into posiion
			shift += 4;											// set to insert next nibble
		}
			
				/* SEE IF IT MATCHES */
				
		if (enteredSerial != validSerial)
			return(false);
			


				/**********************************/
				/* CHECK FOR KNOWN PIRATE NUMBERS */
				/**********************************/
				
				
				/* THEN SEE IF THIS CODE IS IN THE TABLE */
						
		for (j = 0; j < 11; j++)
		{
			for (i = 0; i < SERIAL_LENGTH; i++)
			{
				if (regInfo[i] != pirateNumbers[j][i*2])					// see if doesn't match
					goto next_code;		
			}
			
					/* THIS CODE IS PIRATED */
					
			return(false);
			
	next_code:;		
		}


				/*******************************/
				/* SECONDARY CHECK IN REZ FILE */
				/*******************************/
				//
				// The serials are stored in the Level 1 terrain file
				//

		if (FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Audio:Main.sounds", &spec) == noErr)		// open rez fork
		{
			short fRefNum = FSpOpenResFile(&spec,fsRdPerm);
			
			UseResFile(fRefNum);						// set app rez
		
			c = Count1Resources('savs');						// count how many we've got stored
			for (j = 0; j < c; j++)
			{
				hand = Get1Resource('savs',128+j);			// read the #
			
				for (i = 0; i < SERIAL_LENGTH; i++)
				{
					if (regInfo[i] != (*hand)[i])			// see if doesn't match
						goto next2;		
				}
			
						/* THIS CODE IS PIRATED */
						
				UseResFile(gMainAppRezFile);				// set app rez
				return(false);
			
		next2:		
				ReleaseResource(hand);		
			}
			
			UseResFile(gMainAppRezFile);					// set app rez
		}

		gSerialWasVerified = true;					// set this to verify that hackers didn't bypass this function
	    return(true);
	}

}



/********************** CHECK GAME SERIAL NUMBER *************************/

inline static void CheckGameSerialNumber(Boolean onlyVerify)
{
OSErr   iErr;
FSSpec  spec;
short		fRefNum;
long        	numBytes = SERIAL_LENGTH;

            /* GET SPEC TO REG FILE */

	iErr = FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, gSerialFileName, &spec);
    if (iErr)
        goto game_not_registered;


            /****************************/
            /* VALIDATE THE SERIAL FILE */
            /****************************/

            /* READ SERIAL DATA */

    if (FSpOpenDF(&spec,fsCurPerm,&fRefNum) != noErr)		// if cant open file then not registered yet
        goto game_not_registered;
    	
	FSRead(fRefNum,&numBytes,gRegInfo);	
    FSClose(fRefNum);

            /* VALIDATE THE DATA */

    if (!ValidateSerialNumber(gRegInfo, false))			// if this serial is bad then we're not registered
        goto game_not_registered;        

    gGameIsRegistered = true;
    		
	return;
	

        /* GAME IS NOT REGISTERED YET, SO DO DIALOG */

game_not_registered:

	if (onlyVerify)											// see if let user try to enter it
		gGameIsRegistered = false;	
	else
	    DoSerialDialog(gRegInfo);


    if (gGameIsRegistered)                                  // see if write out reg file
    {
	    FSpDelete(&spec);	                                // delete existing file if any
	    iErr = FSpCreate(&spec,kGameID,'xxxx',-1);
        if (iErr == noErr)
        {
        	numBytes = SERIAL_LENGTH;
			FSpOpenDF(&spec,fsCurPerm,&fRefNum);
			FSWrite(fRefNum,&numBytes,gRegInfo);
		    FSClose(fRefNum);

     	}  
    }
    
}





