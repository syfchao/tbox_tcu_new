#ifndef _TTS_CONTROL_H_
#define _TTS_CONTROL_H_
#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char        uint8;
typedef unsigned short       uint16;
typedef  unsigned int        uint32;

typedef enum {
  TTS_PLAY_UTF16LE,   
  TTS_PLAY_GBK,   
  TTS_MAX_AT_CMD,          
}TTS_CMD;

typedef enum {
  TTS_NONE,
  TTS_PLAY,              //正在播放
  TTS_STOP,              //停止播放
}TTS_STATE;

int simcom_play_tts(TTS_CMD tts_action, uint8 *tts_text);
void simcom_stop_tts();
TTS_STATE simcom_get_tts_state();
#ifdef __cplusplus
}
#endif
#endif // _TTS_CONTROL_H_
