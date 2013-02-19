/*
 *  h264decoder.c
 *  JNI H.264 video decoder module
 *
 *  Copyright (c) 2012, Dropcam
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice, this
 *     list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 *  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdarg.h>

#include <android/log.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>

// (Must match defines in H264Decoder.java)
#define COLOR_FORMAT_YUV420 0
#define COLOR_FORMAT_RGB565LE 1
#define COLOR_FORMAT_BGR32 2

#if DEBUG
#  define  D(x...)  __android_log_print(ANDROID_LOG_INFO, "h264decoder", x)
#else
#  define  D(...)  do {} while (0)
#endif

typedef struct DecoderContext {
  int color_format;
  struct AVCodec *codec;
  struct AVCodecContext *codec_ctx;
  struct AVFrame *src_frame;
  struct AVFrame *dst_frame;
  struct SwsContext *convert_ctx;
  int frame_ready;
} DecoderContext;

static void set_ctx(JNIEnv *env, jobject thiz, void *ctx) {
  jclass cls = (*env)->GetObjectClass(env, thiz);
  jfieldID fid = (*env)->GetFieldID(env, cls, "cdata", "I");
  (*env)->SetIntField(env, thiz, fid, (jint)ctx);
}

static void *get_ctx(JNIEnv *env, jobject thiz) {
  jclass cls = (*env)->GetObjectClass(env, thiz);
  jfieldID fid = (*env)->GetFieldID(env, cls, "cdata", "I");
  return (void*)(*env)->GetIntField(env, thiz, fid);
}

static void av_log_callback(void *ptr, int level, const char *fmt, __va_list vl) {
  static char line[1024] = {0};
  vsnprintf(line, sizeof(line), fmt, vl);
  D(line);
}

JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
  av_register_all();
  av_log_set_callback(av_log_callback);

  return JNI_VERSION_1_4;
}

JNIEXPORT void JNI_OnUnload(JavaVM *vm, void *reserved) {
}

JNIEXPORT void Java_com_dropcam_android_media_H264Decoder_nativeInit(JNIEnv* env, jobject thiz, jint color_format) {
  DecoderContext *ctx = calloc(1, sizeof(DecoderContext));

  D("Creating native H264 decoder context");

  switch (color_format) {
  case COLOR_FORMAT_YUV420:
    ctx->color_format = PIX_FMT_YUV420P;
    break;
  case COLOR_FORMAT_RGB565LE:
    ctx->color_format = PIX_FMT_RGB565LE;
    break;
  case COLOR_FORMAT_BGR32:
    ctx->color_format = PIX_FMT_BGR32;
    break;
  }

  ctx->codec = avcodec_find_decoder(CODEC_ID_H264);
  ctx->codec_ctx = avcodec_alloc_context3(ctx->codec);

  ctx->codec_ctx->pix_fmt = PIX_FMT_YUV420P;
  ctx->codec_ctx->flags2 |= CODEC_FLAG2_CHUNKS;

  ctx->src_frame = avcodec_alloc_frame();
  ctx->dst_frame = avcodec_alloc_frame();

  avcodec_open2(ctx->codec_ctx, ctx->codec, NULL);

  set_ctx(env, thiz, ctx);
}

JNIEXPORT void Java_com_dropcam_android_media_H264Decoder_nativeDestroy(JNIEnv* env, jobject thiz) {
  DecoderContext *ctx = get_ctx(env, thiz);

  D("Destroying native H264 decoder context");

  avcodec_close(ctx->codec_ctx);
  av_free(ctx->codec_ctx);
  av_free(ctx->src_frame);
  av_free(ctx->dst_frame);

  free(ctx);
}

JNIEXPORT jint Java_com_dropcam_android_media_H264Decoder_consumeNalUnitsFromDirectBuffer(JNIEnv* env, jobject thiz, jobject nal_units, jint num_bytes, jlong pkt_pts) {
  DecoderContext *ctx = get_ctx(env, thiz);

  void *buf = NULL;
  if (nal_units == NULL) {
    D("Received null buffer, sending empty packet to decoder");
  }
  else {
    buf = (*env)->GetDirectBufferAddress(env, nal_units);
    if (buf == NULL) {
      D("Error getting direct buffer address");
      return -1;
    }
  }

  AVPacket packet = {
      .data = (uint8_t*)buf,
      .size = num_bytes,
      .pts = pkt_pts
  };

  int frameFinished = 0;
  int res = avcodec_decode_video2(ctx->codec_ctx, ctx->src_frame, &frameFinished, &packet);

  if (frameFinished)
    ctx->frame_ready = 1;

  return res;
}

JNIEXPORT jboolean Java_com_dropcam_android_media_H264Decoder_isFrameReady(JNIEnv* env, jobject thiz) {
  DecoderContext *ctx = get_ctx(env, thiz);
  return ctx->frame_ready ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jint Java_com_dropcam_android_media_H264Decoder_getWidth(JNIEnv* env, jobject thiz) {
  DecoderContext *ctx = get_ctx(env, thiz);
  return ctx->codec_ctx->width;
}

JNIEXPORT jint Java_com_dropcam_android_media_H264Decoder_getHeight(JNIEnv* env, jobject thiz) {
  DecoderContext *ctx = get_ctx(env, thiz);
  return ctx->codec_ctx->height;
}

JNIEXPORT jint Java_com_dropcam_android_media_H264Decoder_getOutputByteSize(JNIEnv* env, jobject thiz) {
  DecoderContext *ctx = get_ctx(env, thiz);
  return avpicture_get_size(ctx->color_format, ctx->codec_ctx->width, ctx->codec_ctx->height);
}

JNIEXPORT jlong Java_com_dropcam_android_media_H264Decoder_decodeFrameToDirectBuffer(JNIEnv* env, jobject thiz, jobject out_buffer) {
  DecoderContext *ctx = get_ctx(env, thiz);

  if (!ctx->frame_ready)
    return -1;

  void *out_buf = (*env)->GetDirectBufferAddress(env, out_buffer);
  if (out_buf == NULL) {
    D("Error getting direct buffer address");
    return -1;
  }

  long out_buf_len = (*env)->GetDirectBufferCapacity(env, out_buffer);

  int pic_buf_size = avpicture_get_size(ctx->color_format, ctx->codec_ctx->width, ctx->codec_ctx->height);

  if (out_buf_len < pic_buf_size) {
    D("Input buffer too small");
    return -1;
  }

  if (ctx->color_format == COLOR_FORMAT_YUV420) {
    memcpy(ctx->src_frame->data, out_buffer, pic_buf_size);
  }
  else {
    if (ctx->convert_ctx == NULL) {
      ctx->convert_ctx = sws_getContext(ctx->codec_ctx->width, ctx->codec_ctx->height, ctx->codec_ctx->pix_fmt,
          ctx->codec_ctx->width, ctx->codec_ctx->height, ctx->color_format, SWS_FAST_BILINEAR, NULL, NULL, NULL);
    }

    avpicture_fill((AVPicture*)ctx->dst_frame, (uint8_t*)out_buf, ctx->color_format, ctx->codec_ctx->width,
        ctx->codec_ctx->height);

    sws_scale(ctx->convert_ctx, (const uint8_t**)ctx->src_frame->data, ctx->src_frame->linesize, 0, ctx->codec_ctx->height,
        ctx->dst_frame->data, ctx->dst_frame->linesize);
  }

  ctx->frame_ready = 0;

  if (ctx->src_frame->pkt_pts == AV_NOPTS_VALUE) {
    D("No PTS was passed from avcodec_decode!");
  }

  return ctx->src_frame->pkt_pts;
}

