/*
 *  H264Decoder.java
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

package com.dropcam.android.media;

import java.nio.ByteBuffer;

public class H264Decoder {

  // (Must match defines in h264decoder.c)
  public static final int COLOR_FORMAT_YUV420 = 0;
  public static final int COLOR_FORMAT_RGB565LE = 1;
  public static final int COLOR_FORMAT_BGR32 = 2;
  
  public H264Decoder(int colorFormat) {
    nativeInit(colorFormat);
  }

  protected void finalize() throws Throwable {
    nativeDestroy();
    super.finalize();
  }

  private int cdata;
  private native void nativeInit(int colorFormat);
  private native void nativeDestroy();

  public native void consumeNalUnitsFromDirectBuffer(ByteBuffer nalUnits, int numBytes, long packetPTS);
  public native boolean isFrameReady();
  public native int getWidth();
  public native int getHeight();
  public native int getOutputByteSize();
  public native long decodeFrameToDirectBuffer(ByteBuffer buffer);

  static {
    System.loadLibrary("videodecoder");
  }
}
