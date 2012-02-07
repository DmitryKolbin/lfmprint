extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/mem.h>
#include <libavutil/error.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}
#include <FingerprintExtractor.h>
#include <utility>
#include <string>
#include <iostream>
#include "include/lfmprint.h"

using namespace std;
using namespace fingerprint;

static pthread_mutex_t mutex;

void lfmprint_init() {
    pthread_mutex_init(&mutex, NULL);
    avcodec_init();
    av_register_all();
}

void lfmprint_destroy() {
    pthread_mutex_destroy(&mutex);
}

pair<const char*,size_t> processPCM(AVFormatContext *pFormatCtx, 
        AVCodecContext *pCodecCtx, int stream) {
    AVPacket    packet;
    int16_t *   samples;
    int         readCond;
    int         duration;
    duration = pFormatCtx->duration / AV_TIME_BASE;

    FingerprintExtractor pfextr;
    pfextr.initForQuery(pCodecCtx->sample_rate, pCodecCtx->channels, duration);
    packet.data = NULL;
    size_t skip = pfextr.getToSkipMs();

    av_seek_frame(pFormatCtx, -1, skip * AV_TIME_BASE / 1000, 0);

    pfextr.process(NULL, (skip / 1000) * pCodecCtx->sample_rate * pCodecCtx->channels, 0);
    readCond = 0;
    readCond = av_read_frame(pFormatCtx, &packet);

    samples = new int16_t[AVCODEC_MAX_AUDIO_FRAME_SIZE / (sizeof (uint16_t))];
    while (readCond >= 0) {
        int data_size;
        int bytesDecoded;
        data_size = AVCODEC_MAX_AUDIO_FRAME_SIZE;

        bytesDecoded = avcodec_decode_audio3(pCodecCtx,samples, &data_size, &packet);
        if (bytesDecoded < 0)
        {
            delete[] samples;
            throw "Error while decoding";
        }
        
        do {
            if (packet.data != NULL) {
                av_free_packet(&packet);
            }
            readCond = av_read_frame(pFormatCtx, &packet);
        } while(packet.stream_index != stream && readCond == 0);
        
        int res = pfextr.process(samples, data_size / av_get_bytes_per_sample(pCodecCtx->sample_fmt), readCond < 0);
        
        if (res == 0 && readCond < 0) {
            delete[] samples;
            throw "need more data";
        }
        if (res) {
            break;
        }
    }
    if (packet.data != NULL) {
        av_free_packet(&packet);
    }
    delete[] samples;
    return pfextr.getFingerprint();
}


fingerprint_data getFingerprint(string file) {
    AVFormatContext   *pFormatCtx;
    int               duration;
    int               audioStream;
    AVCodecContext    *pCodecCtx;
    AVCodec           *pCodecDec;
    int               ret;
    pFormatCtx = avformat_alloc_context(); 
    if ((ret = avformat_open_input(&pFormatCtx, file.c_str(), NULL, NULL)) < 0) {
        char buf[1024];
        av_strerror(ret, buf,1024);
        throw buf;
    }
    pthread_mutex_lock(&mutex);
    if((ret = av_find_stream_info(pFormatCtx)) < 0) {
        char buf[1024];
        pthread_mutex_unlock(&mutex);
        av_close_input_file(pFormatCtx);
        av_strerror(ret, buf,1024);
        throw buf;
    }
    pthread_mutex_unlock(&mutex);
    audioStream = -1;
    for (size_t i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioStream = i;
        }
    }
    if (-1 == audioStream) {
        throw "This file doesn\'t have audio stream";
        av_close_input_file(pFormatCtx);
    }
    pCodecCtx = pFormatCtx->streams[audioStream]->codec;
    pCodecDec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (NULL == pCodecDec) {
        av_close_input_file(pFormatCtx);
        throw "Codec not found";
    }
    
    pthread_mutex_lock(&mutex);
    
    if (avcodec_open2(pCodecCtx, pCodecDec, NULL) < 0) {
        char buf[1024];
        pthread_mutex_unlock(&mutex);
        av_close_input_file(pFormatCtx);      
        av_strerror(ret, buf,1024);
        throw buf;
    } 
    pthread_mutex_unlock(&mutex);
    duration = pFormatCtx->duration / AV_TIME_BASE;
    pair<const char*, size_t> r = processPCM(pFormatCtx, pCodecCtx, audioStream);
    if (r.second == 0) {
       avcodec_close(pCodecCtx);
       av_close_input_file(pFormatCtx);
       throw "unknown error";
    }
    pthread_mutex_lock(&mutex);
    avcodec_close(pCodecCtx);
    pthread_mutex_unlock(&mutex);
    
    av_close_input_file(pFormatCtx);

    fingerprint_data result;
    result.fingerprint = string (r.first, r.second);
    result.duration = duration;
    return result;
}

