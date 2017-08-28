
#include "Audio.h"

#include <iostream>
#include <fstream>
extern "C" {

#include <libswresample\swresample.h>

}

#include <log4cplus/fileappender.h>  
#include <log4cplus/layout.h>  
#include <log4cplus/ndc.h>  
#include <log4cplus/helpers/loglog.h>  
#include <log4cplus/loggingmacros.h>  

#include "VLog.h"

extern bool quit;

AudioState::AudioState(const char* filename)
	:BUFFER_SIZE(182000)
{
	if (filename)
	{
		name = new char[strlen(filename) + 1];
		strcpy(name, filename);
	}
	audio_ctx = nullptr;
	stream_index = -1;
	stream = nullptr;
	audio_clock = 0;

	audio_buff = new uint8_t[BUFFER_SIZE];
	audioq = new PacketQueue();
	audio_buff_size = 0;
	audio_buff_index = 0;
}

AudioState::AudioState(AVCodecContext *audioCtx, int index)
	:BUFFER_SIZE(182000)
{
	audio_ctx = audioCtx;
	stream_index = index;


	audio_buff = new uint8_t[BUFFER_SIZE];
	audio_buff_size = 0;
	audio_buff_index = 0;
}

AudioState::~AudioState()
{
	/*if (dev)
	{
		SDL_CloseAudioDevice(dev);
	}*/
	log4cplus::Logger logger = GetSubLogger(L"Audio");
	LOG4CPLUS_DEBUG(logger, "开始音频资源回收:"<< name);
	SDL_CloseAudio();

	if (audio_buff) {
		delete[] audio_buff;
		audio_buff = nullptr;
	}

	if (audioq) {
		delete audioq;
		audioq = nullptr;
	}

	if (audio_ctx) {
		avcodec_free_context(&audio_ctx);
		audio_ctx = nullptr;
	}
	if (name) {
		delete[] name;
		name = nullptr;
	}
	LOG4CPLUS_DEBUG(logger, "结束音频资源回收");
}


bool AudioState::audio_play()
{
	log4cplus::Logger logger = GetSubLogger(L"Audio");

	SDL_AudioSpec desired;
	if (audio_ctx == nullptr)
	{
		return false;
	}
	desired.freq = audio_ctx->sample_rate;
	desired.channels = audio_ctx->channels;
	desired.format = AUDIO_S16SYS;
	desired.samples = 1024;
	desired.silence = 0;
	desired.userdata = this;
	desired.callback = audio_callback;

	if (SDL_OpenAudio(&desired, nullptr) < 0)
	{
		LOG4CPLUS_DEBUG(logger, "SDL_OpenAudio失敗：" << SDL_GetError());
		return false;
	}

	SDL_PauseAudio(0); // playing
	LOG4CPLUS_DEBUG(logger, "创建音频线程成功！");
	//if (audio_ctx == nullptr)
	//{
	//	return false;
	//}
	//SDL_AudioSpec desired, obtained;

	////SDL_memset(&desired, 0, sizeof(desired)); /* or SDL_zero(want) */
	//desired.freq = audio_ctx->sample_rate;
	//desired.channels = audio_ctx->channels;
	//desired.format = AUDIO_S16SYS;
	//desired.samples = 1024;
	//desired.silence = 0;
	//desired.userdata = this;
	//desired.callback = audio_callback;

	//dev = SDL_OpenAudioDevice(NULL, 0, &desired, &obtained, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
	//if (dev == 0) {
	//	SDL_Log("Failed to open audio: %s", SDL_GetError());
	//}
	//else {
	//	if (obtained.format != desired.format) {
	//		SDL_Log("We didn't get AUDIO_S16SYS audio format.");
	//	}
	//	SDL_PauseAudioDevice(dev, 0);
	//}



	/*SDL_LockAudio();
	SDL_CloseAudio();
	SDL_UnlockAudio();*/
	return true;
}

double AudioState::get_audio_clock()
{
	int hw_buf_size = audio_buff_size - audio_buff_index;
	int bytes_per_sec = stream->codec->sample_rate * audio_ctx->channels * 2;

	double pts = audio_clock - static_cast<double>(hw_buf_size) / bytes_per_sec;


	return pts;
}

/**
* 向设备发送audio数据的回调函数
*/
void audio_callback(void* userdata, Uint8 *stream, int len)
{
	AudioState* audio_state = (AudioState*)userdata;
	SDL_memset(stream, 0, len);

	int audio_size = 0;
	int len1 = 0;
	if (len == 0)
	{
		int temItem = len;
	}
	while (len > 0)// 向设备发送长度为len的数据
	{
		if (quit)
		{
			log4cplus::Logger logger = GetSubLogger(L"Audio");
			LOG4CPLUS_DEBUG(logger, "结束audio数据回调处理函数");
			return;
		}
		if (audio_state->audio_buff_index >= audio_state->audio_buff_size) // 缓冲区中无数据
		{
			// 从packet中解码数据
			audio_size = audio_state->audio_decode_frame(audio_state->audio_buff, sizeof(audio_state->audio_buff));
			if (audio_size < 0) // 没有解码到数据或出错，填充0
			{
				audio_state->audio_buff_size = 0;
				memset(audio_state->audio_buff, 0, audio_state->audio_buff_size);
			}
			else
				audio_state->audio_buff_size = audio_size;

			audio_state->audio_buff_index = 0;
		}
		len1 = audio_state->audio_buff_size - audio_state->audio_buff_index; // 缓冲区中剩下的数据长度
		if (len1 > len) // 向设备发送的数据长度为len
			len1 = len;
		//const Uint8 *mixData;
		//SDL_MixAudio(stream, audio_state->audio_buff + audio_state->audio_buff_index, len, SDL_MIX_MAXVOLUME);
		SDL_MixAudioFormat(stream, audio_state->audio_buff + audio_state->audio_buff_index, AUDIO_S16SYS, len, SDL_MIX_MAXVOLUME);
		len -= len1;
		stream += len1;
		audio_state->audio_buff_index += len1;
	}
}

int AudioState::audio_decode_frame(uint8_t *audio_buf, int buf_size)
{
	AVFrame *frame = av_frame_alloc();
	int data_size = 0;
	AVPacket *pkt = av_packet_alloc();
	SwrContext *swr_ctx = nullptr;
	static double clock = 0;

	if (quit)
	{
		
		av_frame_free(&frame);
		av_packet_free(&pkt);
		log4cplus::Logger logger = GetSubLogger(L"Audio");
		LOG4CPLUS_DEBUG(logger, "结束audio解码frame");
		return -1;
	}
	if (!this->audioq->deQueue(pkt, false))
	{
		av_frame_free(&frame);
		av_packet_free(&pkt);
		return -1;
	}

	if (pkt->pts != AV_NOPTS_VALUE)
	{
		this->audio_clock = av_q2d(this->stream->time_base) * pkt->pts;
	}
	int ret = avcodec_send_packet(this->audio_ctx, pkt);
	if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
	{
		av_frame_free(&frame);
		av_packet_free(&pkt);
		return -1;
	}

	ret = avcodec_receive_frame(this->audio_ctx, frame);
	if (ret < 0 && ret != AVERROR_EOF)
	{
		av_frame_free(&frame);
		av_packet_free(&pkt);
		return -1;
	}


	// 设置通道数或channel_layout
	if (frame->channels > 0 && frame->channel_layout == 0)
		frame->channel_layout = av_get_default_channel_layout(frame->channels);
	else if (frame->channels == 0 && frame->channel_layout > 0)
		frame->channels = av_get_channel_layout_nb_channels(frame->channel_layout);

	AVSampleFormat dst_format = AV_SAMPLE_FMT_S16;//av_get_packed_sample_fmt((AVSampleFormat)frame->format);
	Uint64 dst_layout = av_get_default_channel_layout(frame->channels);
	// 设置转换参数
	swr_ctx = swr_alloc_set_opts(nullptr, dst_layout, dst_format, frame->sample_rate,
		frame->channel_layout, (AVSampleFormat)frame->format, frame->sample_rate, 0, nullptr);
	if (!swr_ctx || swr_init(swr_ctx) < 0)
	{
		av_frame_free(&frame);
		swr_free(&swr_ctx);
		av_packet_free(&pkt);
		return -1;
	}

	// 计算转换后的sample个数 a * b / c
	uint64_t dst_nb_samples = av_rescale_rnd(swr_get_delay(swr_ctx, frame->sample_rate) + frame->nb_samples, frame->sample_rate, frame->sample_rate, AVRounding(1));
	// 转换，返回值为转换后的sample个数
	int nb = swr_convert(swr_ctx, &audio_buf, static_cast<int>(dst_nb_samples), (const uint8_t**)frame->data, frame->nb_samples);
	data_size = frame->channels * nb * av_get_bytes_per_sample(dst_format);

	// 每秒钟音频播放的字节数 sample_rate * channels * sample_format(一个sample占用的字节数)
	this->audio_clock += static_cast<double>(data_size) / (2 * this->stream->codec->channels * this->stream->codec->sample_rate);


	av_frame_free(&frame);
	swr_free(&swr_ctx);
	av_packet_free(&pkt);
	return data_size;
}

