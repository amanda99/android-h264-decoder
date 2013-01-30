#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdarg.h>

#include <android/log.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>

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
  struct AVCodecContext *codecCtx;
  struct AVFrame *srcFrame;
  struct AVFrame *dstFrame;
  struct SwsContext *convertCtx;
  int frameReady;
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

static void av_log_callback(void *ptr, int level, const char *fmt, __va_list vl)
{
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

JNIEXPORT void Java_com_dropcam_android_media_H264Decoder_nativeInit(JNIEnv* env, jobject thiz, jint color_format)
{
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
  ctx->codecCtx = avcodec_alloc_context3(ctx->codec);

  ctx->codecCtx->pix_fmt = PIX_FMT_YUV420P;
  ctx->codecCtx->flags2 |= CODEC_FLAG2_CHUNKS;

  ctx->srcFrame = avcodec_alloc_frame();
  ctx->dstFrame = avcodec_alloc_frame();

  avcodec_open2(ctx->codecCtx, ctx->codec, NULL);

  set_ctx(env, thiz, ctx);
}

JNIEXPORT void Java_com_dropcam_android_media_H264Decoder_nativeDestroy(JNIEnv* env, jobject thiz)
{
  DecoderContext *ctx = get_ctx(env, thiz);

  D("Destroying native H264 decoder context");
  avcodec_close(ctx->codecCtx);
  av_free(ctx->codecCtx);
  av_free(ctx->srcFrame);
  av_free(ctx->dstFrame);

  free(ctx);
}

JNIEXPORT jint Java_com_dropcam_android_media_H264Decoder_consumeNalUnitsFromDirectBuffer(JNIEnv* env, jobject thiz, jobject nal_units, jint num_bytes)
{
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
      .size = num_bytes
  };

  int frameFinished = 0;
  int res = avcodec_decode_video2(ctx->codecCtx, ctx->srcFrame, &frameFinished, &packet);

  if (frameFinished)
    ctx->frameReady = 1;

  return res;
}

JNIEXPORT jboolean Java_com_dropcam_android_media_H264Decoder_isFrameReady(JNIEnv* env, jobject thiz)
{
  DecoderContext *ctx = get_ctx(env, thiz);
  return ctx->frameReady ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jint Java_com_dropcam_android_media_H264Decoder_getWidth(JNIEnv* env, jobject thiz)
{
  DecoderContext *ctx = get_ctx(env, thiz);
  return ctx->codecCtx->width;
}

JNIEXPORT jint Java_com_dropcam_android_media_H264Decoder_getHeight(JNIEnv* env, jobject thiz)
{
  DecoderContext *ctx = get_ctx(env, thiz);
  return ctx->codecCtx->height;
}

JNIEXPORT jint Java_com_dropcam_android_media_H264Decoder_getOutputByteSize(JNIEnv* env, jobject thiz)
{
  DecoderContext *ctx = get_ctx(env, thiz);
  return avpicture_get_size(ctx->color_format, ctx->codecCtx->width, ctx->codecCtx->height);
}

JNIEXPORT jboolean Java_com_dropcam_android_media_H264Decoder_decodeFrameToDirectBuffer(JNIEnv* env, jobject thiz, jobject out_buffer)
{
  DecoderContext *ctx = get_ctx(env, thiz);

  if (!ctx->frameReady)
    return JNI_FALSE;

  void *out_buf = (*env)->GetDirectBufferAddress(env, out_buffer);
  if (out_buf == NULL) {
    D("Error getting direct buffer address");
    return -1;
  }

  long out_buf_len = (*env)->GetDirectBufferCapacity(env, out_buffer);

  int pic_buf_size = avpicture_get_size(ctx->color_format, ctx->codecCtx->width, ctx->codecCtx->height);

  if (out_buf_len < pic_buf_size) {
    D("Input buffer too small");
    return JNI_FALSE;
  }

  if (ctx->color_format == COLOR_FORMAT_YUV420) {
    memcpy(ctx->srcFrame->data, out_buffer, pic_buf_size);
  }
  else {
    if (ctx->convertCtx == NULL) {
      ctx->convertCtx = sws_getContext(ctx->codecCtx->width, ctx->codecCtx->height, ctx->codecCtx->pix_fmt,
          ctx->codecCtx->width, ctx->codecCtx->height, ctx->color_format, SWS_FAST_BILINEAR, NULL, NULL, NULL);
    }

    avpicture_fill((AVPicture*)ctx->dstFrame, (uint8_t*)out_buf, ctx->color_format, ctx->codecCtx->width,
        ctx->codecCtx->height);

    sws_scale(ctx->convertCtx, (const uint8_t**)ctx->srcFrame->data, ctx->srcFrame->linesize, 0, ctx->codecCtx->height,
        ctx->dstFrame->data, ctx->dstFrame->linesize);
  }

  ctx->frameReady = 0;

  return JNI_TRUE;
}

