package me.andreasmelone.autokeyboard;

import com.mojang.logging.LogUtils;
import net.fabricmc.api.ClientModInitializer;

import org.slf4j.Logger;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import me.andreasmelone.autokeyboard.pojav.AndroidLibLoader;
import me.andreasmelone.autokeyboard.pojav.AutoKeyboardAndroidNative;

import static me.andreasmelone.autokeyboard.pojav.AutoKeyboardAndroidNative.*;

public class AutokeyboardClient implements ClientModInitializer {
    private static final Logger LOGGER = LogUtils.getLogger();
    private static boolean initializedNative = false;
    public boolean isPojav = false;

    @Override
    public void onInitializeClient() {
        String packageName = null;
        String userId = null;

        Pattern pattern = Pattern.compile("^/data/user/(\\d+)/([^/]+)/.*");
        for (String path : System.getProperty("java.library.path").split(File.pathSeparator)) {
            Matcher m = pattern.matcher(path);
            if (!m.find()) continue;
            userId = m.group(1);
            packageName = m.group(2);
            break;
        }

        if(packageName != null && userId != null) {
            isPojav = true;
        }

        if(isPojav) {
            AndroidLibLoader.INSTANCE.setPath("/data/user/" + userId + "/" + packageName + "/cache");
            ByteArrayOutputStream dex = new ByteArrayOutputStream();
            Util.extractFile("/dex/classes.dex", dex);
            AutoKeyboardAndroidNative.setDexData(dex.toByteArray());
            int result = AutoKeyboardAndroidNative.init();
            switch (result) {
                case INIT_SUCCESS -> initializedNative = true;
                case INIT_DEX_NOT_INITIALIZED -> LOGGER.error("Dex did not initialize, mod will not function");
                case INIT_DVM_NOT_FOUND -> {
                    LOGGER.warn("Dalvik VM cannot be found, are we running on Android?");
                    LOGGER.error("Mod will not function");
                }
                case INIT_METHOD_NOT_INITIALIZED -> LOGGER.error("JNI could not initialized methods, mod will not function");
                default -> LOGGER.error("An error occurred, mod will not function 0x{}", Integer.toHexString(result).toUpperCase()); // also covers INIT_GENERIC_ERROR
            }
        }
    }

    public static boolean isInitializedNative() {
        return initializedNative;
    }
}
