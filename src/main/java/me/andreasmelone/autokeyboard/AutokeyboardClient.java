package me.andreasmelone.autokeyboard;

import com.mojang.logging.LogUtils;
import net.fabricmc.api.ClientModInitializer;
import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;
import org.slf4j.Logger;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import me.andreasmelone.autokeyboard.pojav.AndroidLibLoader;
import me.andreasmelone.autokeyboard.pojav.PojavIntegrateAndroidNative;

import static me.andreasmelone.autokeyboard.pojav.PojavIntegrateAndroidNative.*;

public class AutokeyboardClient implements ClientModInitializer {
    private static final Logger LOGGER = LogUtils.getLogger();
    private static boolean initializedNative = false;
    public boolean isPojav = false;

    @Override
    public void onInitializeClient() {
        String dataUserPath = null;
        for (String path : System.getProperty("java.library.path").split(File.pathSeparator)) {
            if(getPackageNameFromPath(path) == null) continue;
            dataUserPath = path;
            break;
        }
        if(dataUserPath != null) {
            isPojav = true;
        }

        if(isPojav) {
            AndroidLibLoader.INSTANCE.setPath(dataUserPath);
            ByteArrayOutputStream dex = new ByteArrayOutputStream();
            Util.extractFile("/dex/classes.dex", dex);
            PojavIntegrateAndroidNative.setDexData(dex.toByteArray());
            int result = PojavIntegrateAndroidNative.init();
            switch (result) {
                case INIT_SUCCESS -> initializedNative = true;
                case INIT_DEX_NOT_INITIALIZED -> LOGGER.error("Dex did not initialize, mod will not function");
                case INIT_DVM_NOT_FOUND -> {
                    LOGGER.warn("Dalvik VM cannot be found, are we running on Android?");
                    LOGGER.error("Mod will not function");
                }
                case INIT_METHOD_NOT_INITIALIZED -> LOGGER.error("JNI could not initialized methods, mod will not function");
                default -> LOGGER.error("An error occurred, mod will not function {}", Integer.toHexString(result)); // also covers INIT_GENERIC_ERROR
            }
        }
    }
    @Nullable
    public static String getPackageNameFromPath(@NotNull String path) {
        Pattern p = Pattern.compile("^/data/user/\\d+/([^/]+)/.*");
        Matcher m = p.matcher(path);
        return m.find() ? m.group(1) : null;
    }

    public static boolean isInitializedNative() {
        return initializedNative;
    }
}
