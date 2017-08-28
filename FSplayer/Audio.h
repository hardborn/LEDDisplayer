
#ifndef AUDIO_H
#define AUDIO_H

#include "PacketQueue.h"
extern "C" {

#include <libavformat/avformat.h>

}


/**
 * ����audioʱ����Ҫ�����ݷ�װ
 */
class AudioState
{
public:
	const uint32_t BUFFER_SIZE;// �������Ĵ�С
	char* name;
	PacketQueue *audioq;

	double audio_clock; // audio clock
	AVStream *stream; // audio stream

	uint8_t *audio_buff;       // ��������ݵĻ���ռ�
	uint32_t audio_buff_size;  // buffer�е��ֽ���
	uint32_t audio_buff_index; // buffer��δ�������ݵ�index

	int stream_index;          // audio��index
	AVCodecContext *audio_ctx; // �Ѿ�����avcodec_open2��
	SDL_AudioDeviceID dev;

	AudioState(const char* filename);              //Ĭ�Ϲ��캯��
	AudioState(AVCodecContext *audio_ctx, int audio_stream);

	~AudioState();


	bool audio_play();

	double get_audio_clock();

	/**
	* ���豸����audio���ݵĻص�����
	*/
	

	/**
	* ����Avpacket�е�������䵽����ռ�
	*/
	int audio_decode_frame(uint8_t *audio_buf, int buf_size);
};

void audio_callback(void* userdata, Uint8 *stream, int len);



#endif