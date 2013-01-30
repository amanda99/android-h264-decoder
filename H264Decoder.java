import java.nio.ByteBuffer;

public class H264Decoder {
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

  public native void consumeNalUnitsFromDirectBuffer(ByteBuffer nalUnits, int numBytes);
  public native boolean isFrameReady();
  public native int getWidth();
  public native int getHeight();
  public native int getOutputByteSize();
  public native boolean decodeFrameToDirectBuffer(ByteBuffer buffer);

  static {
    System.loadLibrary("videodecoder");
  }
}
