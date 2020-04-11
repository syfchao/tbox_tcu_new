#ifndef _ALSA_CONTROL_H_
#define _ALSA_CONTROL_H_
#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
* Function Name : set_clvl_value
* Description	: select the volume of the internal loudspeaker audio 
* Input 		: integer value represent loudspeaker volume level    (0-5)
* Output		: None
* Return		: 0 succes	 -1 Fail
* Auther		: lhy
* Date		: 2018.08.21
*****************************************************************************/	
int set_clvl_value(int clvl_value);

/*****************************************************************************
* Function Name : get_clvl_value
* Description	: get  the volume of the internal loudspeaker audio 
* Input 		: None
* Output		: None
* Return		: when success renturn the clvl value, when fail return -1 
* Auther		: lhy
* Date		: 2018.08.21
*****************************************************************************/	
int get_clvl_value(void);



/*****************************************************************************
* Function Name : set_micgain_value
* Description	: adjust mic gain 
* Input 		: integer value represent gain value (0-8)
* Output		: None
* Return		: 0 succes	 -1 Fail
* Auther		: lhy
* Date		: 2018.08.21
*****************************************************************************/	

int set_micgain_value(int micgain_value);


/*****************************************************************************
* Function Name : get_micgain_value
* Description	: get  the mic gain value 
* Input 		: None
* Output		: None
* Return		: when success renturn the mic gain  value, when fail return -1 
* Auther		: lhy
* Date	        : 2018.08.21
*****************************************************************************/	

int get_micgain_value(void);



/*****************************************************************************
* Function Name : set_csdvc_value
* Description	: switch voice channel device 
* Input 		: integer value  1 : handset  3: speaker phone
* Output		: None
* Return		: 0 succes	 -1 Fail
* Auther		: lhy
* Date		: 2018.08.21
*****************************************************************************/	

int set_csdvc_value(int csdvc_value);

/*****************************************************************************
* Function Name : get_csdvc_value
* Description	: get voice channel device 
* Input 		: None
* Output		: None
* Return		: when success renturn the value represent voice channel device , when fail return -1 
* Auther		: lhy
* Date		: 2018.08.21
*****************************************************************************/	

int get_csdvc_value(void);

#ifdef __cplusplus
}
#endif

#endif



