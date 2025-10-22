package me.andreasmelone.autokeyboard.pojav;

import org.intellij.lang.annotations.MagicConstant;

@SuppressWarnings("UnsafeDynamicallyLoadedCode")
public class PojavIntegrateAndroidNative {
    public static final int INIT_SUCCESS = 0x00;
    public static final int INIT_GENERIC_ERROR = 0xFFFFFFFF;
    public static final int INIT_DEX_NOT_INITIALIZED = 0xFFFFFFFE;
    public static final int INIT_DVM_NOT_FOUND = 0xFFFFFFFD;
    public static final int INIT_METHOD_NOT_INITIALIZED = 0xFFFFFFFC;

    @MagicConstant(intValues = {
            INIT_SUCCESS, INIT_GENERIC_ERROR, INIT_DEX_NOT_INITIALIZED, INIT_DVM_NOT_FOUND, INIT_METHOD_NOT_INITIALIZED
    })
    public static native int init();
    public static native void setDexData(byte[] data);
    public static native void setKeyboardState(boolean state);

    static {
        System.load(AndroidLibLoader.INSTANCE.get("pojavintegrate"));
    }
}
